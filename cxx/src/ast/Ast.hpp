#pragma once

// All AST node types for the Hope language.
//
// Design: Expr, Pattern, Type, and Decl are wrapper structs containing a
// std::variant data field and a SourceLocation.  Recursive references use
// std::unique_ptr so that incomplete types are permitted at the point of use.
//
// All "box" helpers (make_expr, make_pat, make_type) create heap-allocated
// nodes; the caller gets a unique_ptr and never manages raw pointers.

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lexer/SourceLocation.hpp"

namespace hope {

// ---------------------------------------------------------------------------
// Forward declarations (required by recursive variant alternatives)
// ---------------------------------------------------------------------------
struct Expr;
struct Pattern;
struct Type;

using ExprPtr   = std::unique_ptr<Expr>;
using PatPtr    = std::unique_ptr<Pattern>;
using TypePtr   = std::unique_ptr<Type>;

// ---------------------------------------------------------------------------
// Literals  (shared by expressions and patterns)
// ---------------------------------------------------------------------------

struct LitNum   { std::string text; };   // integer literal text
struct LitFloat { std::string text; };   // floating-point literal text
struct LitChar  { std::string text; };   // char literal text, including quotes
struct LitStr   { std::string text; };   // string literal text, including quotes

using Literal = std::variant<LitNum, LitFloat, LitChar, LitStr>;

// ---------------------------------------------------------------------------
// Pattern alternatives
// ---------------------------------------------------------------------------

struct PVar    { std::string name; };                       // variable
struct PWild   {};                                          // _
struct PLit    { Literal lit; };                            // 0  'a'  "s"
struct PSucc   { PatPtr inner; };                           // succ(p)
struct PNPlusK { std::string var; int k; };                 // n+k pattern
struct PCons   { std::string name; std::optional<PatPtr> arg; }; // Ctor(p) or Ctor
struct PTuple  { std::vector<PatPtr> elems; };              // (p1, p2)
struct PList   { std::vector<PatPtr> elems; };              // [p1, p2, ...]
struct PInfix  { std::string op; PatPtr left; PatPtr right; }; // p1 :: p2 (or user infix)

struct Pattern {
    using Data = std::variant<PVar, PWild, PLit, PSucc, PNPlusK,
                              PCons, PTuple, PList, PInfix>;
    Data           data;
    SourceLocation loc;

    template<typename T>
    Pattern(T&& v, SourceLocation l)
        : data(std::forward<T>(v)), loc(std::move(l)) {}
};

// ---------------------------------------------------------------------------
// Type alternatives
// ---------------------------------------------------------------------------

struct TVar    { std::string name; };                        // alpha
struct TCons   { std::string name; std::vector<TypePtr> args; }; // list alpha
struct TFun    { TypePtr dom; TypePtr cod; };                // alpha -> beta
struct TProd   { TypePtr left; TypePtr right; };             // alpha # beta
struct TMu     { std::string var; TypePtr body; };           // mu t => body

struct Type {
    using Data = std::variant<TVar, TCons, TFun, TProd, TMu>;
    Data           data;
    SourceLocation loc;

    template<typename T>
    Type(T&& v, SourceLocation l)
        : data(std::forward<T>(v)), loc(std::move(l)) {}
};

// ---------------------------------------------------------------------------
// Expression alternatives
// ---------------------------------------------------------------------------

struct EVar    { std::string name; };                        // foo  or  +
struct ELit    { Literal lit; };
struct EApply  { ExprPtr func; ExprPtr arg; };               // f x
struct EInfix  { std::string op; ExprPtr left; ExprPtr right; }; // x + y
struct ETuple  { std::vector<ExprPtr> elems; };              // (a, b)
struct EList   { std::vector<ExprPtr> elems; };              // [a, b, c]
struct ESection { bool is_left; std::string op; ExprPtr expr; }; // (3+) or (+3)
struct EOpRef  { std::string op; };                          // (++) operator as value

// Lambda: multi-clause, each clause is (patterns, body)
struct LambdaClause { std::vector<PatPtr> pats; ExprPtr body; };
struct ELambda { std::vector<LambdaClause> clauses; };

// Local binding (shared by let/letrec/where/whererec)
struct LocalBind { PatPtr pat; ExprPtr body; SourceLocation loc; };

struct ELet       { std::vector<LocalBind> binds; ExprPtr body; };
struct ELetRec    { std::vector<LocalBind> binds; ExprPtr body; };
struct EWhere     { ExprPtr body; std::vector<LocalBind> binds; };
struct EWhereRec  { ExprPtr body; std::vector<LocalBind> binds; };

struct EIf       { ExprPtr cond; ExprPtr then_; ExprPtr else_; };
struct EWrite    { ExprPtr expr;
                   std::optional<std::string> file_path; }; // write expr [to "file"]
struct EAnnotate { ExprPtr expr; TypePtr type; }; // (expr : type) — type annotation

struct Expr {
    using Data = std::variant<
        EVar, ELit, EApply, EInfix, ETuple, EList,
        ESection, EOpRef, ELambda,
        ELet, ELetRec, EWhere, EWhereRec,
        EIf, EWrite, EAnnotate>;
    Data           data;
    SourceLocation loc;

    template<typename T>
    Expr(T&& v, SourceLocation l)
        : data(std::forward<T>(v)), loc(std::move(l)) {}
};

// ---------------------------------------------------------------------------
// Constructor (in data declarations)
// ---------------------------------------------------------------------------

struct Constructor {
    std::string        name;
    std::optional<TypePtr> arg;  // nullopt for zero-argument constructors
    SourceLocation     loc;
};

// ---------------------------------------------------------------------------
// Equation LHS
// ---------------------------------------------------------------------------

// An equation is:
//   --- f p1 p2 ... <= body          (prefix: func name + arg patterns)
//   --- pat1 op pat2 <= body         (infix: operator name + two patterns)
//
// For prefix equations, args holds all argument patterns in order.
// For infix equations (is_infix=true), args has exactly two elements.
struct EquationLHS {
    std::string           func;     // function or operator being defined
    std::vector<PatPtr>   args;     // argument patterns
    bool                  is_infix = false;
};

// ---------------------------------------------------------------------------
// Declaration alternatives
// ---------------------------------------------------------------------------

struct DInfix   { bool right_assoc; std::string name; int prec; };
struct DTypeVar { std::vector<std::string> names; };

struct DData {
    std::string            name;
    std::vector<std::string> params;
    std::vector<Constructor> alts;
};

struct DType {
    std::string              name;
    std::vector<std::string> params;
    TypePtr                  body;
};

struct DAbsType {
    TypePtr type;  // the abstract type expression (e.g. TFun, TProd, or TCons)
};

struct DDec {
    std::vector<std::string> names;  // one or more names: dec f, g : type;
    TypePtr                  type;
};

struct DEquation {
    EquationLHS lhs;
    ExprPtr     rhs;
};

struct DUses    { std::vector<std::string> module_names; };
struct DPrivate {};
struct DSave    { std::string module_name; };
struct DDisplay {};
struct DEdit    { std::string module_name; }; // empty = edit last loaded / temp
struct DEval    { ExprPtr expr; };   // expr; (evaluate and print)

struct Decl {
    using Data = std::variant<
        DInfix, DTypeVar, DData, DType, DAbsType,
        DDec, DEquation, DUses, DPrivate,
        DSave, DDisplay, DEdit, DEval>;
    Data           data;
    SourceLocation loc;

    template<typename T>
    Decl(T&& v, SourceLocation l)
        : data(std::forward<T>(v)), loc(std::move(l)) {}
};

// ---------------------------------------------------------------------------
// Factory helpers
// ---------------------------------------------------------------------------

template<typename T>
ExprPtr make_expr(T data, SourceLocation loc) {
    return std::make_unique<Expr>(std::move(data), std::move(loc));
}

template<typename T>
PatPtr make_pat(T data, SourceLocation loc) {
    return std::make_unique<Pattern>(std::move(data), std::move(loc));
}

template<typename T>
TypePtr make_type(T data, SourceLocation loc) {
    return std::make_unique<Type>(std::move(data), std::move(loc));
}

} // namespace hope
