#include "parser/Parser.hpp"

#include <cassert>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace hope {

// ============================================================
// Constructor
// ============================================================

Parser::Parser(Lexer lex) : lex_(std::move(lex)) {}

// ============================================================
// Token access
// ============================================================

const Token& Parser::peek() {
    if (!cur_) {
        auto t = lex_.next();
        if (t) cur_ = std::move(t);
        else   cur_ = Token{TokenKind::END, "", {lex_.filename(), 0, 0, 0}};
    }
    return *cur_;
}

Token Parser::advance() {
    peek(); // ensure cur_ is populated
    Token t = std::move(*cur_);
    cur_.reset();
    return t;
}

bool Parser::check(TokenKind k) {
    return peek().kind == k;
}

bool Parser::check_op(std::string_view text) {
    const Token& t = peek();
    return t.kind == TokenKind::OPERATOR && t.text == text;
}

bool Parser::check_ident(std::string_view text) {
    const Token& t = peek();
    return t.kind == TokenKind::IDENT && t.text == text;
}

bool Parser::at_end() {
    return peek().kind == TokenKind::END;
}

Token Parser::expect(TokenKind k, std::string_view context) {
    const Token& t = peek();
    if (t.kind != k) {
        std::ostringstream msg;
        msg << "expected " << token_kind_name(k)
            << " " << context
            << ", got '" << t.text << "'";
        error(msg.str(), t.loc);
    }
    return advance();
}

Token Parser::expect_op(std::string_view text, std::string_view context) {
    const Token& t = peek();
    if (t.kind != TokenKind::OPERATOR || t.text != text) {
        std::ostringstream msg;
        msg << "expected '" << text << "' " << context
            << ", got '" << t.text << "'";
        error(msg.str(), t.loc);
    }
    return advance();
}

bool Parser::match(TokenKind k) {
    if (peek().kind == k) { advance(); return true; }
    return false;
}

bool Parser::match_op(std::string_view text) {
    if (check_op(text)) { advance(); return true; }
    return false;
}

[[noreturn]] void Parser::error(const std::string& msg, SourceLocation loc) {
    throw ParseError(msg, std::move(loc));
}

// ============================================================
// Infix operator detection (for Pratt parser)
// ============================================================

std::optional<OpInfo> Parser::current_infix_op() {
    const Token& t = peek();
    // Operators declared as infix can be OPERATOR tokens or IDENT tokens
    // (e.g. 'div', 'mod', 'and', 'or', 'o').
    if (t.kind == TokenKind::OPERATOR || t.kind == TokenKind::IDENT) {
        return ops_.lookup(t.text);
    }
    return std::nullopt;
}

// ============================================================
// parse_program / parse_decl
// ============================================================

std::vector<Decl> Parser::parse_program() {
    std::vector<Decl> decls;
    while (peek().kind != TokenKind::END) {
        auto d = parse_decl();
        if (d) decls.push_back(std::move(*d));
    }
    return decls;
}

std::optional<Decl> Parser::parse_decl() {
    peek();
    if (cur_->kind == TokenKind::END) return std::nullopt;

    switch (cur_->kind) {
        case TokenKind::KW_INFIX:
        case TokenKind::KW_INFIXR:  return parse_infix_decl();
        case TokenKind::KW_TYPEVAR: return parse_typevar_decl();
        case TokenKind::KW_DATA:    return parse_data_decl();
        case TokenKind::KW_TYPE:    return parse_type_decl();
        case TokenKind::KW_ABSTYPE: return parse_abstype_decl();
        case TokenKind::KW_DEC:     return parse_dec_decl();
        case TokenKind::DASHES:     return parse_equation();
        case TokenKind::KW_USES:    return parse_uses_decl();
        default:                    return parse_command_or_eval();
    }
}

// ============================================================
// Declaration parsers
// ============================================================

// infix op : prec ;
// infixr op : prec ;
// infix op1, op2, ... : prec ;   (multiple operators)
Decl Parser::parse_infix_decl() {
    SourceLocation loc = peek().loc;
    bool right_assoc = (advance().kind == TokenKind::KW_INFIXR);

    // Parse one or more operator names (comma-separated)
    auto parse_one_op = [&]() -> std::string {
        const Token& op_tok = peek();
        if (op_tok.kind != TokenKind::OPERATOR && op_tok.kind != TokenKind::IDENT) {
            error("expected operator name in infix declaration", op_tok.loc);
        }
        return advance().text;
    };

    std::vector<std::string> names;
    names.push_back(parse_one_op());
    while (check(TokenKind::COMMA)) {
        advance(); // consume ','
        names.push_back(parse_one_op());
    }

    expect_op(":", "in infix declaration");

    Token prec_tok = expect(TokenKind::INT_LIT, "in infix declaration");
    int prec = std::stoi(prec_tok.text);
    if (prec < 1 || prec > 9)
        error("operator precedence must be between 1 and 9", prec_tok.loc);

    expect(TokenKind::SEMICOLON, "at end of infix declaration");

    // Update the operator table for all declared names immediately.
    Assoc assoc = right_assoc ? Assoc::Right : Assoc::Left;
    for (auto& n : names)
        ops_.declare(n, prec, assoc);

    // Return a DInfix for the first name (callers that care about names can iterate)
    DInfix d{ right_assoc, std::move(names[0]), prec };
    return Decl{std::move(d), loc};
}

// typevar alpha ;
// typevar alpha, beta, gamma ;
Decl Parser::parse_typevar_decl() {
    SourceLocation loc = peek().loc;
    advance(); // KW_TYPEVAR

    std::vector<std::string> names;
    names.push_back(expect(TokenKind::IDENT, "in typevar declaration").text);
    while (match(TokenKind::COMMA))
        names.push_back(expect(TokenKind::IDENT, "in typevar declaration").text);

    expect(TokenKind::SEMICOLON, "at end of typevar declaration");
    return Decl{DTypeVar{std::move(names)}, loc};
}

// data typecon == cons1 ++ cons2 ++ ... ;
Decl Parser::parse_data_decl() {
    SourceLocation loc = peek().loc;
    advance(); // KW_DATA

    auto [name, params] = parse_typecon_lhs();

    expect_op("==", "in data declaration");

    // Parse alternatives separated by ++
    std::vector<Constructor> alts;
    do {
        SourceLocation cloc = peek().loc;
        std::string cname;
        std::optional<TypePtr> arg;

        // Check for infix constructor form: typearg op typearg
        // e.g. alpha :: list alpha   (where :: is declared infix)
        // Detect: first token is IDENT (type param), followed by an OPERATOR or
        // an infix-declared IDENT (like COMMA after `infix COMMA: 3;`).
        if (peek().kind == TokenKind::IDENT) {
            // Peek ahead: if the token after this IDENT is an OPERATOR (not ++, ==, ;)
            // or an IDENT that is a declared infix operator, treat as infix constructor.
            std::string first = peek().text;
            advance(); // consume the first IDENT (type param or constructor name)
            bool next_is_infix_op =
                (peek().kind == TokenKind::OPERATOR &&
                 peek().text != "++" && peek().text != "==" && peek().text != ";") ||
                (peek().kind == TokenKind::IDENT && ops_.lookup(peek().text));
            if (next_is_infix_op) {
                // Infix constructor: first was a type param, next is the constructor name
                cname = advance().text; // consume operator/ident as constructor name
                // The argument type is a product of the two type args
                // Parse the remaining type arg (right side)
                TypePtr right_type = parse_type_atom();
                // Wrap as a single product type (left # right)
                TypePtr left_type = make_type(TVar{std::move(first)},
                                              cloc);
                arg = make_type(TProd{std::move(left_type), std::move(right_type)}, cloc);
            } else {
                // Normal constructor: first was the constructor name
                cname = std::move(first);
                // Does the constructor take an argument?
                if (peek().kind == TokenKind::IDENT || peek().kind == TokenKind::LPAREN ||
                    peek().kind == TokenKind::LBRACKET) {
                    arg = parse_type_atom();
                }
            }
        } else {
            cname = expect(TokenKind::IDENT, "constructor name").text;
        }

        known_constructors_.insert(cname);
        alts.push_back(Constructor{std::move(cname), std::move(arg), cloc});
    } while (match_op("++"));

    expect(TokenKind::SEMICOLON, "at end of data declaration");

    DData d{ std::move(name), std::move(params), std::move(alts) };
    return Decl{std::move(d), loc};
}

// type typecon == type ;
Decl Parser::parse_type_decl() {
    SourceLocation loc = peek().loc;
    advance(); // KW_TYPE

    auto [name, params] = parse_typecon_lhs();

    expect_op("==", "in type declaration");

    TypePtr body = parse_type();
    expect(TokenKind::SEMICOLON, "at end of type declaration");

    DType d{ std::move(name), std::move(params), std::move(body) };
    return Decl{std::move(d), loc};
}

// abstype type_expr ;
// The type expression can be a full type: neg -> pos, pos # pos, set pos, etc.
Decl Parser::parse_abstype_decl() {
    SourceLocation loc = peek().loc;
    advance(); // KW_ABSTYPE

    TypePtr ty = parse_type();
    expect(TokenKind::SEMICOLON, "at end of abstype declaration");

    return Decl{DAbsType{std::move(ty)}, loc};
}

// dec name : type ;
// dec f, g, h : type ;      (multiple names, share the same type)
// dec + : num # num -> num ; (operator name)
Decl Parser::parse_dec_decl() {
    SourceLocation loc = peek().loc;
    advance(); // KW_DEC

    // Parse one or more comma-separated names (idents or operators).
    std::vector<std::string> names;
    auto parse_one_name = [&]() {
        const Token& nt = peek();
        if (nt.kind == TokenKind::IDENT || nt.kind == TokenKind::OPERATOR)
            names.push_back(advance().text);
        else
            error("expected function name in dec declaration", nt.loc);
    };
    parse_one_name();
    while (check(TokenKind::COMMA)) {
        advance();
        parse_one_name();
    }

    expect_op(":", "in dec declaration");

    TypePtr type = parse_type();
    expect(TokenKind::SEMICOLON, "at end of dec declaration");

    DDec d{ std::move(names), std::move(type) };
    return Decl{std::move(d), loc};
}

// --- lhs <= expr ;
Decl Parser::parse_equation() {
    SourceLocation loc = peek().loc;
    advance(); // DASHES

    EquationLHS lhs = parse_equation_lhs();

    expect_op("<=", "in equation (expected <=)");

    ExprPtr rhs = parse_expr();
    expect(TokenKind::SEMICOLON, "at end of equation");

    DEquation d{ std::move(lhs), std::move(rhs) };
    return Decl{std::move(d), loc};
}

// Scan a .hop file for infix/infixr declarations and register them in ops_.
// This allows 'uses' to load operator precedence info from dependency modules.
// Scans the entire file, skipping non-infix declarations.
void Parser::load_infix_from_file(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f) return;
    std::ostringstream ss;
    ss << f.rdbuf();
    try {
        Lexer sub_lex(ss.str(), filepath);
        Parser sub(std::move(sub_lex));
        // Scan the whole file: extract infix declarations and constructor names.
        while (!sub.at_end()) {
            const Token& t = sub.peek();
            if (t.kind == TokenKind::KW_INFIX || t.kind == TokenKind::KW_INFIXR) {
                auto decl = sub.parse_infix_decl();
                // parse_infix_decl only returns first name; we need all registered ops
                // The sub-parser's ops_ was updated for all names in the declaration.
                // Copy all newly-added ops by re-declaring from the returned decl.
                auto& di = std::get<DInfix>(decl.data);
                ops_.declare(di.name, di.prec,
                             di.right_assoc ? Assoc::Right : Assoc::Left);
                // Note: multi-name infix (e.g. infix =, /= : 3) — the sub-parser
                // registered all names in sub.ops_, but we only copy the first here.
                // For full correctness we'd need all names; for now this is sufficient
                // for the common single-name case.
            } else if (t.kind == TokenKind::KW_DATA) {
                // Parse the data declaration so constructors are registered in sub.
                try { sub.parse_data_decl(); } catch (...) { sub.advance(); }
            } else if (t.kind == TokenKind::KW_USES) {
                // Follow transitive uses to load their operators too
                sub.parse_uses_decl();
                // Copy any ops that the sub-parser loaded transitively
                // (Not easily done without exposing sub.ops_ — skip for now)
            } else {
                // Skip this token to continue scanning for infix decls
                sub.advance();
            }
        }
        // Copy constructor names discovered in the sub-parser (and any transitive
        // modules it loaded) into our own known_constructors_ set.
        for (const auto& name : sub.known_constructors())
            known_constructors_.insert(name);
    } catch (...) {
        // Ignore errors in sub-parsing; just use what we got
    }
}

// uses ModuleName ;
Decl Parser::parse_uses_decl() {
    SourceLocation loc = peek().loc;
    advance(); // KW_USES

    std::string mod = expect(TokenKind::IDENT, "in uses declaration").text;

    // Try to load infix declarations from the module file.
    // Search: same directory as current file, parent/lib, and HOPE_LIB_DIR.
    auto try_load = [&](const std::string& name) {
        std::vector<std::filesystem::path> dirs;
        const std::string& cur_file = lex_.filename();
        if (!cur_file.empty() && cur_file != "<test>") {
            auto p = std::filesystem::path(cur_file).parent_path();
            dirs.push_back(p);                  // same dir (works for lib/*.hop)
            dirs.push_back(p / ".." / "lib");   // sibling lib dir (works for test/*.in)
        }
#ifdef HOPE_LIB_DIR
        dirs.push_back(std::filesystem::path(HOPE_LIB_DIR));
#endif
        for (auto& dir : dirs) {
            auto candidate = (dir / (name + ".hop"));
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec)) {
                load_infix_from_file(candidate.string());
                return;
            }
        }
    };

    try_load(mod);

    std::vector<std::string> all_names;
    all_names.push_back(std::move(mod));

    // Multiple modules: uses list, seq;
    while (match(TokenKind::COMMA)) {
        std::string extra = expect(TokenKind::IDENT, "in uses declaration").text;
        try_load(extra);
        all_names.push_back(std::move(extra));
    }
    expect(TokenKind::SEMICOLON, "at end of uses declaration");

    return Decl{DUses{std::move(all_names)}, loc};
}

Decl Parser::parse_command_or_eval() {
    SourceLocation loc = peek().loc;

    if (match(TokenKind::KW_DISPLAY)) {
        expect(TokenKind::SEMICOLON, "after display");
        return Decl{DDisplay{}, loc};
    }
    if (peek().kind == TokenKind::KW_SAVE) {
        advance();
        std::string name = expect(TokenKind::IDENT, "after save").text;
        expect(TokenKind::SEMICOLON, "after save");
        return Decl{DSave{std::move(name)}, loc};
    }
    if (match(TokenKind::KW_EDIT)) {
        std::string mod;
        if (peek().kind == TokenKind::IDENT)
            mod = advance().text;
        expect(TokenKind::SEMICOLON, "after edit");
        return Decl{DEdit{std::move(mod)}, loc};
    }
    if (match(TokenKind::KW_EXIT)) {
        expect(TokenKind::SEMICOLON, "after exit");
        return Decl{DEval{make_expr(EVar{"exit"}, loc)}, loc}; // treated as eval
    }
    if (peek().kind == TokenKind::KW_PRIVATE) {
        SourceLocation ploc = advance().loc;
        expect(TokenKind::SEMICOLON, "after private");
        return Decl{DPrivate{}, ploc};
    }

    // Everything else is an expression followed by ;
    ExprPtr expr = parse_expr();
    expect(TokenKind::SEMICOLON, "at end of expression statement");
    return Decl{DEval{std::move(expr)}, loc};
}

// ============================================================
// Type constructor LHS helper
// ============================================================

// Parses the LHS of a data/type declaration.  Three forms are supported:
//
//   Name                       data bool == ...
//   Name(alpha, beta)          data maybe(alpha) == ...    (parenthesised)
//   Name alpha beta            data maybe alpha  == ...    (unparenthesised)
//   alpha op beta              type alpha X beta == ...    (infix type ctor)
//
// Returns (constructor_name, params).
std::pair<std::string, std::vector<std::string>>
Parser::parse_typecon_lhs() {
    std::vector<std::string> params;
    std::string name;

    // If the first token is a lowercase IDENT, it could be either:
    //   (a) the constructor name  (data bool == ...)
    //   (b) a type parameter in an infix form  (type alpha X beta == ...)
    // Disambiguate: if the SECOND token is an OPERATOR that is NOT "==" or "#"
    // as parameter separator, AND it looks like a type constructor op, treat as
    // infix; otherwise treat as prefix with the name first.
    //
    // Practical rule: if second token is an OPERATOR (not "==") or an uppercase
    // IDENT, the first token is a type param and the second is the constructor.

    if (peek().kind == TokenKind::IDENT) {
        std::string first_tok = advance().text;
        const Token& second = peek();

        bool infix_form =
            (second.kind == TokenKind::OPERATOR && second.text != "==") ||
            (second.kind == TokenKind::IDENT &&
             !second.text.empty() && std::isupper(static_cast<unsigned char>(second.text[0])));

        if (infix_form) {
            // Infix form: first_tok is a type param, next is the constructor name.
            params.push_back(std::move(first_tok));
            name = advance().text; // consume operator/ident constructor
            // Collect any further IDENT type params (stop at == or non-IDENT)
            while (peek().kind == TokenKind::IDENT && peek().text != "in")
                params.push_back(advance().text);
        } else {
            // Prefix form: first_tok is the constructor name.
            name = std::move(first_tok);
            if (match(TokenKind::LPAREN)) {
                // Parenthesised params: name(alpha, beta)
                params.push_back(expect(TokenKind::IDENT, "type parameter").text);
                while (match(TokenKind::COMMA))
                    params.push_back(expect(TokenKind::IDENT, "type parameter").text);
                expect(TokenKind::RPAREN, "after type parameters");
            } else {
                // Unparenthesised params: name alpha beta (stop at == or non-IDENT)
                while (peek().kind == TokenKind::IDENT && peek().text != "in")
                    params.push_back(advance().text);
            }
        }
    } else {
        // Shouldn't happen in well-formed Hope, but handle gracefully.
        name = expect(TokenKind::IDENT, "type constructor name").text;
    }

    return { std::move(name), std::move(params) };
}

// ============================================================
// Type parsers
// ============================================================

// type ::= term ('->' type)?   right-associative, prec 2
TypePtr Parser::parse_type() {
    return parse_type_prec(0);
}

// Pratt-style type parser.  Handles all infix type operators registered in
// ops_ (including built-ins ->, #/X and user-defined ones like OR).
TypePtr Parser::parse_type_prec(int min_bp) {
    TypePtr left = parse_type_atom();

    while (true) {
        const Token& t = peek();
        std::optional<OpInfo> op_info;
        if (t.kind == TokenKind::OPERATOR || t.kind == TokenKind::IDENT)
            op_info = ops_.lookup(t.text);
        if (!op_info) break;

        auto [token_bp, right_bp] = binding_power(*op_info);
        if (token_bp <= min_bp) break;

        SourceLocation op_loc = t.loc;
        std::string op_text = advance().text;

        TypePtr right = parse_type_prec(right_bp);

        if (op_text == "->") {
            left = make_type(TFun{std::move(left), std::move(right)}, op_loc);
        } else if (op_text == "#" || op_text == "X") {
            left = make_type(TProd{std::move(left), std::move(right)}, op_loc);
        } else {
            // User-defined infix type constructor (e.g. OR at prec 3).
            std::vector<TypePtr> args;
            args.push_back(std::move(left));
            args.push_back(std::move(right));
            left = make_type(TCons{op_text, std::move(args)}, op_loc);
        }
    }
    return left;
}

// atom ::= typevar
//        | TypeCons atom*     (type constructor applied to arguments)
//        | '(' type ')'
//        | '[' type ']'       (list sugar)
TypePtr Parser::parse_type_atom() {
    SourceLocation loc = peek().loc;
    const Token& t = peek();

    if (t.kind == TokenKind::LPAREN) {
        advance();
        TypePtr inner = parse_type();
        expect(TokenKind::RPAREN, "in parenthesised type");
        return inner;
    }

    if (t.kind == TokenKind::LBRACKET) {
        advance();
        TypePtr inner = parse_type();
        expect(TokenKind::RBRACKET, "in list type");
        // Desugar [alpha] to list(alpha)
        std::vector<TypePtr> args;
        args.push_back(std::move(inner));
        return make_type(TCons{"list", std::move(args)}, loc);
    }

    if (t.kind == TokenKind::IDENT) {
        std::string name = advance().text;

        // Collect type arguments (atoms only — no arrow/product without parens).
        // Stop if the next IDENT is a declared infix operator (e.g. OR) — those
        // are infix type constructors handled by the Pratt loop in parse_type_prec.
        std::vector<TypePtr> args;
        while ((peek().kind == TokenKind::IDENT && !ops_.lookup(peek().text)) ||
               peek().kind == TokenKind::LPAREN  ||
               peek().kind == TokenKind::LBRACKET) {
            args.push_back(parse_type_atom());
        }

        if (args.empty())
            return make_type(TVar{std::move(name)}, loc);
        return make_type(TCons{std::move(name), std::move(args)}, loc);
    }

    std::ostringstream msg;
    msg << "unexpected token '" << t.text << "' in type";
    error(msg.str(), t.loc);
}

// ============================================================
// Pattern parsers
// ============================================================

// pattern ::= pattern_app (op pattern)?   where op is :: or any declared infix operator
PatPtr Parser::parse_pattern() {
    SourceLocation loc = peek().loc;
    PatPtr left = parse_pattern_app();

    const Token& next = peek();
    if (next.kind == TokenKind::OPERATOR || next.kind == TokenKind::IDENT) {
        // Copy the operator text before calling advance() — advance() invalidates next.text
        std::string op = next.text;
        // Allow :: explicitly, plus any operator declared infix (OPERATOR or IDENT token)
        // But never consume '=>' (lambda arrow) or 'in' (let/letrec terminator)
        if (op != "in" && op != "=>" && (op == "::" || ops_.lookup(op))) {
            advance();
            PatPtr right = parse_pattern(); // right-recursive
            return make_pat(PInfix{op, std::move(left), std::move(right)}, loc);
        }
    }
    return left;
}

// pattern_app ::= 'succ' '(' pattern ')'
//               | Constructor atom_pattern?
//               | atom_pattern
PatPtr Parser::parse_pattern_app() {
    SourceLocation loc = peek().loc;

    // succ(p) or succ p — special numeric constructor pattern
    if (check_ident("succ")) {
        Token succ_tok = peek();
        advance(); // succ
        if (peek().kind == TokenKind::LPAREN) {
            advance(); // (
            PatPtr inner = parse_pattern();
            expect(TokenKind::RPAREN, "after succ pattern");
            return make_pat(PSucc{std::move(inner)}, loc);
        }
        // succ p — succ applied to an atom pattern (e.g. succ n, succ 0)
        const Token& nx = peek();
        bool atom_follows =
            (nx.kind == TokenKind::IDENT && !nx.text.empty() &&
             std::islower(static_cast<unsigned char>(nx.text[0]))) ||
            nx.kind == TokenKind::INT_LIT  ||
            nx.kind == TokenKind::LBRACKET;
        if (atom_follows) {
            PatPtr inner = parse_pattern_atom();
            return make_pat(PSucc{std::move(inner)}, loc);
        }
        // Bare identifier 'succ'
        return make_pat(PVar{"succ"}, succ_tok.loc);
    }

    // Constructor applied to optional atom argument.
    // In Hope, constructors are declared via 'data' and can be lowercase or uppercase.
    // Detection rules:
    //   1. ident(...)  — always a constructor application (parens are unambiguous)
    //   2. UPPER ident/lit/list — uppercase ident followed by an atom arg (no parens)
    //   3. bare ident — variable pattern
    const Token& t = peek();
    if (t.kind == TokenKind::IDENT) {
        std::string cname = advance().text;

        // n+k pattern: var + integer_literal
        if (check_op("+")) {
            // peek ahead: only treat as n+k if followed by INT_LIT
            // (We've already consumed cname; '+' is next, INT_LIT after that)
            advance(); // consume '+'
            Token k_tok = expect(TokenKind::INT_LIT, "after + in n+k pattern");
            return make_pat(PNPlusK{std::move(cname), std::stoi(k_tok.text)}, loc);
        }

        // Determine if this identifier is a constructor.
        // In Hope, constructors can be lowercase (e.g. node, leaf, empty),
        // so we use the known_constructors_ set populated from data declarations,
        // plus capitalisation as a fallback for constructors not yet declared
        // (e.g. uppercase constructors from modules loaded after parse time).
        bool is_ctor = !cname.empty() &&
                       (std::isupper(static_cast<unsigned char>(cname[0])) ||
                        known_constructors_.count(cname));

        const Token& nx = peek();
        if (nx.kind == TokenKind::LPAREN && is_ctor) {
            // Constructor(arg) — constructor applied to a parenthesised argument.
            PatPtr arg = parse_pattern_atom(); // will consume (...)
            return make_pat(PCons{std::move(cname), std::move(arg)}, loc);
        }
        // No LPAREN: a known constructor can take an atom arg (literal or [ ]).
        // Unknown (lowercase) idents are variables and never consume an atom arg.
        bool arg_follows =
            nx.kind == TokenKind::LBRACKET  ||
            nx.kind == TokenKind::INT_LIT   ||
            nx.kind == TokenKind::CHAR_LIT  ||
            nx.kind == TokenKind::STR_LIT;
        if (arg_follows && is_ctor) {
            PatPtr arg = parse_pattern_atom();
            return make_pat(PCons{std::move(cname), std::move(arg)}, loc);
        }
        // Bare identifier — variable pattern.
        // Constructor vs variable resolution is deferred to the evaluator,
        // which checks constructor_arity_ at runtime.
        return make_pat(PVar{std::move(cname)}, loc);
    }

    return parse_pattern_atom();
}

// atom_pattern ::= var | '_' | num | char | string | '(' pattern (',' pattern)* ')'
//                | '[' (pattern (',' pattern)*)? ']'
PatPtr Parser::parse_pattern_atom() {
    SourceLocation loc = peek().loc;
    const Token& t = peek();

    // Some keywords can appear as variable names in patterns (e.g. in equation LHS
    // like: --- if true then x else y <= x)
    if (t.kind == TokenKind::KW_THEN || t.kind == TokenKind::KW_ELSE) {
        return make_pat(PVar{advance().text}, loc);
    }

    // Operator symbols used as zero-argument constructors in patterns (e.g. {})
    // These arise from dec {} : set alpha; and --- {} <= [];
    // Only treat as constructor if NOT a known infix operator (those are handled
    // by parse_pattern at a higher level, not here).
    if (t.kind == TokenKind::OPERATOR && !ops_.lookup(t.text) && t.text != "::") {
        std::string op = advance().text;
        return make_pat(PVar{std::move(op)}, loc);
    }

    if (t.kind == TokenKind::IDENT) {
        std::string name = advance().text;
        // n+k pattern: var + integer_literal (in pattern context, + cannot be infix)
        if (check_op("+")) {
            advance(); // consume '+'
            Token k_tok = expect(TokenKind::INT_LIT, "after + in n+k pattern");
            return make_pat(PNPlusK{std::move(name), std::stoi(k_tok.text)}, loc);
        }
        return make_pat(PVar{std::move(name)}, loc);
    }

    if (t.kind == TokenKind::INT_LIT) {
        std::string text = advance().text;
        return make_pat(PLit{LitNum{std::move(text)}}, loc);
    }

    if (t.kind == TokenKind::CHAR_LIT) {
        return make_pat(PLit{LitChar{advance().text}}, loc);
    }

    if (t.kind == TokenKind::STR_LIT) {
        return make_pat(PLit{LitStr{advance().text}}, loc);
    }

    // '_' wildcard
    if (t.kind == TokenKind::IDENT && t.text == "_") {
        advance();
        return make_pat(PWild{}, loc);
    }

    if (t.kind == TokenKind::LPAREN) {
        advance();
        // (op) — operator as pattern value (used in equation LHS like --- (^) <= pow)
        // If we see LPAREN OPERATOR RPAREN, it's always an operator reference regardless
        // of whether the operator is declared infix.
        // If we see LPAREN OPERATOR <something else>, it's a normal infix pattern like (a # b)
        // where parse_pattern() handles the infix — fall through to normal pattern parsing.
        if (peek().kind == TokenKind::OPERATOR) {
            std::string op = peek().text;
            SourceLocation op_loc = peek().loc;
            advance(); // consume op
            if (peek().kind == TokenKind::RPAREN) {
                advance(); // consume )
                // Represent as PVar(op) — the operator used as a value pattern
                return make_pat(PVar{std::move(op)}, op_loc);
            }
            // Not a simple (op) — it's an infix pattern like (a # b).
            // We've consumed the operator; now we need to parse what follows as the RHS,
            // but that requires the LHS too. In Hope this shouldn't arise since patterns
            // don't start with an operator. Treat as an error.
            error("unexpected operator in pattern", op_loc);
        }
        // Empty parens is a parse error in patterns — but '()' is valid as unit in some languages.
        // In Hope, '()' doesn't exist; '(p)' is just p.
        PatPtr first = parse_pattern();
        // Post-fixup for constructor-with-arg inside parentheses:
        //
        // Case 1: nullary PCons (uppercase or known-ctor) followed by an arg token
        //   → absorb the arg.  (Just x), (LESS _), (Ctor (a,b)), etc.
        //
        // Case 2: PVar (lowercase) followed by '(' with content
        //   → must be a constructor applied to a parenthesised arg, e.g. (leaf(n)).
        //   Retroactively convert to PCons.
        //
        // Both cases only apply when the next token is not ',' or ')'.
        {
            const Token& nx = peek();
            if (auto* pc = std::get_if<PCons>(&first->data)) {
                if (!pc->arg.has_value()) {
                    bool arg_could_follow =
                        nx.kind == TokenKind::LPAREN   ||
                        nx.kind == TokenKind::LBRACKET ||
                        nx.kind == TokenKind::INT_LIT  ||
                        nx.kind == TokenKind::CHAR_LIT ||
                        nx.kind == TokenKind::STR_LIT  ||
                        (nx.kind == TokenKind::IDENT   &&
                         nx.text != "in" && nx.text != "=>");
                    if (arg_could_follow) {
                        pc->arg = parse_pattern_atom();
                    }
                }
            } else if (auto* pv = std::get_if<PVar>(&first->data)) {
                // PVar followed by a potential argument → constructor(arg) form.
                // E.g. (Yes x) → PCons{"Yes", PVar{"x"}}
                //      (leaf(n)) → PCons{"leaf", PTuple{PVar{"n"}}}
                //      (Ctor []) → PCons{"Ctor", PCons{"nil"}}
                bool arg_could_follow =
                    nx.kind == TokenKind::LPAREN   ||
                    nx.kind == TokenKind::LBRACKET ||
                    nx.kind == TokenKind::INT_LIT  ||
                    nx.kind == TokenKind::CHAR_LIT ||
                    nx.kind == TokenKind::STR_LIT  ||
                    (nx.kind == TokenKind::IDENT   &&
                     nx.text != "in"  && nx.text != "=>" &&
                     nx.text != "_");  // '_' handled separately below
                // Also handle '_' as wildcard arg
                bool wild_arg = (nx.kind == TokenKind::IDENT && nx.text == "_");
                if (arg_could_follow || wild_arg) {
                    std::string cname = pv->name;
                    SourceLocation first_loc = first->loc;
                    PatPtr arg = parse_pattern_atom();
                    first = make_pat(PCons{std::move(cname), std::move(arg)}, first_loc);
                }
            }
        }
        if (match(TokenKind::COMMA)) {
            // Tuple pattern
            std::vector<PatPtr> elems;
            elems.push_back(std::move(first));
            do {
                elems.push_back(parse_pattern());
            } while (match(TokenKind::COMMA));
            expect(TokenKind::RPAREN, "after tuple pattern");
            return make_pat(PTuple{std::move(elems)}, loc);
        }
        expect(TokenKind::RPAREN, "after parenthesised pattern");
        return first;
    }

    if (t.kind == TokenKind::LBRACKET) {
        advance();
        if (match(TokenKind::RBRACKET)) {
            // nil — empty list pattern
            return make_pat(PCons{"nil", std::nullopt}, loc);
        }
        std::vector<PatPtr> elems;
        elems.push_back(parse_pattern());
        while (match(TokenKind::COMMA))
            elems.push_back(parse_pattern());
        expect(TokenKind::RBRACKET, "in list pattern");
        return make_pat(PList{std::move(elems)}, loc);
    }

    std::ostringstream msg;
    msg << "unexpected token '" << t.text << "' in pattern";
    error(msg.str(), t.loc);
}

// ============================================================
// Equation LHS
// ============================================================

// Parses the LHS of an equation after '---'.
//
// Prefix form (most common):
//   f arg1 arg2 ...    e.g.  not false   or  map f xs   or  fold F f
//
// Infix form (operator defined as infix):
//   pat op pat         e.g.  n..m  (when .. is declared infixr)
//                      e.g.  (f /\ g) x   — this is prefix with op as name
//
// We detect infix when the FIRST pattern atom is followed by a DECLARED infix
// operator.  Undeclared operators are treated as prefix function names.
EquationLHS Parser::parse_equation_lhs() {
    SourceLocation loc = peek().loc;
    const Token& first = peek();

    // Handle keywords used as function names in equation LHS (e.g. 'if', 'then', 'else')
    // Hope allows defining functions with keyword names: --- if true then x else y <= x;
    bool is_keyword_func =
        first.kind == TokenKind::KW_IF     ||
        first.kind == TokenKind::KW_THEN   ||
        first.kind == TokenKind::KW_ELSE   ||
        first.kind == TokenKind::KW_LET    ||
        first.kind == TokenKind::KW_LETREC ||
        first.kind == TokenKind::KW_WHERE  ||
        first.kind == TokenKind::KW_LAMBDA ||
        first.kind == TokenKind::KW_WRITE;

    if (first.kind == TokenKind::IDENT || is_keyword_func) {
        std::string fname = advance().text;

        // Check: is the next token a declared infix operator?
        // If so, fname is the left operand pattern, not the function name.
        const Token& next = peek();
        bool is_infix_form = false;
        if (next.kind == TokenKind::OPERATOR && ops_.lookup(next.text))
            is_infix_form = true;
        else if (next.kind == TokenKind::IDENT && ops_.lookup(next.text))
            is_infix_form = true;

        if (is_infix_form) {
            // Parse the first operator.
            std::string op1_name = advance().text; // consume op1
            auto op1_info = ops_.lookup(op1_name);
            int op1_prec = op1_info ? op1_info->prec : 5; // default :: prec

            // Parse the rest as a full pattern (may itself be an infix pattern).
            PatPtr right = parse_pattern();

            // Check if the right side is a PInfix whose operator has LOWER
            // precedence than op1.  If so, op2 is the real function being
            // defined (e.g. --- x::xs || ys  →  function=||, args=[x::xs, ys]).
            if (auto* ri = std::get_if<PInfix>(&right->data)) {
                auto op2_info = ops_.lookup(ri->op);
                int op2_prec = op2_info ? op2_info->prec : 5;
                if (op2_prec < op1_prec) {
                    // Restructure: the left arg of op2 is "fname op1 ri->left"
                    // e.g. (x::xs) is left of ||
                    PatPtr left_of_op1 = make_pat(PVar{std::move(fname)}, loc);
                    PatPtr left_pat = make_pat(
                        PInfix{op1_name, std::move(left_of_op1), std::move(ri->left)},
                        loc);
                    std::string func = ri->op;
                    std::vector<PatPtr> args2;
                    args2.push_back(std::move(left_pat));
                    args2.push_back(std::move(ri->right));
                    return EquationLHS{ std::move(func), std::move(args2), true };
                }
            }

            std::vector<PatPtr> args;
            args.push_back(make_pat(PVar{std::move(fname)}, loc));
            args.push_back(std::move(right));
            return EquationLHS{ std::move(op1_name), std::move(args), true };
        }

        // Prefix form: fname followed by zero or more pattern atoms until <=
        std::vector<PatPtr> args;
        while (!check_op("<=") && !at_end())
            args.push_back(parse_pattern());
        return EquationLHS{ std::move(fname), std::move(args), false };
    }

    // Non-IDENT start: parse the first pattern, then decide based on what it is.
    SourceLocation first_loc = loc;
    PatPtr first_pat = parse_pattern();

    // Case A: first_pat is PVar (from (op) form) — operator reference equation.
    // e.g. --- (^) <= pow;   or   --- (+) <= plus;
    if (std::holds_alternative<PVar>(first_pat->data)) {
        std::string func = std::get<PVar>(first_pat->data).name;
        std::vector<PatPtr> args;
        while (!check_op("<=") && !at_end())
            args.push_back(parse_pattern());
        return EquationLHS{ std::move(func), std::move(args), false };
    }

    // Case B: first_pat is PInfix — infix operator equation.
    // e.g. --- (h :: t) cat1 r <= ...  defines cat1 with args [h::t, r]
    // e.g. --- (f /\ g) x <= ...       defines /\ with args [f, g, x]
    // The PInfix holds: op=function_name, left=first_arg, right=second_arg.
    // Any additional patterns after the PInfix are extra args.
    if (std::holds_alternative<PInfix>(first_pat->data)) {
        auto& pinfix = std::get<PInfix>(first_pat->data);
        std::string func = pinfix.op;
        std::vector<PatPtr> args;
        args.push_back(std::move(pinfix.left));
        args.push_back(std::move(pinfix.right));
        while (!check_op("<=") && !at_end())
            args.push_back(parse_pattern());
        return EquationLHS{ std::move(func), std::move(args), true };
    }

    // Case C: other pattern (PTuple, etc.) — followed by an operator name.
    {
        const Token& op_tok = peek();
        if (op_tok.kind != TokenKind::OPERATOR && op_tok.kind != TokenKind::IDENT) {
            error("expected function name or operator in equation LHS", op_tok.loc);
        }
        std::string op_name = advance().text;
        std::vector<PatPtr> args;
        args.push_back(std::move(first_pat));
        while (!check_op("<=") && !at_end())
            args.push_back(parse_pattern());
        return EquationLHS{ std::move(op_name), std::move(args), true };
    }
}

// ============================================================
// Expression parsers
// ============================================================

ExprPtr Parser::parse_expr() {
    // Handle where/whererec as postfix on the expression
    ExprPtr body = parse_expr_prec(0);

    if (peek().kind == TokenKind::KW_WHERE) {
        SourceLocation loc = advance().loc;
        auto binds = parse_local_binds();
        return make_expr(EWhere{std::move(body), std::move(binds)}, loc);
    }
    if (peek().kind == TokenKind::KW_WHEREREC) {
        SourceLocation loc = advance().loc;
        auto binds = parse_local_binds();
        return make_expr(EWhereRec{std::move(body), std::move(binds)}, loc);
    }
    return body;
}

// Pratt expression parser.
// min_bp = 0 for top-level expressions.
ExprPtr Parser::parse_expr_prec(int min_bp) {
    ExprPtr left = parse_prefix_expr();

    while (true) {
        auto op_info = current_infix_op();
        if (!op_info) break;

        auto [token_bp, right_bp] = binding_power(*op_info);
        if (token_bp <= min_bp) break;

        SourceLocation op_loc = peek().loc;
        std::string op_name = advance().text;

        ExprPtr right = parse_expr_prec(right_bp);
        left = make_expr(EInfix{op_name, std::move(left), std::move(right)}, op_loc);
    }

    return left;
}

// Prefix forms: if, lambda, let, letrec, write, application
ExprPtr Parser::parse_prefix_expr() {
    SourceLocation loc = peek().loc;

    if (peek().kind == TokenKind::KW_IF) {
        advance();
        ExprPtr cond  = parse_expr_prec(0);
        expect(TokenKind::KW_THEN, "after condition in if expression");
        ExprPtr then_ = parse_expr();   // allow where/whererec in then-branch
        expect(TokenKind::KW_ELSE, "after then-branch in if expression");
        ExprPtr else_ = parse_expr_prec(0);
        return make_expr(EIf{std::move(cond), std::move(then_), std::move(else_)}, loc);
    }

    if (peek().kind == TokenKind::KW_LAMBDA) {
        advance();
        auto clauses = parse_lambda_clauses();
        return make_expr(ELambda{std::move(clauses)}, loc);
    }

    if (peek().kind == TokenKind::KW_LET || peek().kind == TokenKind::KW_LETREC) {
        bool is_rec = (advance().kind == TokenKind::KW_LETREC);
        auto binds = parse_local_binds();
        // 'in' is lexed as IDENT("in") — parse_local_binds already consumed it.
        ExprPtr body = parse_expr_prec(0);
        if (is_rec) return make_expr(ELetRec{std::move(binds), std::move(body)}, loc);
        else        return make_expr(ELet{std::move(binds), std::move(body)}, loc);
    }

    if (peek().kind == TokenKind::KW_WRITE) {
        advance();
        ExprPtr arg = parse_expr_prec(0);
        return make_expr(EWrite{std::move(arg)}, loc);
    }

    // Anything else: application chain
    return parse_application();
}

// Left-associative function application: f x y z
// Application has higher precedence than any infix operator.
ExprPtr Parser::parse_application() {
    ExprPtr func = parse_atom();

    while (true) {
        // An atom starts with: IDENT, INT_LIT, FLOAT_LIT, CHAR_LIT, STR_LIT,
        // LPAREN, LBRACKET, KW_IF, KW_LAMBDA, KW_LET, KW_LETREC.
        // Stop if the next token is an infix operator, SEMICOLON, COMMA,
        // RPAREN, RBRACKET, KW_THEN, KW_ELSE, KW_WHERE, KW_WHEREREC, END, DASHES.
        const Token& t = peek();
        bool is_atom_start =
            t.kind == TokenKind::IDENT      ||
            t.kind == TokenKind::INT_LIT    ||
            t.kind == TokenKind::FLOAT_LIT  ||
            t.kind == TokenKind::CHAR_LIT   ||
            t.kind == TokenKind::STR_LIT    ||
            t.kind == TokenKind::LPAREN     ||
            t.kind == TokenKind::LBRACKET   ||
            // Non-infix, non-delimiter OPERATOR tokens can be atoms (e.g. `{}`).
            (t.kind == TokenKind::OPERATOR && !ops_.lookup(t.text) &&
             t.text != "|"  && t.text != "=>" && t.text != "<=" &&
             t.text != "==" && t.text != "++" && t.text != "--" &&
             t.text != "#"  && t.text != ":");

        if (!is_atom_start) break;

        // If this IDENT is an infix operator, stop — let the Pratt parser handle it.
        if (t.kind == TokenKind::IDENT && ops_.lookup(t.text)) break;

        // 'in' terminates let/letrec bindings — stop application chain here.
        if (t.kind == TokenKind::IDENT && t.text == "in") break;

        ExprPtr arg = parse_atom();
        SourceLocation loc = func->loc;
        func = make_expr(EApply{std::move(func), std::move(arg)}, loc);
    }

    return func;
}

ExprPtr Parser::parse_atom() {
    SourceLocation loc = peek().loc;
    const Token& t = peek();

    if (t.kind == TokenKind::IDENT) {
        return make_expr(EVar{advance().text}, loc);
    }

    // Operator symbols used as nullary value expressions (not declared infix,
    // not reserved as grammar delimiters like `|`, `=>`, `<=`, `==`).
    // In Hope, graphic-character sequences such as `{}` can be declared as
    // functions (e.g. `dec {} : set alpha`) and used in expression position.
    // We allow them here when they are NOT grammar delimiters.
    if (t.kind == TokenKind::OPERATOR && !ops_.lookup(t.text)) {
        // Never consume tokens that serve as grammar delimiters.
        const std::string& tx = t.text;
        bool is_delimiter =
            tx == "|"  || tx == "=>" || tx == "<=" ||
            tx == "==" || tx == "++" || tx == "--" ||
            tx == "#"  || tx == ":";
        if (!is_delimiter) {
            return make_expr(EVar{advance().text}, loc);
        }
    }

    if (t.kind == TokenKind::INT_LIT) {
        return make_expr(ELit{LitNum{advance().text}}, loc);
    }

    if (t.kind == TokenKind::FLOAT_LIT) {
        return make_expr(ELit{LitFloat{advance().text}}, loc);
    }

    if (t.kind == TokenKind::CHAR_LIT) {
        return make_expr(ELit{LitChar{advance().text}}, loc);
    }

    if (t.kind == TokenKind::STR_LIT) {
        return make_expr(ELit{LitStr{advance().text}}, loc);
    }

    if (t.kind == TokenKind::LPAREN) {
        return parse_paren_expr();
    }

    if (t.kind == TokenKind::LBRACKET) {
        advance();
        if (match(TokenKind::RBRACKET))
            return make_expr(EList{{}}, loc);

        std::vector<ExprPtr> elems;
        elems.push_back(parse_expr_prec(0));
        while (match(TokenKind::COMMA))
            elems.push_back(parse_expr_prec(0));
        expect(TokenKind::RBRACKET, "at end of list literal");
        return make_expr(EList{std::move(elems)}, loc);
    }

    if (t.kind == TokenKind::KW_NONOP) {
        advance();
        const Token& op_tok = peek();
        if (op_tok.kind != TokenKind::OPERATOR && op_tok.kind != TokenKind::IDENT)
            error("expected operator after nonop", op_tok.loc);
        return make_expr(EVar{advance().text}, loc);
    }

    std::ostringstream msg;
    msg << "unexpected token '" << t.text << "' in expression";
    error(msg.str(), t.loc);
}

// Parenthesised expression:
//   (expr)             — grouping
//   (expr, expr)       — tuple / pair
//   (op)               — operator as value: (++)
//   (expr op)          — left section: (3-)
//   (op expr)          — right section: (-1)
ExprPtr Parser::parse_paren_expr() {
    SourceLocation loc = peek().loc;
    advance(); // '('

    // Empty parens: () — not valid in Hope but handle gracefully
    if (peek().kind == TokenKind::RPAREN) {
        advance();
        error("empty parentheses in expression", loc);
    }

    // Operator as value: (+) or (<>) etc.
    if (peek().kind == TokenKind::OPERATOR) {
        // Could be: (op) — operator reference
        //           (op expr) — right section
        std::string op_text = peek().text;
        SourceLocation op_loc = peek().loc;

        // If op is followed immediately by ')' it's an operator reference.
        // We'll parse the potential right-section expression speculatively:
        // consume the op, then see if we can parse an expression before ')'.

        // Check for (op) — single operator, nothing follows before ')'
        // We do this by checking the token AFTER the operator without consuming.
        // Since we only have one token of lookahead, we peek at the op and then
        // decide based on what follows.
        advance(); // consume op token

        if (peek().kind == TokenKind::RPAREN) {
            // (op) — operator as function value
            advance();
            return make_expr(EOpRef{std::move(op_text)}, op_loc);
        }

        // (op expr) — right section
        ExprPtr rhs = parse_expr_prec(0);
        expect(TokenKind::RPAREN, "at end of right section");
        return make_expr(ESection{false, std::move(op_text), std::move(rhs)}, loc);
    }

    // Parse the leading expression as a prefix expr (not full Pratt).
    // This stops before infix operators, enabling left-section detection below.
    ExprPtr first = parse_prefix_expr();

    // What follows the leading expression?

    // where/whererec immediately after the leading expression (before any operator)
    if (peek().kind == TokenKind::KW_WHERE) {
        SourceLocation wloc = advance().loc;
        auto binds = parse_local_binds();
        first = make_expr(EWhere{std::move(first), std::move(binds)}, wloc);
    } else if (peek().kind == TokenKind::KW_WHEREREC) {
        SourceLocation wloc = advance().loc;
        auto binds = parse_local_binds();
        first = make_expr(EWhereRec{std::move(first), std::move(binds)}, wloc);
    }

    // (expr) — grouping
    if (peek().kind == TokenKind::RPAREN) {
        advance();
        return first;
    }

    // (expr : type) — type annotation
    if (peek().kind == TokenKind::OPERATOR && peek().text == ":") {
        advance(); // consume ':'
        TypePtr ty = parse_type();
        expect(TokenKind::RPAREN, "after type annotation");
        return make_expr(EAnnotate{std::move(first), std::move(ty)}, loc);
    }

    // (expr, expr, ...) — tuple
    if (peek().kind == TokenKind::COMMA) {
        std::vector<ExprPtr> elems;
        elems.push_back(std::move(first));
        while (match(TokenKind::COMMA))
            elems.push_back(parse_expr_prec(0));
        expect(TokenKind::RPAREN, "after tuple elements");
        return make_expr(ETuple{std::move(elems)}, loc);
    }

    // Possible left section OR infix expression.
    // Consume the operator, peek at what follows:
    //   ')'   → left section  (expr op)
    //   other → infix expression; parse right side, run Pratt, expect ')'
    if (peek().kind == TokenKind::OPERATOR || current_infix_op()) {
        SourceLocation op_loc = peek().loc;
        std::string op_text = advance().text;

        if (peek().kind == TokenKind::RPAREN) {
            // (expr op) — left section
            advance();
            return make_expr(ESection{true, std::move(op_text), std::move(first)}, loc);
        }

        // (expr op right...) — parenthesised infix expression.
        // Look up the binding power; default to 0 if operator not yet declared.
        auto op_info = ops_.lookup(op_text);
        int r_bp = op_info ? binding_power(*op_info).right_bp : 1;
        ExprPtr right = parse_expr_prec(r_bp);
        first = make_expr(EInfix{op_text, std::move(first), std::move(right)}, op_loc);

        // Continue the Pratt loop for any additional operators inside the parens.
        while (auto inf = current_infix_op()) {
            auto [tbp, rbp] = binding_power(*inf);
            SourceLocation l2 = peek().loc;
            std::string n2 = advance().text;
            ExprPtr r2 = parse_expr_prec(rbp);
            first = make_expr(EInfix{n2, std::move(first), std::move(r2)}, l2);
        }

        // where/whererec inside parens: (expr op right where ...)
        if (peek().kind == TokenKind::KW_WHERE) {
            SourceLocation wloc = advance().loc;
            auto binds = parse_local_binds();
            first = make_expr(EWhere{std::move(first), std::move(binds)}, wloc);
        } else if (peek().kind == TokenKind::KW_WHEREREC) {
            SourceLocation wloc = advance().loc;
            auto binds = parse_local_binds();
            first = make_expr(EWhereRec{std::move(first), std::move(binds)}, wloc);
        }

        // Could be an annotation: (expr op right : type)
        if (peek().kind == TokenKind::OPERATOR && peek().text == ":") {
            advance();
            TypePtr ty = parse_type();
            expect(TokenKind::RPAREN, "after type annotation");
            return make_expr(EAnnotate{std::move(first), std::move(ty)}, loc);
        }

        // Could still be a tuple: (1 + 2, 3)
        if (peek().kind == TokenKind::COMMA) {
            std::vector<ExprPtr> elems;
            elems.push_back(std::move(first));
            while (match(TokenKind::COMMA))
                elems.push_back(parse_expr_prec(0));
            expect(TokenKind::RPAREN, "after tuple elements");
            return make_expr(ETuple{std::move(elems)}, loc);
        }

        expect(TokenKind::RPAREN, "at end of parenthesised expression");
        return first;
    }

    // Unexpected token
    {
        std::ostringstream msg;
        msg << "unexpected token '" << peek().text << "' in parenthesised expression";
        error(msg.str(), peek().loc);
    }
}

// ============================================================
// Local bindings  (let/letrec/where/whererec)
// ============================================================

// Parses ONE binding: pat == expr, for let/letrec/where/whererec.
// After the binding, consumes 'in' if present (let/letrec form).
std::vector<LocalBind> Parser::parse_local_binds() {
    std::vector<LocalBind> binds;

    SourceLocation loc = peek().loc;
    PatPtr lhs = parse_pattern();      // binding LHS: variable, tuple, or infix pattern
    expect_op("==", "in local binding");
    ExprPtr body = parse_expr_prec(0);
    binds.push_back(LocalBind{std::move(lhs), std::move(body), loc});

    // Consume 'in' if present (let/letrec form only)
    if (check_ident("in")) advance();

    return binds;
}

// ============================================================
// Lambda clauses
// ============================================================

// Lambda body: one or more clauses separated by '|'
// Each clause: pat1 pat2 ... => body
std::vector<LambdaClause> Parser::parse_lambda_clauses() {
    std::vector<LambdaClause> clauses;

    do {
        // Parse one or more patterns (full patterns, not just atoms — allows ::)
        std::vector<PatPtr> pats;
        // Patterns continue until we see '=>'
        while (!check_op("=>") && !at_end()) {
            pats.push_back(parse_pattern());
        }
        expect_op("=>", "in lambda clause");
        ExprPtr body = parse_expr_prec(0);
        clauses.push_back(LambdaClause{std::move(pats), std::move(body)});
    } while (match_op("|"));

    return clauses;
}

} // namespace hope
