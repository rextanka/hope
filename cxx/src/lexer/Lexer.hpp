#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include "Token.hpp"

namespace hope {

// Raised when the lexer encounters a character sequence it cannot tokenize.
class LexError : public std::runtime_error {
public:
    SourceLocation loc;

    LexError(std::string message, SourceLocation loc)
        : std::runtime_error(std::move(message)), loc(std::move(loc)) {}
};

// Hand-written lexer for the Hope programming language.
//
// Usage:
//   Lexer lex(source_text, "filename.hop");
//   while (auto tok = lex.next()) {
//       // use *tok
//   }
//
// Calling next() after the source is exhausted repeatedly returns nullopt.
// LexError is thrown for malformed tokens; the caller (parser or REPL) is
// responsible for catching and reporting it.
class Lexer {
public:
    Lexer(std::string source, std::string filename);

    // Return the next token, skipping whitespace and comments.
    // Returns nullopt at end of input.
    // Throws LexError for unterminated literals or unrecognised characters.
    std::optional<Token> next();

    bool at_end() const;

    const std::string& filename() const { return filename_; }

private:
    std::string source_;
    std::string filename_;
    size_t      pos_  = 0;
    int         line_ = 1;
    int         col_  = 1;

    // Low-level character access
    char   peek(size_t offset = 0) const;
    char   advance();
    void   skip_whitespace_and_comments();

    // Location of the character at pos_ (before advance() is called)
    SourceLocation current_loc() const;

    // Sub-lexers — each is called when the first character has been peeked
    // but NOT yet consumed.
    Token lex_ident_or_keyword();
    Token lex_operator();
    Token lex_number();
    Token lex_char_lit();
    Token lex_string_lit();

    // Consume a backslash-escape sequence (the '\' has already been consumed).
    // Returns the raw escape text (e.g. "\\n") for inclusion in the token text.
    // Throws LexError on an unknown escape.
    std::string lex_escape(const SourceLocation& err_loc);

    // Character classification
    static bool      is_ident_start(char c);
    static bool      is_ident_cont(char c);
    static bool      is_operator_char(char c);
    static TokenKind keyword_kind(std::string_view text);
};

} // namespace hope
