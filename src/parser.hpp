#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "ast.hpp"
#include "token.hpp"

struct ParseError : std::runtime_error {
    int line;
    int col;
    ParseError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    Program parseProgram();

private:
    const Token& peek(int ahead = 0) const;
    const Token& previous() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& advance();
    const Token& expect(TokenType type, const std::string& what);
    [[noreturn]] void fail(const std::string& msg) const;
    [[noreturn]] void failAt(const Token& tok, const std::string& msg) const;

    Type parseType(bool allowVoid);
    FnDecl parseFunction();
    std::vector<Param> parseParams();

    StmtPtr parseStatement();
    StmtPtr parseBlock();
    StmtPtr parseVarDecl();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parsePrint();
    StmtPtr parseAssignOrExprStmt();

    ExprPtr parseExpression();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();

    std::vector<Token> tokens_;
    size_t pos_ = 0;
};
