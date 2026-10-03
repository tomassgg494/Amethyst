#pragma once

#include <string>

enum class TokenType {
    IntLit,
    Ident,

    KwFn,
    KwVar,
    KwInt,
    KwBool,
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

    StrLit,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
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

    Eof,
};

struct Token {
    TokenType type = TokenType::Eof;
    std::string text;
    int line = 0;
    int col = 0;
};
