#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ast/Ast.hpp"
#include "lexer/Lexer.hpp"
#include "parser/OperatorTable.hpp"
#include "parser/ParseError.hpp"

namespace hope {

// Recursive-descent parser for Hope.
//
// Top-level declarations are parsed by parse_decl(). Expressions use a
// Pratt (top-down operator precedence) parser whose binding-power table is
// updated in place as infix/infixr declarations are encountered.
//
// The parser owns the Lexer and the OperatorTable so that infix declarations
// immediately affect the parsing of subsequent expressions in the same file.
class Parser {
public:
    explicit Parser(Lexer lex);

    // Parse and return all declarations until end of input.
    // Throws ParseError on unrecoverable syntax errors.
    std::vector<Decl> parse_program();

    // Parse a single declaration (for interactive / REPL use).
    // Returns nullopt at end of input.
    std::optional<Decl> parse_decl();

    // Expose the operator table so callers can seed it before parsing begins.
    OperatorTable& op_table() { return ops_; }

private:
    Lexer         lex_;
    OperatorTable ops_;

    // One token of lookahead.  The lexer is called lazily on demand.
    std::optional<Token> cur_;

    // --- Token access ---
    const Token&         peek();
    Token                advance();
    bool                 check(TokenKind k);
    bool                 check_op(std::string_view text);
    bool                 check_ident(std::string_view text);
    Token                expect(TokenKind k, std::string_view context);
    Token                expect_op(std::string_view text, std::string_view context);
    bool                 match(TokenKind k);
    bool                 match_op(std::string_view text);
    bool                 at_end();

    // --- Declaration parsers ---
    Decl  parse_infix_decl();
    Decl  parse_typevar_decl();
    Decl  parse_data_decl();
    Decl  parse_type_decl();
    Decl  parse_abstype_decl();
    Decl  parse_dec_decl();
    Decl  parse_equation();
    Decl  parse_uses_decl();
    // Returns DDisplay / DSave / DEdit / DEval / DWrite depending on keyword
    Decl  parse_command_or_eval();

    // --- Type parsers ---
    TypePtr parse_type();
    TypePtr parse_type_term();   // non-arrow, non-product types
    TypePtr parse_type_atom();   // parenthesised, identifier, or list type

    // Parse a type constructor application LHS: name(params) or name
    std::pair<std::string, std::vector<std::string>> parse_typecon_lhs();

    // --- Pattern parsers ---
    PatPtr  parse_pattern();         // top-level pattern, handles :: infix
    PatPtr  parse_pattern_app();     // constructor application
    PatPtr  parse_pattern_atom();    // atom: var, lit, (), [], _

    // Parse the equation LHS after '---': either prefix or infix form.
    EquationLHS parse_equation_lhs();

    // --- Expression parsers (Pratt) ---
    ExprPtr parse_expr();
    ExprPtr parse_expr_prec(int min_bp);
    ExprPtr parse_prefix_expr();   // if/lambda/let/letrec/write + application
    ExprPtr parse_application();   // left-associative application chain
    ExprPtr parse_atom();

    // Parse a parenthesised expression: could be (expr), (op), (e op), (op e), or (e,e)
    ExprPtr parse_paren_expr();

    // Parse zero or more local bindings for let/letrec/where/whererec.
    // Each binding is: name == expr
    std::vector<LocalBind> parse_local_binds();

    // Parse one or more lambda clauses separated by '|'
    std::vector<LambdaClause> parse_lambda_clauses();

    // Helpers
    SourceLocation loc_of(const Token& t) const { return t.loc; }
    [[noreturn]] void error(const std::string& msg, SourceLocation loc);

    // Scan a .hop file for infix declarations and import them into ops_.
    void load_infix_from_file(const std::string& filepath);

    // Is the current token a potential infix operator in expression context?
    // Returns the OpInfo if so.
    std::optional<OpInfo> current_infix_op();
};

} // namespace hope
