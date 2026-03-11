#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "lexer/Lexer.hpp"

using namespace hope;
using ::testing::ElementsAre;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Tokenize a source string and return all tokens (not including END).
static std::vector<Token> tokenize(std::string_view src) {
    Lexer lex(std::string(src), "<test>");
    std::vector<Token> tokens;
    while (auto tok = lex.next())
        tokens.push_back(*tok);
    return tokens;
}

// Return just the token kinds.
static std::vector<TokenKind> kinds(std::string_view src) {
    std::vector<TokenKind> ks;
    for (auto& t : tokenize(src))
        ks.push_back(t.kind);
    return ks;
}

// Return just the token texts.
static std::vector<std::string> texts(std::string_view src) {
    std::vector<std::string> ts;
    for (auto& t : tokenize(src))
        ts.push_back(t.text);
    return ts;
}

// ---------------------------------------------------------------------------
// Empty / whitespace / comments
// ---------------------------------------------------------------------------

TEST(LexerTest, EmptyInput) {
    EXPECT_TRUE(tokenize("").empty());
}

TEST(LexerTest, WhitespaceOnly) {
    EXPECT_TRUE(tokenize("   \t\n\r\n  ").empty());
}

TEST(LexerTest, CommentOnly) {
    EXPECT_TRUE(tokenize("! this is a comment").empty());
}

TEST(LexerTest, CommentSkipped) {
    // '!' to end of line is a comment; the 'x' on the next line is real.
    auto toks = tokenize("! comment\nx");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IDENT);
    EXPECT_EQ(toks[0].text, "x");
}

TEST(LexerTest, CommentDoesNotConsumeNewline) {
    // The newline after '!' should still advance the line counter correctly.
    Lexer lex("! line 1\nx", "<test>");
    auto tok = lex.next();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(tok->loc.line, 2);
    EXPECT_EQ(tok->loc.column, 1);
}

TEST(LexerTest, MultipleComments) {
    auto toks = tokenize("! first\n! second\nfoo");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].text, "foo");
    EXPECT_EQ(toks[0].loc.line, 3);
}

// ---------------------------------------------------------------------------
// Keywords — all 23 must be recognised
// ---------------------------------------------------------------------------

TEST(LexerTest, Keywords_TypeDecl) {
    EXPECT_THAT(kinds("dec data type typevar abstype"),
        ElementsAre(
            TokenKind::KW_DEC, TokenKind::KW_DATA, TokenKind::KW_TYPE,
            TokenKind::KW_TYPEVAR, TokenKind::KW_ABSTYPE));
}

TEST(LexerTest, Keywords_OperatorDecl) {
    EXPECT_THAT(kinds("infix infixr"),
        ElementsAre(TokenKind::KW_INFIX, TokenKind::KW_INFIXR));
}

TEST(LexerTest, Keywords_Expressions) {
    EXPECT_THAT(kinds("lambda let letrec where whererec if then else"),
        ElementsAre(
            TokenKind::KW_LAMBDA, TokenKind::KW_LET, TokenKind::KW_LETREC,
            TokenKind::KW_WHERE, TokenKind::KW_WHEREREC,
            TokenKind::KW_IF, TokenKind::KW_THEN, TokenKind::KW_ELSE));
}

TEST(LexerTest, Keywords_Module) {
    EXPECT_THAT(kinds("uses private"),
        ElementsAre(TokenKind::KW_USES, TokenKind::KW_PRIVATE));
}

TEST(LexerTest, Keywords_Commands) {
    EXPECT_THAT(kinds("save write display edit exit nonop"),
        ElementsAre(
            TokenKind::KW_SAVE, TokenKind::KW_WRITE, TokenKind::KW_DISPLAY,
            TokenKind::KW_EDIT, TokenKind::KW_EXIT, TokenKind::KW_NONOP));
}

// Keywords are case-sensitive: capitalised versions are identifiers.
TEST(LexerTest, Keywords_CaseSensitive) {
    EXPECT_THAT(kinds("Dec Data Lambda If"),
        ElementsAre(
            TokenKind::IDENT, TokenKind::IDENT,
            TokenKind::IDENT, TokenKind::IDENT));
}

// ---------------------------------------------------------------------------
// Identifiers
// ---------------------------------------------------------------------------

TEST(LexerTest, Ident_Simple) {
    auto toks = tokenize("foo");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IDENT);
    EXPECT_EQ(toks[0].text, "foo");
}

TEST(LexerTest, Ident_Uppercase) {
    EXPECT_THAT(texts("Foo Bar"), ElementsAre("Foo", "Bar"));
}

TEST(LexerTest, Ident_WithUnderscore) {
    EXPECT_THAT(kinds("foo_bar _x x_"), ElementsAre(
        TokenKind::IDENT, TokenKind::IDENT, TokenKind::IDENT));
}

TEST(LexerTest, Ident_UnderscoreAlone) {
    // '_' alone is a valid identifier (wildcard pattern)
    auto toks = tokenize("_");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IDENT);
    EXPECT_EQ(toks[0].text, "_");
}

TEST(LexerTest, Ident_AlphaNum) {
    auto toks = tokenize("x1 a2b3");
    EXPECT_THAT(texts("x1 a2b3"), ElementsAre("x1", "a2b3"));
}

// Keywords are NOT identifiers
TEST(LexerTest, Ident_NotKeyword) {
    auto toks = tokenize("lambda");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_NE(toks[0].kind, TokenKind::IDENT);
    EXPECT_EQ(toks[0].kind, TokenKind::KW_LAMBDA);
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

TEST(LexerTest, Operator_SingleChar) {
    EXPECT_THAT(kinds("+ - * / < > = @ ^ | ~"),
        ElementsAre(
            TokenKind::OPERATOR, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::OPERATOR, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::OPERATOR, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::OPERATOR, TokenKind::OPERATOR));
}

TEST(LexerTest, Operator_Common_HopeOperators) {
    // Standard Hope operators used throughout the library
    EXPECT_THAT(texts("-> # <> :: <= >= /= == ++ .."),
        ElementsAre("->", "#", "<>", "::", "<=", ">=", "/=", "==", "++", ".."));
    EXPECT_THAT(kinds("-> # <> :: <= >= /= == ++ .."),
        ElementsAre(
            TokenKind::OPERATOR, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::OPERATOR, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::OPERATOR, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::OPERATOR));
}

TEST(LexerTest, Operator_FunctionComposition) {
    // 'o' is an identifier declared as infix in Standard.hop; the lexer
    // produces it as IDENT, not OPERATOR.
    EXPECT_THAT(kinds("f o g"), ElementsAre(
        TokenKind::IDENT, TokenKind::IDENT, TokenKind::IDENT));
}

// sums.hop: infixr \/ : 1;  — backslash-slash is a single operator
TEST(LexerTest, Operator_BackslashSlash) {
    auto toks = tokenize("\\/");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::OPERATOR);
    EXPECT_EQ(toks[0].text, "\\/");
}

// products.hop: /\ is the fork combinator — single operator
TEST(LexerTest, Operator_SlashBackslash) {
    auto toks = tokenize("/\\");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::OPERATOR);
    EXPECT_EQ(toks[0].text, "/\\");
}

// list.hop: || is zip
TEST(LexerTest, Operator_DoublePipe) {
    auto toks = tokenize("||");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::OPERATOR);
    EXPECT_EQ(toks[0].text, "||");
}

// lists.hop: -- is list difference (NOT a DASHES token — only 2 chars)
TEST(LexerTest, Operator_DoubleDash_IsNotDashes) {
    auto toks = tokenize("--");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::OPERATOR);
    EXPECT_EQ(toks[0].text, "--");
}

// diag.hop: // is diagonal enumeration
TEST(LexerTest, Operator_DoubleSlash) {
    auto toks = tokenize("//");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::OPERATOR);
}

// ---------------------------------------------------------------------------
// DASHES — equation marker (three or more '-')
// ---------------------------------------------------------------------------

TEST(LexerTest, Dashes_Three) {
    auto toks = tokenize("---");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::DASHES);
    EXPECT_EQ(toks[0].text, "---");
}

TEST(LexerTest, Dashes_Four) {
    auto toks = tokenize("----");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::DASHES);
}

TEST(LexerTest, Dashes_ManyDashes) {
    auto toks = tokenize("---------");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::DASHES);
}

TEST(LexerTest, Dashes_TwoIsNotDashes) {
    EXPECT_EQ(tokenize("--")[0].kind, TokenKind::OPERATOR);
}

TEST(LexerTest, Dashes_OneIsNotDashes) {
    EXPECT_EQ(tokenize("-")[0].kind, TokenKind::OPERATOR);
}

// In real Hope source: --- fact(0) <= 1;
TEST(LexerTest, Dashes_InEquation) {
    EXPECT_THAT(kinds("--- fact(0) <= 1;"),
        ElementsAre(
            TokenKind::DASHES, TokenKind::IDENT,
            TokenKind::LPAREN, TokenKind::INT_LIT, TokenKind::RPAREN,
            TokenKind::OPERATOR, TokenKind::INT_LIT, TokenKind::SEMICOLON));
}

// ---------------------------------------------------------------------------
// Lambda shorthand: backslash alone
// ---------------------------------------------------------------------------

TEST(LexerTest, Lambda_Backslash_IsKwLambda) {
    auto toks = tokenize("\\");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::KW_LAMBDA);
    EXPECT_EQ(toks[0].text, "\\");
}

TEST(LexerTest, Lambda_BackslashInExpr) {
    // \ x => x is the identity lambda
    EXPECT_THAT(kinds("\\ x => x"),
        ElementsAre(
            TokenKind::KW_LAMBDA, TokenKind::IDENT,
            TokenKind::OPERATOR,  TokenKind::IDENT));
}

// ---------------------------------------------------------------------------
// Integer literals
// ---------------------------------------------------------------------------

TEST(LexerTest, IntLit_Simple) {
    auto toks = tokenize("42");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::INT_LIT);
    EXPECT_EQ(toks[0].text, "42");
}

TEST(LexerTest, IntLit_Zero) {
    EXPECT_EQ(tokenize("0")[0].text, "0");
}

TEST(LexerTest, IntLit_LargeNumber) {
    auto toks = tokenize("1000000");
    EXPECT_EQ(toks[0].kind, TokenKind::INT_LIT);
    EXPECT_EQ(toks[0].text, "1000000");
}

// ---------------------------------------------------------------------------
// Float literals
// ---------------------------------------------------------------------------

TEST(LexerTest, FloatLit_Simple) {
    auto toks = tokenize("3.14");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FLOAT_LIT);
    EXPECT_EQ(toks[0].text, "3.14");
}

TEST(LexerTest, FloatLit_Exponent) {
    auto toks = tokenize("1.0e10");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FLOAT_LIT);
    EXPECT_EQ(toks[0].text, "1.0e10");
}

TEST(LexerTest, FloatLit_NegativeExponent) {
    auto toks = tokenize("2.5E-3");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FLOAT_LIT);
    EXPECT_EQ(toks[0].text, "2.5E-3");
}

// "42." — the dot is NOT followed by a digit, so it is a separate operator
TEST(LexerTest, IntLit_DotNotConsumed) {
    auto toks = tokenize("42.f");
    EXPECT_EQ(toks[0].kind, TokenKind::INT_LIT);
    EXPECT_EQ(toks[0].text, "42");
    EXPECT_EQ(toks[1].kind, TokenKind::OPERATOR); // '.'
    EXPECT_EQ(toks[2].kind, TokenKind::IDENT);    // 'f'
}

// ---------------------------------------------------------------------------
// Character literals
// ---------------------------------------------------------------------------

TEST(LexerTest, CharLit_Simple) {
    auto toks = tokenize("'a'");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::CHAR_LIT);
    EXPECT_EQ(toks[0].text, "'a'");
}

TEST(LexerTest, CharLit_Space) {
    auto toks = tokenize("' '");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::CHAR_LIT);
    EXPECT_EQ(toks[0].text, "' '");
}

TEST(LexerTest, CharLit_Escape_Newline) {
    auto toks = tokenize("'\\n'");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::CHAR_LIT);
    EXPECT_EQ(toks[0].text, "'\\n'");
}

TEST(LexerTest, CharLit_Escape_Tab) {
    EXPECT_EQ(tokenize("'\\t'")[0].kind, TokenKind::CHAR_LIT);
}

TEST(LexerTest, CharLit_Escape_Backslash) {
    EXPECT_EQ(tokenize("'\\\\'")[0].kind, TokenKind::CHAR_LIT);
}

TEST(LexerTest, CharLit_Escape_SingleQuote) {
    EXPECT_EQ(tokenize("'\\''")[0].kind, TokenKind::CHAR_LIT);
}

TEST(LexerTest, CharLit_Error_Empty) {
    EXPECT_THROW(tokenize("''"), LexError);
}

TEST(LexerTest, CharLit_Error_Unterminated) {
    EXPECT_THROW(tokenize("'a"), LexError);
}

TEST(LexerTest, CharLit_Error_UnknownEscape) {
    EXPECT_THROW(tokenize("'\\q'"), LexError);
}

// ---------------------------------------------------------------------------
// String literals
// ---------------------------------------------------------------------------

TEST(LexerTest, StrLit_Empty) {
    auto toks = tokenize("\"\"");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::STR_LIT);
    EXPECT_EQ(toks[0].text, "\"\"");
}

TEST(LexerTest, StrLit_Simple) {
    auto toks = tokenize("\"hello\"");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::STR_LIT);
    EXPECT_EQ(toks[0].text, "\"hello\"");
}

TEST(LexerTest, StrLit_WithEscapes) {
    auto toks = tokenize("\"line1\\nline2\"");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::STR_LIT);
}

TEST(LexerTest, StrLit_WithDoubleQuoteEscape) {
    auto toks = tokenize("\"say \\\"hi\\\"\"");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::STR_LIT);
}

TEST(LexerTest, StrLit_Error_Unterminated) {
    EXPECT_THROW(tokenize("\"unclosed"), LexError);
}

TEST(LexerTest, StrLit_Error_NewlineInside) {
    EXPECT_THROW(tokenize("\"line1\nline2\""), LexError);
}

// ---------------------------------------------------------------------------
// Punctuation
// ---------------------------------------------------------------------------

TEST(LexerTest, Punctuation_All) {
    EXPECT_THAT(kinds("( ) [ ] , ;"),
        ElementsAre(
            TokenKind::LPAREN,   TokenKind::RPAREN,
            TokenKind::LBRACKET, TokenKind::RBRACKET,
            TokenKind::COMMA,    TokenKind::SEMICOLON));
}

// ---------------------------------------------------------------------------
// Source locations
// ---------------------------------------------------------------------------

TEST(LexerTest, SourceLocation_FirstToken) {
    Lexer lex("abc", "<test>");
    auto tok = lex.next();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(tok->loc.file,   "<test>");
    EXPECT_EQ(tok->loc.line,   1);
    EXPECT_EQ(tok->loc.column, 1);
    EXPECT_EQ(tok->loc.length, 3);
}

TEST(LexerTest, SourceLocation_Column) {
    Lexer lex("  foo", "<test>");
    auto tok = lex.next();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(tok->loc.column, 3); // 1-based, after two spaces
}

TEST(LexerTest, SourceLocation_SecondLine) {
    Lexer lex("x\ny", "<test>");
    lex.next(); // x
    auto tok = lex.next(); // y
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(tok->loc.line,   2);
    EXPECT_EQ(tok->loc.column, 1);
}

TEST(LexerTest, SourceLocation_AfterComment) {
    Lexer lex("! comment\nfoo", "<test>");
    auto tok = lex.next();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(tok->loc.line,   2);
    EXPECT_EQ(tok->loc.column, 1);
}

TEST(LexerTest, SourceLocation_OperatorLength) {
    Lexer lex("<>", "<test>");
    auto tok = lex.next();
    ASSERT_TRUE(tok.has_value());
    EXPECT_EQ(tok->loc.length, 2);
}

// ---------------------------------------------------------------------------
// Section ambiguity: (-1) must lex as LPAREN OPERATOR INT_LIT RPAREN.
// This is a right section for "subtract 1 from something", NOT the number -1.
// The negative number -1 does not exist at the lexer level in Hope;
// negation is an operator applied to a positive literal.
// ---------------------------------------------------------------------------

TEST(LexerTest, Section_NegativeOneIsSection) {
    // (-1) — right section: subtract 1
    EXPECT_THAT(kinds("(-1)"),
        ElementsAre(
            TokenKind::LPAREN, TokenKind::OPERATOR,
            TokenKind::INT_LIT, TokenKind::RPAREN));
    EXPECT_EQ(texts("(-1)")[1], "-"); // the operator is just '-'
}

TEST(LexerTest, Section_MinusAlone) {
    // (-) — the subtraction operator as a value
    EXPECT_THAT(kinds("(-)"),
        ElementsAre(TokenKind::LPAREN, TokenKind::OPERATOR, TokenKind::RPAREN));
}

TEST(LexerTest, Section_LeftSection) {
    // (3-) — left section: subtract from 3
    EXPECT_THAT(kinds("(3-)"),
        ElementsAre(
            TokenKind::LPAREN, TokenKind::INT_LIT,
            TokenKind::OPERATOR, TokenKind::RPAREN));
}

// ---------------------------------------------------------------------------
// Real Hope snippets — end-to-end tokenization
// ---------------------------------------------------------------------------

TEST(LexerTest, Snippet_DataDecl) {
    // data bool == false ++ true;
    EXPECT_THAT(kinds("data bool == false ++ true;"),
        ElementsAre(
            TokenKind::KW_DATA, TokenKind::IDENT,
            TokenKind::OPERATOR, // ==
            TokenKind::IDENT, TokenKind::OPERATOR, // ++
            TokenKind::IDENT, TokenKind::SEMICOLON));
}

TEST(LexerTest, Snippet_DecDecl) {
    // dec max: num # num -> num;
    EXPECT_THAT(kinds("dec max: num # num -> num;"),
        ElementsAre(
            TokenKind::KW_DEC, TokenKind::IDENT, TokenKind::OPERATOR, // :
            TokenKind::IDENT, TokenKind::OPERATOR, // #
            TokenKind::IDENT, TokenKind::OPERATOR, // ->
            TokenKind::IDENT, TokenKind::SEMICOLON));
}

TEST(LexerTest, Snippet_Equation) {
    // --- max(x,y) <= if x>y then x else y;
    EXPECT_THAT(kinds("--- max(x,y) <= if x>y then x else y;"),
        ElementsAre(
            TokenKind::DASHES,
            TokenKind::IDENT, TokenKind::LPAREN,
            TokenKind::IDENT, TokenKind::COMMA, TokenKind::IDENT,
            TokenKind::RPAREN,
            TokenKind::OPERATOR, // <=
            TokenKind::KW_IF,
            TokenKind::IDENT, TokenKind::OPERATOR, // >
            TokenKind::IDENT,
            TokenKind::KW_THEN, TokenKind::IDENT,
            TokenKind::KW_ELSE, TokenKind::IDENT,
            TokenKind::SEMICOLON));
}

TEST(LexerTest, Snippet_InfixDecl) {
    // infix <> : 5;
    EXPECT_THAT(kinds("infix <> : 5;"),
        ElementsAre(
            TokenKind::KW_INFIX, TokenKind::OPERATOR, TokenKind::OPERATOR,
            TokenKind::INT_LIT, TokenKind::SEMICOLON));
}

TEST(LexerTest, Snippet_ListLiteral) {
    // [1, 2, 3]
    EXPECT_THAT(kinds("[1, 2, 3]"),
        ElementsAre(
            TokenKind::LBRACKET,
            TokenKind::INT_LIT, TokenKind::COMMA,
            TokenKind::INT_LIT, TokenKind::COMMA,
            TokenKind::INT_LIT,
            TokenKind::RBRACKET));
}

TEST(LexerTest, Snippet_NonopPlus) {
    // reduce([1,2,3], nonop +, 0)
    EXPECT_THAT(kinds("nonop +"),
        ElementsAre(TokenKind::KW_NONOP, TokenKind::OPERATOR));
}

TEST(LexerTest, Snippet_LambdaMultiClause) {
    // lambda 0 => 0 | succ(n) => n
    EXPECT_THAT(kinds("lambda 0 => 0 | succ(n) => n"),
        ElementsAre(
            TokenKind::KW_LAMBDA,
            TokenKind::INT_LIT, TokenKind::OPERATOR, // =>
            TokenKind::INT_LIT, TokenKind::OPERATOR, // |
            TokenKind::IDENT, TokenKind::LPAREN, TokenKind::IDENT, TokenKind::RPAREN,
            TokenKind::OPERATOR, // =>
            TokenKind::IDENT));
}

TEST(LexerTest, Snippet_UsesDecl) {
    // uses list, seq;
    EXPECT_THAT(kinds("uses list, seq;"),
        ElementsAre(
            TokenKind::KW_USES, TokenKind::IDENT,
            TokenKind::COMMA, TokenKind::IDENT,
            TokenKind::SEMICOLON));
}
