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
    void emitExpr(const Expr& expr);  // result in %rax (a float is its raw
                                      // 8-byte pattern, moved to %xmm0 only
                                      // for SSE operations and at call sites)
    void emitExprBool(const Expr& expr);
    void emitCall(const Expr& expr);

    void emitBoundsCheck(const Expr& indexExpr);  // index in %rax on entry
    void emitArrayBase(const Expr& arr, const std::string& reg);
    void emitArrayLength(const Expr& arr);        // result in %rax
    void emitDivGuard();                          // %rcx is the divisor; aborts if 0
    void emitIntOp(TokenType op);                 // lhs %rax, rhs %rcx → result %rax
    void emitFloatOp(TokenType op);               // lhs %xmm0, rhs %xmm1 → xmm0
    void emitAlignedCall(const std::string& target);  // uses current rsp state

    int slotOffset(int slot) const;
    std::string newLabel(const std::string& base);

    std::string out_;
    int labelCounter_ = 0;
    int stackDepth_ = 0;
    int strCounter_ = 0;
    int floatCounter_ = 0;
    Type currentReturn_ = Type::Void;  // return type of the function being emitted
    std::vector<std::pair<std::string, std::string>> strings_;  // label, content
    std::vector<std::pair<std::string, double>> floats_;        // label, value

    // loop stack: {continueLabel, breakLabel}
    std::vector<std::pair<std::string, std::string>> loopStack_;
};
