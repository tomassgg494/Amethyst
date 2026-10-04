#include "sema.hpp"

#include <algorithm>

// `int[]` is the generic (slice) spelling; locals are printed as `int[10]`.
static std::string fixedArrayName(Type t, int n) {
    return std::string(t == Type::ArrayBool ? "bool" : "int") + "[" +
           std::to_string(n) + "]";
}

void Sema::fail(const std::string& msg, int line, int col) const {
    throw SemaError(msg, line, col);
}

void Sema::pushScope() {
    scopes_.emplace_back();
}

void Sema::popScope() {
    for (auto& v : scopes_.back().vars) {
        if (!v.second.used && !v.second.isParam) {
            warnings_.push_back(
                {v.second.declLine, v.second.declCol,
                 "unused variable '" + v.first + "' (declared but never referenced)"});
        }
    }
    scopes_.pop_back();
}

Sema::VarInfo* Sema::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        for (auto& v : it->vars) {
            if (v.first == name) return &v.second;
        }
    }
    return nullptr;
}

void Sema::declare(const std::string& name, const VarInfo& info, int line, int col) {
    auto& scope = scopes_.back();
    for (auto& v : scope.vars) {
        if (v.first == name) {
            fail("redeclaration of '" + name + "' in the same scope", line, col);
        }
    }
    VarInfo stored = info;
    stored.declLine = line;
    stored.declCol = col;
    scope.vars.emplace_back(name, stored);
}

void Sema::collectFunctions(Program& program) {
    fnInfos_.clear();
    fnNames_.clear();
    for (size_t i = 0; i < program.functions.size(); ++i) {
        FnDecl& fn = program.functions[i];
        if (fn.name == "len") {
            fail("'len' is a builtin and cannot be redefined", fn.line, fn.col);
        }
        for (const auto& existing : fnNames_) {
            if (existing == fn.name) {
                fail("redefinition of function '" + fn.name + "'", fn.line, fn.col);
            }
        }
        std::vector<Type> paramTypes;
        for (const auto& p : fn.params) paramTypes.push_back(p.type);
        fnInfos_.push_back(FnInfo{fn.returnType, paramTypes, static_cast<int>(i)});
        fnNames_.push_back(fn.name);
    }
}

void Sema::checkMain(Program& program) {
    for (const auto& fn : program.functions) {
        if (fn.name == "main") {
            if (fn.returnType != Type::Int) {
                fail("main must return int", fn.line, fn.col);
            }
            if (!fn.params.empty()) {
                fail("main must not take parameters", fn.line, fn.col);
            }
            return;
        }
    }
    fail("program must define 'main'", 1, 1);
}

void Sema::analyze(Program& program) {
    warnings_.clear();
    collectFunctions(program);
    checkMain(program);
    for (auto& fn : program.functions) {
        checkFunction(fn);
    }
}

void Sema::checkFunction(FnDecl& fn) {
    scopes_.clear();
    nextSlot_ = 0;
    loopDepth_ = 0;
    pushScope();

    for (auto& p : fn.params) {
        int units = isArrayType(p.type) ? 2 : 1;  // slice = {ptr, len}
        p.slot = nextSlot_;
        nextSlot_ += units;
        VarInfo pi{p.type, p.slot, isArrayType(p.type) ? -1 : 0};
        pi.isParam = true;
        declare(p.name, pi, p.line, p.col);
    }

    if (fn.body == nullptr || fn.body->kind != StmtKind::Block) {
        fail("function body must be a block", fn.line, fn.col);
    }

    if (fn.returnType != Type::Void && !stmtAlwaysReturns(*fn.body)) {
        fail("not all control paths of non-void function '" + fn.name +
                 "' return a value",
             fn.line, fn.col);
    }

    checkBlock(*fn.body, fn.returnType);
    fn.slotCount = nextSlot_;
    popScope();
}

bool Sema::stmtAlwaysReturns(const Stmt& stmt) const {
    switch (stmt.kind) {
        case StmtKind::Return:
            return true;
        case StmtKind::Block:
            for (const auto& s : stmt.stmts) {
                if (stmtAlwaysReturns(*s)) return true;
            }
            return false;
        case StmtKind::If:
            if (!stmt.thenBlock || !stmtAlwaysReturns(*stmt.thenBlock)) {
                return false;
            }
            if (!stmt.elseBlock) return false;
            return stmtAlwaysReturns(*stmt.elseBlock);
        default:
            return false;
    }
}

bool Sema::stmtAlwaysTerminates(const Stmt& stmt) const {
    switch (stmt.kind) {
        case StmtKind::Return:
        case StmtKind::Break:
        case StmtKind::Continue:
            return true;
        case StmtKind::Block:
            for (const auto& s : stmt.stmts) {
                if (stmtAlwaysTerminates(*s)) return true;
            }
            return false;
        case StmtKind::If:
            if (!stmt.thenBlock || !stmtAlwaysTerminates(*stmt.thenBlock)) {
                return false;
            }
            if (!stmt.elseBlock) return false;
            return stmtAlwaysTerminates(*stmt.elseBlock);
        default:
            return false;
    }
}

void Sema::checkBlock(Stmt& block, Type fnReturn) {
    pushScope();
    for (size_t i = 0; i < block.stmts.size(); ++i) {
        checkStmt(*block.stmts[i], fnReturn);
        if (i + 1 < block.stmts.size() &&
            stmtAlwaysTerminates(*block.stmts[i])) {
            warnings_.push_back({block.stmts[i + 1]->line,
                                 block.stmts[i + 1]->col,
                                 "unreachable code"});
        }
    }
    popScope();
}

void Sema::requireUsable(const Expr& expr, Type t, const char* what) {
    if (t == Type::Str) {
        fail(std::string(what) + ": string literals can only be used with print",
             expr.line, expr.col);
    }
    if (isArrayType(t)) {
        fail(std::string(what) +
                 ": arrays cannot be used in this context (only var "
                 "initializers, indexing, len() and passing to functions)",
             expr.line, expr.col);
    }
}

void Sema::checkStmt(Stmt& stmt, Type fnReturn) {
    switch (stmt.kind) {
        case StmtKind::Block:
            checkBlock(stmt, fnReturn);
            break;

        case StmtKind::VarDecl: {
            Type initType = checkExpr(*stmt.expr);

            if (isArrayType(initType) && stmt.expr->kind != ExprKind::ArrayLit) {
                fail("array variable '" + stmt.name +
                         "' must be initialized with an array literal "
                         "(copy elements one by one)",
                     stmt.line, stmt.col);
            }

            if (stmt.typeInferred) {
                if (initType == Type::Error || initType == Type::Void) {
                    fail("cannot infer type of '" + stmt.name + "' from this initializer",
                         stmt.line, stmt.col);
                }
                if (initType == Type::Str) {
                    fail("cannot store a string in a variable (v1.1: strings only in print)",
                         stmt.line, stmt.col);
                }
                stmt.declaredType = initType;
                if (isArrayType(initType)) {
                    stmt.declaredSize = stmt.expr->arraySize;
                }
            } else if (isArrayType(stmt.declaredType)) {
                if (initType != stmt.declaredType) {
                    fail(std::string("cannot initialize '") +
                             fixedArrayName(stmt.declaredType,
                                            stmt.declaredSize) +
                             " " + stmt.name + "' with value of type '" +
                             typeName(initType) + "'",
                         stmt.line, stmt.col);
                }
                if (stmt.expr->arraySize != stmt.declaredSize) {
                    fail(std::string("array size mismatch: '") + stmt.name + "' is " +
                             std::to_string(stmt.declaredSize) + " long, initializer has " +
                             std::to_string(stmt.expr->arraySize) + " elements",
                         stmt.line, stmt.col);
                }
            } else {
                if (initType == Type::Str) {
                    fail(std::string("cannot initialize '") +
                             typeName(stmt.declaredType) + " " + stmt.name +
                             "' with a string literal",
                         stmt.line, stmt.col);
                }
                if (initType != stmt.declaredType) {
                    fail(std::string("cannot initialize '") +
                             typeName(stmt.declaredType) + " " + stmt.name +
                             "' with value of type '" + typeName(initType) + "'",
                         stmt.line, stmt.col);
                }
            }

            VarInfo info{stmt.declaredType, -1,
                         isArrayType(stmt.declaredType) ? stmt.declaredSize : 0};
            if (isArrayType(stmt.declaredType)) {
                info.slot = nextSlot_;
                nextSlot_ += stmt.declaredSize;
            } else {
                info.slot = nextSlot_++;
            }
            stmt.slot = info.slot;
            declare(stmt.name, info, stmt.line, stmt.col);
            break;
        }

        case StmtKind::Assign: {
            VarInfo* var = lookup(stmt.name);
            if (!var) {
                fail("assignment to undeclared variable '" + stmt.name + "'",
                     stmt.line, stmt.col);
            }
            var->used = true;
            if (isArrayType(var->type)) {
                fail("cannot assign to whole array '" + stmt.name +
                         "' (assign elements: " + stmt.name + "[i] = ...)",
                     stmt.line, stmt.col);
            }
            Type valueType = checkExpr(*stmt.expr);
            requireUsable(*stmt.expr, valueType, "assignment");
            if (stmt.compoundOp != TokenType::Eof) {
                if (var->type != Type::Int || valueType != Type::Int) {
                    fail(std::string("'") + compoundOpText(stmt.compoundOp) +
                             "' expects int on both sides (got '" +
                             typeName(var->type) + "' and '" +
                             typeName(valueType) + "')",
                         stmt.line, stmt.col);
                }
                stmt.slot = var->slot;
                break;
            }
            if (valueType != var->type) {
                fail(std::string("cannot assign '") + typeName(valueType) +
                         "' to variable '" + stmt.name + "' of type '" +
                         typeName(var->type) + "'",
                     stmt.line, stmt.col);
            }
            stmt.slot = var->slot;
            break;
        }

        case StmtKind::AssignIndex: {
            if (stmt.target == nullptr || stmt.target->kind != ExprKind::Index) {
                fail("invalid index assignment target", stmt.line, stmt.col);
            }
            // checkExpr on the Index node returns the *element* type and
            // fills lhs->type / slot.
            Type elemType = checkExpr(*stmt.target);
            Type baseType = stmt.target->lhs->type;
            Type valueType = checkExpr(*stmt.value);
            requireUsable(*stmt.value, valueType, "assignment");
            if (!isArrayType(baseType)) {
                fail("cannot index-assign into a non-array value", stmt.line,
                     stmt.col);
            }
            if (stmt.compoundOp != TokenType::Eof) {
                if (elemType != Type::Int || valueType != Type::Int) {
                    fail(std::string("'") + compoundOpText(stmt.compoundOp) +
                             "' expects int on both sides (got '" +
                             typeName(elemType) + "' and '" +
                             typeName(valueType) + "')",
                         stmt.line, stmt.col);
                }
                stmt.slot = stmt.target->slot;
                break;
            }
            if (valueType != elemType) {
                fail(std::string("cannot assign '") + typeName(valueType) +
                         "' to array element of type '" + typeName(elemType) + "'",
                     stmt.line, stmt.col);
            }
            stmt.slot = stmt.target->slot;
            break;
        }

        case StmtKind::If: {
            Type cond = checkExpr(*stmt.expr);
            if (cond != Type::Bool) {
                fail(std::string("if condition must be bool, got '") +
                         typeName(cond) + "'",
                     stmt.line, stmt.col);
            }
            checkBlock(*stmt.thenBlock, fnReturn);
            if (stmt.elseBlock) {
                if (stmt.elseBlock->kind == StmtKind::Block) {
                    checkBlock(*stmt.elseBlock, fnReturn);
                } else {
                    checkStmt(*stmt.elseBlock, fnReturn);
                }
            }
            break;
        }

        case StmtKind::While: {
            Type cond = checkExpr(*stmt.expr);
            if (cond != Type::Bool) {
                fail(std::string("while condition must be bool, got '") +
                         typeName(cond) + "'",
                     stmt.line, stmt.col);
            }
            loopDepth_++;
            checkBlock(*stmt.thenBlock, fnReturn);
            loopDepth_--;
            break;
        }

        case StmtKind::For: {
            Type startT = checkExpr(*stmt.expr);
            Type endT = checkExpr(*stmt.exprEnd);
            if (startT != Type::Int) {
                fail(std::string("for range start must be int, got '") +
                         typeName(startT) + "'",
                     stmt.line, stmt.col);
            }
            if (endT != Type::Int) {
                fail(std::string("for range end must be int, got '") +
                         typeName(endT) + "'",
                     stmt.line, stmt.col);
            }

            pushScope();
            stmt.loopSlot = nextSlot_++;
            stmt.endSlot = nextSlot_++;
            declare(stmt.loopVar, VarInfo{Type::Int, stmt.loopSlot, 0},
                    stmt.line, stmt.col);

            loopDepth_++;
            checkBlock(*stmt.thenBlock, fnReturn);
            loopDepth_--;
            popScope();
            break;
        }

        case StmtKind::Break:
            if (loopDepth_ == 0) {
                fail("'break' outside of a loop", stmt.line, stmt.col);
            }
            break;

        case StmtKind::Continue:
            if (loopDepth_ == 0) {
                fail("'continue' outside of a loop", stmt.line, stmt.col);
            }
            break;

        case StmtKind::Return: {
            if (stmt.expr == nullptr) {
                if (fnReturn != Type::Void) {
                    fail("non-void function must return a value", stmt.line,
                         stmt.col);
                }
            } else {
                Type valueType = checkExpr(*stmt.expr);
                requireUsable(*stmt.expr, valueType, "return");
                if (fnReturn == Type::Void) {
                    fail("void function cannot return a value", stmt.line,
                         stmt.col);
                }
                if (valueType != fnReturn) {
                    fail(std::string("return type mismatch: expected '") +
                             typeName(fnReturn) + "', got '" +
                             typeName(valueType) + "'",
                         stmt.line, stmt.col);
                }
            }
            break;
        }

        case StmtKind::ExprStmt: {
            Type t = checkExpr(*stmt.expr);
            requireUsable(*stmt.expr, t, "expression statement");
            break;
        }

        case StmtKind::Print: {
            Type t = checkExpr(*stmt.expr);
            if (t == Type::Str) break;  // print("...") ok
            if (t != Type::Int && t != Type::Bool) {
                fail(std::string("print expects int, bool or string, got '") +
                         typeName(t) + "'",
                     stmt.line, stmt.col);
            }
            break;
        }
    }
}

Type Sema::checkExpr(Expr& expr) {
    switch (expr.kind) {
        case ExprKind::IntLit:
            expr.type = Type::Int;
            return expr.type;
        case ExprKind::BoolLit:
            expr.type = Type::Bool;
            return expr.type;
        case ExprKind::StrLit:
            expr.type = Type::Str;
            expr.arraySize = 0;
            return expr.type;
        case ExprKind::ArrayLit:
            expr.type = checkArrayLit(expr);
            return expr.type;
        case ExprKind::Ident:
            expr.type = checkIdent(expr);
            return expr.type;
        case ExprKind::Index:
            expr.type = checkIndex(expr);
            return expr.type;
        case ExprKind::Unary:
            expr.type = checkUnary(expr);
            return expr.type;
        case ExprKind::Binary:
            expr.type = checkBinary(expr);
            return expr.type;
        case ExprKind::Call:
            expr.type = checkCall(expr);
            return expr.type;
    }
    return Type::Error;
}

Type Sema::checkIdent(Expr& expr) {
    VarInfo* var = lookup(expr.name);
    if (!var) {
        fail("use of undeclared variable '" + expr.name + "'", expr.line,
             expr.col);
    }
    var->used = true;
    expr.slot = var->slot;
    expr.arraySize = var->arraySize;
    return var->type;
}

Type Sema::checkArrayLit(Expr& expr) {
    if (expr.args.empty()) {
        fail("array literal must not be empty", expr.line, expr.col);
    }
    Type elem = checkExpr(*expr.args[0]);
    if (elem != Type::Int && elem != Type::Bool) {
        fail(std::string("array elements must be int or bool, got '") +
                 typeName(elem) + "'",
             expr.args[0]->line, expr.args[0]->col);
    }
    for (size_t i = 1; i < expr.args.size(); ++i) {
        Type t = checkExpr(*expr.args[i]);
        if (t != elem) {
            fail(std::string("array element ") + std::to_string(i + 1) +
                     " has type '" + typeName(t) + "', expected '" +
                     typeName(elem) + "' (all elements must match)",
                 expr.args[i]->line, expr.args[i]->col);
        }
    }
    expr.arraySize = static_cast<int>(expr.args.size());
    return elem == Type::Bool ? Type::ArrayBool : Type::ArrayInt;
}

Type Sema::checkIndex(Expr& expr) {
    Type base = checkExpr(*expr.lhs);
    Type idx = checkExpr(*expr.rhs);

    if (!isArrayType(base)) {
        fail(std::string("cannot index into a value of type '") +
                 typeName(base) + "'",
             expr.line, expr.col);
    }
    if (idx != Type::Int) {
        fail(std::string("array index must be int, got '") + typeName(idx) + "'",
             expr.rhs->line, expr.rhs->col);
    }
    expr.slot = expr.lhs->slot;
    expr.arraySize = expr.lhs->arraySize;
    return arrayElemType(base);
}

Type Sema::checkUnary(Expr& expr) {
    Type operand = checkExpr(*expr.lhs);
    switch (expr.op) {
        case TokenType::Bang:
            if (operand != Type::Bool) {
                fail(std::string("'!' expects bool, got '") +
                         typeName(operand) + "'",
                     expr.line, expr.col);
            }
            return Type::Bool;
        case TokenType::Minus:
            if (operand != Type::Int) {
                fail(std::string("unary '-' expects int, got '") +
                         typeName(operand) + "'",
                     expr.line, expr.col);
            }
            return Type::Int;
        default:
            fail("invalid unary operator", expr.line, expr.col);
    }
}

Type Sema::checkBinary(Expr& expr) {
    Type left = checkExpr(*expr.lhs);
    Type right = checkExpr(*expr.rhs);

    auto bothInt = [&]() {
        if (left != Type::Int || right != Type::Int) {
            fail(std::string("operator expects int operands, got '") +
                     typeName(left) + "' and '" + typeName(right) + "'",
                 expr.line, expr.col);
        }
    };
    auto bothBool = [&]() {
        if (left != Type::Bool || right != Type::Bool) {
            fail(std::string("operator expects bool operands, got '") +
                     typeName(left) + "' and '" + typeName(right) + "'",
                 expr.line, expr.col);
        }
    };

    switch (expr.op) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            bothInt();
            return Type::Int;
        case TokenType::Lt:
        case TokenType::Le:
        case TokenType::Gt:
        case TokenType::Ge:
            bothInt();
            return Type::Bool;
        case TokenType::EqEq:
        case TokenType::NotEq:
            if (left == Type::Str || isArrayType(left)) {
                fail("cannot compare strings or arrays with == / !=", expr.line,
                     expr.col);
            }
            if (left != right || left == Type::Void || left == Type::Error) {
                fail(std::string("cannot compare '") + typeName(left) +
                         "' with '" + typeName(right) + "'",
                     expr.line, expr.col);
            }
            return Type::Bool;
        case TokenType::AmpAmp:
        case TokenType::PipePipe:
            bothBool();
            return Type::Bool;
        default:
            fail("invalid binary operator", expr.line, expr.col);
    }
}

Type Sema::checkCall(Expr& expr) {
    if (expr.name == "print") {
        fail("print is a statement, not a function; use print(expr);",
             expr.line, expr.col);
    }

    if (expr.name == "len") {
        if (expr.args.size() != 1) {
            fail("'len' expects 1 argument(s), got " +
                     std::to_string(expr.args.size()),
                 expr.line, expr.col);
        }
        Type argType = checkExpr(*expr.args[0]);
        if (!isArrayType(argType)) {
            fail(std::string("'len' expects an array argument, got '") +
                     typeName(argType) + "'",
                 expr.args[0]->line, expr.args[0]->col);
        }
        // 0 ≥ 0 → size known at compile time; -1 → runtime length (slice)
        expr.arraySize = expr.args[0]->arraySize;
        return Type::Int;
    }

    for (size_t i = 0; i < fnNames_.size(); ++i) {
        if (fnNames_[i] == expr.name) {
            const FnInfo& info = fnInfos_[i];
            if (expr.args.size() != info.paramTypes.size()) {
                fail("function '" + expr.name + "' expects " +
                         std::to_string(info.paramTypes.size()) +
                         " argument(s), got " +
                         std::to_string(expr.args.size()),
                     expr.line, expr.col);
            }
            for (size_t a = 0; a < expr.args.size(); ++a) {
                Type argType = checkExpr(*expr.args[a]);
                if (isArrayType(info.paramTypes[a])) {
                    if (isArrayType(argType) &&
                        expr.args[a]->kind != ExprKind::Ident) {
                        fail("array argument " + std::to_string(a + 1) + " of '" +
                                 expr.name +
                                 "' must be a variable (assign the array first)",
                             expr.args[a]->line, expr.args[a]->col);
                    }
                } else {
                    requireUsable(*expr.args[a], argType, "argument");
                }
                if (argType != info.paramTypes[a]) {
                    fail("argument " + std::to_string(a + 1) + " of '" +
                             expr.name + "': expected '" +
                             typeName(info.paramTypes[a]) + "', got '" +
                             typeName(argType) + "'",
                         expr.args[a]->line, expr.args[a]->col);
                }
            }
            expr.fnIndex = info.index;
            return info.returnType;
        }
    }
    fail("call to undeclared function '" + expr.name + "'", expr.line,
         expr.col);
}
