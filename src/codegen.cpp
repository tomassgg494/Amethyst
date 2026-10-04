#include "codegen.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

std::string Codegen::newLabel(const std::string& base) {
    return "." + base + "_" + std::to_string(labelCounter_++);
}

// Spelling that the assembler reads back as exactly the same double.
static std::string floatLiteral(double v) {
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10) << v;
    return ss.str();
}

int Codegen::slotOffset(int slot) const {
    return -8 * (slot + 1);
}

std::string Codegen::emit(const Program& program) {
    out_.clear();
    program_ = &program;
    labelCounter_ = 0;
    stackDepth_ = 0;
    strCounter_ = 0;
    floatCounter_ = 0;
    strings_.clear();
    floats_.clear();
    loopStack_.clear();

    out_ += "# Amethyst generated assembly (x86-64 System V)\n";
    out_ += ".section .note.GNU-stack,\"\",@progbits\n";
    out_ += ".section .rodata\n";
    out_ += ".fmt_int:\n";
    out_ += "    .string \"%ld\\n\"\n";
    out_ += ".fmt_float:\n";
    out_ += "    .string \"%.15g\\n\"\n";
    out_ += ".fmt_bounds:\n";
    out_ += "    .string \"Amethyst runtime error: index %ld out of bounds for array of size %ld\\n\"\n";
    out_ += ".fmt_divzero:\n";
    out_ += "    .string \"Amethyst runtime error: division by zero\\n\"\n";
    out_ += ".fmt_null:\n";
    out_ += "    .string \"Amethyst runtime error: null reference (%s at line %ld)\\n\"\n";
    out_ += ".fmt_oom:\n";
    out_ += "    .string \"Amethyst runtime error: out of memory\\n\"\n";
    out_ += ".fmt_size:\n";
    out_ += "    .string \"Amethyst runtime error: array size must be positive (got %ld)\\n\"\n";
    out_ += ".text\n";

    // shared bounds-failure handler (never returns)
    out_ += ".globl __amethyst_bounds_fail\n";
    out_ += ".type __amethyst_bounds_fail, @function\n";
    out_ += "__amethyst_bounds_fail:\n";
    out_ += "    subq $8, %rsp\n";            // entry rsp ≡ 8 (mod 16) → align for printf
    out_ += "    movq %rsi, %rdx\n";          // rdx = size
    out_ += "    movq %rdi, %rsi\n";          // rsi = index
    out_ += "    leaq .fmt_bounds(%rip), %rdi\n";
    out_ += "    xorl %eax, %eax\n";
    out_ += "    call printf@PLT\n";
    out_ += "    movl $1, %edi\n";
    out_ += "    call exit@PLT\n";
    out_ += ".size __amethyst_bounds_fail, .-__amethyst_bounds_fail\n";

    // shared division-by-zero handler (never returns)
    out_ += ".globl __amethyst_div_fail\n";
    out_ += ".type __amethyst_div_fail, @function\n";
    out_ += "__amethyst_div_fail:\n";
    out_ += "    subq $8, %rsp\n";            // entry rsp ≡ 8 (mod 16) → align for printf
    out_ += "    leaq .fmt_divzero(%rip), %rdi\n";
    out_ += "    xorl %eax, %eax\n";
    out_ += "    call printf@PLT\n";
    out_ += "    movl $1, %edi\n";
    out_ += "    call exit@PLT\n";
    out_ += ".size __amethyst_div_fail, .-__amethyst_div_fail\n";

    // shared null-reference handler (never returns)
    // entry: %rdi = description ("field 'x'"), %rsi = source line
    out_ += ".globl __amethyst_null_fail\n";
    out_ += ".type __amethyst_null_fail, @function\n";
    out_ += "__amethyst_null_fail:\n";
    out_ += "    subq $8, %rsp\n";            // entry rsp ≡ 8 (mod 16) → align for printf
    out_ += "    movq %rsi, %rdx\n";          // rdx = line
    out_ += "    movq %rdi, %rsi\n";          // rsi = description
    out_ += "    leaq .fmt_null(%rip), %rdi\n";
    out_ += "    xorl %eax, %eax\n";
    out_ += "    call printf@PLT\n";
    out_ += "    movl $1, %edi\n";
    out_ += "    call exit@PLT\n";
    out_ += ".size __amethyst_null_fail, .-__amethyst_null_fail\n";

    // shared out-of-memory handler (never returns)
    out_ += ".globl __amethyst_oom_fail\n";
    out_ += ".type __amethyst_oom_fail, @function\n";
    out_ += "__amethyst_oom_fail:\n";
    out_ += "    subq $8, %rsp\n";            // entry rsp ≡ 8 (mod 16) → align for printf
    out_ += "    leaq .fmt_oom(%rip), %rdi\n";
    out_ += "    xorl %eax, %eax\n";
    out_ += "    call printf@PLT\n";
    out_ += "    movl $1, %edi\n";
    out_ += "    call exit@PLT\n";
    out_ += ".size __amethyst_oom_fail, .-__amethyst_oom_fail\n";

    // shared negative-size handler for `new T[n]` (never returns)
    // entry: %rdi = requested element count
    out_ += ".globl __amethyst_size_fail\n";
    out_ += ".type __amethyst_size_fail, @function\n";
    out_ += "__amethyst_size_fail:\n";
    out_ += "    subq $8, %rsp\n";            // entry rsp ≡ 8 (mod 16) → align for printf
    out_ += "    movq %rdi, %rsi\n";          // rsi = size
    out_ += "    leaq .fmt_size(%rip), %rdi\n";
    out_ += "    xorl %eax, %eax\n";
    out_ += "    call printf@PLT\n";
    out_ += "    movl $1, %edi\n";
    out_ += "    call exit@PLT\n";
    out_ += ".size __amethyst_size_fail, .-__amethyst_size_fail\n";

    for (const auto& fn : program.functions) {
        emitFunction(fn, program);
    }

    if (!strings_.empty() || !floats_.empty()) {
        out_ += ".section .rodata\n";
        for (const auto& s : strings_) {
            out_ += s.first + ":\n";
            // escape for GAS .string
            std::string esc;
            for (char c : s.second) {
                switch (c) {
                    case '\\': esc += "\\\\"; break;
                    case '"': esc += "\\\""; break;
                    case '\n': esc += "\\n"; break;
                    case '\t': esc += "\\t"; break;
                    default: esc += c; break;
                }
            }
            out_ += "    .string \"" + esc + "\"\n";
        }
        for (const auto& f : floats_) {
            out_ += f.first + ":\n";
            out_ += "    .double " + floatLiteral(f.second) + "\n";
        }
        out_ += ".text\n";
    }
    return out_;
}

void Codegen::emitFunction(const FnDecl& fn, const Program& program) {
    stackDepth_ = 0;
    loopStack_.clear();
    currentReturn_ = fn.returnType;

    out_ += ".globl " + fn.name + "\n";
    out_ += ".type " + fn.name + ", @function\n";
    out_ += fn.name + ":\n";
    out_ += "    pushq %rbp\n";
    out_ += "    movq %rsp, %rbp\n";

    int frameBytes = fn.slotCount * 8;
    if (frameBytes % 16 != 0) frameBytes += 8;
    if (frameBytes > 0) {
        out_ += "    subq $" + std::to_string(frameBytes) + ", %rsp\n";
    }

    static const char* intRegs[6] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    static const char* sseRegs[8] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3",
                                     "%xmm4", "%xmm5", "%xmm6", "%xmm7"};
    int intIdx = 0;
    int sseIdx = 0;
    int stackOff = 16;
    for (const auto& p : fn.params) {
        // a slice is {ptr, len} and therefore occupies two integer units
        int units = isArrayType(p.type) ? 2 : 1;
        std::string ptrSlot = std::to_string(slotOffset(p.slot)) + "(%rbp)";
        std::string lenSlot =
            std::to_string(slotOffset(p.slot + 1)) + "(%rbp)";

        if (p.type == Type::Float) {
            if (sseIdx < 8) {
                out_ += "    movq " + std::string(sseRegs[sseIdx]) + ", " +
                        ptrSlot + "\n";
                sseIdx++;
            } else {
                out_ += "    movq " + std::to_string(stackOff) + "(%rbp), %rax\n";
                out_ += "    movq %rax, " + ptrSlot + "\n";
                stackOff += 8;
            }
            continue;
        }

        if (intIdx + units <= 6) {
            out_ += "    movq " + std::string(intRegs[intIdx]) + ", " + ptrSlot + "\n";
            if (units == 2) {
                out_ += "    movq " + std::string(intRegs[intIdx + 1]) + ", " +
                        lenSlot + "\n";
            }
            intIdx += units;
        } else {
            out_ += "    movq " + std::to_string(stackOff) + "(%rbp), %rax\n";
            out_ += "    movq %rax, " + ptrSlot + "\n";
            if (units == 2) {
                out_ += "    movq " + std::to_string(stackOff + 8) + "(%rbp), %rax\n";
                out_ += "    movq %rax, " + lenSlot + "\n";
            }
            stackOff += units * 8;
        }
    }

    emitStmt(*fn.body, program);

    out_ += "    movq $0, %rax\n";
    if (fn.returnType == Type::Float) {
        out_ += "    movq %rax, %xmm0\n";  // float results come back in %xmm0
    }
    out_ += "    leave\n";
    out_ += "    ret\n";
    out_ += ".size " + fn.name + ", .-" + fn.name + "\n";
}

void Codegen::emitBoundsCheck(const Expr& idx) {
    // index in %rax; idx carries slot + arraySize (-1 → length in memory)
    std::string okL = newLabel("idx_ok");
    std::string failL = newLabel("idx_fail");
    std::string lenRef =
        std::to_string(slotOffset(idx.slot) - 8) + "(%rbp)";

    out_ += "    cmpq $0, %rax\n";
    out_ += "    jl " + failL + "\n";
    if (idx.arraySize >= 0) {
        out_ += "    cmpq $" + std::to_string(idx.arraySize) + ", %rax\n";
    } else {
        out_ += "    cmpq " + lenRef + ", %rax\n";
    }
    out_ += "    jge " + failL + "\n";
    out_ += "    jmp " + okL + "\n";
    out_ += failL + ":\n";
    out_ += "    movq %rax, %rdi\n";  // index
    if (idx.arraySize >= 0) {
        out_ += "    movq $" + std::to_string(idx.arraySize) + ", %rsi\n";
    } else {
        out_ += "    movq " + lenRef + ", %rsi\n";
    }
    // Force 16-byte alignment regardless of live pushes, then call (noreturn).
    emitAlignedCall("__amethyst_bounds_fail");
    out_ += okL + ":\n";
}

void Codegen::emitArrayBase(const Expr& arr, const std::string& reg) {
    // local array → address of element 0; slice → the stored pointer
    std::string slot = std::to_string(slotOffset(arr.slot)) + "(%rbp)";
    if (arr.arraySize >= 0) {
        out_ += "    leaq " + slot + ", " + reg + "\n";
    } else {
        out_ += "    movq " + slot + ", " + reg + "\n";
    }
}

void Codegen::emitArrayLength(const Expr& arr) {
    if (arr.arraySize >= 0) {
        out_ += "    movq $" + std::to_string(arr.arraySize) + ", %rax\n";
    } else {
        out_ += "    movq " + std::to_string(slotOffset(arr.slot) - 8) +
                "(%rbp), %rax\n";
    }
}

void Codegen::emitAddr(const Expr& lvalue) {
    // result in %rax; %rcx is clobbered
    switch (lvalue.kind) {
        case ExprKind::Ident:
            out_ += "    leaq " + std::to_string(slotOffset(lvalue.slot)) +
                    "(%rbp), %rax\n";
            break;

        case ExprKind::Field: {
            // the base can be any expression that yields an object pointer
            // (a variable, an element, a nested field or a call result)
            emitExpr(*lvalue.lhs);
            emitNullCheck(lvalue.line, "field '" + lvalue.name + "'");
            out_ += "    addq $" + std::to_string(lvalue.fieldOffset) +
                    ", %rax\n";
            break;
        }

        case ExprKind::Index: {
            emitExpr(*lvalue.rhs);  // index → %rax
            emitBoundsCheck(lvalue);
            out_ += "    shlq $3, %rax\n";
            emitArrayBase(lvalue, "%rcx");
            out_ += "    subq %rax, %rcx\n";
            out_ += "    movq %rcx, %rax\n";
            break;
        }

        default:
            out_ += "    # invalid assignment target\n";
            break;
    }
}

void Codegen::emitNewArray(const Expr& expr, int slot) {
    // `new T[n]`: zeroed with calloc, stored like a slice — pointer in
    // `slot`, element count in `slot + 1`, and element 0 at the highest
    // address so that element i lives at base - 8*i (as everywhere else)
    emitExpr(*expr.lhs);  // count → %rax

    std::string sizeOk = newLabel("size_ok");
    out_ += "    cmpq $0, %rax\n";
    out_ += "    jg " + sizeOk + "\n";
    out_ += "    movq %rax, %rdi\n";  // the offending size
    emitAlignedCall("__amethyst_size_fail");
    out_ += sizeOk + ":\n";

    out_ += "    movq %rax, %rdi\n";  // nmemb
    out_ += "    movq $8, %rsi\n";    // size of one element
    out_ += "    pushq %rax\n";       // keep the count across the call
    stackDepth_++;
    emitAlignedCall("calloc");

    std::string allocOk = newLabel("alloc_ok");
    out_ += "    testq %rax, %rax\n";
    out_ += "    jne " + allocOk + "\n";
    emitAlignedCall("__amethyst_oom_fail");
    out_ += allocOk + ":\n";

    out_ += "    movq %rax, %r11\n";  // raw block
    out_ += "    popq %rax\n";        // count
    stackDepth_--;
    out_ += "    movq %rax, " +
            std::to_string(slotOffset(slot + 1)) + "(%rbp)\n";  // length
    out_ += "    shlq $3, %rax\n";
    out_ += "    addq %rax, %r11\n";
    out_ += "    subq $8, %r11\n";  // base = raw + 8*(count - 1)
    out_ += "    movq %r11, " + std::to_string(slotOffset(slot)) + "(%rbp)\n";
}

void Codegen::emitNullCheck(int line, const std::string& what) {
    // pointer in %rax
    std::string okL = newLabel("not_null");
    out_ += "    testq %rax, %rax\n";
    out_ += "    jne " + okL + "\n";
    std::string label = ".Lnull_" + std::to_string(strCounter_++);
    strings_.push_back({label, what});
    out_ += "    leaq " + label + "(%rip), %rdi\n";
    out_ += "    movq $" + std::to_string(line) + ", %rsi\n";
    emitAlignedCall("__amethyst_null_fail");  // never returns
    out_ += okL + ":\n";
}

void Codegen::emitAlignedCall(const std::string& target) {
    // rsp must be 16-byte aligned at the call. Park it in %rbx, which is
    // callee-saved and therefore still valid after the call (unlike %r11),
    // then restore the frame exactly — also when rsp had to shift by 8.
    out_ += "    pushq %rbx\n";
    out_ += "    movq %rsp, %rbx\n";
    out_ += "    andq $-16, %rsp\n";
    out_ += "    call " + target + "@PLT\n";
    out_ += "    movq %rbx, %rsp\n";
    out_ += "    popq %rbx\n";
}

void Codegen::emitDivGuard() {
    // divisor in %rcx
    std::string okL = newLabel("div_ok");

    out_ += "    testq %rcx, %rcx\n";
    out_ += "    jne " + okL + "\n";
    // Force 16-byte alignment regardless of live pushes, then call (noreturn).
    emitAlignedCall("__amethyst_div_fail");
    out_ += okL + ":\n";
}

void Codegen::emitIntOp(TokenType op) {
    // lhs in %rax, rhs in %rcx → result in %rax
    // accepts both the plain and the compound spelling of the operator
    switch (op) {
        case TokenType::Plus:
        case TokenType::PlusEq:
            out_ += "    addq %rcx, %rax\n";
            break;
        case TokenType::Minus:
        case TokenType::MinusEq:
            out_ += "    subq %rcx, %rax\n";
            break;
        case TokenType::Star:
        case TokenType::StarEq:
            out_ += "    imulq %rcx, %rax\n";
            break;
        case TokenType::Slash:
        case TokenType::SlashEq:
            emitDivGuard();
            out_ += "    cqto\n";
            out_ += "    idivq %rcx\n";
            break;
        case TokenType::Percent:
        case TokenType::PercentEq:
            emitDivGuard();
            out_ += "    cqto\n";
            out_ += "    idivq %rcx\n";
            out_ += "    movq %rdx, %rax\n";
            break;
        default:
            break;
    }
}

void Codegen::emitFloatOp(TokenType op) {
    // lhs in %rax, rhs in %rcx (raw double patterns) → result in %rax
    out_ += "    movq %rax, %xmm0\n";
    out_ += "    movq %rcx, %xmm1\n";
    switch (op) {
        case TokenType::Plus:
        case TokenType::PlusEq:
            out_ += "    addsd %xmm1, %xmm0\n";
            break;
        case TokenType::Minus:
        case TokenType::MinusEq:
            out_ += "    subsd %xmm1, %xmm0\n";
            break;
        case TokenType::Star:
        case TokenType::StarEq:
            out_ += "    mulsd %xmm1, %xmm0\n";
            break;
        case TokenType::Slash:
        case TokenType::SlashEq:
            // IEEE-754: x/0.0 yields ±inf (or NaN), never a trap
            out_ += "    divsd %xmm1, %xmm0\n";
            break;
        default:
            out_ += "    # unhandled float operator\n";
            break;
    }
    out_ += "    movq %xmm0, %rax\n";
}

void Codegen::emitStmt(const Stmt& stmt, const Program& program) {
    switch (stmt.kind) {
        case StmtKind::Block: {
            for (const auto& s : stmt.stmts) emitStmt(*s, program);
            break;
        }

        case StmtKind::VarDecl: {
            if (isArrayType(stmt.declaredType)) {
                if (stmt.expr->kind == ExprKind::New) {
                    // heap array: {pointer, length} in two slots
                    emitNewArray(*stmt.expr, stmt.slot);
                } else {
                    // store each element into consecutive slots
                    for (size_t i = 0; i < stmt.expr->args.size(); ++i) {
                        emitExpr(*stmt.expr->args[i]);
                        out_ += "    movq %rax, " +
                                std::to_string(
                                    slotOffset(stmt.slot + static_cast<int>(i))) +
                                "(%rbp)\n";
                    }
                }
            } else if (stmt.expr) {
                emitExpr(*stmt.expr);
                out_ += "    movq %rax, " +
                        std::to_string(slotOffset(stmt.slot)) + "(%rbp)\n";
            } else {
                // `var x: T;` — sema guarantees no read before the first store
                out_ += "    movq $0, " +
                        std::to_string(slotOffset(stmt.slot)) + "(%rbp)\n";
            }
            break;
        }

        case StmtKind::Assign: {
            // the value is evaluated first (as before for `arr[i] = ...`),
            // then the address of the target is computed
            emitExpr(*stmt.value);
            out_ += "    pushq %rax\n";
            stackDepth_++;
            emitAddr(*stmt.target);  // %rax = &target
            out_ += "    movq %rax, %rcx\n";

            if (stmt.compoundOp != TokenType::Eof) {
                // stack: [rhs]; rcx = &target
                out_ += "    pushq %rcx\n";
                stackDepth_++;
                out_ += "    movq (%rcx), %rax\n";   // old value
                out_ += "    movq 8(%rsp), %rcx\n";  // saved rhs
                if (stmt.declaredType == Type::Float) {
                    emitFloatOp(stmt.compoundOp);
                } else {
                    emitIntOp(stmt.compoundOp);
                }
                out_ += "    popq %rcx\n";
                stackDepth_--;
                out_ += "    movq %rax, (%rcx)\n";
                out_ += "    addq $8, %rsp\n";  // drop saved rhs
                stackDepth_--;
                break;
            }
            out_ += "    popq %rax\n";
            stackDepth_--;
            out_ += "    movq %rax, (%rcx)\n";
            break;
        }

        case StmtKind::Free: {
            emitAddr(*stmt.target);  // %rax = &lvalue
            out_ += "    pushq %rax\n";
            stackDepth_++;

            if (stmt.target->type.kind == Type::Kind::Array) {
                // the block starts below the base: raw = base - 8*(len-1);
                // a null pointer (already freed) is a no-op
                std::string skipL = newLabel("free_skip");
                out_ += "    movq (%rax), %rdi\n";
                out_ += "    testq %rdi, %rdi\n";
                out_ += "    je " + skipL + "\n";
                out_ += "    movq -8(%rax), %rcx\n";  // length
                out_ += "    shlq $3, %rcx\n";
                out_ += "    subq %rcx, %rdi\n";
                out_ += "    addq $8, %rdi\n";  // raw block
                emitAlignedCall("free");
                out_ += skipL + ":\n";
                out_ += "    popq %rax\n";
                stackDepth_--;
                out_ += "    movq $0, (%rax)\n";    // pointer = null
                out_ += "    movq $0, -8(%rax)\n";  // length = 0
                break;
            }

            out_ += "    movq (%rax), %rdi\n";
            emitAlignedCall("free");
            out_ += "    popq %rax\n";
            stackDepth_--;
            out_ += "    movq $0, (%rax)\n";  // the variable is now null
            break;
        }

        case StmtKind::If: {
            std::string elseL = newLabel("if_else");
            std::string endL = newLabel("if_end");
            emitExprBool(*stmt.expr);
            out_ += "    cmpq $0, %rax\n";
            out_ += "    je " + elseL + "\n";
            emitStmt(*stmt.thenBlock, program);
            out_ += "    jmp " + endL + "\n";
            out_ += elseL + ":\n";
            if (stmt.elseBlock) emitStmt(*stmt.elseBlock, program);
            out_ += endL + ":\n";
            break;
        }

        case StmtKind::While: {
            std::string condL = newLabel("while_cond");
            std::string endL = newLabel("while_end");
            out_ += condL + ":\n";
            emitExprBool(*stmt.expr);
            out_ += "    cmpq $0, %rax\n";
            out_ += "    je " + endL + "\n";
            loopStack_.push_back({condL, endL});  // {continue, break}
            emitStmt(*stmt.thenBlock, program);
            loopStack_.pop_back();
            out_ += "    jmp " + condL + "\n";
            out_ += endL + ":\n";
            break;
        }

        case StmtKind::For: {
            std::string condL = newLabel("for_cond");
            std::string contL = newLabel("for_cont");
            std::string endL = newLabel("for_end");

            // loop var = start; endSlot = end  (end evaluated once)
            emitExpr(*stmt.expr);
            out_ += "    movq %rax, " +
                    std::to_string(slotOffset(stmt.loopSlot)) + "(%rbp)\n";
            emitExpr(*stmt.exprEnd);
            out_ += "    movq %rax, " +
                    std::to_string(slotOffset(stmt.endSlot)) + "(%rbp)\n";

            out_ += condL + ":\n";
            out_ += "    movq " + std::to_string(slotOffset(stmt.loopSlot)) +
                    "(%rbp), %rax\n";
            out_ += "    cmpq " + std::to_string(slotOffset(stmt.endSlot)) +
                    "(%rbp), %rax\n";
            out_ += "    jge " + endL + "\n";  // signed: i >= end → done

            loopStack_.push_back({contL, endL});
            emitStmt(*stmt.thenBlock, program);
            loopStack_.pop_back();

            out_ += contL + ":\n";
            out_ += "    addq $1, " +
                    std::to_string(slotOffset(stmt.loopSlot)) + "(%rbp)\n";
            out_ += "    jmp " + condL + "\n";
            out_ += endL + ":\n";
            break;
        }

        case StmtKind::Break: {
            out_ += "    jmp " + loopStack_.back().second + "\n";
            break;
        }

        case StmtKind::Continue: {
            out_ += "    jmp " + loopStack_.back().first + "\n";
            break;
        }

        case StmtKind::Return: {
            if (stmt.expr) {
                emitExpr(*stmt.expr);
            } else {
                out_ += "    movq $0, %rax\n";
            }
            if (currentReturn_ == Type::Float) {
                out_ += "    movq %rax, %xmm0\n";  // float results come back in %xmm0
            }
            out_ += "    leave\n";
            out_ += "    ret\n";
            break;
        }

        case StmtKind::ExprStmt: {
            emitExpr(*stmt.expr);
            break;
        }

        case StmtKind::Print: {
            if (stmt.expr->type == Type::Str) {
                emitExpr(*stmt.expr);  // %rax = pointer to NUL-terminated text
                out_ += "    movq %rax, %rdi\n";
                out_ += "    call puts@PLT\n";
            } else if (stmt.expr->type == Type::Float) {
                emitExpr(*stmt.expr);  // %rax = raw double pattern
                out_ += "    movq %rax, %xmm0\n";
                out_ += "    leaq .fmt_float(%rip), %rdi\n";
                out_ += "    movl $1, %eax\n";  // one vector argument
                out_ += "    call printf@PLT\n";
            } else {
                emitExpr(*stmt.expr);
                out_ += "    leaq .fmt_int(%rip), %rdi\n";
                out_ += "    movq %rax, %rsi\n";
                out_ += "    xorl %eax, %eax\n";
                out_ += "    call printf@PLT\n";
            }
            break;
        }
    }
}

void Codegen::emitExpr(const Expr& expr) {
    switch (expr.kind) {
        case ExprKind::IntLit:
            out_ += "    movq $" + std::to_string(expr.intValue) + ", %rax\n";
            break;

        case ExprKind::BoolLit:
            out_ += "    movq $" + std::string(expr.boolValue ? "1" : "0") +
                    ", %rax\n";
            break;

        case ExprKind::FloatLit: {
            // raw 8-byte pattern, integer-loaded into %rax
            std::string label = ".Lf_" + std::to_string(floatCounter_++);
            floats_.push_back({label, expr.floatValue});
            out_ += "    movq " + label + "(%rip), %rax\n";
            break;
        }

        case ExprKind::StrLit:
            // only reachable if sema allowed it (print handles separately;
            // defensive: treat as empty string via label)
            {
                std::string label = ".Lstr_" + std::to_string(strCounter_++);
                strings_.push_back({label, expr.strValue});
                out_ += "    leaq " + label + "(%rip), %rax\n";
            }
            break;

        case ExprKind::ArrayLit:
            // only as initializer (handled in VarDecl); defensive no-op
            out_ += "    leaq 0, %rax\n";
            break;

        case ExprKind::Ident:
            if (isArrayType(expr.type)) {
                // local array → address of element 0; slice → stored pointer
                emitArrayBase(expr, "%rax");
            } else {
                out_ += "    movq " + std::to_string(slotOffset(expr.slot)) +
                        "(%rbp), %rax\n";
            }
            break;

        case ExprKind::Index: {
            emitExpr(*expr.rhs);  // index → rax
            emitBoundsCheck(expr);
            out_ += "    shlq $3, %rax\n";
            emitArrayBase(expr, "%rcx");
            out_ += "    subq %rax, %rcx\n";
            out_ += "    movq (%rcx), %rax\n";
            break;
        }

        case ExprKind::Null:
            out_ += "    movq $0, %rax\n";
            break;

        case ExprKind::Field:
            emitAddr(expr);
            out_ += "    movq (%rax), %rax\n";
            break;

        case ExprKind::New: {
            if (expr.type.kind == Type::Kind::Array) {
                // sema only accepts `new T[n]` as a variable initializer,
                // which is emitted by emitNewArray
                out_ += "    # array 'new' outside a declaration\n";
                break;
            }
            // a struct is one heap block, one 8-byte slot per field
            const StructDecl& sd = program_->structs[expr.type.id];
            int bytes = sd.sizeBytes;
            if (bytes == 0) bytes = static_cast<int>(sd.fields.size()) * 8;

            for (const auto& field : expr.args) {
                emitExpr(*field);
                out_ += "    pushq %rax\n";
                stackDepth_++;
            }
            out_ += "    movq $" + std::to_string(bytes) + ", %rdi\n";
            emitAlignedCall("malloc");

            std::string okL = newLabel("alloc_ok");
            out_ += "    testq %rax, %rax\n";
            out_ += "    jne " + okL + "\n";
            emitAlignedCall("__amethyst_oom_fail");
            out_ += okL + ":\n";

            // %r11 = object; the field values come back off the stack in
            // reverse (sema wrote them in declaration order)
            out_ += "    movq %rax, %r11\n";
            for (int i = static_cast<int>(expr.args.size()) - 1; i >= 0; --i) {
                out_ += "    popq %rax\n";
                stackDepth_--;
                out_ += "    movq %rax, " + std::to_string(i * 8) +
                        "(%r11)\n";
            }
            out_ += "    movq %r11, %rax\n";
            break;
        }

        case ExprKind::Unary: {
            emitExpr(*expr.lhs);
            if (expr.op == TokenType::Minus) {
                if (expr.type == Type::Float) {
                    // flip the IEEE sign bit; arithmetic neg is wrong for doubles
                    out_ += "    movabsq $0x8000000000000000, %rcx\n";
                    out_ += "    xorq %rcx, %rax\n";
                } else {
                    out_ += "    negq %rax\n";
                }
            } else {
                out_ += "    cmpq $0, %rax\n";
                out_ += "    sete %al\n";
                out_ += "    movzbq %al, %rax\n";
            }
            break;
        }

        case ExprKind::Binary: {
            TokenType op = expr.op;
            if (op == TokenType::AmpAmp || op == TokenType::PipePipe) {
                std::string shortL =
                    newLabel(op == TokenType::AmpAmp ? "and_false" : "or_true");
                std::string endL =
                    newLabel(op == TokenType::AmpAmp ? "and_end" : "or_end");

                emitExprBool(*expr.lhs);
                out_ += "    cmpq $0, %rax\n";
                if (op == TokenType::AmpAmp) {
                    out_ += "    je " + shortL + "\n";
                } else {
                    out_ += "    jne " + shortL + "\n";
                }
                emitExprBool(*expr.rhs);
                out_ += "    cmpq $0, %rax\n";
                out_ += "    setne %al\n";
                out_ += "    movzbq %al, %rax\n";
                out_ += "    jmp " + endL + "\n";
                out_ += shortL + ":\n";
                if (op == TokenType::AmpAmp) {
                    out_ += "    movq $0, %rax\n";
                } else {
                    out_ += "    movq $1, %rax\n";
                }
                out_ += endL + ":\n";
                break;
            }

            emitExpr(*expr.lhs);
            out_ += "    pushq %rax\n";
            stackDepth_++;
            emitExpr(*expr.rhs);
            out_ += "    movq %rax, %rcx\n";
            out_ += "    popq %rax\n";
            stackDepth_--;

            // strings compare by content, not by pointer
            if (expr.lhs->type == Type::Str &&
                (op == TokenType::EqEq || op == TokenType::NotEq)) {
                out_ += "    movq %rax, %rdi\n";  // lhs → rdi
                out_ += "    movq %rcx, %rsi\n";  // rhs → rsi
                emitAlignedCall("strcmp");
                out_ += "    testl %eax, %eax\n";
                out_ += std::string("    ") +
                        (op == TokenType::EqEq ? "sete" : "setne") + " %al\n";
                out_ += "    movzbq %al, %rax\n";
                break;
            }

            // doubles: load the raw patterns and let SSE do the work
            if (expr.lhs->type == Type::Float) {
                bool isCmp = op == TokenType::Lt || op == TokenType::Le ||
                             op == TokenType::Gt || op == TokenType::Ge ||
                             op == TokenType::EqEq || op == TokenType::NotEq;
                if (isCmp) {
                    out_ += "    movq %rax, %xmm0\n";  // lhs
                    out_ += "    movq %rcx, %xmm1\n";  // rhs
                    out_ += "    ucomisd %xmm1, %xmm0\n";
                    // a comparison with NaN is false for < <= > >= == and
                    // true for !=, so the unordered flag (PF) is folded in
                    switch (op) {
                        case TokenType::Lt:
                            out_ += "    setb %al\n    setnp %cl\n";
                            out_ += "    andb %cl, %al\n";
                            break;
                        case TokenType::Le:
                            out_ += "    setbe %al\n    setnp %cl\n";
                            out_ += "    andb %cl, %al\n";
                            break;
                        case TokenType::Gt:
                            out_ += "    seta %al\n";
                            break;
                        case TokenType::Ge:
                            out_ += "    setae %al\n";
                            break;
                        case TokenType::EqEq:
                            out_ += "    sete %al\n    setnp %cl\n";
                            out_ += "    andb %cl, %al\n";
                            break;
                        default:
                            out_ += "    setne %al\n    setp %cl\n";
                            out_ += "    orb %cl, %al\n";
                            break;
                    }
                    out_ += "    movzbq %al, %rax\n";
                } else {
                    emitFloatOp(op);
                }
                break;
            }

            switch (op) {
                case TokenType::Plus:
                    out_ += "    addq %rcx, %rax\n";
                    break;
                case TokenType::Minus:
                    out_ += "    subq %rcx, %rax\n";
                    break;
                case TokenType::Star:
                    out_ += "    imulq %rcx, %rax\n";
                    break;
                case TokenType::Slash:
                    emitDivGuard();
                    out_ += "    cqto\n";
                    out_ += "    idivq %rcx\n";
                    break;
                case TokenType::Percent:
                    emitDivGuard();
                    out_ += "    cqto\n";
                    out_ += "    idivq %rcx\n";
                    out_ += "    movq %rdx, %rax\n";
                    break;
                case TokenType::EqEq:
                    out_ += "    cmpq %rcx, %rax\n";
                    out_ += "    sete %al\n";
                    out_ += "    movzbq %al, %rax\n";
                    break;
                case TokenType::NotEq:
                    out_ += "    cmpq %rcx, %rax\n";
                    out_ += "    setne %al\n";
                    out_ += "    movzbq %al, %rax\n";
                    break;
                case TokenType::Lt:
                    out_ += "    cmpq %rcx, %rax\n";
                    out_ += "    setl %al\n";
                    out_ += "    movzbq %al, %rax\n";
                    break;
                case TokenType::Le:
                    out_ += "    cmpq %rcx, %rax\n";
                    out_ += "    setle %al\n";
                    out_ += "    movzbq %al, %rax\n";
                    break;
                case TokenType::Gt:
                    out_ += "    cmpq %rcx, %rax\n";
                    out_ += "    setg %al\n";
                    out_ += "    movzbq %al, %rax\n";
                    break;
                case TokenType::Ge:
                    out_ += "    cmpq %rcx, %rax\n";
                    out_ += "    setge %al\n";
                    out_ += "    movzbq %al, %rax\n";
                    break;
                default:
                    out_ += "    # unhandled binary op\n";
                    break;
            }
            break;
        }

        case ExprKind::Call:
            emitCall(expr);
            break;
    }
}

void Codegen::emitExprBool(const Expr& expr) {
    emitExpr(expr);
    out_ += "    cmpq $0, %rax\n";
    out_ += "    setne %al\n";
    out_ += "    movzbq %al, %rax\n";
}

void Codegen::emitCall(const Expr& expr) {
    static const char* intRegs[6] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    static const char* sseRegs[8] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3",
                                     "%xmm4", "%xmm5", "%xmm6", "%xmm7"};

    if (expr.name == "len") {
        if (expr.args[0]->type == Type::Str) {
            emitExpr(*expr.args[0]);  // %rax = pointer
            out_ += "    movq %rax, %rdi\n";
            emitAlignedCall("strlen");
        } else {
            emitArrayLength(*expr.args[0]);
        }
        return;
    }
    if (expr.name == "float") {  // int → float
        emitExpr(*expr.args[0]);
        out_ += "    cvtsi2sd %rax, %xmm0\n";
        out_ += "    movq %xmm0, %rax\n";
        return;
    }
    if (expr.name == "int") {  // float → int, truncating toward zero
        emitExpr(*expr.args[0]);
        out_ += "    movq %rax, %xmm0\n";
        out_ += "    cvttsd2si %xmm0, %rax\n";
        return;
    }

    size_t n = expr.args.size();
    std::vector<size_t> units(n, 1);
    std::vector<bool> inRegs(n, false);
    std::vector<bool> useSse(n, false);
    size_t intUnits = 0;
    size_t sseUnits = 0;
    size_t stackUnits = 0;
    for (size_t i = 0; i < n; ++i) {
        if (isArrayType(expr.args[i]->type)) units[i] = 2;
        // System V: floats take an SSE register, everything else an integer
        // register; a slice needs two integer registers. Whatever does not
        // fit goes on the stack — caller and callee classify identically.
        if (expr.args[i]->type == Type::Float) {
            if (sseUnits < 8) {
                inRegs[i] = true;
                useSse[i] = true;
                sseUnits++;
            } else {
                stackUnits += units[i];
            }
        } else if (intUnits + units[i] <= 6) {
            inRegs[i] = true;
            intUnits += units[i];
        } else {
            stackUnits += units[i];
        }
    }

    size_t stackStorage = stackUnits * 8;
    size_t regStorage = (intUnits + sseUnits) * 8;

    size_t total = stackStorage + regStorage;
    int live = stackDepth_ * 8;
    while ((live + static_cast<int>(total)) % 16 != 0) total += 8;

    if (total > 0) {
        out_ += "    subq $" + std::to_string(total) + ", %rsp\n";
    }

    // temp slots: stack args come first (that is what the callee reads),
    // then the payload only used to load the argument registers
    std::vector<size_t> off(n, 0);
    size_t stackCur = 0;
    size_t regCur = stackStorage;
    for (size_t i = 0; i < n; ++i) {
        if (inRegs[i]) {
            off[i] = regCur;
            regCur += units[i] * 8;
        } else {
            off[i] = stackCur;
            stackCur += units[i] * 8;
        }
    }

    for (size_t i = 0; i < n; ++i) {
        emitExpr(*expr.args[i]);
        out_ += "    movq %rax, " + std::to_string(off[i]) + "(%rsp)\n";
        if (units[i] == 2) {
            emitArrayLength(*expr.args[i]);
            out_ += "    movq %rax, " + std::to_string(off[i] + 8) +
                    "(%rsp)\n";
        }
    }

    size_t intIdx = 0;
    size_t sseIdx = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!inRegs[i]) continue;
        std::string slot = std::to_string(off[i]) + "(%rsp)";
        if (useSse[i]) {
            out_ += "    movq " + slot + ", " + std::string(sseRegs[sseIdx]) + "\n";
            sseIdx++;
            continue;
        }
        out_ += "    movq " + slot + ", " + std::string(intRegs[intIdx]) + "\n";
        if (units[i] == 2) {
            out_ += "    movq " + std::to_string(off[i] + 8) + "(%rsp), " +
                    std::string(intRegs[intIdx + 1]) + "\n";
        }
        intIdx += units[i];
    }

    out_ += "    call " + expr.name + "@PLT\n";
    if (expr.type == Type::Float) {
        out_ += "    movq %xmm0, %rax\n";  // float results come back in %xmm0
    }

    if (total > 0) {
        out_ += "    addq $" + std::to_string(total) + ", %rsp\n";
    }
}
