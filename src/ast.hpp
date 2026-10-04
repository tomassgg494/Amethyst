#pragma once

#include <memory>
#include <string>
#include <vector>

#include "token.hpp"

// A type is a small value type: three words, copied and compared by value.
// Scalars carry only `kind`. An array also carries the kind (and struct id)
// of its element — arrays never nest, so nothing here is recursive. A struct
// carries its index in Program::structs.
struct Type {
    enum class Kind {
        Int,
        Bool,
        Float,   // 64-bit IEEE 754 binary64
        Void,
        Str,     // string value (immutable text in .rodata)
        Error,   // error-recovery sentinel; never reported to the user
        Array,   // int[N] / bool[N] / Point[] — the length lives on the declaration
        Struct,  // heap-allocated record
        Null,    // the `null` literal; only struct-typed slots accept it
    };

    Kind kind = Kind::Error;
    Kind elemKind = Kind::Error;  // Array: kind of the element type
    int id = -1;                  // Struct / Array-of-Struct: Program::structs index

    static const Type Int;
    static const Type Bool;
    static const Type Float;
    static const Type Void;
    static const Type Str;
    static const Type Null;
    static const Type Error;

    static Type arrayOf(const Type& elem) {
        return Type{Kind::Array, elem.kind,
                    elem.kind == Kind::Struct ? elem.id : -1};
    }

    static Type structOf(int structId) {
        return Type{Kind::Struct, Kind::Error, structId};
    }

    // Element type of an array type; Kind::Error for anything else.
    Type element() const {
        if (kind != Kind::Array) return Error;
        if (elemKind == Kind::Struct) return structOf(id);
        return Type{elemKind, Kind::Error, -1};
    }

    friend bool operator==(const Type& a, const Type& b) {
        return a.kind == b.kind && a.elemKind == b.elemKind && a.id == b.id;
    }
    friend bool operator!=(const Type& a, const Type& b) { return !(a == b); }
};

inline const Type Type::Int{Type::Kind::Int};
inline const Type Type::Bool{Type::Kind::Bool};
inline const Type Type::Float{Type::Kind::Float};
inline const Type Type::Void{Type::Kind::Void};
inline const Type Type::Str{Type::Kind::Str};
inline const Type Type::Null{Type::Kind::Null};
inline const Type Type::Error{Type::Kind::Error};

struct StructField {
    std::string name;
    Type type = Type::Error;
    int line = 0;
    int col = 0;
};

struct StructDecl {
    std::string name;
    std::vector<StructField> fields;
    int line = 0;
    int col = 0;
    int sizeBytes = 0;  // filled by sema: fields * 8 (all fields are one word)
};

// `structs` is passed in so a struct type can print as its declared name.
inline std::string typeName(const Type& t,
                            const std::vector<StructDecl>& structs) {
    switch (t.kind) {
        case Type::Kind::Int: return "int";
        case Type::Kind::Bool: return "bool";
        case Type::Kind::Float: return "float";
        case Type::Kind::Void: return "void";
        case Type::Kind::Str: return "string";
        case Type::Kind::Null: return "null";
        case Type::Kind::Error: return "<error>";
        case Type::Kind::Array:
            return typeName(t.element(), structs) + "[]";
        case Type::Kind::Struct:
            if (t.id >= 0 && t.id < static_cast<int>(structs.size())) {
                return structs[t.id].name;
            }
            return "<struct>";
    }
    return "?";
}

inline bool isArrayType(const Type& t) {
    return t.kind == Type::Kind::Array;
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
    Null,
    ArrayLit,
    Ident,
    Index,
    Field,   // obj.field
    Unary,
    Binary,
    Call,
    New,     // new Point { ... }
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
    // ArrayLit: args (elements); Index: lhs (array), rhs (index expr);
    // Field: lhs (object), name (field);
    // New: struct → args (values) with fieldNames parallel to them, in
    //      declaration order after sema; array → lhs is the element count
    //      and type.kind == Kind::Array tells the two apart
    ExprPtr lhs;
    ExprPtr rhs;
    std::vector<ExprPtr> args;
    std::vector<std::string> fieldNames;  // New only

    // Sema results.
    int slot = -1;         // Ident / Index base: frame slot
    int fnIndex = -1;      // Call: index into Program::functions
    int arraySize = 0;     // Array literal / array Ident / Index: element count
    int fieldOffset = -1;  // Field: byte offset inside the struct object

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
    static ExprPtr makeNull(int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Null;
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeField(ExprPtr object, std::string name, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::Field;
        e->lhs = std::move(object);
        e->name = std::move(name);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeNew(Type type, std::vector<ExprPtr> fields, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::New;
        e->type = type;
        e->args = std::move(fields);
        e->line = line;
        e->col = col;
        return e;
    }
    static ExprPtr makeNewArray(Type type, ExprPtr count, int line, int col) {
        auto e = std::make_unique<Expr>();
        e->kind = ExprKind::New;
        e->type = type;
        e->lhs = std::move(count);
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
    Assign,   // target is an lvalue expression: Ident, Index or Field chain
    If,
    While,
    For,
    Break,
    Continue,
    Return,
    ExprStmt,
    Print,
    Free,
    Block,
};

struct Stmt {
    StmtKind kind;
    int line = 0;
    int col = 0;

    // VarDecl / Assign
    std::string name;
    Type declaredType = Type::Void;  // VarDecl: written type; Assign(compound): operand type
    int declaredSize = 0;            // VarDecl arrays: N in int[N]
    bool typeInferred = false;       // VarDecl: written as `var x = e`

    // Assign: Eof for plain '=', otherwise the compound operator
    TokenType compoundOp = TokenType::Eof;

    // VarDecl / Return / ExprStmt / Print / If-cond / While-cond /
    // For range start (expr) and end (exprEnd)
    ExprPtr expr;
    ExprPtr exprEnd;

    // For
    std::string loopVar;
    int loopSlot = -1;
    int endSlot = -1;   // hidden slot holding range end value
    int stepSlot = -1;  // unused in v1.1 (step is always +1)

    // Assign: the lvalue (Ident / Index / Field chain) and the value;
    // Free: the lvalue to release
    ExprPtr target;
    ExprPtr value;

    // If / While / For / Block / ExprStmt-side structures
    StmtPtr thenBlock;  // If: then-branch; While/For: body
    StmtPtr elseBlock;  // If: else-branch, may be null

    // VarDecl / Assign: filled by sema
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
    static StmtPtr makeAssign(ExprPtr target, ExprPtr value, int line, int col,
                              TokenType compoundOp = TokenType::Eof) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Assign;
        s->target = std::move(target);
        s->value = std::move(value);
        s->compoundOp = compoundOp;
        s->line = line;
        s->col = col;
        return s;
    }
    static StmtPtr makeFree(ExprPtr target, int line, int col) {
        auto s = std::make_unique<Stmt>();
        s->kind = StmtKind::Free;
        s->target = std::move(target);
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
    std::vector<StructDecl> structs;
    std::vector<FnDecl> functions;
};
