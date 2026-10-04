#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ast.hpp"

class Codegen {
public:
    std::string emit(const Program& program);

private:
    void emitFunction(const FnDecl& fn, const Program& program);
    void emitStmt(const Stmt& stmt, const Program& program);
    void emitExpr(const Expr& expr);    // result in %rax
    void emitExprBool(const Expr& expr);
    void emitCall(const Expr& expr);

    void emitBoundsCheck(const Expr& indexExpr);  // index in %rax on entry
    void emitArrayBase(const Expr& arr, const std::string& reg);
    void emitArrayLength(const Expr& arr);        // result in %rax
    void emitDivGuard();                          // %rcx is the divisor; aborts if 0
    void emitIntOp(TokenType op);                 // lhs %rax, rhs %rcx → result %rax
    void emitAlignedCall(const std::string& target);  // uses current rsp state

    int slotOffset(int slot) const;
    std::string newLabel(const std::string& base);

    std::string out_;
    int labelCounter_ = 0;
    int stackDepth_ = 0;
    int strCounter_ = 0;
    std::vector<std::pair<std::string, std::string>> strings_;  // label, content

    // loop stack: {continueLabel, breakLabel}
    std::vector<std::pair<std::string, std::string>> loopStack_;
};
