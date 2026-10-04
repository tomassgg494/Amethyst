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
    } else if (match(TokenType::KwFloat)) {
        return Type::Float;
    } else if (match(TokenType::KwString)) {
        return Type::Str;
    } else if (allowVoid && match(TokenType::KwVoid)) {
        return Type::Void;
    } else if (check(TokenType::Ident)) {
        const Token& tok = advance();
        auto it = structIds_.find(tok.text);
        if (it == structIds_.end()) {
            failAt(tok, "unknown type '" + tok.text + "'");
        }
        base = Type::structOf(it->second);
    } else {
        fail("expected type (int, bool, float, string" +
             std::string(allowVoid ? ", void" : "") + ", or a struct name)");
    }

    // slice parameter type: int[] / bool[] / Point[]
    if (allowSlice && match(TokenType::LBracket)) {
        if (check(TokenType::IntLit)) {
            fail("array parameters must be written as a slice, e.g. 'int[]' "
                 "or 'Point[]' (the length is not part of the type)");
        }
        expect(TokenType::RBracket, "']' after '['");
        return Type::arrayOf(base);
    }
    return base;
}

void Parser::collectStructNames(const std::vector<Token>& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].type != TokenType::KwStruct) continue;
        if (i + 1 >= tokens.size() || tokens[i + 1].type != TokenType::Ident) {
            failAt(tokens[i], "expected struct name after 'struct'");
        }
        const Token& nameTok = tokens[i + 1];
        if (structIds_.count(nameTok.text)) {
            failAt(nameTok, "redefinition of struct '" + nameTok.text + "'");
        }
        structIds_[nameTok.text] = static_cast<int>(prog_->structs.size());
        prog_->structs.push_back(
            StructDecl{nameTok.text, {}, nameTok.line, nameTok.col, 0});
    }
}

void Parser::parseStruct() {
    // 'struct' already consumed; the name was registered by collectStructNames.
    const Token& nameTok = expect(TokenType::Ident, "struct name");
    const std::string& structName = nameTok.text;
    StructDecl& decl = prog_->structs[structIds_.at(structName)];

    expect(TokenType::LBrace, "'{' after struct name");
    if (check(TokenType::RBrace)) {
        failAt(nameTok, "struct '" + structName + "' must declare at least one field");
    }
    do {
        const Token& fieldTok = expect(TokenType::Ident, "field name");
        expect(TokenType::Colon, "':' after field name");
        Type fieldType = parseType(false);
        if (check(TokenType::LBracket)) {
            fail("array fields are not supported yet (struct '" + structName +
                 "'); store an index and keep the data separately");
        }
        for (const auto& f : decl.fields) {
            if (f.name == fieldTok.text) {
                failAt(fieldTok, "duplicate field '" + fieldTok.text +
                                     "' in struct '" + structName + "'");
            }
        }
        decl.fields.push_back(
            StructField{fieldTok.text, fieldType, fieldTok.line, fieldTok.col});
    } while (match(TokenType::Comma) && !check(TokenType::RBrace));
    expect(TokenType::RBrace, "'}' to close struct declaration");
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
    prog_ = &prog;
    collectStructNames(tokens_);
    while (!check(TokenType::Eof)) {
        if (match(TokenType::KwStruct)) {
            parseStruct();
        } else if (check(TokenType::KwFn)) {
            prog.functions.push_back(parseFunction());
        } else {
            fail("expected function or struct declaration ('fn' or 'struct')");
        }
    }
    prog_ = nullptr;
    structIds_.clear();
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
        // array suffix: int[10] / bool[4] / float[3] / Point[5], or int[] for
        // a heap array (`var a: int[] = new int[10];` — checked by sema)
        if (check(TokenType::LBracket)) {
            advance();
            if (check(TokenType::RBracket)) {
                expect(TokenType::RBracket, "']' after '['");
                ty = Type::arrayOf(ty);
                size = 0;
            } else {
                const Token& sizeTok = expect(TokenType::IntLit, "array size");
                long long n = std::strtoll(sizeTok.text.c_str(), nullptr, 10);
                if (n <= 0 || n > 10000000) {
                    failAt(sizeTok, "array size must be between 1 and 10000000");
                }
                expect(TokenType::RBracket, "']' after array size");
                if (ty == Type::Void) {
                    fail("arrays cannot hold void values");
                }
                ty = Type::arrayOf(ty);
                size = static_cast<int>(n);
            }
        }
    } else {
        inferred = true;
        ty = Type::Error;  // sema fills from initializer
    }

    ExprPtr init;
    if (match(TokenType::Assign)) {
        init = parseExpression();
    } else if (inferred) {
        failAt(nameTok,
               "variable declaration needs an initializer when the type is "
               "omitted");
    } else if (isArrayType(ty)) {
        failAt(nameTok,
               "array variables must be initialized, e.g. 'int[3] = [1, 2, 3]'");
    }
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

StmtPtr Parser::parseFree() {
    const Token& kw = expect(TokenType::KwFree, "'free'");
    expect(TokenType::LParen, "'(' after 'free'");
    ExprPtr target = parseExpression();
    expect(TokenType::RParen, "')' after the value to free");
    expect(TokenType::Semicolon, "';' after free");
    return Stmt::makeFree(std::move(target), kw.line, kw.col);
}

StmtPtr Parser::parseAssignOrExprStmt() {
    ExprPtr target = parseExpression();

    if (isAssignOp(peek().type)) {
        const TokenType op = advance().type;
        const int line = target->line;
        const int col = target->col;
        ExprPtr value = parseExpression();
        expect(TokenType::Semicolon, "';' after assignment");
        return Stmt::makeAssign(std::move(target), std::move(value), line, col,
                                op == TokenType::Assign ? TokenType::Eof : op);
    }

    const Token& semi = expect(TokenType::Semicolon, "';' after expression");
    return Stmt::makeExprStmt(std::move(target), semi.line, semi.col);
}

StmtPtr Parser::parseStatement() {
    if (check(TokenType::LBrace)) return parseBlock();
    if (check(TokenType::KwVar)) return parseVarDecl();
    if (check(TokenType::KwIf)) return parseIf();
    if (check(TokenType::KwWhile)) return parseWhile();
    if (check(TokenType::KwFor)) return parseFor();
    if (check(TokenType::KwReturn)) return parseReturn();
    if (check(TokenType::KwPrint)) return parsePrint();
    if (check(TokenType::KwFree)) return parseFree();
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
    for (;;) {
        if (check(TokenType::LBracket)) {
            int line = peek().line;
            int col = peek().col;
            advance();  // '['
            ExprPtr index = parseExpression();
            expect(TokenType::RBracket, "']' after index expression");
            expr = Expr::makeIndex(std::move(expr), std::move(index), line, col);
            continue;
        }
        if (check(TokenType::Dot)) {
            advance();  // '.'
            const Token& fieldTok = expect(TokenType::Ident, "field name after '.'");
            expr = Expr::makeField(std::move(expr), fieldTok.text, fieldTok.line,
                                   fieldTok.col);
            continue;
        }
        break;
    }
    return expr;
}

ExprPtr Parser::parseNew() {
    const Token& kw = expect(TokenType::KwNew, "'new'");
    Type ty = parseType(false);

    // heap array: `new int[n]` / `new Point[count]`
    if (check(TokenType::LBracket)) {
        advance();
        if (ty == Type::Void) {
            failAt(kw, "'new' cannot allocate void values");
        }
        ExprPtr count = parseExpression();
        expect(TokenType::RBracket, "']' after the array size");
        return Expr::makeNewArray(Type::arrayOf(ty), std::move(count), kw.line,
                                  kw.col);
    }

    if (ty.kind != Type::Kind::Struct) {
        const std::string scalar = typeName(ty, prog_->structs);
        failAt(kw, std::string("cannot allocate a value of type '") + scalar +
                       "' with 'new' (use 'new " + scalar +
                       "[n]' for an array, or a struct type)");
    }
    const std::string tyName = typeName(ty, prog_->structs);
    expect(TokenType::LBrace, "'{' after 'new " + tyName + "'");

    std::vector<ExprPtr> fields;
    std::vector<std::string> names;
    if (!check(TokenType::RBrace)) {
        do {
            const Token& fieldTok = expect(TokenType::Ident, "field name");
            expect(TokenType::Colon, "':' after field name in 'new'");
            fields.push_back(parseExpression());
            names.push_back(fieldTok.text);
        } while (match(TokenType::Comma) && !check(TokenType::RBrace));
    }
    expect(TokenType::RBrace, "'}' to close the 'new' initializer");

    ExprPtr expr = Expr::makeNew(ty, std::move(fields), kw.line, kw.col);
    expr->fieldNames = std::move(names);
    return expr;
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::IntLit)) {
        const Token& tok = advance();
        long long v = std::strtoll(tok.text.c_str(), nullptr, 10);
        return Expr::makeInt(v, tok.line, tok.col);
    }
    // conversion builtins spelled with a type keyword: int(x), float(x)
    if ((check(TokenType::KwInt) || check(TokenType::KwFloat)) &&
        peek(1).type == TokenType::LParen) {
        const Token& kw = advance();
        std::vector<ExprPtr> args;
        expect(TokenType::LParen, "'(' after '" + kw.text + "'");
        if (!check(TokenType::RParen)) {
            do {
                args.push_back(parseExpression());
            } while (match(TokenType::Comma));
        }
        expect(TokenType::RParen, "')' after arguments");
        return Expr::makeCall(kw.text, std::move(args), kw.line, kw.col);
    }
    if (check(TokenType::KwNull)) {
        const Token& tok = advance();
        return Expr::makeNull(tok.line, tok.col);
    }
    if (check(TokenType::KwNew)) {
        return parseNew();
    }
    if (check(TokenType::FloatLit)) {
        const Token& tok = advance();
        return Expr::makeFloat(tok.num, tok.line, tok.col);
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
