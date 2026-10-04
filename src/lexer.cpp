#include "lexer.hpp"

#include <cctype>
#include <cmath>
#include <unordered_map>

namespace {

const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"fn", TokenType::KwFn},
        {"var", TokenType::KwVar},
        {"int", TokenType::KwInt},
        {"bool", TokenType::KwBool},
        {"float", TokenType::KwFloat},
        {"string", TokenType::KwString},
        {"void", TokenType::KwVoid},
        {"return", TokenType::KwReturn},
        {"if", TokenType::KwIf},
        {"else", TokenType::KwElse},
        {"while", TokenType::KwWhile},
        {"print", TokenType::KwPrint},
        {"true", TokenType::KwTrue},
        {"false", TokenType::KwFalse},
        {"break", TokenType::KwBreak},
        {"continue", TokenType::KwContinue},
        {"for", TokenType::KwFor},
        {"in", TokenType::KwIn},
        {"struct", TokenType::KwStruct},
        {"new", TokenType::KwNew},
        {"free", TokenType::KwFree},
        {"null", TokenType::KwNull},
    };
    return kw;
}

}  // namespace

Lexer::Lexer(std::string source) : src_(std::move(source)) {}

char Lexer::peek(int ahead) const {
    size_t i = pos_ + static_cast<size_t>(ahead);
    if (i >= src_.size()) return '\0';
    return src_[i];
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    pos_++;
    col_++;
    return true;
}

void Lexer::fail(const std::string& msg) const {
    throw LexError(msg, line_, col_);
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        char c = peek();
        if (c == '\n') {
            pos_++;
            line_++;
            col_ = 1;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            pos_++;
            col_++;
        } else if (c == '/' && peek(1) == '/') {
            while (peek() != '\0' && peek() != '\n') {
                pos_++;
                col_++;
            }
        } else {
            break;
        }
    }
}

Token Lexer::make(TokenType type, const std::string& text, int line, int col) {
    return Token{type, text, line, col};
}

Token Lexer::identifierOrKeyword() {
    int line = line_;
    int col = col_;
    std::string text;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        text.push_back(peek());
        pos_++;
        col_++;
    }
    auto it = keywords().find(text);
    if (it != keywords().end()) return make(it->second, text, line, col);
    return make(TokenType::Ident, text, line, col);
}

Token Lexer::number() {
    int line = line_;
    int col = col_;
    std::string text;
    bool isFloat = false;

    auto digits = [&]() {
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            text.push_back(peek());
            pos_++;
            col_++;
        }
    };

    digits();

    // "1.5" is a float, but "0..10" must stay an int followed by ".."
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        isFloat = true;
        text.push_back(peek());
        pos_++;
        col_++;
        digits();
    }

    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        text.push_back(peek());
        pos_++;
        col_++;
        if (peek() == '+' || peek() == '-') {
            text.push_back(peek());
            pos_++;
            col_++;
        }
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw LexError("invalid number literal", line, col);
        }
        digits();
    }

    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
        throw LexError("invalid number literal", line, col);
    }

    if (isFloat) {
        double v = std::strtod(text.c_str(), nullptr);
        if (!std::isfinite(v)) {
            throw LexError("number literal out of range", line, col);
        }
        Token tok = make(TokenType::FloatLit, text, line, col);
        tok.num = v;
        return tok;
    }
    return make(TokenType::IntLit, text, line, col);
}

Token Lexer::string() {
    int line = line_;
    int col = col_;
    // opening quote already positioned at '"'
    pos_++;
    col_++;
    std::string value;
    for (;;) {
        char c = peek();
        if (c == '\0') throw LexError("unterminated string literal", line, col);
        if (c == '\n') throw LexError("unterminated string literal", line, col);
        if (c == '"') {
            pos_++;
            col_++;
            break;
        }
        if (c == '\\') {
            pos_++;
            col_++;
            char e = peek();
            switch (e) {
                case 'n': value.push_back('\n'); break;
                case 't': value.push_back('\t'); break;
                case '\\': value.push_back('\\'); break;
                case '"': value.push_back('"'); break;
                default:
                    throw LexError("invalid escape sequence in string", line_,
                                   col_);
            }
            if (e != '\0') {
                pos_++;
                col_++;
            }
            continue;
        }
        value.push_back(c);
        pos_++;
        col_++;
    }
    return make(TokenType::StrLit, value, line, col);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    for (;;) {
        skipWhitespaceAndComments();
        int line = line_;
        int col = col_;
        char c = peek();
        if (c == '\0') break;

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(identifierOrKeyword());
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(number());
            continue;
        }
        if (c == '"') {
            tokens.push_back(string());
            continue;
        }

        pos_++;
        col_++;
        switch (c) {
            case '+':
                if (match('=')) {
                    tokens.push_back(make(TokenType::PlusEq, "+=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Plus, "+", line, col));
                }
                break;
            case '-':
                if (match('>')) {
                    tokens.push_back(make(TokenType::Arrow, "->", line, col));
                } else if (match('=')) {
                    tokens.push_back(make(TokenType::MinusEq, "-=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Minus, "-", line, col));
                }
                break;
            case '*':
                if (match('=')) {
                    tokens.push_back(make(TokenType::StarEq, "*=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Star, "*", line, col));
                }
                break;
            case '/':
                if (match('=')) {
                    tokens.push_back(make(TokenType::SlashEq, "/=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Slash, "/", line, col));
                }
                break;
            case '%':
                if (match('=')) {
                    tokens.push_back(make(TokenType::PercentEq, "%=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Percent, "%", line, col));
                }
                break;
            case '=':
                if (match('=')) {
                    tokens.push_back(make(TokenType::EqEq, "==", line, col));
                } else {
                    tokens.push_back(make(TokenType::Assign, "=", line, col));
                }
                break;
            case '!':
                if (match('=')) {
                    tokens.push_back(make(TokenType::NotEq, "!=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Bang, "!", line, col));
                }
                break;
            case '<':
                if (match('=')) {
                    tokens.push_back(make(TokenType::Le, "<=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Lt, "<", line, col));
                }
                break;
            case '>':
                if (match('=')) {
                    tokens.push_back(make(TokenType::Ge, ">=", line, col));
                } else {
                    tokens.push_back(make(TokenType::Gt, ">", line, col));
                }
                break;
            case '&':
                if (match('&')) {
                    tokens.push_back(make(TokenType::AmpAmp, "&&", line, col));
                } else {
                    fail("unexpected character '&' (did you mean '&&'?)");
                }
                break;
            case '|':
                if (match('|')) {
                    tokens.push_back(make(TokenType::PipePipe, "||", line, col));
                } else {
                    fail("unexpected character '|' (did you mean '||'?)");
                }
                break;
            case '(':
                tokens.push_back(make(TokenType::LParen, "(", line, col));
                break;
            case ')':
                tokens.push_back(make(TokenType::RParen, ")", line, col));
                break;
            case '{':
                tokens.push_back(make(TokenType::LBrace, "{", line, col));
                break;
            case '}':
                tokens.push_back(make(TokenType::RBrace, "}", line, col));
                break;
            case ',':
                tokens.push_back(make(TokenType::Comma, ",", line, col));
                break;
            case ';':
                tokens.push_back(make(TokenType::Semicolon, ";", line, col));
                break;
            case ':':
                tokens.push_back(make(TokenType::Colon, ":", line, col));
                break;
            case '.':
                if (match('.')) {
                    tokens.push_back(make(TokenType::DotDot, "..", line, col));
                } else {
                    tokens.push_back(make(TokenType::Dot, ".", line, col));
                }
                break;
            case '[':
                tokens.push_back(make(TokenType::LBracket, "[", line, col));
                break;
            case ']':
                tokens.push_back(make(TokenType::RBracket, "]", line, col));
                break;
            default:
                fail(std::string("unexpected character '") + c + "'");
        }
    }
    tokens.push_back(make(TokenType::Eof, "", line_, col_));
    return tokens;
}
