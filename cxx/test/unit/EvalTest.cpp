// EvalTest.cpp — unit tests for the Hope runtime evaluator (Phase 3).
//
// Tests construct AST nodes directly and call Evaluator::eval_top.
// No file I/O or module loading is required.

#include <gtest/gtest.h>

#include "ast/Ast.hpp"
#include "runtime/Evaluator.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/Value.hpp"
#include "types/TypeEnv.hpp"

namespace hope {
namespace {

// ---------------------------------------------------------------------------
// Helpers: dummy source location and AST builder functions
// ---------------------------------------------------------------------------

static SourceLocation loc() {
    return SourceLocation{"<eval-test>", 1, 1, 0};
}

// Expression builders
static ExprPtr evar(std::string name) {
    return make_expr(EVar{std::move(name)}, loc());
}

static ExprPtr elit_num(std::string text) {
    return make_expr(ELit{LitNum{std::move(text)}}, loc());
}

static ExprPtr elit_char(std::string text) {
    return make_expr(ELit{LitChar{std::move(text)}}, loc());
}

static ExprPtr elit_str(std::string text) {
    return make_expr(ELit{LitStr{std::move(text)}}, loc());
}

static ExprPtr eapply(ExprPtr f, ExprPtr a) {
    return make_expr(EApply{std::move(f), std::move(a)}, loc());
}

static ExprPtr einfix(std::string op, ExprPtr l, ExprPtr r) {
    return make_expr(EInfix{std::move(op), std::move(l), std::move(r)}, loc());
}

static ExprPtr etuple(std::vector<ExprPtr> elems) {
    return make_expr(ETuple{std::move(elems)}, loc());
}

static ExprPtr elist(std::vector<ExprPtr> elems) {
    return make_expr(EList{std::move(elems)}, loc());
}

static ExprPtr eif(ExprPtr c, ExprPtr t, ExprPtr e) {
    return make_expr(EIf{std::move(c), std::move(t), std::move(e)}, loc());
}

static ExprPtr elet(PatPtr pat, ExprPtr bind_body, ExprPtr body) {
    LocalBind b;
    b.pat  = std::move(pat);
    b.body = std::move(bind_body);
    b.loc  = loc();
    std::vector<LocalBind> binds;
    binds.push_back(std::move(b));
    return make_expr(ELet{std::move(binds), std::move(body)}, loc());
}

static ExprPtr eletrec(PatPtr pat, ExprPtr bind_body, ExprPtr body) {
    LocalBind b;
    b.pat  = std::move(pat);
    b.body = std::move(bind_body);
    b.loc  = loc();
    std::vector<LocalBind> binds;
    binds.push_back(std::move(b));
    return make_expr(ELetRec{std::move(binds), std::move(body)}, loc());
}

// Pattern builders
static PatPtr pvar(std::string name) {
    return make_pat(PVar{std::move(name)}, loc());
}

static PatPtr pwild() {
    return make_pat(PWild{}, loc());
}

static PatPtr plit_num(std::string text) {
    return make_pat(PLit{LitNum{std::move(text)}}, loc());
}

static PatPtr pcons0(std::string name) {
    return make_pat(PCons{std::move(name), std::nullopt}, loc());
}

static PatPtr psucc(PatPtr inner) {
    return make_pat(PSucc{std::move(inner)}, loc());
}

static PatPtr pinfix_cons(PatPtr l, PatPtr r) {
    return make_pat(PInfix{"::", std::move(l), std::move(r)}, loc());
}

// Build a lambda with multiple clauses, each having 1 pattern and a body.
// Each element of pairs is {pattern, body}.
static ExprPtr elambda_multi(
    std::vector<std::pair<PatPtr, ExprPtr>> clause_data) {
    std::vector<LambdaClause> clauses;
    for (auto& [pat, body] : clause_data) {
        LambdaClause cl;
        cl.pats.push_back(std::move(pat));
        cl.body = std::move(body);
        clauses.push_back(std::move(cl));
    }
    return make_expr(ELambda{std::move(clauses)}, loc());
}

static ExprPtr elambda1(PatPtr pat, ExprPtr body) {
    std::vector<LambdaClause> clauses;
    LambdaClause cl;
    cl.pats.push_back(std::move(pat));
    cl.body = std::move(body);
    clauses.push_back(std::move(cl));
    return make_expr(ELambda{std::move(clauses)}, loc());
}

// Build a DEquation decl (non-owning pattern/body pointers into the AST)
// Since DEquation owns its lhs/rhs, we build it with actual patterns.
static Decl make_equation(std::string fname,
                           std::vector<PatPtr> args,
                           ExprPtr rhs) {
    EquationLHS lhs;
    lhs.func     = std::move(fname);
    lhs.args     = std::move(args);
    lhs.is_infix = false;
    DEquation eq;
    eq.lhs = std::move(lhs);
    eq.rhs = std::move(rhs);
    return Decl(std::move(eq), loc());
}

// ---------------------------------------------------------------------------
// Test fixture: a fresh TypeEnv and Evaluator for each test.
// ---------------------------------------------------------------------------

struct EV {
    TypeEnv  env;
    Evaluator eval{env};
};

// ---------------------------------------------------------------------------
// Forced value accessors (for assertions)
// ---------------------------------------------------------------------------

static double get_num(Evaluator& ev, ValRef v) {
    ValRef f = ev.force(v);
    auto* vn = std::get_if<VNum>(&f->data);
    if (!vn) throw std::runtime_error("get_num: not a VNum");
    return vn->n;
}

static char get_char(Evaluator& ev, ValRef v) {
    ValRef f = ev.force(v);
    auto* vc = std::get_if<VChar>(&f->data);
    if (!vc) throw std::runtime_error("get_char: not a VChar");
    return vc->c;
}

static std::string get_cons_name(Evaluator& ev, ValRef v) {
    ValRef f = ev.force(v);
    auto* vc = std::get_if<VCons>(&f->data);
    if (!vc) throw std::runtime_error("get_cons_name: not a VCons");
    return vc->name;
}

// Collect a list-of-chars into a std::string (for string literal tests)
static std::string list_to_string(Evaluator& ev, ValRef v) {
    std::string result;
    ValRef cur = ev.force(v);
    while (true) {
        auto* vc = std::get_if<VCons>(&cur->data);
        if (!vc || vc->name == "nil") break;
        if (vc->name != "::") break;
        ValRef arg = ev.force(vc->arg);
        auto* pr = std::get_if<VPair>(&arg->data);
        if (!pr) break;
        ValRef head = ev.force(pr->left);
        auto* ch = std::get_if<VChar>(&head->data);
        if (!ch) break;
        result += ch->c;
        cur = ev.force(pr->right);
    }
    return result;
}

// ---------------------------------------------------------------------------
// ============================================================
// Literal tests
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Literal_Num) {
    EV ev;
    auto e = elit_num("42");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 42.0);
}

TEST(Eval, Literal_Num_Float) {
    EV ev;
    auto e = make_expr(ELit{LitFloat{"3.14"}}, loc());
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_NEAR(get_num(ev.eval, v), 3.14, 1e-9);
}

TEST(Eval, Literal_Char) {
    EV ev;
    auto e = elit_char("'a'");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(get_char(ev.eval, v), 'a');
}

TEST(Eval, Literal_Char_Escape) {
    EV ev;
    auto e = elit_char("'\\n'");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(get_char(ev.eval, v), '\n');
}

TEST(Eval, Literal_String) {
    EV ev;
    auto e = elit_str("\"hi\"");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(list_to_string(ev.eval, v), "hi");
}

TEST(Eval, Literal_String_Escape) {
    EV ev;
    auto e = elit_str("\"a\\nb\"");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(list_to_string(ev.eval, v), "a\nb");
}

TEST(Eval, Literal_String_Empty) {
    EV ev;
    auto e = elit_str("\"\"");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(list_to_string(ev.eval, v), "");
    // Should be nil
    ValRef f = ev.eval.force(v);
    ASSERT_EQ(get_cons_name(ev.eval, f), "nil");
}

// ---------------------------------------------------------------------------
// ============================================================
// Built-in arithmetic
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Apply_Add) {
    EV ev;
    // 3 + 5 via EInfix
    auto e = einfix("+", elit_num("3"), elit_num("5"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 8.0);
}

TEST(Eval, Apply_Sub) {
    EV ev;
    auto e = einfix("-", elit_num("10"), elit_num("3"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 7.0);
}

TEST(Eval, Apply_Mul) {
    EV ev;
    auto e = einfix("*", elit_num("3"), elit_num("4"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 12.0);
}

TEST(Eval, Apply_Div) {
    EV ev;
    auto e = einfix("/", elit_num("10"), elit_num("4"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 2.5);
}

TEST(Eval, Apply_IntDiv) {
    EV ev;
    auto e = einfix("div", elit_num("10"), elit_num("3"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 3.0);
}

TEST(Eval, Apply_Mod) {
    EV ev;
    auto e = einfix("mod", elit_num("10"), elit_num("3"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 1.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// Comparison and boolean
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Compare_Eq_True) {
    EV ev;
    auto e = einfix("=", elit_num("5"), elit_num("5"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(get_cons_name(ev.eval, v), "true");
}

TEST(Eval, Compare_Eq_False) {
    EV ev;
    auto e = einfix("=", elit_num("5"), elit_num("6"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(get_cons_name(ev.eval, v), "false");
}

TEST(Eval, Compare_Lt) {
    EV ev;
    auto e = einfix("<", elit_num("3"), elit_num("5"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(get_cons_name(ev.eval, v), "true");
}

// ---------------------------------------------------------------------------
// ============================================================
// if/then/else
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, If_True) {
    EV ev;
    auto cond = evar("true");
    auto e = eif(std::move(cond), elit_num("1"), elit_num("2"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 1.0);
}

TEST(Eval, If_False) {
    EV ev;
    auto cond = evar("false");
    auto e = eif(std::move(cond), elit_num("1"), elit_num("2"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 2.0);
}

TEST(Eval, If_Nested) {
    EV ev;
    // if true then (if false then 10 else 20) else 30
    auto inner = eif(evar("false"), elit_num("10"), elit_num("20"));
    auto e = eif(evar("true"), std::move(inner), elit_num("30"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 20.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// Lambda and application
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Lambda_Identity) {
    EV ev;
    // (lambda x => x) 42
    auto lam = elambda1(pvar("x"), evar("x"));
    auto e = eapply(std::move(lam), elit_num("42"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 42.0);
}

TEST(Eval, Lambda_Add_One) {
    EV ev;
    // (lambda x => x + 1) 5
    auto body = einfix("+", evar("x"), elit_num("1"));
    auto lam = elambda1(pvar("x"), std::move(body));
    auto e = eapply(std::move(lam), elit_num("5"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 6.0);
}

TEST(Eval, Lambda_Multi_Clause) {
    EV ev;
    // (lambda 0 => 99 | n => n * 2) 0 = 99
    // (lambda 0 => 99 | n => n * 2) 5 = 10
    std::vector<std::pair<PatPtr, ExprPtr>> clause_data0;
    // Build it once to test both
    {
        // clause 1: 0 => 99
        // clause 2: n => n * 2
        std::vector<LambdaClause> clauses;
        {
            LambdaClause cl;
            cl.pats.push_back(plit_num("0"));
            cl.body = elit_num("99");
            clauses.push_back(std::move(cl));
        }
        {
            LambdaClause cl;
            cl.pats.push_back(pvar("n"));
            cl.body = einfix("*", evar("n"), elit_num("2"));
            clauses.push_back(std::move(cl));
        }
        auto lam = make_expr(ELambda{std::move(clauses)}, loc());
        auto e = eapply(std::move(lam), elit_num("0"));
        ValRef v = ev.eval.eval_top(*e);
        ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 99.0);
    }
    {
        std::vector<LambdaClause> clauses;
        {
            LambdaClause cl;
            cl.pats.push_back(plit_num("0"));
            cl.body = elit_num("99");
            clauses.push_back(std::move(cl));
        }
        {
            LambdaClause cl;
            cl.pats.push_back(pvar("n"));
            cl.body = einfix("*", evar("n"), elit_num("2"));
            clauses.push_back(std::move(cl));
        }
        auto lam = make_expr(ELambda{std::move(clauses)}, loc());
        auto e = eapply(std::move(lam), elit_num("5"));
        ValRef v = ev.eval.eval_top(*e);
        ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 10.0);
    }
}

// ---------------------------------------------------------------------------
// ============================================================
// Tuples / pairs
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Tuple_Pair) {
    EV ev;
    // (1, 2) = VPair{1, 2}
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_num("2"));
    auto e = etuple(std::move(elems));
    ValRef v = ev.eval.eval_top(*e);
    ValRef f = ev.eval.force(v);
    auto* pr = std::get_if<VPair>(&f->data);
    ASSERT_NE(pr, nullptr);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, pr->left),  1.0);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, pr->right), 2.0);
}

TEST(Eval, Tuple_Triple_Right_Nested) {
    EV ev;
    // (1, 2, 3) = VPair{1, VPair{2, 3}}
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_num("2"));
    elems.push_back(elit_num("3"));
    auto e = etuple(std::move(elems));
    ValRef v = ev.eval.eval_top(*e);
    ValRef f = ev.eval.force(v);
    auto* pr = std::get_if<VPair>(&f->data);
    ASSERT_NE(pr, nullptr);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, pr->left), 1.0);
    ValRef right = ev.eval.force(pr->right);
    auto* pr2 = std::get_if<VPair>(&right->data);
    ASSERT_NE(pr2, nullptr);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, pr2->left),  2.0);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, pr2->right), 3.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// Lists
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, List_Empty) {
    EV ev;
    auto e = elist({});
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(get_cons_name(ev.eval, v), "nil");
}

TEST(Eval, List_Cons) {
    EV ev;
    // [1, 2, 3]
    std::vector<ExprPtr> elems;
    elems.push_back(elit_num("1"));
    elems.push_back(elit_num("2"));
    elems.push_back(elit_num("3"));
    auto e = elist(std::move(elems));
    ValRef v = ev.eval.eval_top(*e);

    // Walk the list and check elements
    std::vector<double> nums;
    ValRef cur = ev.eval.force(v);
    while (true) {
        auto* vc = std::get_if<VCons>(&cur->data);
        ASSERT_NE(vc, nullptr);
        if (vc->name == "nil") break;
        ASSERT_EQ(vc->name, "::");
        ValRef arg = ev.eval.force(vc->arg);
        auto* pr = std::get_if<VPair>(&arg->data);
        ASSERT_NE(pr, nullptr);
        nums.push_back(get_num(ev.eval, pr->left));
        cur = ev.eval.force(pr->right);
    }
    ASSERT_EQ(nums.size(), 3u);
    ASSERT_DOUBLE_EQ(nums[0], 1.0);
    ASSERT_DOUBLE_EQ(nums[1], 2.0);
    ASSERT_DOUBLE_EQ(nums[2], 3.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// let binding
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Let_Binding) {
    EV ev;
    // let x == 5 in x + 1
    auto body = einfix("+", evar("x"), elit_num("1"));
    auto e = elet(pvar("x"), elit_num("5"), std::move(body));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 6.0);
}

TEST(Eval, Let_Shadowing) {
    EV ev;
    // let x == 3 in let x == x + 1 in x
    // The inner x shadows the outer one.
    auto inner_body = evar("x");
    auto inner_bind = einfix("+", evar("x"), elit_num("1"));
    auto inner_let = elet(pvar("x"), std::move(inner_bind), std::move(inner_body));
    auto e = elet(pvar("x"), elit_num("3"), std::move(inner_let));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 4.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// letrec (recursive binding)
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, LetRec_Binding) {
    EV ev;
    // letrec fact == lambda 0 => 1 | n => n * fact (n - 1) in fact 5
    // Build the lambda body
    std::vector<LambdaClause> clauses;
    {
        // clause 1: 0 => 1
        LambdaClause cl;
        cl.pats.push_back(plit_num("0"));
        cl.body = elit_num("1");
        clauses.push_back(std::move(cl));
    }
    {
        // clause 2: n => n * fact (n - 1)
        LambdaClause cl;
        cl.pats.push_back(pvar("n"));
        // fact (n - 1)
        auto n_minus_1 = einfix("-", evar("n"), elit_num("1"));
        auto recursive_call = eapply(evar("fact"), std::move(n_minus_1));
        cl.body = einfix("*", evar("n"), std::move(recursive_call));
        clauses.push_back(std::move(cl));
    }
    auto lam = make_expr(ELambda{std::move(clauses)}, loc());

    // letrec fact == lam in fact 5
    auto apply_5 = eapply(evar("fact"), elit_num("5"));
    auto e = eletrec(pvar("fact"), std::move(lam), std::move(apply_5));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 120.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// Equation-defined functions (DEquation declarations)
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Equation_Definition_Simple) {
    EV ev;

    // dec double (optional — evaluator doesn't need type declarations)
    // --- double x <= x * 2;
    // double 7
    {
        std::vector<PatPtr> args;
        args.push_back(pvar("x"));
        Decl d = make_equation("double", std::move(args),
                               einfix("*", evar("x"), elit_num("2")));
        ev.eval.add_decl(std::move(d));
    }

    auto e = eapply(evar("double"), elit_num("7"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 14.0);
}

TEST(Eval, Equation_Definition_Recursive) {
    EV ev;

    // --- fib 0 <= 0;
    // --- fib 1 <= 1;
    // --- fib n <= fib (n - 1) + fib (n - 2);
    {
        std::vector<PatPtr> args;
        args.push_back(plit_num("0"));
        ev.eval.add_decl(make_equation("fib", std::move(args), elit_num("0")));
    }
    {
        std::vector<PatPtr> args;
        args.push_back(plit_num("1"));
        ev.eval.add_decl(make_equation("fib", std::move(args), elit_num("1")));
    }
    {
        std::vector<PatPtr> args;
        args.push_back(pvar("n"));
        auto call1 = eapply(evar("fib"), einfix("-", evar("n"), elit_num("1")));
        auto call2 = eapply(evar("fib"), einfix("-", evar("n"), elit_num("2")));
        auto body  = einfix("+", std::move(call1), std::move(call2));
        ev.eval.add_decl(make_equation("fib", std::move(args), std::move(body)));
    }

    // fib 8 = 21
    auto e = eapply(evar("fib"), elit_num("8"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 21.0);
}

TEST(Eval, Equation_Two_Args) {
    EV ev;

    // --- add x y <= x + y;
    // add 3 4 = 7
    {
        std::vector<PatPtr> args;
        args.push_back(pvar("x"));
        args.push_back(pvar("y"));
        auto body = einfix("+", evar("x"), evar("y"));
        ev.eval.add_decl(make_equation("add", std::move(args), std::move(body)));
    }

    auto e = eapply(eapply(evar("add"), elit_num("3")), elit_num("4"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 7.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// Pattern matching — constructors
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Pattern_Constructor_Nullary) {
    EV ev;

    // data colour == red | green | blue;
    // Register the data type via DData
    {
        std::vector<Constructor> alts;
        alts.push_back(Constructor{"red",   std::nullopt, loc()});
        alts.push_back(Constructor{"green", std::nullopt, loc()});
        alts.push_back(Constructor{"blue",  std::nullopt, loc()});
        Decl d(DData{"colour", {}, std::move(alts)}, loc());
        ev.eval.add_decl(std::move(d));
    }

    // --- is_red red   <= true;
    // --- is_red _     <= false;
    {
        std::vector<PatPtr> args;
        args.push_back(pcons0("red"));
        ev.eval.add_decl(make_equation("is_red", std::move(args), evar("true")));
    }
    {
        std::vector<PatPtr> args;
        args.push_back(pwild());
        ev.eval.add_decl(make_equation("is_red", std::move(args), evar("false")));
    }

    ASSERT_EQ(get_cons_name(ev.eval, ev.eval.eval_top(*eapply(evar("is_red"), evar("red")))),   "true");
    ASSERT_EQ(get_cons_name(ev.eval, ev.eval.eval_top(*eapply(evar("is_red"), evar("green")))), "false");
    ASSERT_EQ(get_cons_name(ev.eval, ev.eval.eval_top(*eapply(evar("is_red"), evar("blue")))),  "false");
}

TEST(Eval, Pattern_List) {
    EV ev;

    // --- sum nil        <= 0;
    // --- sum (h :: t)   <= h + sum t;
    {
        std::vector<PatPtr> args;
        args.push_back(pcons0("nil"));
        ev.eval.add_decl(make_equation("sum", std::move(args), elit_num("0")));
    }
    {
        std::vector<PatPtr> args;
        args.push_back(pinfix_cons(pvar("h"), pvar("t")));
        auto call = eapply(evar("sum"), evar("t"));
        auto body = einfix("+", evar("h"), std::move(call));
        ev.eval.add_decl(make_equation("sum", std::move(args), std::move(body)));
    }

    // sum [1, 2, 3, 4, 5] = 15
    std::vector<ExprPtr> elems;
    for (int i = 1; i <= 5; ++i)
        elems.push_back(elit_num(std::to_string(i)));
    auto e = eapply(evar("sum"), elist(std::move(elems)));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 15.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// Lazy evaluation / thunks
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, LazyEval_Thunk) {
    EV ev;

    // Confirm that the argument to a function is not evaluated before it
    // needs to be.  We use 'const' (lambda x => lambda _ => x) applied to
    // 42, and then to an expression that would throw if evaluated.

    // Build: const_fn = lambda x => lambda _ => x
    std::vector<LambdaClause> inner_cls;
    {
        LambdaClause cl;
        cl.pats.push_back(pwild());
        cl.body = evar("x");
        inner_cls.push_back(std::move(cl));
    }
    auto inner_lam = make_expr(ELambda{std::move(inner_cls)}, loc());

    std::vector<LambdaClause> outer_cls;
    {
        LambdaClause cl;
        cl.pats.push_back(pvar("x"));
        cl.body = std::move(inner_lam);
        outer_cls.push_back(std::move(cl));
    }
    auto const_fn = make_expr(ELambda{std::move(outer_cls)}, loc());

    // Apply const_fn 42 to (1/0) — the 1/0 should never be evaluated.
    // (1/0) expressed as EInfix{"/", 1, 0}
    auto div_by_zero = einfix("/", elit_num("1"), elit_num("0"));

    auto partial = eapply(std::move(const_fn), elit_num("42"));
    auto e = eapply(std::move(partial), std::move(div_by_zero));

    // Should return 42 without throwing
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 42.0);
}

TEST(Eval, BlackHole_Detected) {
    EV ev;

    // A directly infinite loop: letrec x == x in x
    // When forced, this should throw RuntimeError (black hole).
    auto e = eletrec(pvar("x"), evar("x"), evar("x"));
    ValRef v = ev.eval.eval_top(*e);
    EXPECT_THROW(ev.eval.force(v), RuntimeError);
}

// ---------------------------------------------------------------------------
// ============================================================
// Section expressions
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Section_Left) {
    EV ev;
    // (3+) 5 = 8: left section
    auto sect = make_expr(ESection{true, "+", elit_num("3")}, loc());
    auto e = eapply(std::move(sect), elit_num("5"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 8.0);
}

TEST(Eval, Section_Right) {
    EV ev;
    // (+5) 3 = 8: right section
    auto sect = make_expr(ESection{false, "+", elit_num("5")}, loc());
    auto e = eapply(std::move(sect), elit_num("3"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 8.0);
}

// ---------------------------------------------------------------------------
// ============================================================
// print_value
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, PrintValue_Num) {
    EV ev;
    auto v = make_num(42.0);
    ASSERT_EQ(ev.eval.print_value(v), "42");
}

TEST(Eval, PrintValue_Char) {
    EV ev;
    auto v = make_char('x');
    ASSERT_EQ(ev.eval.print_value(v), "'x'");
}

TEST(Eval, PrintValue_Bool_True) {
    EV ev;
    ASSERT_EQ(ev.eval.print_value(make_bool(true)),  "true");
}

TEST(Eval, PrintValue_Bool_False) {
    EV ev;
    ASSERT_EQ(ev.eval.print_value(make_bool(false)), "false");
}

TEST(Eval, PrintValue_List_Nums) {
    EV ev;
    auto v = make_cons(make_num(1), make_cons(make_num(2), make_nil()));
    ASSERT_EQ(ev.eval.print_value(v), "[1, 2]");
}

TEST(Eval, PrintValue_String) {
    EV ev;
    auto e = elit_str("\"hello\"");
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_EQ(ev.eval.print_value(v), "\"hello\"");
}

TEST(Eval, PrintValue_Pair) {
    EV ev;
    auto v = make_pair(make_num(1), make_num(2));
    // Pairs print as (1, 2)
    std::string s = ev.eval.print_value(v);
    ASSERT_EQ(s, "(1, 2)");
}

TEST(Eval, PrintValue_Nil) {
    EV ev;
    ASSERT_EQ(ev.eval.print_value(make_nil()), "nil");
}

// ---------------------------------------------------------------------------
// ============================================================
// succ pattern
// ============================================================
// ---------------------------------------------------------------------------

TEST(Eval, Pattern_Succ) {
    EV ev;
    // --- pred_of (succ n) <= n;
    // pred_of 5 = 4
    {
        std::vector<PatPtr> args;
        args.push_back(psucc(pvar("n")));
        ev.eval.add_decl(make_equation("pred_of", std::move(args), evar("n")));
    }
    auto e = eapply(evar("pred_of"), elit_num("5"));
    ValRef v = ev.eval.eval_top(*e);
    ASSERT_DOUBLE_EQ(get_num(ev.eval, v), 4.0);
}

// succ 0 should fail (no match)
TEST(Eval, Pattern_Succ_ZeroFails) {
    EV ev;
    {
        std::vector<PatPtr> args;
        args.push_back(psucc(pvar("n")));
        ev.eval.add_decl(make_equation("pred_of", std::move(args), evar("n")));
    }
    auto e = eapply(evar("pred_of"), elit_num("0"));
    EXPECT_THROW(ev.eval.eval_top(*e), RuntimeError);
}

} // namespace
} // namespace hope
