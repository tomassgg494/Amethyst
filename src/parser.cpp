#include "parser.hpp"

#include <cstdlib>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::peek(int ahead) const {
    size_t i = pos_ + static_cast<size_t>(ahead);
    if (i >= tokens_.size()) return tokens_.back();
    return tokens_[i];
}

const Token& Parser::previous() const {
    return tokens_[pos_ - 1];
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

const Token& Parser::advance() {
    if (peek().type != TokenType::Eof) pos_++;
    return previous();
}

void Parser::fail(const std::string& msg) const {
    failAt(peek(), msg);
}

void Parser::failAt(const Token& tok, const std::string& msg) const {
    throw ParseError(msg, tok.line, tok.col);
}

const Token& Parser::expect(TokenType type, const std::string& what) {
    if (check(type)) return advance();
    std::string got = peek().type == TokenType::Eof ? "end of file" : "'" + peek().text + "'";
    fail("expected " + what + ", got " + got);
}

Type Parser::parseType(bool allowVoid, bool allowSlice) {
    Type base = Type::Error;
    if (match(TokenType::KwInt)) {
        base = Type::Int;
    } else if (match(TokenType::KwBool)) {
        base = Type::Bool;
    } else if (allowVoid && match(TokenType::KwVoid)) {
        return Type::Void;
    } else {
        fail("expected type (int, bool" + std::string(allowVoid ? ", void" : "") + ")");
    }

    // slice parameter type: int[] / bool[]
    if (allowSlice && match(TokenType::LBracket)) {
        if (check(TokenType::IntLit)) {
            fail("array parameters must be written as 'int[]' or 'bool[]' "
                 "(the length is not part of the type)");
        }
        expect(TokenType::RBracket, "']' after '['");
        return base == Type::Bool ? Type::ArrayBool : Type::ArrayInt;
    }
    return base;
}

std::vector<Param> Parser::parseParams() {
    std::vector<Param> params;
    if (check(TokenType::RParen)) {
        advance();
        return params;
    }
    do {
        const Token& nameTok = expect(TokenType::Ident, "parameter name");
        expect(TokenType::Colon, "':' after parameter name");
        Type ty = parseType(false, true);
        params.push_back(Param{nameTok.text, ty, nameTok.line, nameTok.col, -1});
    } while (match(TokenType::Comma));
    expect(TokenType::RParen, "')' after parameters");
    return params;
}

FnDecl Parser::parseFunction() {
    const Token& fnTok = expect(TokenType::KwFn, "'fn'");
    const Token& nameTok = expect(TokenType::Ident, "function name");
    expect(TokenType::LParen, "'(' after function name");
    std::vector<Param> params = parseParams();
    expect(TokenType::Arrow, "'->' before return type");
    Type ret = parseType(true);
    if (ret == Type::Void && nameTok.text == "main") {
        failAt(nameTok, "main must return int");
    }
    if (ret == Type::Void) {
        // allowed for any function
    }
    std::unique_ptr<Stmt> body = parseBlock();

    FnDecl fn;
    fn.name = nameTok.text;
    fn.returnType = ret;
    fn.params = std::move(params);
    fn.body = std::move(body);
    fn.line = fnTok.line;
    fn.col = fnTok.col;
    return fn;
}

Program Parser::parseProgram() {
    Program prog;
    while (!check(TokenType::Eof)) {
        if (!check(TokenType::KwFn)) {
            fail("expected function declaration ('fn')");
        }
        prog.functions.push_back(parseFunction());
    }
    return prog;
}

StmtPtr Parser::parseBlock() {
    const Token& lbrace = expect(TokenType::LBrace, "'{'");
    auto block = Stmt::makeBlock(lbrace.line, lbrace.col);
    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        block->stmts.push_back(parseStatement());
    }
    expect(TokenType::RBrace, "'}' to close block");
    return block;
}

StmtPtr Parser::parseVarDecl() {
    const Token& kw = expect(TokenType::KwVar, "'var'");
    const Token& nameTok = expect(TokenType::Ident, "variable name");

    Type ty = Type::Error;
    int size = 0;
    bool inferred = false;

    if (match(TokenType::Colon)) {
        ty = parseType(false);
        // array suffix: int[10] / bool[4]
        if (check(TokenType::LBracket)) {
            advance();
            if (check(TokenType::RBracket)) {
                fail("local arrays need a fixed size, e.g. 'int[10]' "
                     "(slices are only parameter types)");
            }
            const Token& sizeTok = expect(TokenType::IntLit, "array size");
            long long n = std::strtoll(sizeTok.text.c_str(), nullptr, 10);
            if (n <= 0 || n > 10000000) {
                failAt(sizeTok, "array size must be between 1 and 10000000");
            }
            expect(TokenType::RBracket, "']' after array size");
            if (ty == Type::Int) {
                ty = Type::ArrayInt;
                size = static_cast<int>(n);
            } else if (ty == Type::Bool) {
                ty = Type::ArrayBool;
                size = static_cast<int>(n);
            } else {
                fail("array element type must be int or bool");
            }
        }
    } else {
        inferred = true;
        ty = Type::Error;  // sema fills from initializer
    }

    expect(TokenType::Assign, "'=' in variable declaration");
    ExprPtr init = parseExpression();
    expect(TokenType::Semicolon, "';' after variable declaration");

    auto s = Stmt::makeVarDecl(nameTok.text, ty, std::move(init), kw.line, kw.col);
    s->declaredSize = size;
    s->typeInferred = inferred;
    return s;
}

StmtPtr Parser::parseIf() {
    const Token& kw = expect(TokenType::KwIf, "'if'");
    ExprPtr cond = parseExpression();
    StmtPtr thenB = parseBlock();
    StmtPtr elseB;
    if (match(TokenType::KwElse)) {
        if (check(TokenType::KwIf)) {
            elseB = parseIf();
        } else {
            elseB = parseBlock();
        }
    }
    return Stmt::makeIf(std::move(cond), std::move(thenB), std::move(elseB), kw.line, kw.col);
}

StmtPtr Parser::parseWhile() {
    const Token& kw = expect(TokenType::KwWhile, "'while'");
    ExprPtr cond = parseExpression();
    StmtPtr body = parseBlock();
    return Stmt::makeWhile(std::move(cond), std::move(body), kw.line, kw.col);
}

StmtPtr Parser::parseFor() {
    const Token& kw = expect(TokenType::KwFor, "'for'");
    const Token& varTok = expect(TokenType::Ident, "loop variable");
    expect(TokenType::KwIn, "'in' after loop variable");
    ExprPtr start = parseExpression();
    expect(TokenType::DotDot, "'..' in for range");
    ExprPtr end = parseExpression();
    StmtPtr body = parseBlock();
    return Stmt::makeFor(varTok.text, std::move(start), std::move(end),
                         std::move(body), kw.line, kw.col);
}

StmtPtr Parser::parseReturn() {
    const Token& kw = expect(TokenType::KwReturn, "'return'");
    ExprPtr value;
    if (!check(TokenType::Semicolon)) {
        value = parseExpression();
    }
    expect(TokenType::Semicolon, "';' after return");
    return Stmt::makeReturn(std::move(value), kw.line, kw.col);
}

StmtPtr Parser::parsePrint() {
    const Token& kw = expect(TokenType::KwPrint, "'print'");
    expect(TokenType::LParen, "'(' after print");
    ExprPtr value = parseExpression();
    expect(TokenType::RParen, "')' after print argument");
    expect(TokenType::Semicolon, "';' after print");
    return Stmt::makePrint(std::move(value), kw.line, kw.col);
}

StmtPtr Parser::parseAssignOrExprStmt() {
    if (check(TokenType::Ident) && isAssignOp(peek(1).type)) {
        const Token& nameTok = advance();
        const TokenType op = advance().type;  // '=' or compound operator
        ExprPtr value = parseExpression();
        expect(TokenType::Semicolon, "';' after assignment");
        return Stmt::makeAssign(nameTok.text, std::move(value), nameTok.line,
                                nameTok.col,
                                op == TokenType::Assign ? TokenType::Eof : op);
    }

    // arr[i] = value;  /  arr[i] += value;
    if (check(TokenType::Ident) && peek(1).type == TokenType::LBracket) {
        // Peek ahead: Ident '[' ... ']' <assign-op>  → index assignment
        size_t save = pos_;
        const Token& nameTok = advance();  // ident
        advance();                          // '['
        // parse index expression from current position
        ExprPtr index = parseExpression();
        expect(TokenType::RBracket, "']' after index");
        if (isAssignOp(peek().type)) {
            const TokenType raw = advance().type;
            const TokenType op =
                raw == TokenType::Assign ? TokenType::Eof : raw;
            ExprPtr base = Expr::makeIdent(nameTok.text, nameTok.line, nameTok.col);
            ExprPtr target =
                Expr::makeIndex(std::move(base), std::move(index), nameTok.line, nameTok.col);
            ExprPtr value = parseExpression();
            expect(TokenType::Semicolon, "';' after assignment");
            return Stmt::makeAssignIndex(std::move(target), std::move(value),
                                         nameTok.line, nameTok.col, op);
        }
        // Not an assignment: rewind and parse as expression statement.
        pos_ = save;
    }

    ExprPtr expr = parseExpression();
    const Token& semi = expect(TokenType::Semicolon, "';' after expression");
    return Stmt::makeExprStmt(std::move(expr), semi.line, semi.col);
}

StmtPtr Parser::parseStatement() {
    if (check(TokenType::LBrace)) return parseBlock();
    if (check(TokenType::KwVar)) return parseVarDecl();
    if (check(TokenType::KwIf)) return parseIf();
    if (check(TokenType::KwWhile)) return parseWhile();
    if (check(TokenType::KwFor)) return parseFor();
    if (check(TokenType::KwReturn)) return parseReturn();
    if (check(TokenType::KwPrint)) return parsePrint();
    if (check(TokenType::KwBreak)) {
        const Token& t = advance();
        expect(TokenType::Semicolon, "';' after break");
        return Stmt::makeBreak(t.line, t.col);
    }
    if (check(TokenType::KwContinue)) {
        const Token& t = advance();
        expect(TokenType::Semicolon, "';' after continue");
        return Stmt::makeContinue(t.line, t.col);
    }
    return parseAssignOrExprStmt();
}

ExprPtr Parser::parseExpression() {
    return parseOr();
}

ExprPtr Parser::parseOr() {
    ExprPtr expr = parseAnd();
    while (match(TokenType::PipePipe)) {
        int line = previous().line;
        int col = previous().col;
        ExprPtr rhs = parseAnd();
        expr = Expr::makeBinary(TokenType::PipePipe, std::move(expr), std::move(rhs), line, col);
    }
    return expr;
}

ExprPtr Parser::parseAnd() {
    ExprPtr expr = parseEquality();
    while (match(TokenType::AmpAmp)) {
        int line = previous().line;
        int col = previous().col;
        ExprPtr rhs = parseEquality();
        expr = Expr::makeBinary(TokenType::AmpAmp, std::move(expr), std::move(rhs), line, col);
    }
    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();
    while (check(TokenType::EqEq) || check(TokenType::NotEq)) {
        TokenType op = advance().type;
        int line = previous().line;
        int col = previous().col;
        ExprPtr rhs = parseComparison();
        expr = Expr::makeBinary(op, std::move(expr), std::move(rhs), line, col);
    }
    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseTerm();
    while (check(TokenType::Lt) || check(TokenType::Le) ||
           check(TokenType::Gt) || check(TokenType::Ge)) {
        TokenType op = advance().type;
        int line = previous().line;
        int col = previous().col;
        ExprPtr rhs = parseTerm();
        expr = Expr::makeBinary(op, std::move(expr), std::move(rhs), line, col);
    }
    return expr;
}

ExprPtr Parser::parseTerm() {
    ExprPtr expr = parseFactor();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        TokenType op = advance().type;
        int line = previous().line;
        int col = previous().col;
        ExprPtr rhs = parseFactor();
        expr = Expr::makeBinary(op, std::move(expr), std::move(rhs), line, col);
    }
    return expr;
}

ExprPtr Parser::parseFactor() {
    ExprPtr expr = parseUnary();
    while (check(TokenType::Star) || check(TokenType::Slash) ||
           check(TokenType::Percent)) {
        TokenType op = advance().type;
        int line = previous().line;
        int col = previous().col;
        ExprPtr rhs = parseUnary();
        expr = Expr::makeBinary(op, std::move(expr), std::move(rhs), line, col);
    }
    return expr;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::Bang) || check(TokenType::Minus)) {
        Token opTok = advance();
        ExprPtr operand = parseUnary();
        return Expr::makeUnary(opTok.type, std::move(operand), opTok.line, opTok.col);
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();
    while (check(TokenType::LBracket)) {
        int line = peek().line;
        int col = peek().col;
        advance();  // '['
        ExprPtr index = parseExpression();
        expect(TokenType::RBracket, "']' after index expression");
        expr = Expr::makeIndex(std::move(expr), std::move(index), line, col);
    }
    return expr;
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::IntLit)) {
        const Token& tok = advance();
        long long v = std::strtoll(tok.text.c_str(), nullptr, 10);
        return Expr::makeInt(v, tok.line, tok.col);
    }
    if (check(TokenType::KwTrue)) {
        const Token& tok = advance();
        return Expr::makeBool(true, tok.line, tok.col);
    }
    if (check(TokenType::KwFalse)) {
        const Token& tok = advance();
        return Expr::makeBool(false, tok.line, tok.col);
    }
    if (check(TokenType::StrLit)) {
        const Token& tok = advance();
        return Expr::makeStr(tok.text, tok.line, tok.col);
    }
    if (check(TokenType::LBracket)) {
        // array literal: [1, 2, 3]
        const Token& tok = advance();
        std::vector<ExprPtr> elems;
        if (!check(TokenType::RBracket)) {
            do {
                elems.push_back(parseExpression());
            } while (match(TokenType::Comma));
        }
        expect(TokenType::RBracket, "']' after array literal");
        if (elems.empty()) {
            failAt(tok, "array literal must not be empty");
        }
        return Expr::makeArrayLit(std::move(elems), tok.line, tok.col);
    }
    if (check(TokenType::LParen)) {
        advance();
        ExprPtr expr = parseExpression();
        expect(TokenType::RParen, "')' after expression");
        return expr;
    }
    if (check(TokenType::Ident)) {
        const Token& tok = advance();
        if (match(TokenType::LParen)) {
            std::vector<ExprPtr> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::Comma));
            }
            expect(TokenType::RParen, "')' after arguments");
            return Expr::makeCall(tok.text, std::move(args), tok.line, tok.col);
        }
        return Expr::makeIdent(tok.text, tok.line, tok.col);
    }
    fail("expected expression");
}
