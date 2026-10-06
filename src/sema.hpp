#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "ast.hpp"

struct SemaError : std::runtime_error {
    int line;
    int col;
    SemaError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

struct SemaWarning {
    int line;
    int col;
    std::string msg;
};

class Sema {
public:
    void analyze(Program& program);
    const std::vector<SemaWarning>& warnings() const { return warnings_; }

private:
    void collectFunctions(Program& program);
    void collectImpls(Program& program);
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
        int arraySize = 0;  // for array vars: element count, -1 for a slice
        int declLine = 0;
        int declCol = 0;
        bool used = false;   // referenced after declaration (read or assigned)
        bool isParam = false;
        bool assigned = false;  // definitely assigned before any read
        bool heapArray = false;  // created with `new` in this function
        bool isSelf = false;     // the `self` parameter of a method
    };

    // One entry per method, grouped by the id of the struct it belongs to.
    struct MethodInfo {
        const FnDecl* fn;
        std::string typeName;  // struct name (messages and the symbol)
    };

    struct Scope {
        std::vector<std::pair<std::string, VarInfo>> vars;  // ordered for stable errors
    };

    void pushScope();
    void popScope();
    VarInfo* lookup(const std::string& name);
    void declare(const std::string& name, const VarInfo& info, int line, int col);

    // Definite-assignment: the state is one flag per live variable, keyed by
    // the declaration position (unique per `var`). Branches are analyzed from
    // the same state and their states are intersected where both can complete;
    // a loop body is analyzed from the state before it, because it may run
    // zero times, and the state after the loop is the state before it.
    using FlowState = std::map<std::pair<int, int>, bool>;
    FlowState snapshotFlow() const;
    FlowState mergeFlow(const FlowState& a, const FlowState& b) const;
    void applyFlow(const FlowState& s);

    void checkStmt(Stmt& stmt, Type fnReturn);
    void checkBlock(Stmt& block, Type fnReturn);
    bool stmtAlwaysReturns(const Stmt& stmt) const;
    bool stmtAlwaysTerminates(const Stmt& stmt) const;
    Type checkExpr(Expr& expr);
    Type checkBinary(Expr& expr);
    Type checkUnary(Expr& expr);
    Type checkCall(Expr& expr);
    Type checkIdent(Expr& expr);
    Type checkIndex(Expr& expr);
    Type checkField(Expr& expr);
    Type checkNew(Expr& expr);
    Type checkArrayLit(Expr& expr);

    // Display name of a type, with the program's struct table behind it.
    std::string tyName(const Type& t) const;

    // Reject types that may not appear in general expression positions.
    void requireUsable(const Expr& expr, Type t, const char* what);

    [[noreturn]] void fail(const std::string& msg, int line, int col) const;

    std::vector<FnInfo> fnInfos_;
    std::vector<std::string> fnNames_;
    std::vector<std::vector<MethodInfo>> methods_;  // struct id → methods
    std::vector<Scope> scopes_;
    std::vector<SemaWarning> warnings_;
    const Program* program_ = nullptr;  // valid for the current analyze()
    int nextSlot_ = 0;
    int loopDepth_ = 0;
};
