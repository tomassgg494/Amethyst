#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "token.hpp"

struct LexError : std::runtime_error {
    int line;
    int col;
    LexError(const std::string& msg, int line, int col)
        : std::runtime_error(msg), line(line), col(col) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();

private:
    char peek(int ahead = 0) const;
    bool match(char expected);
    void skipWhitespaceAndComments();
    Token identifierOrKeyword();
    Token number();
    Token string();
    Token make(TokenType type, const std::string& text, int line, int col);

    [[noreturn]] void fail(const std::string& msg) const;

    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;
};
