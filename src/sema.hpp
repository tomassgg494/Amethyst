#pragma once

#include <stdexcept>
#include <string>

#include "ast.hpp"

struct SemaError : std::runtime_error {
    int line;
    int col;
    SemaError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

class Sema {
public:
    void analyze(Program& program);

private:
    void collectFunctions(Program& program);
    void checkMain(Program& program);
    void checkFunction(FnDecl& fn);

    struct FnInfo {
        Type returnType;
        std::vector<Type> paramTypes;
        int index;
    };

    struct VarInfo {
        Type type;
        int slot;
        int arraySize = 0;  // for array vars
    };

    struct Scope {
        std::vector<std::pair<std::string, VarInfo>> vars;  // ordered for stable errors
    };

    void pushScope();
    void popScope();
    VarInfo* lookup(const std::string& name);
    void declare(const std::string& name, const VarInfo& info, int line, int col);

    void checkStmt(Stmt& stmt, Type fnReturn);
    void checkBlock(Stmt& block, Type fnReturn);
    bool stmtAlwaysReturns(const Stmt& stmt) const;
    Type checkExpr(Expr& expr);
    Type checkBinary(Expr& expr);
    Type checkUnary(Expr& expr);
    Type checkCall(Expr& expr);
    Type checkIdent(Expr& expr);
    Type checkIndex(Expr& expr);
    Type checkArrayLit(Expr& expr);

    // Reject types that may not appear in general expression positions.
    void requireUsable(const Expr& expr, Type t, const char* what);

    [[noreturn]] void fail(const std::string& msg, int line, int col) const;

    std::vector<FnInfo> fnInfos_;
    std::vector<std::string> fnNames_;
    std::vector<Scope> scopes_;
    int nextSlot_ = 0;
    int loopDepth_ = 0;
};
