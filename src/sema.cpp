#include "sema.hpp"

#include <algorithm>

// `int[]` is the generic (slice) spelling; locals are printed as `int[10]`.
static std::string fixedArrayName(const Type& elem, int n) {
    return typeName(elem, {}) + "[" + std::to_string(n) + "]";
}

void Sema::fail(const std::string& msg, int line, int col) const {
    throw SemaError(msg, line, col);
}

std::string Sema::tyName(const Type& t) const {
    static const std::vector<StructDecl> noStructs;
    return typeName(t, program_ ? program_->structs : noStructs);
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

Sema::FlowState Sema::snapshotFlow() const {
    FlowState st;
    for (const auto& scope : scopes_) {
        for (const auto& v : scope.vars) {
            st[{v.second.declLine, v.second.declCol}] = v.second.assigned;
        }
    }
    return st;
}

Sema::FlowState Sema::mergeFlow(const FlowState& a, const FlowState& b) const {
    FlowState out;
    for (const auto& scope : scopes_) {
        for (const auto& v : scope.vars) {
            auto key = std::make_pair(v.second.declLine, v.second.declCol);
            auto ia = a.find(key);
            auto ib = b.find(key);
            out[key] = (ia != a.end() && ib != b.end()) && ia->second && ib->second;
        }
    }
    return out;
}

void Sema::applyFlow(const FlowState& s) {
    for (auto& scope : scopes_) {
        for (auto& v : scope.vars) {
            auto key = std::make_pair(v.second.declLine, v.second.declCol);
            auto it = s.find(key);
            v.second.assigned = (it != s.end()) && it->second;
        }
    }
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
    program_ = &program;
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
        pi.assigned = true;
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
            if (stmt.expr == nullptr) {
                // `var x: T;` — parser guarantees an explicit non-array type
                VarInfo info{stmt.declaredType, nextSlot_++, 0};
                stmt.slot = info.slot;
                declare(stmt.name, info, stmt.line, stmt.col);
                break;
            }
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
                stmt.declaredType = initType;
                if (isArrayType(initType)) {
                    stmt.declaredSize = stmt.expr->arraySize;
                }
            } else if (isArrayType(stmt.declaredType)) {
                if (initType != stmt.declaredType) {
                    fail(std::string("cannot initialize '") +
                             fixedArrayName(stmt.declaredType.element(),
                                            stmt.declaredSize) +
                             " " + stmt.name + "' with value of type '" +
                             tyName(initType) + "'",
                         stmt.line, stmt.col);
                }
                if (stmt.expr->arraySize != stmt.declaredSize) {
                    fail(std::string("array size mismatch: '") + stmt.name + "' is " +
                             std::to_string(stmt.declaredSize) + " long, initializer has " +
                             std::to_string(stmt.expr->arraySize) + " elements",
                         stmt.line, stmt.col);
                }
            } else {
                if (initType == Type::Str && stmt.declaredType != Type::Str) {
                    fail(std::string("cannot initialize '") +
                             tyName(stmt.declaredType) + " " + stmt.name +
                             "' with a string literal",
                         stmt.line, stmt.col);
                }
                if (initType != stmt.declaredType) {
                    fail(std::string("cannot initialize '") +
                             tyName(stmt.declaredType) + " " + stmt.name +
                             "' with value of type '" + tyName(initType) + "'",
                         stmt.line, stmt.col);
                }
            }

            VarInfo info{stmt.declaredType, -1,
                         isArrayType(stmt.declaredType) ? stmt.declaredSize : 0};
            info.assigned = true;
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
            if (stmt.compoundOp != TokenType::Eof && !var->assigned) {
                fail("variable '" + stmt.name +
                         "' is read before it is definitely assigned",
                     stmt.line, stmt.col);
            }
            Type valueType = checkExpr(*stmt.expr);
            requireUsable(*stmt.expr, valueType, "assignment");
            if (stmt.compoundOp != TokenType::Eof) {
                bool bothInt =
                    var->type == Type::Int && valueType == Type::Int;
                bool bothFloat =
                    var->type == Type::Float && valueType == Type::Float;
                if (!bothInt && !bothFloat) {
                    if (var->type == Type::Float || valueType == Type::Float) {
                        fail(std::string("'") +
                                 compoundOpText(stmt.compoundOp) +
                                 "' expects floats on both sides (got '" +
                                 tyName(var->type) + "' and '" +
                                 tyName(valueType) + "')",
                             stmt.line, stmt.col);
                    }
                    fail(std::string("'") + compoundOpText(stmt.compoundOp) +
                             "' expects int on both sides (got '" +
                             tyName(var->type) + "' and '" +
                             tyName(valueType) + "')",
                         stmt.line, stmt.col);
                }
                stmt.declaredType = var->type;  // codegen: int or float op
                stmt.slot = var->slot;
                var->assigned = true;
                break;
            }
            if (valueType != var->type) {
                fail(std::string("cannot assign '") + tyName(valueType) +
                         "' to variable '" + stmt.name + "' of type '" +
                         tyName(var->type) + "'",
                     stmt.line, stmt.col);
            }
            stmt.slot = var->slot;
            var->assigned = true;
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
                             tyName(elemType) + "' and '" +
                             tyName(valueType) + "')",
                         stmt.line, stmt.col);
                }
                stmt.slot = stmt.target->slot;
                break;
            }
            if (valueType != elemType) {
                fail(std::string("cannot assign '") + tyName(valueType) +
                         "' to array element of type '" + tyName(elemType) + "'",
                     stmt.line, stmt.col);
            }
            stmt.slot = stmt.target->slot;
            break;
        }

        case StmtKind::If: {
            Type cond = checkExpr(*stmt.expr);
            if (cond != Type::Bool) {
                fail(std::string("if condition must be bool, got '") +
                         tyName(cond) + "'",
                     stmt.line, stmt.col);
            }
            FlowState before = snapshotFlow();
            checkBlock(*stmt.thenBlock, fnReturn);
            FlowState afterThen = snapshotFlow();

            // Both branches start from the same state; a variable counts as
            // assigned afterwards only if it is assigned on every path that
            // can get there. A branch that never completes is not a path.
            FlowState afterElse = before;  // implicit else assigns nothing
            if (stmt.elseBlock) {
                applyFlow(before);
                if (stmt.elseBlock->kind == StmtKind::Block) {
                    checkBlock(*stmt.elseBlock, fnReturn);
                } else {
                    checkStmt(*stmt.elseBlock, fnReturn);
                }
                afterElse = snapshotFlow();
            }

            bool thenFalls = !stmtAlwaysTerminates(*stmt.thenBlock);
            bool elseFalls =
                stmt.elseBlock ? !stmtAlwaysTerminates(*stmt.elseBlock) : true;
            if (thenFalls && elseFalls) {
                applyFlow(mergeFlow(afterThen, afterElse));
            } else if (thenFalls) {
                applyFlow(afterThen);   // else branch always terminates
            } else {
                applyFlow(afterElse);   // then branch always terminates
            }
            break;
        }

        case StmtKind::While: {
            Type cond = checkExpr(*stmt.expr);
            if (cond != Type::Bool) {
                fail(std::string("while condition must be bool, got '") +
                         tyName(cond) + "'",
                     stmt.line, stmt.col);
            }
            FlowState before = snapshotFlow();
            loopDepth_++;
            checkBlock(*stmt.thenBlock, fnReturn);
            loopDepth_--;
            // the body may run zero times, so it assigns nothing afterwards
            applyFlow(before);
            break;
        }

        case StmtKind::For: {
            Type startT = checkExpr(*stmt.expr);
            Type endT = checkExpr(*stmt.exprEnd);
            if (startT != Type::Int) {
                fail(std::string("for range start must be int, got '") +
                         tyName(startT) + "'",
                     stmt.line, stmt.col);
            }
            if (endT != Type::Int) {
                fail(std::string("for range end must be int, got '") +
                         tyName(endT) + "'",
                     stmt.line, stmt.col);
            }

            pushScope();
            stmt.loopSlot = nextSlot_++;
            stmt.endSlot = nextSlot_++;
            VarInfo loopVar{Type::Int, stmt.loopSlot, 0};
            loopVar.assigned = true;
            declare(stmt.loopVar, loopVar, stmt.line, stmt.col);

            FlowState before = snapshotFlow();
            loopDepth_++;
            checkBlock(*stmt.thenBlock, fnReturn);
            loopDepth_--;
            popScope();
            // the body may run zero times, so it assigns nothing afterwards
            applyFlow(before);
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
                             tyName(fnReturn) + "', got '" +
                             tyName(valueType) + "'",
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
            if (t != Type::Int && t != Type::Bool && t != Type::Float) {
                fail(std::string("print expects int, bool, float or string, got '") +
                         tyName(t) + "'",
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
        case ExprKind::FloatLit:
            expr.type = Type::Float;
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
    if (!var->assigned) {
        fail("variable '" + expr.name +
                 "' is read before it is definitely assigned",
             expr.line, expr.col);
    }
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
                 tyName(elem) + "'",
             expr.args[0]->line, expr.args[0]->col);
    }
    for (size_t i = 1; i < expr.args.size(); ++i) {
        Type t = checkExpr(*expr.args[i]);
        if (t != elem) {
            fail(std::string("array element ") + std::to_string(i + 1) +
                     " has type '" + tyName(t) + "', expected '" +
                     tyName(elem) + "' (all elements must match)",
                 expr.args[i]->line, expr.args[i]->col);
        }
    }
    expr.arraySize = static_cast<int>(expr.args.size());
    return Type::arrayOf(elem);
}

Type Sema::checkIndex(Expr& expr) {
    Type base = checkExpr(*expr.lhs);
    Type idx = checkExpr(*expr.rhs);

    if (!isArrayType(base)) {
        fail(std::string("cannot index into a value of type '") +
                 tyName(base) + "'",
             expr.line, expr.col);
    }
    if (idx != Type::Int) {
        fail(std::string("array index must be int, got '") + tyName(idx) + "'",
             expr.rhs->line, expr.rhs->col);
    }
    expr.slot = expr.lhs->slot;
    expr.arraySize = expr.lhs->arraySize;
    return base.element();
}

Type Sema::checkUnary(Expr& expr) {
    Type operand = checkExpr(*expr.lhs);
    switch (expr.op) {
        case TokenType::Bang:
            if (operand != Type::Bool) {
                fail(std::string("'!' expects bool, got '") +
                         tyName(operand) + "'",
                     expr.line, expr.col);
            }
            return Type::Bool;
        case TokenType::Minus:
            if (operand != Type::Int && operand != Type::Float) {
                fail(std::string("unary '-' expects int or float, got '") +
                         tyName(operand) + "'",
                     expr.line, expr.col);
            }
            return operand;
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
                     tyName(left) + "' and '" + tyName(right) + "'",
                 expr.line, expr.col);
        }
    };
    auto bothBool = [&]() {
        if (left != Type::Bool || right != Type::Bool) {
            fail(std::string("operator expects bool operands, got '") +
                     tyName(left) + "' and '" + tyName(right) + "'",
                 expr.line, expr.col);
        }
    };
    auto bothFloat = [&]() {
        if (left != Type::Float || right != Type::Float) {
            fail(std::string("operator expects two floats, got '") +
                     tyName(left) + "' and '" + tyName(right) + "'",
                 expr.line, expr.col);
        }
    };

    switch (expr.op) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
            if (left == Type::Float || right == Type::Float) {
                bothFloat();
                return Type::Float;
            }
            bothInt();
            return Type::Int;
        case TokenType::Percent:
            if (left == Type::Float || right == Type::Float) {
                fail("'%' has no float version (use int(x) to truncate)",
                     expr.line, expr.col);
            }
            bothInt();
            return Type::Int;
        case TokenType::Lt:
        case TokenType::Le:
        case TokenType::Gt:
        case TokenType::Ge:
            if (left == Type::Float || right == Type::Float) {
                bothFloat();
                return Type::Bool;
            }
            bothInt();
            return Type::Bool;
        case TokenType::EqEq:
        case TokenType::NotEq:
            if (left == Type::Float || right == Type::Float) {
                if (left != right) {
                    fail(std::string("cannot compare '") + tyName(left) +
                             "' with '" + tyName(right) + "' with == / !=",
                         expr.line, expr.col);
                }
                return Type::Bool;
            }
            if (left == Type::Str) {
                if (right != Type::Str) {
                    fail(std::string("cannot compare 'string' with '") +
                             tyName(right) + "' with == / !=",
                         expr.line, expr.col);
                }
                return Type::Bool;
            }
            if (isArrayType(left)) {
                fail("cannot compare arrays with == / !=", expr.line, expr.col);
            }
            if (left != right || left == Type::Void || left == Type::Error) {
                fail(std::string("cannot compare '") + tyName(left) +
                         "' with '" + tyName(right) + "'",
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
        if (!isArrayType(argType) && argType != Type::Str) {
            fail(std::string("'len' expects an array or string argument, got '") +
                     tyName(argType) + "'",
                 expr.args[0]->line, expr.args[0]->col);
        }
        // 0 ≥ 0 → size known at compile time; -1 → runtime length (slice);
        // strings always need strlen at runtime
        expr.arraySize =
            argType == Type::Str ? -1 : expr.args[0]->arraySize;
        return Type::Int;
    }

    // int(x) / float(x): the only conversions (no implicit widening)
    if (expr.name == "int" || expr.name == "float") {
        if (expr.args.size() != 1) {
            fail("'" + expr.name + "' expects 1 argument(s), got " +
                     std::to_string(expr.args.size()),
                 expr.line, expr.col);
        }
        Type argType = checkExpr(*expr.args[0]);
        if (expr.name == "float") {
            if (argType != Type::Int) {
                fail(std::string("'float' expects an int argument, got '") +
                         tyName(argType) + "'",
                     expr.args[0]->line, expr.args[0]->col);
            }
            return Type::Float;
        }
        if (argType != Type::Float) {
            fail(std::string("'int' expects a float argument, got '") +
                     tyName(argType) + "'",
                 expr.args[0]->line, expr.args[0]->col);
        }
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
                             tyName(info.paramTypes[a]) + "', got '" +
                             tyName(argType) + "'",
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
