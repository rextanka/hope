#pragma once

#include <string>
#include <string_view>
#include "SourceLocation.hpp"

namespace hope {

enum class TokenKind {
    // --- End of input ---
    END,

    // --- Keywords ---
    // Type declarations
    KW_DEC,       // dec
    KW_DATA,      // data
    KW_TYPE,      // type
    KW_TYPEVAR,   // typevar
    KW_ABSTYPE,   // abstype
    // Operator declarations
    KW_INFIX,     // infix
    KW_INFIXR,    // infixr
    // Expressions
    KW_LAMBDA,    // lambda  — also produced for the backslash shorthand '\'
    KW_LET,       // let
    KW_LETREC,    // letrec
    KW_WHERE,     // where
    KW_WHEREREC,  // whererec
    KW_IF,        // if
    KW_THEN,      // then
    KW_ELSE,      // else
    // Module system
    KW_USES,      // uses
    KW_PRIVATE,   // private
    // Interactive commands
    KW_SAVE,      // save
    KW_WRITE,     // write
    KW_DISPLAY,   // display
    KW_EDIT,      // edit
    KW_EXIT,      // exit
    // Operator-as-value
    KW_NONOP,     // nonop  e.g. nonop +

    // --- Literals ---
    INT_LIT,      // integer literal:        42
    FLOAT_LIT,    // floating-point literal: 3.14
    CHAR_LIT,     // character literal:      'a'  or  '\n'
    STR_LIT,      // string literal:         "hello"

    // --- Identifier ---
    IDENT,        // alphanumeric starting with a letter or '_'

    // --- Operator ---
    // Sequence of graphic chars: # $ % & * + - . / : < = > ? @ \ ^ | ~
    // The backslash shorthand '\' alone is returned as KW_LAMBDA instead.
    // Three or more consecutive '-' chars are returned as DASHES instead.
    OPERATOR,

    // --- Equation marker ---
    // Three or more consecutive '-' characters: ---  ----  etc.
    DASHES,

    // --- Punctuation ---
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    COMMA,        // ,
    SEMICOLON,    // ;
};

// Human-readable name for a token kind (used in error messages and tests).
std::string_view token_kind_name(TokenKind kind);

struct Token {
    TokenKind      kind;
    std::string    text;   // raw source text of this token (including quotes for literals)
    SourceLocation loc;
};

} // namespace hope
