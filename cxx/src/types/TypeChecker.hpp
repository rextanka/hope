#pragma once

// TypeChecker — Hindley-Milner type inference for the Hope language.
//
// Usage:
//   TypeEnv env;
//   TypeChecker tc(env);
//   tc.check_program(decls);          // throws TypeError on failure
//
// For REPL use, check_decl() processes one declaration at a time, and
// infer_top_expr() infers the type of a bare expression.

#include <string>
#include <unordered_map>
#include <vector>

#include "ast/Ast.hpp"
#include "types/Type.hpp"
#include "types/TypeEnv.hpp"
#include "types/TypeError.hpp"

namespace hope {

class TypeChecker {
public:
    explicit TypeChecker(TypeEnv& env);

    // Check an entire parsed program.  Throws TypeError on failure.
    void check_program(const std::vector<Decl>& decls);

    // Check a single declaration (for REPL use).  Throws TypeError on failure.
    void check_decl(const Decl& decl);

    // Infer the type of an expression in the empty local environment.
    // Returns a TyRef representing the inferred type (with possible free vars).
    TyRef infer_top_expr(const Expr& expr);

private:
    TypeEnv& env_;
    int      next_var_id_ = 0;

    // -----------------------------------------------------------------------
    // Trail for unification backtracking.
    // -----------------------------------------------------------------------
    struct TrailEntry {
        TyRef       node;
        TyNodeData  old_data;
    };
    std::vector<TrailEntry> trail_;

    // -----------------------------------------------------------------------
    // Union-find
    // -----------------------------------------------------------------------

    // Create a fresh unification variable.
    TyRef new_tvar();

    // Create a fresh frozen (skolem) variable.
    TyRef new_frozen();

    // Follow a chain of TyVar bindings until a non-var (or unbound var) is reached.
    TyRef deref(TyRef t) const;

    // Assign `val` to the TyVar node `var`, recording on the trail.
    void assign(TyRef var, TyRef val);

    // Checkpoint / restore for backtracking.
    size_t save_trail() const { return trail_.size(); }
    void   restore_trail(size_t saved);

    // -----------------------------------------------------------------------
    // Unification
    // -----------------------------------------------------------------------

    // Attempt to unify t1 and t2.  On success, bindings are committed to the
    // trail and true is returned.  On failure, all bindings since the call are
    // undone and false is returned.
    bool unify(TyRef t1, TyRef t2);

    // Internal recursive worker; `visited` is used to detect cycles in the
    // presence of recursive type synonyms (basic occurs check).
    bool real_unify(TyRef t1, TyRef t2,
                    std::vector<std::pair<TyRef, TyRef>>& visited);

    // -----------------------------------------------------------------------
    // AST type → TyRef conversion and instantiation
    // -----------------------------------------------------------------------

    // Convert an AST Type node to a TyRef, replacing free type-variable names
    // using `sub`.  Unknown names not in sub are treated as 0-arity TyCons.
    TyRef ast_to_tyref(const Type& t,
                       const std::unordered_map<std::string, TyRef>& sub);

    // Instantiate a declared type scheme by replacing each param in `params`
    // with a fresh unification variable.
    TyRef instantiate(const std::vector<std::string>& params, const TypePtr& type);

    // Instantiate a declared type scheme with frozen (skolem) variables.
    // Used in match_declared to prevent the declared vars from being bound.
    TyRef instantiate_frozen(const std::vector<std::string>& params,
                             const TypePtr& type);

    // -----------------------------------------------------------------------
    // Collect free type-variable names from an AST type expression.
    // -----------------------------------------------------------------------
    static void collect_tvars(const Type& t, std::vector<std::string>& out);

    // -----------------------------------------------------------------------
    // Local variable environment
    // -----------------------------------------------------------------------
    using VarEnv = std::unordered_map<std::string, TyRef>;

    // -----------------------------------------------------------------------
    // Expression type inference
    // -----------------------------------------------------------------------

    TyRef infer_expr(const Expr& e, VarEnv& vars);
    TyRef infer_lambda(const std::vector<LambdaClause>& clauses, VarEnv& vars);

    // -----------------------------------------------------------------------
    // Pattern type inference
    // -----------------------------------------------------------------------

    // Infer the type of a pattern; any newly-bound names are added to out_vars.
    TyRef infer_pattern(const Pattern& p, VarEnv& out_vars);

    // -----------------------------------------------------------------------
    // Declaration processing
    // -----------------------------------------------------------------------

    void process_infix(const DInfix&);       // no-op type-wise
    void process_typevar(const DTypeVar&);   // no-op
    void process_data(const DData&);
    void process_type(const DType&);
    void process_abstype(const DAbsType&);
    void process_dec(const DDec&);
    void process_equation(const DEquation&, SourceLocation loc);
    void process_eval(const DEval&, SourceLocation loc);

    // -----------------------------------------------------------------------
    // Instance checking
    // -----------------------------------------------------------------------

    // Check that `inferred` is an instance of (or unifiable with) the frozen
    // instantiation of `decl`.  Throws TypeError if the check fails.
    void match_declared(const std::string&  name,
                        TyRef               inferred,
                        const FuncDecl&     decl,
                        SourceLocation      loc);

    // -----------------------------------------------------------------------
    // Error helper
    // -----------------------------------------------------------------------

    [[noreturn]] void error(const std::string& msg, SourceLocation loc);
};

} // namespace hope
