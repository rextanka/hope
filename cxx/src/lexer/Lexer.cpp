#include "Lexer.hpp"

#include <cassert>
#include <cctype>

namespace hope {

// ============================================================
// token_kind_name  (defined here to avoid a separate Token.cpp)
// ============================================================

std::string_view token_kind_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::END:        return "end-of-input";
        case TokenKind::KW_DEC:     return "dec";
        case TokenKind::KW_DATA:    return "data";
        case TokenKind::KW_TYPE:    return "type";
        case TokenKind::KW_TYPEVAR: return "typevar";
        case TokenKind::KW_ABSTYPE: return "abstype";
        case TokenKind::KW_INFIX:   return "infix";
        case TokenKind::KW_INFIXR:  return "infixr";
        case TokenKind::KW_LAMBDA:  return "lambda";
        case TokenKind::KW_LET:     return "let";
        case TokenKind::KW_LETREC:  return "letrec";
        case TokenKind::KW_WHERE:   return "where";
        case TokenKind::KW_WHEREREC:return "whererec";
        case TokenKind::KW_IF:      return "if";
        case TokenKind::KW_THEN:    return "then";
        case TokenKind::KW_ELSE:    return "else";
        case TokenKind::KW_USES:    return "uses";
        case TokenKind::KW_PRIVATE: return "private";
        case TokenKind::KW_SAVE:    return "save";
        case TokenKind::KW_WRITE:   return "write";
        case TokenKind::KW_DISPLAY: return "display";
        case TokenKind::KW_EDIT:    return "edit";
        case TokenKind::KW_EXIT:    return "exit";
        case TokenKind::KW_NONOP:   return "nonop";
        case TokenKind::INT_LIT:    return "integer-literal";
        case TokenKind::FLOAT_LIT:  return "float-literal";
        case TokenKind::CHAR_LIT:   return "char-literal";
        case TokenKind::STR_LIT:    return "string-literal";
        case TokenKind::IDENT:      return "identifier";
        case TokenKind::OPERATOR:   return "operator";
        case TokenKind::DASHES:     return "---";
        case TokenKind::LPAREN:     return "(";
        case TokenKind::RPAREN:     return ")";
        case TokenKind::LBRACKET:   return "[";
        case TokenKind::RBRACKET:   return "]";
        case TokenKind::COMMA:      return ",";
        case TokenKind::SEMICOLON:  return ";";
    }
    return "unknown";
}

// ============================================================
// Character classification
// ============================================================

bool Lexer::is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::is_ident_cont(char c) {
    // Allow apostrophe (prime) as an identifier continuation: alpha', x', words'
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '\'';
}

// Graphic characters that form operator tokens.
//
// Includes '\' so that multi-char operators like '\/' (from sums.hop) lex
// as a single token.  A lone '\' is converted to KW_LAMBDA by lex_operator().
//
// Excludes:
//   '!'  — line comment starter
//   '('  ')' '[' ']' ',' ';'  — punctuation
//   '"'  '\''  — literal delimiters
//   alphanumeric, '_'  — identifier characters
//   whitespace
bool Lexer::is_operator_char(char c) {
    switch (c) {
        case '#': case '$': case '%': case '&':
        case '*': case '+': case '-': case '.':
        case '/': case ':': case '<': case '=':
        case '>': case '?': case '@': case '\\':
        case '^': case '|': case '~':
        case '{': case '}':
            return true;
        default:
            return false;
    }
}

// ============================================================
// Keyword table
// ============================================================

TokenKind Lexer::keyword_kind(std::string_view text) {
    // Sorted roughly by expected frequency to give a slightly faster path
    // in the common case; a hash map would be faster but this is adequate.
    if (text == "if")        return TokenKind::KW_IF;
    if (text == "then")      return TokenKind::KW_THEN;
    if (text == "else")      return TokenKind::KW_ELSE;
    if (text == "let")       return TokenKind::KW_LET;
    if (text == "where")     return TokenKind::KW_WHERE;
    if (text == "lambda")    return TokenKind::KW_LAMBDA;
    if (text == "dec")       return TokenKind::KW_DEC;
    if (text == "data")      return TokenKind::KW_DATA;
    if (text == "type")      return TokenKind::KW_TYPE;
    if (text == "typevar")   return TokenKind::KW_TYPEVAR;
    if (text == "abstype")   return TokenKind::KW_ABSTYPE;
    if (text == "infix")     return TokenKind::KW_INFIX;
    if (text == "infixr")    return TokenKind::KW_INFIXR;
    if (text == "letrec")    return TokenKind::KW_LETREC;
    if (text == "whererec")  return TokenKind::KW_WHEREREC;
    if (text == "uses")      return TokenKind::KW_USES;
    if (text == "private")   return TokenKind::KW_PRIVATE;
    if (text == "write")     return TokenKind::KW_WRITE;
    if (text == "save")      return TokenKind::KW_SAVE;
    if (text == "display")   return TokenKind::KW_DISPLAY;
    if (text == "edit")      return TokenKind::KW_EDIT;
    if (text == "exit")      return TokenKind::KW_EXIT;
    if (text == "nonop")     return TokenKind::KW_NONOP;
    return TokenKind::IDENT;
}

// ============================================================
// Lexer constructor
// ============================================================

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

// ============================================================
// Low-level character access
// ============================================================

bool Lexer::at_end() const {
    return pos_ >= source_.size();
}

char Lexer::peek(size_t offset) const {
    const size_t idx = pos_ + offset;
    return idx < source_.size() ? source_[idx] : '\0';
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

SourceLocation Lexer::current_loc() const {
    return { filename_, line_, col_, 0 };
}

// ============================================================
// Whitespace and comment skipping
// ============================================================

void Lexer::skip_whitespace_and_comments() {
    while (!at_end()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        } else if (c == '!') {
            // '!' starts a line comment: skip to end of line (not including '\n').
            // The '\n' itself is consumed by the next iteration as whitespace,
            // correctly incrementing the line counter.
            while (!at_end() && peek() != '\n')
                advance();
        } else {
            break;
        }
    }
}

// ============================================================
// next() — main dispatch
// ============================================================

std::optional<Token> Lexer::next() {
    skip_whitespace_and_comments();

    if (at_end())
        return std::nullopt;

    const char c = peek();

    if (is_ident_start(c))               return lex_ident_or_keyword();
    if (is_operator_char(c))             return lex_operator();
    if (std::isdigit(static_cast<unsigned char>(c))) return lex_number();
    if (c == '\'')                       return lex_char_lit();
    if (c == '"')                        return lex_string_lit();

    // Single-character punctuation
    SourceLocation loc = current_loc();
    advance();
    loc.length = 1;
    const std::string text(1, c);

    switch (c) {
        case '(':  return Token{TokenKind::LPAREN,    text, loc};
        case ')':  return Token{TokenKind::RPAREN,    text, loc};
        case '[':  return Token{TokenKind::LBRACKET,  text, loc};
        case ']':  return Token{TokenKind::RBRACKET,  text, loc};
        case ',':  return Token{TokenKind::COMMA,     text, loc};
        case ';':  return Token{TokenKind::SEMICOLON, text, loc};
        default:
            throw LexError(
                std::string("unexpected character '") + c + "'",
                loc
            );
    }
}

// ============================================================
// Sub-lexers
// ============================================================

Token Lexer::lex_ident_or_keyword() {
    SourceLocation loc = current_loc();
    std::string text;

    while (!at_end() && is_ident_cont(peek()))
        text += advance();

    loc.length = static_cast<int>(text.size());
    return Token{keyword_kind(text), std::move(text), loc};
}

Token Lexer::lex_operator() {
    SourceLocation loc = current_loc();
    std::string text;

    while (!at_end() && is_operator_char(peek()))
        text += advance();

    loc.length = static_cast<int>(text.size());

    // Three or more consecutive dashes with no other characters: equation marker.
    // Two dashes ('--') is the list-difference operator from lists.hop.
    if (text.size() >= 3 && text.find_first_not_of('-') == std::string::npos)
        return Token{TokenKind::DASHES, std::move(text), loc};

    // Lone backslash: lambda shorthand (same as the 'lambda' keyword).
    // Multi-char sequences starting with '\' (e.g. '\/') are regular operators.
    if (text == "\\")
        return Token{TokenKind::KW_LAMBDA, std::move(text), loc};

    return Token{TokenKind::OPERATOR, std::move(text), loc};
}

Token Lexer::lex_number() {
    SourceLocation loc = current_loc();
    std::string text;

    while (!at_end() && std::isdigit(static_cast<unsigned char>(peek())))
        text += advance();

    // Fractional part: only if followed by digit (avoids greedily consuming '.'
    // when the dot is being used as an operator, e.g. in "42.rest")
    bool is_float = false;
    if (!at_end() && peek(0) == '.' &&
        std::isdigit(static_cast<unsigned char>(peek(1)))) {
        is_float = true;
        text += advance(); // '.'
        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek())))
            text += advance();
    }

    // Exponent part
    if (!at_end() && (peek() == 'e' || peek() == 'E')) {
        is_float = true;
        text += advance(); // 'e' or 'E'
        if (!at_end() && (peek() == '+' || peek() == '-'))
            text += advance();
        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek())))
            text += advance();
    }

    loc.length = static_cast<int>(text.size());
    return Token{is_float ? TokenKind::FLOAT_LIT : TokenKind::INT_LIT,
                 std::move(text), loc};
}

// Consume a backslash-escape sequence.  The leading '\' has already been
// consumed.  Returns the raw characters of the escape (e.g. "\\n") for
// inclusion in the token's text field.
std::string Lexer::lex_escape(const SourceLocation& err_loc) {
    if (at_end())
        throw LexError("unterminated escape sequence in literal", err_loc);

    const char c = advance();
    switch (c) {
        case 'n': case 't': case 'r': case '\\':
        case '\'': case '"': case 'a': case 'b':
        case 'f':  case 'v':
            return std::string(1, '\\') + c;
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': {
            // Octal escape: \d, \dd, or \ddd (up to 3 octal digits total)
            int count = 1;
            while (count < 3 && !at_end() && peek() >= '0' && peek() <= '7') {
                advance();
                ++count;
            }
            return std::string(1, '\\') + c; // raw text already captured via source_ substr
        }
        case 'x': {
            // Hex escape: \xNN (1 or 2 hex digits)
            if (at_end() || !std::isxdigit(static_cast<unsigned char>(peek())))
                throw LexError("invalid hex escape sequence", err_loc);
            advance();
            if (!at_end() && std::isxdigit(static_cast<unsigned char>(peek())))
                advance();
            return "\\x";
        }
        default:
            throw LexError(
                std::string("unknown escape sequence '\\") + c + "'",
                err_loc
            );
    }
}

Token Lexer::lex_char_lit() {
    SourceLocation loc = current_loc();
    const size_t start_pos = pos_;

    advance(); // opening '\''

    if (at_end() || peek() == '\n')
        throw LexError("unterminated character literal", loc);

    if (peek() == '\'')
        throw LexError("empty character literal", loc);

    if (peek() == '\\') {
        advance(); // backslash
        lex_escape(loc);
    } else {
        advance(); // the character itself
    }

    if (at_end() || peek() != '\'')
        throw LexError("unterminated character literal", loc);
    advance(); // closing '\''

    std::string text = source_.substr(start_pos, pos_ - start_pos);
    loc.length = static_cast<int>(text.size());
    return Token{TokenKind::CHAR_LIT, std::move(text), loc};
}

Token Lexer::lex_string_lit() {
    SourceLocation loc = current_loc();
    const size_t start_pos = pos_;

    advance(); // opening '"'

    while (true) {
        if (at_end() || peek() == '\n')
            throw LexError("unterminated string literal", loc);

        if (peek() == '"') {
            advance(); // closing '"'
            break;
        }

        if (peek() == '\\') {
            advance(); // backslash
            lex_escape(loc);
        } else {
            advance();
        }
    }

    std::string text = source_.substr(start_pos, pos_ - start_pos);
    loc.length = static_cast<int>(text.size());
    return Token{TokenKind::STR_LIT, std::move(text), loc};
}

} // namespace hope
