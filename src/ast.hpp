#pragma once

#include <memory>
#include <string>
#include <vector>

#include "token.hpp"

enum class Type {
    Int,
    Bool,
    Float,      // 64-bit IEEE 754 binary64
    Void,
    Str,        // string value (immutable text in .rodata)
    ArrayInt,   // int[N]
    ArrayBool,  // bool[N]
    Error,      // error-recovery sentinel; never reported to the user
};

inline const char* typeName(Type t) {
    switch (t) {
        case Type::Int: return "int";
        case Type::Bool: return "bool";
        case Type::Float: return "float";
        case Type::Void: return "void";
        case Type::Str: return "string";
        case Type::ArrayInt: return "int[]";
        case Type::ArrayBool: return "bool[]";
        case Type::Error: return "<error>";
    }
    return "?";
}

inline bool isArrayType(Type t) {
    return t == Type::ArrayInt || t == Type::ArrayBool;
}

inline Type arrayElemType(Type t) {
    return t == Type::ArrayBool ? Type::Bool : Type::Int;
}

struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

enum class ExprKind {
    IntLit,
    FloatLit,
    BoolLit,
    StrLit,
    ArrayLit,
    Ident,
    Index,
    Unary,
    Binary,
    Call,
};

struct Expr {
    ExprKind kind;
    int line = 0;
    int col = 0;

    // Filled by sema.
    Type type = Type::Error;

    // IntLit / BoolLit
    long long intValue = 0;
    bool boolValue = false;

    // FloatLit
    double floatValue = 0.0;

    // StrLit (decoded content, may contain \n etc.)
    std::string strValue;

    // Unary / Binary
    TokenType op = TokenType::Eof;

    // Ident / Call
    std::string name;

    // Unary: lhs; Binary: lhs, rhs; Call: args;
    // ArrayLit: args (elements); Index: lhs (array), rhs (index expr)
    ExprPtr lhs;
    ExprPtr rhs;
    std::vector<ExprPtr> args;

    // Sema results.
    int slot = -1;         // Ident / Index base: frame slot
    int fnIndex = -1;      // Call: index into Program::functions
    int arraySize = 0;     // Array literal / array Ident / Index: element count

    static ExprPtr makeInt(long long v, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::IntLit;
        e->intValue = v;
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeFloat(double v, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::FloatLit;
        e->floatValue = v;
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeBool(bool v, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::BoolLit;
        e->boolValue = v;
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeStr(std::string v, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::StrLit;
        e->strValue = std::move(v);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeArrayLit(std::vector<ExprPtr> elems, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::ArrayLit;
        e->args = std::move(elems);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeIdent(std::string name, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Ident;
        e->name = std::move(name);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeIndex(ExprPtr base, ExprPtr index, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Index;
        e->lhs = std::move(base);
        e->rhs = std::move(index);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeUnary(TokenType op, ExprPtr operand, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Unary;
        e->op = op;
        e->lhs = std::move(operand);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeBinary(TokenType op, ExprPtr lhs, ExprPtr rhs, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Binary;
        e->op = op;
        e->lhs = std::move(lhs);
        e->rhs = std::move(rhs);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeCall(std::string name, std::vector<ExprPtr> args, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Call;
        e->name = std::move(name);
        e->args = std::move(args);
        e->line = line;
        e->col = col;
        return e;
    }
};

enum class StmtKind {
    VarDecl,
    Assign,
    AssignIndex,  // arr[i] = value
    If,
    While,
    For,
    Break,
    Continue,
    Return,
    ExprStmt,
    Print,
    Block,
};

struct Stmt {
    StmtKind kind;
    int line = 0;
    int col = 0;

    // VarDecl / Assign / AssignIndex
    std::string name;
    Type declaredType = Type::Void;  // VarDecl: written type; Assign(compound): operand type
    int declaredSize = 0;            // VarDecl arrays: N in int[N]
    bool typeInferred = false;       // VarDecl: written as `var x = e`

    // Assign / AssignIndex: Eof for plain '=', otherwise the compound operator
    TokenType compoundOp = TokenType::Eof;

    // VarDecl / Assign / Return / ExprStmt / Print / If-cond / While-cond /
    // For range start (expr) and end (exprEnd)
    ExprPtr expr;
    ExprPtr exprEnd;

    // For
    std::string loopVar;
    int loopSlot = -1;
    int endSlot = -1;   // hidden slot holding range end value
    int stepSlot = -1;  // unused in v1.1 (step is always +1)

    // AssignIndex: arr and index
    ExprPtr target;   // Index expr (lhs=arr ident, rhs=index)
    ExprPtr value;    // value assigned

    // If / While / For / Block / ExprStmt-side structures
    StmtPtr thenBlock;  // If: then-branch; While/For: body
    StmtPtr elseBlock;  // If: else-branch, may be null

    // VarDecl / Assign / AssignIndex: filled by sema
    int slot = -1;

    // Block
    std::vector<StmtPtr> stmts;

    static StmtPtr makeVarDecl(std::string name, Type type, ExprPtr init, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::VarDecl;
        s->name = std::move(name);
        s->declaredType = type;
        s->expr = std::move(init);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeAssign(std::string name, ExprPtr value, int line, int col,
                              TokenType compoundOp = TokenType::Eof) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Assign;
        s->name = std::move(name);
        s->expr = std::move(value);
        s->compoundOp = compoundOp;
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeAssignIndex(ExprPtr target, ExprPtr value, int line, int col,
                                   TokenType compoundOp = TokenType::Eof) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::AssignIndex;
        s->target = std::move(target);
        s->value = std::move(value);
        s->compoundOp = compoundOp;
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeIf(ExprPtr cond, StmtPtr thenB, StmtPtr elseB, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::If;
        s->expr = std::move(cond);
        s->thenBlock = std::move(thenB);
        s->elseBlock = std::move(elseB);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeWhile(ExprPtr cond, StmtPtr body, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::While;
        s->expr = std::move(cond);
        s->thenBlock = std::move(body);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeFor(std::string var, ExprPtr start, ExprPtr end,
                           StmtPtr body, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::For;
        s->loopVar = std::move(var);
        s->expr = std::move(start);
        s->exprEnd = std::move(end);
        s->thenBlock = std::move(body);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeBreak(int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Break;
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeContinue(int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Continue;
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeReturn(ExprPtr value, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Return;
        s->expr = std::move(value);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeExprStmt(ExprPtr value, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::ExprStmt;
        s->expr = std::move(value);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makePrint(ExprPtr value, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Print;
        s->expr = std::move(value);
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeBlock(int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Block;
        s->line = line;
        s->col = col;
        return s;
    }
};

struct Param {
    std::string name;
    Type type = Type::Int;
    int line = 0;
    int col = 0;
    int slot = -1;  // filled by sema
};

struct FnDecl {
    std::string name;
    Type returnType = Type::Void;
    std::vector<Param> params;
    std::unique_ptr<Stmt> body;  // StmtKind::Block
    int line = 0;
    int col = 0;
    int slotCount = 0;  // filled by sema: total frame slots (params + locals)
};

struct Program {
    std::vector<FnDecl> functions;
};
