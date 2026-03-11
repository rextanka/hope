// ParserTest.cpp — unit and component tests for the Hope parser.
//
// Conventions:
//   parse(src)       — parse src and return the decl vector (throws on error)
//   parse1(src)      — parse src, assert exactly one decl, return it
//   expr(src)        — parse a single DEval decl and return the wrapped ExprPtr&
//   is<T>(v)         — true if std::variant v holds alternative T
//   as<T>(v)         — std::get<T>(v)
//
// Tests are grouped by the node type they exercise.

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "ast/Ast.hpp"
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/ParseError.hpp"

namespace hope {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::vector<Decl> parse(std::string src) {
    Lexer   lex(std::move(src), "<test>");
    Parser  p(std::move(lex));
    return p.parse_program();
}

Decl parse1(std::string src) {
    auto decls = parse(std::move(src));
    EXPECT_EQ(decls.size(), 1u);
    return std::move(decls[0]);
}

// Parse an expression statement "src;" and return the Expr.
const Expr& expr_decl(const Decl& d) {
    return *std::get<DEval>(d.data).expr;
}

// Shorthand: parse "src;" and return the Expr.
Decl eval_decl(std::string src) {
    return parse1(src + ";");
}

// True if the Decl holds the given alternative.
template<typename T> bool is_decl(const Decl& d) {
    return std::holds_alternative<T>(d.data);
}
template<typename T> const T& as_decl(const Decl& d) {
    return std::get<T>(d.data);
}

template<typename T> bool is_expr(const Expr& e) {
    return std::holds_alternative<T>(e.data);
}
template<typename T> const T& as_expr(const Expr& e) {
    return std::get<T>(e.data);
}

template<typename T> bool is_pat(const Pattern& p) {
    return std::holds_alternative<T>(p.data);
}
template<typename T> const T& as_pat(const Pattern& p) {
    return std::get<T>(p.data);
}

template<typename T> bool is_type(const Type& t) {
    return std::holds_alternative<T>(t.data);
}
template<typename T> const T& as_type(const Type& t) {
    return std::get<T>(t.data);
}

// ---------------------------------------------------------------------------
// infix / infixr declarations
// ---------------------------------------------------------------------------

TEST(ParserDecl, InfixLeft) {
    auto d = parse1("infix xor : 5;");
    ASSERT_TRUE(is_decl<DInfix>(d));
    auto& di = as_decl<DInfix>(d);
    EXPECT_EQ(di.name, "xor");
    EXPECT_EQ(di.prec, 5);
    EXPECT_FALSE(di.right_assoc);
}

TEST(ParserDecl, InfixRight) {
    auto d = parse1("infixr ** : 8;");
    ASSERT_TRUE(is_decl<DInfix>(d));
    auto& di = as_decl<DInfix>(d);
    EXPECT_EQ(di.name, "**");
    EXPECT_EQ(di.prec, 8);
    EXPECT_TRUE(di.right_assoc);
}

TEST(ParserDecl, InfixOperatorSymbol) {
    auto d = parse1("infix <+> : 4;");
    ASSERT_TRUE(is_decl<DInfix>(d));
    EXPECT_EQ(as_decl<DInfix>(d).name, "<+>");
}

TEST(ParserDecl, InfixPrecRange) {
    EXPECT_NO_THROW(parse("infix foo : 1;"));
    EXPECT_NO_THROW(parse("infix foo : 9;"));
    EXPECT_THROW(parse("infix foo : 0;"), ParseError);
    EXPECT_THROW(parse("infix foo : 10;"), ParseError);
}

TEST(ParserDecl, InfixUpdatesExprParser) {
    // After declaring 'xor' infix 5, expressions should parse it as infix.
    auto decls = parse("infix xor : 5;\n a xor b;");
    ASSERT_EQ(decls.size(), 2u);
    // Second decl is DEval wrapping EInfix("xor", ...)
    ASSERT_TRUE(is_decl<DEval>(decls[1]));
    auto& e = expr_decl(decls[1]);
    ASSERT_TRUE(is_expr<EInfix>(e));
    EXPECT_EQ(as_expr<EInfix>(e).op, "xor");
}

// ---------------------------------------------------------------------------
// typevar
// ---------------------------------------------------------------------------

TEST(ParserDecl, TypeVar_Single) {
    auto d = parse1("typevar alpha;");
    ASSERT_TRUE(is_decl<DTypeVar>(d));
    auto& dtv = as_decl<DTypeVar>(d);
    ASSERT_EQ(dtv.names.size(), 1u);
    EXPECT_EQ(dtv.names[0], "alpha");
}

TEST(ParserDecl, TypeVar_Multiple) {
    auto d = parse1("typevar alpha, beta, gamma;");
    ASSERT_TRUE(is_decl<DTypeVar>(d));
    auto& names = as_decl<DTypeVar>(d).names;
    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "beta");
    EXPECT_EQ(names[2], "gamma");
}

// ---------------------------------------------------------------------------
// data declarations
// ---------------------------------------------------------------------------

TEST(ParserDecl, Data_NoArgs) {
    auto d = parse1("data bool == false ++ true;");
    ASSERT_TRUE(is_decl<DData>(d));
    auto& dd = as_decl<DData>(d);
    EXPECT_EQ(dd.name, "bool");
    EXPECT_TRUE(dd.params.empty());
    ASSERT_EQ(dd.alts.size(), 2u);
    EXPECT_EQ(dd.alts[0].name, "false");
    EXPECT_EQ(dd.alts[1].name, "true");
    EXPECT_FALSE(dd.alts[0].arg.has_value());
    EXPECT_FALSE(dd.alts[1].arg.has_value());
}

TEST(ParserDecl, Data_OneParam) {
    auto d = parse1("data maybe(alpha) == nothing ++ just alpha;");
    ASSERT_TRUE(is_decl<DData>(d));
    auto& dd = as_decl<DData>(d);
    EXPECT_EQ(dd.name, "maybe");
    ASSERT_EQ(dd.params.size(), 1u);
    EXPECT_EQ(dd.params[0], "alpha");
    ASSERT_EQ(dd.alts.size(), 2u);
    EXPECT_EQ(dd.alts[0].name, "nothing");
    EXPECT_FALSE(dd.alts[0].arg.has_value());
    EXPECT_EQ(dd.alts[1].name, "just");
    EXPECT_TRUE(dd.alts[1].arg.has_value());
}

TEST(ParserDecl, Data_SingleAlt) {
    auto d = parse1("data wrap(alpha) == wrap alpha;");
    ASSERT_TRUE(is_decl<DData>(d));
    auto& dd = as_decl<DData>(d);
    ASSERT_EQ(dd.alts.size(), 1u);
    EXPECT_EQ(dd.alts[0].name, "wrap");
    EXPECT_TRUE(dd.alts[0].arg.has_value());
}

// ---------------------------------------------------------------------------
// type declarations
// ---------------------------------------------------------------------------

TEST(ParserDecl, TypeAlias_Simple) {
    auto d = parse1("type string == list char;");
    ASSERT_TRUE(is_decl<DType>(d));
    auto& dt = as_decl<DType>(d);
    EXPECT_EQ(dt.name, "string");
    EXPECT_TRUE(dt.params.empty());
    // body: TCons("list", [TVar("char")])
    ASSERT_TRUE(is_type<TCons>(*dt.body));
    auto& tc = as_type<TCons>(*dt.body);
    EXPECT_EQ(tc.name, "list");
    ASSERT_EQ(tc.args.size(), 1u);
    EXPECT_TRUE(is_type<TVar>(*tc.args[0]));
    EXPECT_EQ(as_type<TVar>(*tc.args[0]).name, "char");
}

TEST(ParserDecl, TypeAlias_WithParam) {
    auto d = parse1("type pair(alpha) == alpha # alpha;");
    ASSERT_TRUE(is_decl<DType>(d));
    auto& dt = as_decl<DType>(d);
    EXPECT_EQ(dt.name, "pair");
    ASSERT_EQ(dt.params.size(), 1u);
}

// ---------------------------------------------------------------------------
// abstype
// ---------------------------------------------------------------------------

TEST(ParserDecl, AbsType) {
    auto d = parse1("abstype counter;");
    ASSERT_TRUE(is_decl<DAbsType>(d));
    // "counter" parses as TVar since it has no args
    ASSERT_TRUE(is_type<TVar>(*as_decl<DAbsType>(d).type));
    EXPECT_EQ(as_type<TVar>(*as_decl<DAbsType>(d).type).name, "counter");
}

TEST(ParserDecl, AbsType_WithParam) {
    auto d = parse1("abstype queue alpha;");  // unparenthesised param
    ASSERT_TRUE(is_decl<DAbsType>(d));
    // "queue alpha" parses as TCons("queue", [TVar("alpha")])
    ASSERT_TRUE(is_type<TCons>(*as_decl<DAbsType>(d).type));
    auto& tc = as_type<TCons>(*as_decl<DAbsType>(d).type);
    EXPECT_EQ(tc.name, "queue");
    ASSERT_EQ(tc.args.size(), 1u);
}

TEST(ParserDecl, AbsType_FunType) {
    // Standard.hop: infixr -> : 2; abstype neg -> pos;
    auto decls = parse("infixr -> : 2;\nabstype neg -> pos;");
    ASSERT_EQ(decls.size(), 2u);
    ASSERT_TRUE(is_decl<DAbsType>(decls[1]));
    ASSERT_TRUE(is_type<TFun>(*as_decl<DAbsType>(decls[1]).type));
}

// ---------------------------------------------------------------------------
// dec declarations
// ---------------------------------------------------------------------------

TEST(ParserDecl, Dec_Simple) {
    auto d = parse1("dec not : bool -> bool;");
    ASSERT_TRUE(is_decl<DDec>(d));
    auto& dd = as_decl<DDec>(d);
    ASSERT_EQ(dd.names.size(), 1u);
    EXPECT_EQ(dd.names[0], "not");
    ASSERT_TRUE(is_type<TFun>(*dd.type));
}

TEST(ParserDecl, Dec_MultiName) {
    auto d = parse1("dec f, g : num -> num;");
    ASSERT_TRUE(is_decl<DDec>(d));
    auto& dd = as_decl<DDec>(d);
    ASSERT_EQ(dd.names.size(), 2u);
    EXPECT_EQ(dd.names[0], "f");
    EXPECT_EQ(dd.names[1], "g");
}

TEST(ParserDecl, Dec_HigherOrder) {
    auto d = parse1("dec map : (alpha -> beta) -> list alpha -> list beta;");
    ASSERT_TRUE(is_decl<DDec>(d));
    EXPECT_EQ(as_decl<DDec>(d).names[0], "map");
}

TEST(ParserDecl, Dec_Operator) {
    auto d = parse1("dec + : num -> num -> num;");
    ASSERT_TRUE(is_decl<DDec>(d));
    EXPECT_EQ(as_decl<DDec>(d).names[0], "+");
}

// ---------------------------------------------------------------------------
// equations
// ---------------------------------------------------------------------------

TEST(ParserDecl, Equation_Simple) {
    auto d = parse1("--- not false <= true;");
    ASSERT_TRUE(is_decl<DEquation>(d));
    auto& de = as_decl<DEquation>(d);
    EXPECT_EQ(de.lhs.func, "not");
    ASSERT_EQ(de.lhs.args.size(), 1u);
    // "false" starts lowercase — parsed as PVar (parser can't distinguish
    // constructors from variables without type information)
    ASSERT_TRUE(is_pat<PVar>(*de.lhs.args[0]));
    EXPECT_EQ(as_pat<PVar>(*de.lhs.args[0]).name, "false");
    EXPECT_FALSE(de.lhs.is_infix);
    ASSERT_TRUE(is_expr<EVar>(*de.rhs));
    EXPECT_EQ(as_expr<EVar>(*de.rhs).name, "true");
}

TEST(ParserDecl, Equation_Var) {
    auto d = parse1("--- id x <= x;");
    ASSERT_TRUE(is_decl<DEquation>(d));
    auto& de = as_decl<DEquation>(d);
    EXPECT_EQ(de.lhs.func, "id");
    ASSERT_EQ(de.lhs.args.size(), 1u);
    ASSERT_TRUE(is_pat<PVar>(*de.lhs.args[0]));
    EXPECT_EQ(as_pat<PVar>(*de.lhs.args[0]).name, "x");
}

TEST(ParserDecl, Equation_MultiArg) {
    auto d = parse1("--- max x y <= if x > y then x else y;");
    ASSERT_TRUE(is_decl<DEquation>(d));
    auto& de = as_decl<DEquation>(d);
    EXPECT_EQ(de.lhs.func, "max");
    ASSERT_EQ(de.lhs.args.size(), 2u);
    EXPECT_FALSE(de.lhs.is_infix);
}

TEST(ParserDecl, Equation_RhsInfix) {
    auto d = parse1("--- succ_of n <= n + 1;");
    ASSERT_TRUE(is_decl<DEquation>(d));
    auto& de = as_decl<DEquation>(d);
    ASSERT_TRUE(is_expr<EInfix>(*de.rhs));
    EXPECT_EQ(as_expr<EInfix>(*de.rhs).op, "+");
}

TEST(ParserDecl, Equation_OpRef_LHS) {
    // --- (^) <= pow;  — operator reference in LHS
    auto d = parse1("--- (^) <= pow;");
    ASSERT_TRUE(is_decl<DEquation>(d));
    auto& de = as_decl<DEquation>(d);
    EXPECT_EQ(de.lhs.func, "^");
    EXPECT_EQ(de.lhs.args.size(), 0u);
    EXPECT_FALSE(de.lhs.is_infix);
}

// ---------------------------------------------------------------------------
// uses
// ---------------------------------------------------------------------------

TEST(ParserDecl, Uses) {
    auto d = parse1("uses Standard;");
    ASSERT_TRUE(is_decl<DUses>(d));
    EXPECT_EQ(as_decl<DUses>(d).module_name, "Standard");
}

// ---------------------------------------------------------------------------
// commands
// ---------------------------------------------------------------------------

TEST(ParserDecl, Display) {
    auto d = parse1("display;");
    EXPECT_TRUE(is_decl<DDisplay>(d));
}

TEST(ParserDecl, Save) {
    auto d = parse1("save MyMod;");
    ASSERT_TRUE(is_decl<DSave>(d));
    EXPECT_EQ(as_decl<DSave>(d).module_name, "MyMod");
}

// ---------------------------------------------------------------------------
// Type parsing
// ---------------------------------------------------------------------------

TEST(ParserType, Var) {
    auto d = parse1("dec f : alpha;");
    ASSERT_TRUE(is_type<TVar>(*as_decl<DDec>(d).type));
    EXPECT_EQ(as_type<TVar>(*as_decl<DDec>(d).type).name, "alpha");
}

TEST(ParserType, Fun_RightAssoc) {
    // alpha -> beta -> gamma  parses as  alpha -> (beta -> gamma)
    auto d = parse1("dec f : alpha -> beta -> gamma;");
    auto& ty = *as_decl<DDec>(d).type;
    ASSERT_TRUE(is_type<TFun>(ty));
    auto& outer = as_type<TFun>(ty);
    ASSERT_TRUE(is_type<TVar>(*outer.dom));
    EXPECT_EQ(as_type<TVar>(*outer.dom).name, "alpha");
    // codomain is beta -> gamma
    ASSERT_TRUE(is_type<TFun>(*outer.cod));
}

TEST(ParserType, Prod) {
    auto d = parse1("dec f : alpha # beta;");
    auto& ty = *as_decl<DDec>(d).type;
    ASSERT_TRUE(is_type<TProd>(ty));
}

TEST(ParserType, Prod_HigherThanFun) {
    // alpha # beta -> gamma  parses as  (alpha # beta) -> gamma
    auto d = parse1("dec f : alpha # beta -> gamma;");
    auto& ty = *as_decl<DDec>(d).type;
    ASSERT_TRUE(is_type<TFun>(ty));
    auto& tf = as_type<TFun>(ty);
    ASSERT_TRUE(is_type<TProd>(*tf.dom));
}

TEST(ParserType, List_Sugar) {
    // [alpha]  =>  TCons("list", [TVar("alpha")])
    auto d = parse1("dec f : [alpha];");
    auto& ty = *as_decl<DDec>(d).type;
    ASSERT_TRUE(is_type<TCons>(ty));
    auto& tc = as_type<TCons>(ty);
    EXPECT_EQ(tc.name, "list");
    ASSERT_EQ(tc.args.size(), 1u);
}

TEST(ParserType, TypeCons_Applied) {
    auto d = parse1("dec f : list alpha;");
    auto& ty = *as_decl<DDec>(d).type;
    ASSERT_TRUE(is_type<TCons>(ty));
    EXPECT_EQ(as_type<TCons>(ty).name, "list");
}

TEST(ParserType, Parenthesised) {
    auto d = parse1("dec f : (alpha -> beta) -> gamma;");
    auto& ty = *as_decl<DDec>(d).type;
    ASSERT_TRUE(is_type<TFun>(ty));
    auto& tf = as_type<TFun>(ty);
    // dom is a TFun wrapped in parens
    ASSERT_TRUE(is_type<TFun>(*tf.dom));
}

// ---------------------------------------------------------------------------
// Pattern parsing
// ---------------------------------------------------------------------------

TEST(ParserPattern, Var) {
    auto d = parse1("--- f x <= x;");
    auto& args = as_decl<DEquation>(d).lhs.args;
    ASSERT_EQ(args.size(), 1u);
    ASSERT_TRUE(is_pat<PVar>(*args[0]));
}

TEST(ParserPattern, Constructor_NoArg) {
    // "false" is lowercase → parsed as PVar (no type info at parse time)
    auto d = parse1("--- not false <= true;");
    auto& args = as_decl<DEquation>(d).lhs.args;
    ASSERT_EQ(args.size(), 1u);
    ASSERT_TRUE(is_pat<PVar>(*args[0]));
    EXPECT_EQ(as_pat<PVar>(*args[0]).name, "false");
}

TEST(ParserPattern, Constructor_WithArg) {
    // "Just" starts uppercase → parsed as PCons
    auto d = parse1("--- f (Just x) <= x;");
    auto& args = as_decl<DEquation>(d).lhs.args;
    ASSERT_EQ(args.size(), 1u);
    ASSERT_TRUE(is_pat<PCons>(*args[0]));
    EXPECT_EQ(as_pat<PCons>(*args[0]).name, "Just");
    EXPECT_TRUE(as_pat<PCons>(*args[0]).arg.has_value());
}

TEST(ParserPattern, Succ) {
    // "--- f (succ(n)) <= n;" — succ(n) is PSucc
    auto d = parse1("--- f (succ(n)) <= n;");
    auto& args = as_decl<DEquation>(d).lhs.args;
    ASSERT_EQ(args.size(), 1u);
    ASSERT_TRUE(is_pat<PSucc>(*args[0]));
    ASSERT_TRUE(is_pat<PVar>(*as_pat<PSucc>(*args[0]).inner));
    EXPECT_EQ(as_pat<PVar>(*as_pat<PSucc>(*args[0]).inner).name, "n");
}

TEST(ParserPattern, NilList) {
    auto d = parse1("--- f [] <= 0;");
    auto& args = as_decl<DEquation>(d).lhs.args;
    ASSERT_EQ(args.size(), 1u);
    ASSERT_TRUE(is_pat<PCons>(*args[0]));
    EXPECT_EQ(as_pat<PCons>(*args[0]).name, "nil");
}

TEST(ParserPattern, ConsList_WithColons) {
    // In a full pattern context (not equation lhs atom):
    // pattern "x :: xs" is PInfix("::", PVar(x), PVar(xs))
    // We test this via type — but equation lhs uses parse_pattern_atom.
    // Instead, parse a lambda pattern which uses parse_pattern() (full).
    auto d = eval_decl(R"(\ x :: xs => x)");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELambda>(e));
    auto& lm = as_expr<ELambda>(e);
    ASSERT_EQ(lm.clauses.size(), 1u);
    ASSERT_EQ(lm.clauses[0].pats.size(), 1u);
    auto& p = *lm.clauses[0].pats[0];
    ASSERT_TRUE(is_pat<PInfix>(p));
    EXPECT_EQ(as_pat<PInfix>(p).op, "::");
}

TEST(ParserPattern, Tuple) {
    // Lambda with tuple pattern: \ (x, y) => x
    auto d = eval_decl(R"(\ (x, y) => x)");
    auto& e = expr_decl(d);
    auto& lm = as_expr<ELambda>(e);
    auto& p = *lm.clauses[0].pats[0];
    ASSERT_TRUE(is_pat<PTuple>(p));
    EXPECT_EQ(as_pat<PTuple>(p).elems.size(), 2u);
}

TEST(ParserPattern, ListLiteral) {
    // Lambda with list literal pattern: \ [a, b] => a
    auto d = eval_decl(R"(\ [a, b] => a)");
    auto& e = expr_decl(d);
    auto& lm = as_expr<ELambda>(e);
    auto& p = *lm.clauses[0].pats[0];
    ASSERT_TRUE(is_pat<PList>(p));
    EXPECT_EQ(as_pat<PList>(p).elems.size(), 2u);
}

TEST(ParserPattern, IntLiteral) {
    auto d = eval_decl(R"(\ 0 => true)");
    auto& lm = as_expr<ELambda>(expr_decl(d));
    ASSERT_TRUE(is_pat<PLit>(*lm.clauses[0].pats[0]));
}

TEST(ParserPattern, NPlusK) {
    // n+k pattern inside a lambda: \ n+1 => n
    auto d = eval_decl(R"(\ n+1 => n)");
    auto& lm = as_expr<ELambda>(expr_decl(d));
    auto& p = *lm.clauses[0].pats[0];
    ASSERT_TRUE(is_pat<PNPlusK>(p));
    EXPECT_EQ(as_pat<PNPlusK>(p).k, 1);
}

TEST(ParserPattern, Wildcard) {
    auto d = eval_decl(R"(\ _ => 0)");
    auto& lm = as_expr<ELambda>(expr_decl(d));
    // '_' is an IDENT, parsed as PVar("_") — check the name
    ASSERT_TRUE(is_pat<PVar>(*lm.clauses[0].pats[0]));
    EXPECT_EQ(as_pat<PVar>(*lm.clauses[0].pats[0]).name, "_");
}

// ---------------------------------------------------------------------------
// Expression parsing — atoms and application
// ---------------------------------------------------------------------------

TEST(ParserExpr, Var) {
    auto d = eval_decl("foo");
    ASSERT_TRUE(is_expr<EVar>(expr_decl(d)));
    EXPECT_EQ(as_expr<EVar>(expr_decl(d)).name, "foo");
}

TEST(ParserExpr, IntLit) {
    auto d = eval_decl("42");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELit>(e));
    EXPECT_TRUE(std::holds_alternative<LitNum>(as_expr<ELit>(e).lit));
    EXPECT_EQ(std::get<LitNum>(as_expr<ELit>(e).lit).text, "42");
}

TEST(ParserExpr, FloatLit) {
    auto d = eval_decl("3.14");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELit>(e));
    EXPECT_TRUE(std::holds_alternative<LitFloat>(as_expr<ELit>(e).lit));
}

TEST(ParserExpr, CharLit) {
    auto d = eval_decl("'a'");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELit>(e));
    EXPECT_TRUE(std::holds_alternative<LitChar>(as_expr<ELit>(e).lit));
}

TEST(ParserExpr, StrLit) {
    auto d = eval_decl(R"("hello")");
    ASSERT_TRUE(is_expr<ELit>(expr_decl(d)));
}

TEST(ParserExpr, Application) {
    auto d = eval_decl("f x");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EApply>(e));
    auto& ap = as_expr<EApply>(e);
    ASSERT_TRUE(is_expr<EVar>(*ap.func));
    EXPECT_EQ(as_expr<EVar>(*ap.func).name, "f");
    ASSERT_TRUE(is_expr<EVar>(*ap.arg));
    EXPECT_EQ(as_expr<EVar>(*ap.arg).name, "x");
}

TEST(ParserExpr, Application_MultiArg) {
    // f x y  =>  (f x) y
    auto d = eval_decl("f x y");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EApply>(e));
    auto& outer = as_expr<EApply>(e);
    ASSERT_TRUE(is_expr<EApply>(*outer.func));  // f x
    ASSERT_TRUE(is_expr<EVar>(*outer.arg));
    EXPECT_EQ(as_expr<EVar>(*outer.arg).name, "y");
}

TEST(ParserExpr, ListEmpty) {
    auto d = eval_decl("[]");
    ASSERT_TRUE(is_expr<EList>(expr_decl(d)));
    EXPECT_EQ(as_expr<EList>(expr_decl(d)).elems.size(), 0u);
}

TEST(ParserExpr, ListLiteral) {
    auto d = eval_decl("[1, 2, 3]");
    ASSERT_TRUE(is_expr<EList>(expr_decl(d)));
    EXPECT_EQ(as_expr<EList>(expr_decl(d)).elems.size(), 3u);
}

TEST(ParserExpr, Tuple) {
    auto d = eval_decl("(1, 2)");
    ASSERT_TRUE(is_expr<ETuple>(expr_decl(d)));
    EXPECT_EQ(as_expr<ETuple>(expr_decl(d)).elems.size(), 2u);
}

TEST(ParserExpr, Tuple_Triple) {
    auto d = eval_decl("(1, 2, 3)");
    ASSERT_TRUE(is_expr<ETuple>(expr_decl(d)));
    EXPECT_EQ(as_expr<ETuple>(expr_decl(d)).elems.size(), 3u);
}

// ---------------------------------------------------------------------------
// Expression parsing — Pratt operator precedence
// ---------------------------------------------------------------------------

TEST(ParserExpr, Infix_Basic) {
    auto d = eval_decl("a + b");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EInfix>(e));
    EXPECT_EQ(as_expr<EInfix>(e).op, "+");
}

TEST(ParserExpr, Infix_Prec_MulOverAdd) {
    // 1 + 2 * 3  =>  1 + (2 * 3)  because * has prec 7, + has prec 6
    auto d = eval_decl("1 + 2 * 3");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EInfix>(e));
    auto& add = as_expr<EInfix>(e);
    EXPECT_EQ(add.op, "+");
    // right side is 2 * 3
    ASSERT_TRUE(is_expr<EInfix>(*add.right));
    EXPECT_EQ(as_expr<EInfix>(*add.right).op, "*");
}

TEST(ParserExpr, Infix_Prec_AddOverMul_Reversed) {
    // 1 * 2 + 3  =>  (1 * 2) + 3
    auto d = eval_decl("1 * 2 + 3");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EInfix>(e));
    auto& add = as_expr<EInfix>(e);
    EXPECT_EQ(add.op, "+");
    // left side is 1 * 2
    ASSERT_TRUE(is_expr<EInfix>(*add.left));
    EXPECT_EQ(as_expr<EInfix>(*add.left).op, "*");
}

TEST(ParserExpr, Infix_LeftAssoc) {
    // 1 + 2 + 3  =>  (1 + 2) + 3
    auto d = eval_decl("1 + 2 + 3");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EInfix>(e));
    auto& outer = as_expr<EInfix>(e);
    EXPECT_EQ(outer.op, "+");
    // left is 1 + 2
    ASSERT_TRUE(is_expr<EInfix>(*outer.left));
    EXPECT_EQ(as_expr<EInfix>(*outer.left).op, "+");
}

TEST(ParserExpr, Infix_RightAssoc_Cons) {
    // a :: b :: c  =>  a :: (b :: c)  because :: is infixr
    auto d = eval_decl("a :: b :: c");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EInfix>(e));
    auto& outer = as_expr<EInfix>(e);
    EXPECT_EQ(outer.op, "::");
    // right is b :: c
    ASSERT_TRUE(is_expr<EInfix>(*outer.right));
    EXPECT_EQ(as_expr<EInfix>(*outer.right).op, "::");
}

TEST(ParserExpr, Infix_RightAssoc_Arrow) {
    // Checking -> right-assoc in expressions isn't applicable (it's a type op).
    // Test infixr explicitly: declare one, then use it.
    auto decls = parse("infixr ** : 8;\n a ** b ** c;");
    ASSERT_EQ(decls.size(), 2u);
    auto& e = expr_decl(decls[1]);
    ASSERT_TRUE(is_expr<EInfix>(e));
    auto& outer = as_expr<EInfix>(e);
    EXPECT_EQ(outer.op, "**");
    // right is b ** c
    ASSERT_TRUE(is_expr<EInfix>(*outer.right));
}

TEST(ParserExpr, Infix_ComparePrec) {
    // x + y = z  =>  (x + y) = z  because + (6) > = (4)
    auto d = eval_decl("x + y = z");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EInfix>(e));
    auto& outer = as_expr<EInfix>(e);
    EXPECT_EQ(outer.op, "=");
    ASSERT_TRUE(is_expr<EInfix>(*outer.left));
    EXPECT_EQ(as_expr<EInfix>(*outer.left).op, "+");
}

// ---------------------------------------------------------------------------
// Operator sections
// ---------------------------------------------------------------------------

TEST(ParserExpr, OpRef) {
    // (+) — operator as first-class value
    auto d = eval_decl("(+)");
    ASSERT_TRUE(is_expr<EOpRef>(expr_decl(d)));
    EXPECT_EQ(as_expr<EOpRef>(expr_decl(d)).op, "+");
}

TEST(ParserExpr, LeftSection) {
    // (3+) — left section
    auto d = eval_decl("(3+)");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ESection>(e));
    auto& sec = as_expr<ESection>(e);
    EXPECT_TRUE(sec.is_left);
    EXPECT_EQ(sec.op, "+");
    ASSERT_TRUE(is_expr<ELit>(*sec.expr));
}

TEST(ParserExpr, RightSection) {
    // (+3) — right section
    auto d = eval_decl("(+3)");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ESection>(e));
    auto& sec = as_expr<ESection>(e);
    EXPECT_FALSE(sec.is_left);
    EXPECT_EQ(sec.op, "+");
}

TEST(ParserExpr, NegativeRightSection) {
    // (-1) — right section with minus, not negation
    auto d = eval_decl("(-1)");
    auto& e = expr_decl(d);
    // This is ESection{false, "-", ELit(1)}
    ASSERT_TRUE(is_expr<ESection>(e));
    auto& sec = as_expr<ESection>(e);
    EXPECT_FALSE(sec.is_left);
    EXPECT_EQ(sec.op, "-");
}

TEST(ParserExpr, MinusOpRef) {
    // (-) — minus as operator reference
    auto d = eval_decl("(-)");
    ASSERT_TRUE(is_expr<EOpRef>(expr_decl(d)));
    EXPECT_EQ(as_expr<EOpRef>(expr_decl(d)).op, "-");
}

// ---------------------------------------------------------------------------
// if / then / else
// ---------------------------------------------------------------------------

TEST(ParserExpr, If) {
    auto d = eval_decl("if x then y else z");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EIf>(e));
    auto& eif = as_expr<EIf>(e);
    ASSERT_TRUE(is_expr<EVar>(*eif.cond));
    ASSERT_TRUE(is_expr<EVar>(*eif.then_));
    ASSERT_TRUE(is_expr<EVar>(*eif.else_));
}

TEST(ParserExpr, If_Nested) {
    auto d = eval_decl("if a then if b then c else d else e");
    ASSERT_TRUE(is_expr<EIf>(expr_decl(d)));
}

// ---------------------------------------------------------------------------
// Lambda
// ---------------------------------------------------------------------------

TEST(ParserExpr, Lambda_Single) {
    auto d = eval_decl(R"(\ x => x)");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELambda>(e));
    auto& lm = as_expr<ELambda>(e);
    ASSERT_EQ(lm.clauses.size(), 1u);
    ASSERT_EQ(lm.clauses[0].pats.size(), 1u);
    ASSERT_TRUE(is_expr<EVar>(*lm.clauses[0].body));
}

TEST(ParserExpr, Lambda_MultiClause) {
    auto d = eval_decl(R"(\ 0 => 1 | n => n * 2)");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELambda>(e));
    auto& lm = as_expr<ELambda>(e);
    ASSERT_EQ(lm.clauses.size(), 2u);
}

TEST(ParserExpr, Lambda_MultiPat) {
    // \ x y => x
    auto d = eval_decl(R"(\ x y => x)");
    auto& lm = as_expr<ELambda>(expr_decl(d));
    ASSERT_EQ(lm.clauses[0].pats.size(), 2u);
}

// ---------------------------------------------------------------------------
// let / letrec / where / whererec
// ---------------------------------------------------------------------------

TEST(ParserExpr, Let) {
    auto d = eval_decl("let x == 1 in x");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<ELet>(e));
    auto& el = as_expr<ELet>(e);
    ASSERT_EQ(el.binds.size(), 1u);
    ASSERT_TRUE(is_pat<PVar>(*el.binds[0].pat));
    EXPECT_EQ(as_pat<PVar>(*el.binds[0].pat).name, "x");
    ASSERT_TRUE(is_expr<EVar>(*el.body));
}

TEST(ParserExpr, LetRec) {
    // letrec with a single binding — just check it parses without error
    EXPECT_NO_THROW(eval_decl("letrec f == f in f"));
}

// Hope's let/letrec has ONE binding only — multiple binds test removed.
TEST(ParserExpr, Let_SingleBind) {
    auto d = eval_decl("let x == 1 in x");
    auto& el = as_expr<ELet>(expr_decl(d));
    ASSERT_EQ(el.binds.size(), 1u);
    ASSERT_TRUE(is_pat<PVar>(*el.binds[0].pat));
    EXPECT_EQ(as_pat<PVar>(*el.binds[0].pat).name, "x");
}

TEST(ParserExpr, Where) {
    // "f x where x == 1;"
    auto d = eval_decl("f x where x == 1");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EWhere>(e));
    auto& ew = as_expr<EWhere>(e);
    ASSERT_EQ(ew.binds.size(), 1u);
    ASSERT_TRUE(is_pat<PVar>(*ew.binds[0].pat));
    EXPECT_EQ(as_pat<PVar>(*ew.binds[0].pat).name, "x");
}

TEST(ParserExpr, WhereRec) {
    auto d = eval_decl("f x whererec g == 42");
    ASSERT_TRUE(is_expr<EWhereRec>(expr_decl(d)));
}

// ---------------------------------------------------------------------------
// write command
// ---------------------------------------------------------------------------

TEST(ParserExpr, Write) {
    auto d = parse1("write 42;");
    ASSERT_TRUE(is_decl<DEval>(d));
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EWrite>(e));
}

// ---------------------------------------------------------------------------
// Complex / integration expressions
// ---------------------------------------------------------------------------

TEST(ParserExpr, Complex_FibLike) {
    // let fib == \ 0 => 0 | 1 => 1 | n => fib (n-1) + fib (n-2) in fib 10
    EXPECT_NO_THROW(eval_decl(
        "let fib == \\ 0 => 0 | 1 => 1 | n => n - 1 in fib 10"
    ));
}

TEST(ParserExpr, Complex_NestedApplication) {
    // f (g x) (h y)
    auto d = eval_decl("f (g x) (h y)");
    auto& e = expr_decl(d);
    ASSERT_TRUE(is_expr<EApply>(e));
}

TEST(ParserExpr, Complex_MixedPrecedence) {
    EXPECT_NO_THROW(eval_decl("not (x = y) where x == 1"));
}

// ---------------------------------------------------------------------------
// Parse errors
// ---------------------------------------------------------------------------

TEST(ParserError, MissingSemicolon) {
    EXPECT_THROW(parse("dec f : num"), ParseError);
}

TEST(ParserError, BadInfixPrec) {
    EXPECT_THROW(parse("infix + : 0;"), ParseError);
    EXPECT_THROW(parse("infixr + : 10;"), ParseError);
}

TEST(ParserError, BadTypeToken) {
    EXPECT_THROW(parse("dec f : 42;"), ParseError);
}

TEST(ParserError, EmptyParens) {
    EXPECT_THROW(eval_decl("()"), ParseError);
}

TEST(ParserError, MissingThen) {
    EXPECT_THROW(eval_decl("if x y"), ParseError);
}

TEST(ParserError, MissingElse) {
    EXPECT_THROW(eval_decl("if x then y"), ParseError);
}

TEST(ParserError, MissingLambdaArrow) {
    EXPECT_THROW(eval_decl("\\ x x"), ParseError);
}

// ---------------------------------------------------------------------------
// Parsing real Hope source files from lib/
// ---------------------------------------------------------------------------

// Helper: try to parse a .hop file; return true on success.
bool parse_hop_file(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    try {
        Lexer  lex(ss.str(), path.string());  // full path so Parser can find dependencies
        Parser p(std::move(lex));
        p.parse_program();
        return true;
    } catch (const ParseError& e) {
        std::fprintf(stderr, "  parse error in %s at %d:%d: %s\n",
                     path.filename().c_str(), e.loc.line, e.loc.column, e.what());
        return false;
    } catch (const std::exception& e) {
        // Print error so test output shows what went wrong
        std::fprintf(stderr, "  parse error in %s: %s\n",
                     path.filename().c_str(), e.what());
        return false;
    }
}

class LibParseTest : public ::testing::TestWithParam<std::filesystem::path> {};

TEST_P(LibParseTest, ParsesWithoutError) {
    EXPECT_TRUE(parse_hop_file(GetParam()))
        << "Failed to parse: " << GetParam();
}

// Build the parameter list from ../../../lib/*.hop relative to the test binary.
// At configure time the lib directory is at <repo_root>/lib.
// We embed the path via CMake compile definition HOPE_LIB_DIR.
std::vector<std::filesystem::path> hop_lib_files() {
    std::vector<std::filesystem::path> files;
#ifdef HOPE_LIB_DIR
    std::filesystem::path dir(HOPE_LIB_DIR);
    if (std::filesystem::exists(dir)) {
        for (auto& e : std::filesystem::directory_iterator(dir)) {
            if (e.path().extension() == ".hop")
                files.push_back(e.path());
        }
    }
#endif
    return files;
}

INSTANTIATE_TEST_SUITE_P(
    HopLib, LibParseTest,
    ::testing::ValuesIn(hop_lib_files()),
    [](const auto& info) {
        return info.param.stem().string();
    }
);

// ---------------------------------------------------------------------------
// Parsing real Hope test input files from test/
// ---------------------------------------------------------------------------

std::vector<std::filesystem::path> hop_test_files() {
    std::vector<std::filesystem::path> files;
#ifdef HOPE_TEST_DIR
    std::filesystem::path dir(HOPE_TEST_DIR);
    if (std::filesystem::exists(dir)) {
        for (auto& e : std::filesystem::directory_iterator(dir)) {
            if (e.path().extension() == ".in")
                files.push_back(e.path());
        }
    }
#endif
    return files;
}

class TestInputParseTest : public ::testing::TestWithParam<std::filesystem::path> {};

TEST_P(TestInputParseTest, ParsesWithoutError) {
    EXPECT_TRUE(parse_hop_file(GetParam()))
        << "Failed to parse: " << GetParam();
}

INSTANTIATE_TEST_SUITE_P(
    HopeTests, TestInputParseTest,
    ::testing::ValuesIn(hop_test_files()),
    [](const auto& info) {
        return info.param.stem().string();
    }
);

} // namespace
} // namespace hope
