#pragma once

// Tree-walking lazy evaluator for the Hope language.
//
// Evaluation strategy: call-by-need.  Every function argument is wrapped in a
// VThunk.  Thunks are forced (evaluated and updated in place) the first time
// their value is actually needed (for pattern matching, arithmetic, printing,
// etc.).  This gives true laziness: arguments that are never needed are never
// evaluated, and arguments that are needed more than once are evaluated only
// once.

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast/Ast.hpp"
#include "printer/ExprPrinter.hpp"
#include "runtime/Value.hpp"
#include "types/TypeEnv.hpp"

namespace hope {

class Evaluator {
public:
    explicit Evaluator(TypeEnv& type_env);

    // Register a declaration with the interpreter (takes ownership).
    // DEquation declarations are stored in owned_decls_ so that raw
    // pointers into their AST nodes (patterns, body) remain valid.
    void add_decl(Decl decl);

    // Process all declarations in a program (takes ownership of the vector).
    void load_program(std::vector<Decl> decls);

    // Evaluate an expression in the global environment.
    // Throws RuntimeError on errors.
    ValRef eval_top(const Expr& e);

    // Force a value to head-normal form.
    // If the value is a thunk, evaluate it and update in place.
    // Returns the forced value (always the same shared_ptr, updated in place).
    ValRef force(ValRef v);

    // Force to full normal form (deep forcing, for printing).
    ValRef force_full(ValRef v);

    // Pretty-print a value to a string.
    std::string print_value(ValRef v);

    // Register a named built-in function.
    void register_builtin(const std::string& name,
                          std::function<ValRef(ValRef)> fn);

    // Register a named value directly (e.g. argv, input).
    void register_value(const std::string& name, ValRef val);

    // Apply a (forced) function value to an argument value.
    // Made public so that Builtins and other helpers can use it.
    ValRef apply(ValRef fun, ValRef arg);

    // Public wrappers around private methods, for use by helpers in
    // Evaluator.cpp's anonymous namespace (e.g., CurriedApply).
    bool   match_pat(const Pattern& p, ValRef v, Env& env) { return match(p, v, env); }
    ValRef eval_in(const Expr* e, Env env) { return eval(*e, env); }

private:
    TypeEnv& type_env_;

public:
    // A clause in a function definition.  Uses raw pointers into the AST
    // (which is stable because owned_decls_ keeps the Decls alive).
    // Made public so the anonymous-namespace CurriedApply helper can use it.
    struct FuncClause {
        std::vector<const Pattern*> pats;        // argument patterns (raw, non-owning)
        const Expr*                 body;        // body expression (raw, non-owning)
        bool                        is_infix_pair = false; // infix operator: match pats[0,1]
                                                            // against left/right of a single VPair
    };

private:

    // Global function definitions: name -> ordered clauses
    struct FuncDef {
        std::string name;
        std::vector<FuncClause> clauses;
    };

    // Owned DEquation declarations.  Stored as heap-allocated unique_ptr
    // so that raw pointers into their AST nodes (patterns, body) remain
    // stable even as more declarations are added.
    std::vector<std::unique_ptr<Decl>> owned_decls_;

    // Synthetic patterns created for infix equations (owned here so that raw
    // pointers into them from FuncClause remain stable).
    std::vector<std::unique_ptr<Pattern>> synthetic_pats_;

    std::unordered_map<std::string, FuncDef> functions_;

    // Constructor arity registry: name -> 0 or 1
    std::unordered_map<std::string, int> constructor_arity_;

    // Global environment (built-ins + top-level function vals)
    Env global_env_;

    // ---------------------------------------------------------------------------
    // Core evaluation
    // ---------------------------------------------------------------------------

    // Evaluate expression e in environment env.
    ValRef eval(const Expr& e, Env env);

    // Try each clause of a multi-clause function in order.
    // Returns the result of the first matching clause.
    // Throws RuntimeError("non-exhaustive patterns") if none match.
    ValRef apply_clauses(const std::string& name,
                         const std::vector<FuncClause>& clauses,
                         ValRef arg, Env closed_env);

    // ---------------------------------------------------------------------------
    // Pattern matching
    // ---------------------------------------------------------------------------

    // Try to match value v against pattern p.
    // On success, extends env with the matched bindings and returns true.
    // On failure, env is left unchanged and false is returned.
    bool match(const Pattern& p, ValRef v, Env& env);

    bool match_infix(const PInfix& pi, ValRef v, Env& env);

    // ---------------------------------------------------------------------------
    // Declaration processing
    // ---------------------------------------------------------------------------

    void process_data(const DData& d);
    void process_equation(const DEquation& d, SourceLocation loc);
    void process_dec(const DDec& d);
    void process_uses(const DUses& d);
    void process_eval(const DEval& d, SourceLocation loc);

    // ---------------------------------------------------------------------------
    // Built-ins
    // ---------------------------------------------------------------------------

    void init_builtins();

    // Build a VFun value that dispatches to the named function's clauses.
    ValRef make_function_val(const std::string& name);

    // ---------------------------------------------------------------------------
    // Literal parsing helpers
    // ---------------------------------------------------------------------------

    static double   parse_num_literal(const std::string& text);
    static char     parse_char_literal(const std::string& text);
    static ValRef   string_to_list(const std::string& text); // converts "hello" -> list of VChar

    // ---------------------------------------------------------------------------
    // Repr helpers
    // ---------------------------------------------------------------------------

    // Build an ExprSubst mapping free variable names to their value reprs,
    // used when computing printable representations of lambdas and sections.
    ExprSubst build_repr_subst(const Env& env) const;

};

} // namespace hope
