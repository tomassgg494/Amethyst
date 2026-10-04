#pragma once

#include <string>

enum class TokenType {
    IntLit,
    FloatLit,
    Ident,

    KwFn,
    KwVar,
    KwInt,
    KwBool,
    KwFloat,
    KwString,
    KwVoid,
    KwReturn,
    KwIf,
    KwElse,
    KwWhile,
    KwPrint,
    KwTrue,
    KwFalse,
    KwBreak,
    KwContinue,
    KwFor,
    KwIn,
    KwStruct,
    KwNew,
    KwFree,
    KwNull,

    StrLit,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    PlusEq,
    MinusEq,
    StarEq,
    SlashEq,
    PercentEq,
    EqEq,
    NotEq,
    Lt,
    Le,
    Gt,
    Ge,
    AmpAmp,
    PipePipe,
    Bang,
    Assign,
    DotDot,

    LBracket,
    RBracket,

    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Semicolon,
    Colon,
    Arrow,
    Dot,

    Eof,
};

struct Token {
    TokenType type = TokenType::Eof;
    std::string text;
    int line = 0;
    int col = 0;
    double num = 0.0;  // FloatLit value (text keeps the source spelling)
};

// Source spelling of a compound assignment operator ("" if not one).
inline const char* compoundOpText(TokenType t) {
    switch (t) {
        case TokenType::PlusEq: return "+=";
        case TokenType::MinusEq: return "-=";
        case TokenType::StarEq: return "*=";
        case TokenType::SlashEq: return "/=";
        case TokenType::PercentEq: return "%=";
        default: return "=";
    }
}

inline bool isAssignOp(TokenType t) {
    return t == TokenType::Assign || t == TokenType::PlusEq ||
           t == TokenType::MinusEq || t == TokenType::StarEq ||
           t == TokenType::SlashEq || t == TokenType::PercentEq;
}
