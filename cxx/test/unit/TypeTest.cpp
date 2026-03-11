// TypeTest.cpp — unit tests for the Hope type system (Phase 2).
//
// Tests cover:
//   - TyNode factory helpers
//   - Unification (success, failure, frozen variables, rollback)
//   - Expression type inference via TypeChecker::infer_top_expr
//   - Declaration processing via TypeChecker::check_decl

#include <gtest/gtest.h>

#include "ast/Ast.hpp"
#include "types/Type.hpp"
#include "types/TypeEnv.hpp"
#include "types/TypeChecker.hpp"
#include "types/TypeError.hpp"

namespace hope {
namespace {

// ---------------------------------------------------------------------------
// Helpers: build AST nodes with a dummy location
// ---------------------------------------------------------------------------

static SourceLocation loc() {
    return SourceLocation{"<test>", 1, 1, 0};
}

// Build a simple expression.
static ExprPtr evar(std::string name) {
    return make_expr(EVar{std::move(name)}, loc());
}

static ExprPtr elit_num(std::string text = "42") {
    return make_expr(ELit{LitNum{std::move(text)}}, loc());
}

static ExprPtr elit_char(std::string text = "'a'") {
    return make_expr(ELit{LitChar{std::move(text)}}, loc());
}

static ExprPtr elit_str(std::string text = "\"hello\"") {
    return make_expr(ELit{LitStr{std::move(text)}}, loc());
}

static ExprPtr eapply(ExprPtr f, ExprPtr a) {
    return make_expr(EApply{std::move(f), std::move(a)}, loc());
}

static ExprPtr eif(ExprPtr c, ExprPtr t, ExprPtr e) {
    return make_expr(EIf{std::move(c), std::move(t), std::move(e)}, loc());
}

static ExprPtr elist(std::vector<ExprPtr> elems) {
    return make_expr(EList{std::move(elems)}, loc());
}

static ExprPtr etuple(std::vector<ExprPtr> elems) {
    return make_expr(ETuple{std::move(elems)}, loc());
}

// Build a pattern.
static PatPtr pvar(std::string name) {
    return make_pat(PVar{std::move(name)}, loc());
}

static PatPtr pwild() {
    return make_pat(PWild{}, loc());
}

static PatPtr plit_num(std::string text = "0") {
    return make_pat(PLit{LitNum{std::move(text)}}, loc());
}

// Build a type.
static TypePtr tvar_ast(std::string name) {
    return make_type(TVar{std::move(name)}, loc());
}

static TypePtr tcons_ast(std::string name, std::vector<TypePtr> args = {}) {
    return make_type(TCons{std::move(name), std::move(args)}, loc());
}

static TypePtr tfun_ast(TypePtr dom, TypePtr cod) {
    return make_type(TFun{std::move(dom), std::move(cod)}, loc());
}

// ---------------------------------------------------------------------------
// Helper: make a fresh TypeChecker with a fresh TypeEnv
// ---------------------------------------------------------------------------

struct TC {
    TypeEnv     env;
    TypeChecker checker{env};
};

// ---------------------------------------------------------------------------
// Helper: print a TyRef for diagnostics (simple, not pretty)
// ---------------------------------------------------------------------------

static std::string show(TyRef t);

static std::string show_data(const TyNodeData& d) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TyVar>) {
            if (v.binding) return show(*v.binding);
            return "?t" + std::to_string(v.id);
        } else if constexpr (std::is_same_v<T, TyFrozen>) {
            return "!f" + std::to_string(v.id);
        } else {
            std::string s = v.name;
            if (!v.args.empty()) {
                s += "(";
                for (size_t i = 0; i < v.args.size(); ++i) {
                    if (i) s += ",";
                    s += show(v.args[i]);
                }
                s += ")";
            }
            return s;
        }
    }, d);
}

static std::string show(TyRef t) {
    if (!t) return "<null>";
    return show_data(t->data);
}

// ---------------------------------------------------------------------------
// ============================================================
// Unification tests
// ============================================================
// ---------------------------------------------------------------------------

// We test unification indirectly through infer_top_expr.
// For direct unification tests, we use a tiny helper TypeChecker.

// Two TyVars unify with each other (one gets bound to the other).
TEST(Unify, VarUnifiesWithCons) {
    TC tc;
    // infer `42` — result should be num
    auto e = elit_num();
    TyRef t = tc.checker.infer_top_expr(*e);
    // After deref, should be TyCons "num".
    ASSERT_NE(t, nullptr);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << "expected TyCons, got: " << show(t);
    EXPECT_EQ(c->name, "num");
    EXPECT_TRUE(c->args.empty());
}

TEST(Unify, TwoCons_SameName_SameArgs_Succeeds) {
    TC tc;
    // [ 1, 2, 3 ]  — all num, should succeed and give list(num)
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_num("2"));
    elems.push_back(elit_num("3"));
    auto e = elist(std::move(elems));
    TyRef t = tc.checker.infer_top_expr(*e);
    ASSERT_NE(t, nullptr);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "list");
    ASSERT_EQ(c->args.size(), 1u);
    auto* inner = std::get_if<TyCons>(&c->args[0]->data);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name, "num");
}

TEST(Unify, TwoCons_DifferentName_Fails) {
    TC tc;
    // [ 1, 'a' ]  — mixed types, should throw
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_char("'a'"));
    auto e = elist(std::move(elems));
    EXPECT_THROW(tc.checker.infer_top_expr(*e), TypeError);
}

TEST(Unify, FrozenCannotBeInstantiated) {
    // dec f : alpha -> alpha;
    // f 1;
    // This should work (num is an instance of alpha -> alpha).
    TC tc;

    // Declare: dec f : alpha -> alpha;
    {
        Decl d(DDec{
            {"f"},
            tfun_ast(tvar_ast("alpha"), tvar_ast("alpha"))
        }, loc());
        tc.checker.check_decl(d);
    }

    // Equation: f x <= x;
    {
        std::vector<PatPtr> args;
        args.push_back(pvar("x"));
        Decl d(DEquation{
            EquationLHS{"f", std::move(args), false},
            evar("x")
        }, loc());
        tc.checker.check_decl(d);
    }

    // The above should not throw — alpha -> alpha is the declared type
    // and the identity function is an instance of alpha -> alpha.
    // (The frozen-var check ensures the INFERRED type unifies with the frozen
    //  instantiation of alpha -> alpha.)
}

TEST(Unify, Rollback_OnFailure) {
    TC tc;
    // Infer a list with incompatible types.  After the throw, the checker
    // should be in a clean state.  Run a second (valid) inference after.
    {
        std::vector<ExprPtr> bad;
        bad.push_back(elit_num());
        bad.push_back(elit_char());
        auto e = elist(std::move(bad));
        EXPECT_THROW(tc.checker.infer_top_expr(*e), TypeError);
    }
    // Should still work for a valid expression.
    {
        auto e = elit_num();
        TyRef t = tc.checker.infer_top_expr(*e);
        auto* c = std::get_if<TyCons>(&t->data);
        ASSERT_NE(c, nullptr);
        EXPECT_EQ(c->name, "num");
    }
}

// ---------------------------------------------------------------------------
// ============================================================
// Type inference — literals
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeInfer, Literal_Num) {
    TC tc;
    auto e = elit_num("123");
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "num");
}

TEST(TypeInfer, Literal_Char) {
    TC tc;
    auto e = elit_char("'z'");
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "char");
}

TEST(TypeInfer, Literal_Str) {
    TC tc;
    auto e = elit_str("\"hi\"");
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "list") << show(t);
    ASSERT_EQ(c->args.size(), 1u);
    auto* inner = std::get_if<TyCons>(&c->args[0]->data);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name, "char");
}

// ---------------------------------------------------------------------------
// ============================================================
// Type inference — variables and errors
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeInfer, Var_Unbound_Errors) {
    TC tc;
    auto e = evar("zzz_undefined_zzz");
    EXPECT_THROW(tc.checker.infer_top_expr(*e), TypeError);
}

// ---------------------------------------------------------------------------
// ============================================================
// Type inference — if/then/else
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeInfer, If_CondMustBeBool) {
    TC tc;
    // if 42 then 1 else 2  — condition is num, not bool
    auto e = eif(elit_num("42"), elit_num("1"), elit_num("2"));
    EXPECT_THROW(tc.checker.infer_top_expr(*e), TypeError);
}

TEST(TypeInfer, If_BranchesMustMatch) {
    TC tc;
    // Declare 'true' and 'false' are already seeded in the env.
    // if true then 1 else 'a'  — branches have different types
    auto true_e = evar("true");
    auto e = eif(std::move(true_e), elit_num("1"), elit_char("'a'"));
    EXPECT_THROW(tc.checker.infer_top_expr(*e), TypeError);
}

TEST(TypeInfer, If_Valid) {
    TC tc;
    // if true then 1 else 2  — should give num
    auto e = eif(evar("true"), elit_num("1"), elit_num("2"));
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "num");
}

// ---------------------------------------------------------------------------
// ============================================================
// Type inference — lambda
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeInfer, Lambda_Identity) {
    TC tc;
    // lambda x => x  : a -> a  (where a is a fresh var)
    std::vector<PatPtr> pats;
    pats.push_back(pvar("x"));
    std::vector<LambdaClause> clauses;
    LambdaClause cl;
    cl.pats = std::move(pats);
    cl.body = evar("x");
    clauses.push_back(std::move(cl));
    auto e = make_expr(ELambda{std::move(clauses)}, loc());

    TyRef t = tc.checker.infer_top_expr(*e);
    // Should be a -> a, i.e. a TyCons "->" with two args that are the same var.
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "->");
    ASSERT_EQ(c->args.size(), 2u);
    // Both sides should be the same unification variable (or the same deref).
    // After deref, arg[0] and arg[1] should point to the same node.
    // (They were unified through the body `x`.)
}

TEST(TypeInfer, Lambda_Const) {
    TC tc;
    // lambda x => lambda y => x  : a -> b -> a
    std::vector<LambdaClause> inner_clauses;
    {
        LambdaClause cl;
        cl.pats.push_back(pvar("y"));
        cl.body = evar("x");
        inner_clauses.push_back(std::move(cl));
    }
    auto inner_lambda = make_expr(ELambda{std::move(inner_clauses)}, loc());

    std::vector<LambdaClause> outer_clauses;
    {
        LambdaClause cl;
        cl.pats.push_back(pvar("x"));
        cl.body = std::move(inner_lambda);
        outer_clauses.push_back(std::move(cl));
    }
    auto e = make_expr(ELambda{std::move(outer_clauses)}, loc());

    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "->");
}

// ---------------------------------------------------------------------------
// ============================================================
// Type inference — application
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeInfer, Application_Basic) {
    TC tc;
    // Declare:  dec succ : num -> num;
    {
        Decl d(DDec{
            {"succ"},
            tfun_ast(tcons_ast("num"), tcons_ast("num"))
        }, loc());
        tc.checker.check_decl(d);
    }
    // succ 42  : num
    auto e = eapply(evar("succ"), elit_num("42"));
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "num");
}

TEST(TypeInfer, Application_TypeMismatch) {
    TC tc;
    // Declare:  dec succ : num -> num;
    {
        Decl d(DDec{
            {"succ"},
            tfun_ast(tcons_ast("num"), tcons_ast("num"))
        }, loc());
        tc.checker.check_decl(d);
    }
    // succ 'a'  — char is not num
    auto e = eapply(evar("succ"), elit_char("'a'"));
    EXPECT_THROW(tc.checker.infer_top_expr(*e), TypeError);
}

// ---------------------------------------------------------------------------
// ============================================================
// Type inference — lists and tuples
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeInfer, List_Empty) {
    TC tc;
    auto e = elist({});
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "list");
    ASSERT_EQ(c->args.size(), 1u);
    // The element type is a fresh unification variable — that's fine.
}

TEST(TypeInfer, List_Homogeneous) {
    TC tc;
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_num("2"));
    auto e = elist(std::move(elems));
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "list");
    ASSERT_EQ(c->args.size(), 1u);
    auto* inner = std::get_if<TyCons>(&c->args[0]->data);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name, "num");
}

TEST(TypeInfer, Tuple_Pair) {
    TC tc;
    // (1, 'a')  : num # char
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_char("'a'"));
    auto e = etuple(std::move(elems));
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "#");
    ASSERT_EQ(c->args.size(), 2u);
    auto* l = std::get_if<TyCons>(&c->args[0]->data);
    auto* r = std::get_if<TyCons>(&c->args[1]->data);
    ASSERT_NE(l, nullptr);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(l->name, "num");
    EXPECT_EQ(r->name, "char");
}

TEST(TypeInfer, Tuple_Triple_RightNested) {
    TC tc;
    // (1, 'a', 2)  : num # (char # num)
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_char("'a'"));
    elems.push_back(elit_num("2"));
    auto e = etuple(std::move(elems));
    TyRef t = tc.checker.infer_top_expr(*e);
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "#");
    ASSERT_EQ(c->args.size(), 2u);
    auto* r = std::get_if<TyCons>(&c->args[1]->data);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->name, "#");
}

// ---------------------------------------------------------------------------
// ============================================================
// Declaration tests — data types
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeDecl, Data_Bool_Constructors) {
    TC tc;
    // 'true' and 'false' are seeded in the env.
    const ConInfo* t = tc.env.lookup_con("true");
    const ConInfo* f = tc.env.lookup_con("false");
    ASSERT_NE(t, nullptr);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(t->type_name, "bool");
    EXPECT_EQ(f->type_name, "bool");
    EXPECT_FALSE(t->arg.has_value());
    EXPECT_FALSE(f->arg.has_value());
}

TEST(TypeDecl, Data_List_Constructors) {
    TC tc;
    const ConInfo* nil = tc.env.lookup_con("nil");
    const ConInfo* cons = tc.env.lookup_con("::");
    ASSERT_NE(nil, nullptr);
    ASSERT_NE(cons, nullptr);
    EXPECT_EQ(nil->type_name, "list");
    EXPECT_EQ(cons->type_name, "list");
    EXPECT_FALSE(nil->arg.has_value());
    EXPECT_TRUE(cons->arg.has_value());
}

TEST(TypeDecl, Data_UserDefined) {
    TC tc;
    // data colour == red | green | blue;
    std::vector<Constructor> alts;
    alts.push_back(Constructor{"red",   std::nullopt, loc()});
    alts.push_back(Constructor{"green", std::nullopt, loc()});
    alts.push_back(Constructor{"blue",  std::nullopt, loc()});
    Decl d(DData{"colour", {}, std::move(alts)}, loc());
    tc.checker.check_decl(d);

    EXPECT_NE(tc.env.lookup_type("colour"), nullptr);
    EXPECT_NE(tc.env.lookup_con("red"),   nullptr);
    EXPECT_NE(tc.env.lookup_con("green"), nullptr);
    EXPECT_NE(tc.env.lookup_con("blue"),  nullptr);

    // 'red' should type-check as 'colour'
    TyRef t = tc.checker.infer_top_expr(*evar("red"));
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr) << show(t);
    EXPECT_EQ(c->name, "colour");
}

// ---------------------------------------------------------------------------
// ============================================================
// Declaration tests — function declarations
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeDecl, Dec_SingleName) {
    TC tc;
    // dec f : num -> num;
    Decl d(DDec{
        {"f"},
        tfun_ast(tcons_ast("num"), tcons_ast("num"))
    }, loc());
    tc.checker.check_decl(d);
    EXPECT_NE(tc.env.lookup_func("f"), nullptr);
}

TEST(TypeDecl, Dec_MultiName) {
    TC tc;
    // dec f, g : num -> num;
    Decl d(DDec{
        {"f", "g"},
        tfun_ast(tcons_ast("num"), tcons_ast("num"))
    }, loc());
    tc.checker.check_decl(d);
    EXPECT_NE(tc.env.lookup_func("f"), nullptr);
    EXPECT_NE(tc.env.lookup_func("g"), nullptr);
}

// ---------------------------------------------------------------------------
// ============================================================
// Declaration tests — equations and instance checking
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeDecl, Equation_MatchesDec) {
    TC tc;
    // dec double : num -> num;
    // double x <= x;  (wrong, but we can't add numbers without a decl for +)
    // For a simpler test: dec id : alpha -> alpha; id x <= x;
    {
        Decl d(DDec{
            {"id"},
            tfun_ast(tvar_ast("alpha"), tvar_ast("alpha"))
        }, loc());
        tc.checker.check_decl(d);
    }
    {
        std::vector<PatPtr> args;
        args.push_back(pvar("x"));
        Decl d(DEquation{
            EquationLHS{"id", std::move(args), false},
            evar("x")
        }, loc());
        EXPECT_NO_THROW(tc.checker.check_decl(d));
    }
}

TEST(TypeDecl, Equation_ConflictsDec_Error) {
    TC tc;
    // dec f : num -> num;
    // f x <= 'a';   — body is char, not num
    {
        Decl d(DDec{
            {"f"},
            tfun_ast(tcons_ast("num"), tcons_ast("num"))
        }, loc());
        tc.checker.check_decl(d);
    }
    {
        std::vector<PatPtr> args;
        args.push_back(pwild());
        Decl d(DEquation{
            EquationLHS{"f", std::move(args), false},
            elit_char("'a'")
        }, loc());
        EXPECT_THROW(tc.checker.check_decl(d), TypeError);
    }
}

TEST(TypeDecl, Equation_NoDecl_InfersFreely) {
    TC tc;
    // No dec; just an equation: f x <= x;
    // Should not throw — type is inferred freely.
    std::vector<PatPtr> args;
    args.push_back(pvar("x"));
    Decl d(DEquation{
        EquationLHS{"f", std::move(args), false},
        evar("x")
    }, loc());
    EXPECT_NO_THROW(tc.checker.check_decl(d));
}

TEST(TypeDecl, Equation_Nullary_Constructors) {
    TC tc;
    // Register a data type: data shape == circle | square;
    {
        std::vector<Constructor> alts;
        alts.push_back(Constructor{"circle", std::nullopt, loc()});
        alts.push_back(Constructor{"square", std::nullopt, loc()});
        Decl d(DData{"shape", {}, std::move(alts)}, loc());
        tc.checker.check_decl(d);
    }

    // circle  : shape
    TyRef t = tc.checker.infer_top_expr(*evar("circle"));
    auto* c = std::get_if<TyCons>(&t->data);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->name, "shape");
}

// ---------------------------------------------------------------------------
// ============================================================
// Type synonyms
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeDecl, TypeSynonym) {
    TC tc;
    // type mynum == num;
    Decl d(DType{"mynum", {}, tcons_ast("num")}, loc());
    tc.checker.check_decl(d);
    const TypeDef* td = tc.env.lookup_type("mynum");
    ASSERT_NE(td, nullptr);
    EXPECT_TRUE(td->is_synonym());
}

// ---------------------------------------------------------------------------
// ============================================================
// Built-in type lookup
// ============================================================
// ---------------------------------------------------------------------------

TEST(TypeEnvTest, BuiltinTypes_Present) {
    TypeEnv env;
    EXPECT_NE(env.lookup_type("num"),    nullptr);
    EXPECT_NE(env.lookup_type("char"),   nullptr);
    EXPECT_NE(env.lookup_type("bool"),   nullptr);
    EXPECT_NE(env.lookup_type("truval"), nullptr);
    EXPECT_NE(env.lookup_type("list"),   nullptr);
    EXPECT_NE(env.lookup_type("->"),     nullptr);
    EXPECT_NE(env.lookup_type("#"),      nullptr);
}

TEST(TypeEnvTest, BuiltinTypes_Kinds) {
    TypeEnv env;
    EXPECT_TRUE(env.lookup_type("num")->is_abstract());
    EXPECT_TRUE(env.lookup_type("char")->is_abstract());
    EXPECT_TRUE(env.lookup_type("bool")->is_data());
    EXPECT_TRUE(env.lookup_type("truval")->is_synonym());
    EXPECT_TRUE(env.lookup_type("list")->is_data());
    EXPECT_TRUE(env.lookup_type("->")->is_abstract());
    EXPECT_TRUE(env.lookup_type("#")->is_abstract());
}

} // namespace
} // namespace hope
