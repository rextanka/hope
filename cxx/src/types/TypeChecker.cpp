#include "types/TypeChecker.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <sstream>

#include "printer/ExprPrinter.hpp"
#include "printer/TypePrinter.hpp"

namespace hope {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

TypeChecker::TypeChecker(TypeEnv& env) : env_(env) {}

// ---------------------------------------------------------------------------
// Top-level entry points
// ---------------------------------------------------------------------------

void TypeChecker::check_program(const std::vector<Decl>& decls) {
    for (const auto& d : decls)
        check_decl(d);
}

void TypeChecker::check_decl(const Decl& decl) {
    std::visit([&](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if      constexpr (std::is_same_v<T, DInfix>)    process_infix(alt);
        else if constexpr (std::is_same_v<T, DTypeVar>)  process_typevar(alt);
        else if constexpr (std::is_same_v<T, DData>)     process_data(alt);
        else if constexpr (std::is_same_v<T, DType>)     process_type(alt);
        else if constexpr (std::is_same_v<T, DAbsType>)  process_abstype(alt);
        else if constexpr (std::is_same_v<T, DDec>)      process_dec(alt);
        else if constexpr (std::is_same_v<T, DEquation>) process_equation(alt, decl.loc);
        else if constexpr (std::is_same_v<T, DEval>)     process_eval(alt, decl.loc);
        // DUses, DPrivate, DSave, DDisplay, DEdit — no type-checking needed
    }, decl.data);
}

TyRef TypeChecker::infer_top_expr(const Expr& expr) {
    VarEnv vars;
    TyRef result = infer_expr(expr, vars);
    return deref(result);
}

// ---------------------------------------------------------------------------
// Union-find helpers
// ---------------------------------------------------------------------------

TyRef TypeChecker::new_tvar() {
    return make_tyvar(next_var_id_++);
}

TyRef TypeChecker::new_frozen() {
    return make_frozen(next_var_id_++);
}

TyRef TypeChecker::deref(TyRef t) const {
    while (true) {
        auto* tv = std::get_if<TyVar>(&t->data);
        if (!tv || !tv->binding) break;
        t = *tv->binding;
    }
    return t;
}

void TypeChecker::assign(TyRef var, TyRef val) {
    trail_.push_back({var, var->data});
    auto& tv = std::get<TyVar>(var->data);
    tv.binding = val;
}

void TypeChecker::restore_trail(size_t saved) {
    while (trail_.size() > saved) {
        auto& e = trail_.back();
        e.node->data = std::move(e.old_data);
        trail_.pop_back();
    }
}

// ---------------------------------------------------------------------------
// Unification
// ---------------------------------------------------------------------------

bool TypeChecker::unify(TyRef t1, TyRef t2) {
    std::vector<std::pair<TyRef, TyRef>> visited;
    size_t saved = save_trail();
    int budget = 100;
    if (real_unify(t1, t2, visited, budget)) return true;
    restore_trail(saved);
    return false;
}

bool TypeChecker::real_unify(TyRef t1, TyRef t2,
                              std::vector<std::pair<TyRef, TyRef>>& visited,
                              int& expand_budget) {
    t1 = deref(t1);
    t2 = deref(t2);

    // Pointer equality — same node.
    if (t1.get() == t2.get()) return true;

    // Cycle / regularity check: have we already tried to unify this pair?
    //
    // We compare by SHALLOW STRUCTURAL KEY rather than just pointer identity.
    // This is necessary for recursive type synonyms like
    //   type seq alpha == alpha # seq alpha
    // where each expansion of seq(T) creates a fresh allocation, yet each
    // fresh node is structurally identical (same name, same arg pointers).
    // Pointer identity alone would never detect the cycle.
    //
    // Shallow key: for a TyCons, identity = (name, arg-pointers).
    // For any other node, identity = the TyRef pointer itself.
    auto ty_match = [](const TyRef& a, const TyRef& b) -> bool {
        if (a.get() == b.get()) return true;
        auto* ca = std::get_if<TyCons>(&a->data);
        auto* cb = std::get_if<TyCons>(&b->data);
        if (!ca || !cb) return false;
        if (ca->name != cb->name || ca->args.size() != cb->args.size()) return false;
        for (size_t i = 0; i < ca->args.size(); ++i)
            if (ca->args[i].get() != cb->args[i].get()) return false;
        return true;
    };

    for (auto& [a, b] : visited) {
        if (ty_match(a, t1) && ty_match(b, t2)) return true;
        if (ty_match(a, t2) && ty_match(b, t1)) return true;
    }
    visited.push_back({t1, t2});

    // ---- TyVar on left ----
    if (std::get_if<TyVar>(&t1->data)) {
        assign(t1, t2);
        return true;
    }

    // ---- TyVar on right ----
    if (std::get_if<TyVar>(&t2->data)) {
        assign(t2, t1);
        return true;
    }

    // ---- Both frozen ----
    if (auto* f1 = std::get_if<TyFrozen>(&t1->data)) {
        if (auto* f2 = std::get_if<TyFrozen>(&t2->data)) {
            return f1->id == f2->id;
        }
        // Frozen cannot be unified with non-var, non-frozen.
        return false;
    }
    // Frozen on right: frozen can't be bound, fail.
    if (std::get_if<TyFrozen>(&t2->data)) {
        return false;
    }

    // ---- Both TyCons ----
    auto* c1 = std::get_if<TyCons>(&t1->data);
    auto* c2 = std::get_if<TyCons>(&t2->data);
    if (!c1 || !c2) return false; // shouldn't happen

    if (c1->name != c2->name) {
        // Names differ: try expanding one or both as type synonyms.
        // Expansion order: try t1 first, then t2.
        auto expand = [&](const TyCons* c) -> TyRef {
            if (expand_budget <= 0) return nullptr;
            const TypeDef* td = env_.lookup_type(c->name);
            if (!td || !td->is_synonym()) return nullptr;
            if (td->params.size() != c->args.size()) return nullptr;
            --expand_budget;
            std::unordered_map<std::string, TyRef> sub;
            for (size_t i = 0; i < td->params.size(); ++i)
                sub[td->params[i]] = c->args[i];
            return ast_to_tyref(*td->synonym_body(), sub);
        };

        TyRef exp1 = expand(c1);
        if (exp1) return real_unify(exp1, t2, visited, expand_budget);

        TyRef exp2 = expand(c2);
        if (exp2) return real_unify(t1, exp2, visited, expand_budget);

        return false;
    }
    if (c1->args.size() != c2->args.size()) return false;

    for (size_t i = 0; i < c1->args.size(); ++i) {
        if (!real_unify(c1->args[i], c2->args[i], visited, expand_budget))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// collect_tvars — gather free type-variable names from an AST type
// ---------------------------------------------------------------------------

void TypeChecker::collect_tvars(const Type& t, std::vector<std::string>& out) {
    std::visit([&](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, TVar>) {
            // Only treat as a type variable if NOT a known type constructor.
            // In Hope, bare idents in types are parsed as TVar; those that
            // are known type constructors (e.g. "num", "bool", "char") should
            // NOT be treated as universally-quantified type parameters.
            if (!env_.lookup_type(alt.name)) {
                // Add if not already present.
                if (std::find(out.begin(), out.end(), alt.name) == out.end())
                    out.push_back(alt.name);
            }
        } else if constexpr (std::is_same_v<T, TCons>) {
            for (const auto& a : alt.args)
                collect_tvars(*a, out);
        } else if constexpr (std::is_same_v<T, TFun>) {
            collect_tvars(*alt.dom, out);
            collect_tvars(*alt.cod, out);
        } else if constexpr (std::is_same_v<T, TProd>) {
            collect_tvars(*alt.left, out);
            collect_tvars(*alt.right, out);
        } else if constexpr (std::is_same_v<T, TMu>) {
            collect_tvars(*alt.body, out);
        }
    }, t.data);
}

// ---------------------------------------------------------------------------
// ast_to_tyref — convert AST Type → TyRef
// ---------------------------------------------------------------------------

TyRef TypeChecker::ast_to_tyref(const Type& t,
                                 const std::unordered_map<std::string, TyRef>& sub) {
    return std::visit([&](const auto& alt) -> TyRef {
        using T = std::decay_t<decltype(alt)>;

        if constexpr (std::is_same_v<T, TVar>) {
            auto it = sub.find(alt.name);
            if (it != sub.end()) return it->second;
            // Unknown name: treat as 0-arity type constructor.
            return make_tycons(alt.name, {});
        }

        else if constexpr (std::is_same_v<T, TCons>) {
            std::vector<TyRef> args;
            args.reserve(alt.args.size());
            for (const auto& a : alt.args)
                args.push_back(ast_to_tyref(*a, sub));
            return make_tycons(alt.name, std::move(args));
        }

        else if constexpr (std::is_same_v<T, TFun>) {
            return make_fun_type(ast_to_tyref(*alt.dom, sub),
                                 ast_to_tyref(*alt.cod, sub));
        }

        else if constexpr (std::is_same_v<T, TProd>) {
            return make_prod_type(ast_to_tyref(*alt.left, sub),
                                  ast_to_tyref(*alt.right, sub));
        }

        else { // TMu — expand by treating as a TyCons for now
            static_assert(std::is_same_v<T, TMu>);
            // Recursion guard: just produce the body converted.
            // Full equirecursive support deferred to a later phase.
            return ast_to_tyref(*alt.body, sub);
        }
    }, t.data);
}

// ---------------------------------------------------------------------------
// instantiate — replace params with fresh unification variables
// ---------------------------------------------------------------------------

TyRef TypeChecker::instantiate(const std::vector<std::string>& params,
                                const TypePtr& type) {
    std::unordered_map<std::string, TyRef> sub;
    for (const auto& p : params)
        sub[p] = new_tvar();
    return ast_to_tyref(*type, sub);
}

// ---------------------------------------------------------------------------
// instantiate_frozen — replace params with fresh frozen (skolem) variables
// ---------------------------------------------------------------------------

TyRef TypeChecker::instantiate_frozen(const std::vector<std::string>& params,
                                       const TypePtr& type) {
    std::unordered_map<std::string, TyRef> sub;
    for (const auto& p : params)
        sub[p] = new_frozen();
    return ast_to_tyref(*type, sub);
}

// ---------------------------------------------------------------------------
// Pattern inference
// ---------------------------------------------------------------------------

TyRef TypeChecker::infer_pattern(const Pattern& p, VarEnv& out_vars) {
    return std::visit([&](const auto& alt) -> TyRef {
        using T = std::decay_t<decltype(alt)>;

        // PVar — bind a fresh variable, unless the name is a known nullary constructor
        if constexpr (std::is_same_v<T, PVar>) {
            // Check if name is a known nullary constructor (e.g. nil, true, false).
            // The parser emits PVar for bare lowercase names like nil when not
            // followed by `(`, but the type checker needs to treat them as PCons.
            const ConInfo* ci = env_.lookup_con(alt.name);
            if (ci && !ci->arg) {
                // Nullary constructor — same logic as PCons nullary case.
                std::unordered_map<std::string, TyRef> sub;
                for (const auto& param : ci->params)
                    sub[param] = new_tvar();
                std::vector<TyRef> result_args;
                for (const auto& param : ci->params)
                    result_args.push_back(sub.at(param));
                return make_tycons(ci->type_name, std::move(result_args));
            }
            TyRef tv = new_tvar();
            out_vars[alt.name] = tv;
            return tv;
        }

        // PWild — fresh variable, not bound
        else if constexpr (std::is_same_v<T, PWild>) {
            return new_tvar();
        }

        // PLit — derive type from literal kind
        else if constexpr (std::is_same_v<T, PLit>) {
            return std::visit([&](const auto& lit) -> TyRef {
                using L = std::decay_t<decltype(lit)>;
                if constexpr (std::is_same_v<L, LitNum>   ||
                              std::is_same_v<L, LitFloat>)
                    return make_num_type();
                else if constexpr (std::is_same_v<L, LitChar>)
                    return make_char_type();
                else { // LitStr
                    return make_list_type(make_char_type());
                }
            }, alt.lit);
        }

        // PSucc — inner must be num, whole pattern is num
        else if constexpr (std::is_same_v<T, PSucc>) {
            TyRef inner_ty = infer_pattern(*alt.inner, out_vars);
            TyRef num_ty   = make_num_type();
            if (!unify(inner_ty, num_ty))
                error("succ pattern: inner must have type num", p.loc);
            return num_ty;
        }

        // PNPlusK — variable bound to num
        else if constexpr (std::is_same_v<T, PNPlusK>) {
            TyRef num_ty = make_num_type();
            out_vars[alt.var] = num_ty;
            return num_ty;
        }

        // PCons — look up constructor and instantiate
        else if constexpr (std::is_same_v<T, PCons>) {
            const ConInfo* ci = env_.lookup_con(alt.name);
            if (!ci)
                error("unknown constructor '" + alt.name + "'", p.loc);

            // Build a sub map from the type's params to fresh vars.
            std::unordered_map<std::string, TyRef> sub;
            for (const auto& param : ci->params)
                sub[param] = new_tvar();

            // The result type is TCons{type_name, fresh_vars...}
            std::vector<TyRef> result_args;
            result_args.reserve(ci->params.size());
            for (const auto& param : ci->params)
                result_args.push_back(sub.at(param));
            TyRef result_ty = make_tycons(ci->type_name, std::move(result_args));

            if (!ci->arg) {
                // Nullary constructor
                if (alt.arg)
                    error("constructor '" + alt.name + "' takes no argument", p.loc);
                return result_ty;
            } else {
                // Constructor with an argument
                if (!alt.arg)
                    error("constructor '" + alt.name + "' requires an argument", p.loc);
                TyRef arg_ty  = ast_to_tyref(*(*ci->arg), sub);
                TyRef pat_ty  = infer_pattern(**alt.arg, out_vars);
                if (!unify(arg_ty, pat_ty))
                    error("constructor '" + alt.name + "' argument type mismatch", p.loc);
                return result_ty;
            }
        }

        // PTuple — right-nested products
        else if constexpr (std::is_same_v<T, PTuple>) {
            if (alt.elems.empty())
                error("empty tuple pattern", p.loc);
            if (alt.elems.size() == 1)
                return infer_pattern(*alt.elems[0], out_vars);
            // Fold right: (a, b, c) -> a # (b # c)
            TyRef acc = infer_pattern(*alt.elems.back(), out_vars);
            for (int i = static_cast<int>(alt.elems.size()) - 2; i >= 0; --i)
                acc = make_prod_type(infer_pattern(*alt.elems[i], out_vars), acc);
            return acc;
        }

        // PList — all elems must unify to same type
        else if constexpr (std::is_same_v<T, PList>) {
            TyRef elem_ty = new_tvar();
            for (const auto& ep : alt.elems) {
                TyRef et = infer_pattern(*ep, out_vars);
                if (!unify(elem_ty, et))
                    error("list pattern: element types do not match", p.loc);
            }
            return make_list_type(elem_ty);
        }

        // PInfix — infix constructor pattern (e.g. h :: t)
        else { // PInfix
            static_assert(std::is_same_v<T, PInfix>);

            if (alt.op == "::") {
                // Special-case the list cons constructor.
                TyRef tl = infer_pattern(*alt.left, out_vars);
                TyRef tr = infer_pattern(*alt.right, out_vars);
                TyRef list_tl = make_list_type(tl);
                if (!unify(tr, list_tl))
                    error(":: pattern: right side must be list of same type as left", p.loc);
                return list_tl;
            }

            // User-defined infix constructor.
            const ConInfo* ci = env_.lookup_con(alt.op);
            if (!ci)
                error("unknown infix constructor '" + alt.op + "'", p.loc);

            std::unordered_map<std::string, TyRef> sub;
            for (const auto& param : ci->params)
                sub[param] = new_tvar();

            std::vector<TyRef> result_args;
            for (const auto& param : ci->params)
                result_args.push_back(sub.at(param));
            TyRef result_ty = make_tycons(ci->type_name, std::move(result_args));

            if (!ci->arg)
                error("infix constructor '" + alt.op + "' is nullary", p.loc);

            // The constructor's arg should be a product type.
            TyRef arg_ty = ast_to_tyref(*(*ci->arg), sub);
            TyRef tl = infer_pattern(*alt.left, out_vars);
            TyRef tr = infer_pattern(*alt.right, out_vars);
            TyRef pair_ty = make_prod_type(tl, tr);
            if (!unify(arg_ty, pair_ty))
                error("infix constructor '" + alt.op + "' argument type mismatch", p.loc);
            return result_ty;
        }
    }, p.data);
}

// ---------------------------------------------------------------------------
// Expression inference
// ---------------------------------------------------------------------------

TyRef TypeChecker::infer_expr(const Expr& e, VarEnv& vars) {
    return std::visit([&](const auto& alt) -> TyRef {
        using T = std::decay_t<decltype(alt)>;

        // ---- ELit ----
        if constexpr (std::is_same_v<T, ELit>) {
            return std::visit([&](const auto& lit) -> TyRef {
                using L = std::decay_t<decltype(lit)>;
                if constexpr (std::is_same_v<L, LitNum>   ||
                              std::is_same_v<L, LitFloat>)
                    return make_num_type();
                else if constexpr (std::is_same_v<L, LitChar>)
                    return make_char_type();
                else // LitStr
                    return make_list_type(make_char_type());
            }, alt.lit);
        }

        // ---- EVar ----
        else if constexpr (std::is_same_v<T, EVar>) {
            const std::string& name = alt.name;

            // 1) Local variable
            {
                auto it = vars.find(name);
                if (it != vars.end()) return it->second;
            }

            // 2) Declared function
            {
                const FuncDecl* fd = env_.lookup_func(name);
                if (fd && fd->type) {
                    return instantiate(fd->params, fd->type);
                }
            }

            // 3) Constructor
            {
                const ConInfo* ci = env_.lookup_con(name);
                if (ci) {
                    std::unordered_map<std::string, TyRef> sub;
                    for (const auto& p : ci->params)
                        sub[p] = new_tvar();

                    std::vector<TyRef> result_args;
                    for (const auto& p : ci->params)
                        result_args.push_back(sub.at(p));
                    TyRef result_ty = make_tycons(ci->type_name, std::move(result_args));

                    if (!ci->arg) return result_ty;
                    TyRef arg_ty = ast_to_tyref(*(*ci->arg), sub);
                    return make_fun_type(arg_ty, result_ty);
                }
            }

            error("unbound variable '" + name + "'", e.loc);
        }

        // ---- EOpRef ----
        else if constexpr (std::is_same_v<T, EOpRef>) {
            const std::string& name = alt.op;

            // 1) Local variable
            {
                auto it = vars.find(name);
                if (it != vars.end()) return it->second;
            }

            // 2) Declared function
            {
                const FuncDecl* fd = env_.lookup_func(name);
                if (fd && fd->type) {
                    return instantiate(fd->params, fd->type);
                }
            }

            // 3) Constructor
            {
                const ConInfo* ci = env_.lookup_con(name);
                if (ci) {
                    std::unordered_map<std::string, TyRef> sub;
                    for (const auto& p : ci->params)
                        sub[p] = new_tvar();

                    std::vector<TyRef> result_args;
                    for (const auto& p : ci->params)
                        result_args.push_back(sub.at(p));
                    TyRef result_ty = make_tycons(ci->type_name, std::move(result_args));

                    if (!ci->arg) return result_ty;
                    TyRef arg_ty = ast_to_tyref(*(*ci->arg), sub);
                    return make_fun_type(arg_ty, result_ty);
                }
            }

            error("unbound operator '" + name + "'", e.loc);
        }

        // ---- EApply ----
        else if constexpr (std::is_same_v<T, EApply>) {
            TyRef tf  = infer_expr(*alt.func, vars);
            TyRef ta  = infer_expr(*alt.arg,  vars);
            TyRef ret = new_tvar();
            TyRef fun_ty = make_fun_type(ta, ret);
            if (!unify(tf, fun_ty)) {
                // Build context lines: expression, function type, argument type.
                std::vector<std::string> ctx;
                try { ctx.push_back("  " + print_expr(e)); } catch (...) {}
                try { ctx.push_back("  " + print_expr(*alt.func) + " : " + print_type(deref(tf))); } catch (...) {}
                try { ctx.push_back("  " + print_expr(*alt.arg)  + " : " + print_type(deref(ta))); } catch (...) {}
                error("argument has wrong type", e.loc, std::move(ctx));
            }
            return deref(ret);
        }

        // ---- EInfix ----
        else if constexpr (std::is_same_v<T, EInfix>) {
            // infer the operator as a variable/constructor
            TyRef top = [&]() -> TyRef {
                // Look up operator
                auto it = vars.find(alt.op);
                if (it != vars.end()) return it->second;
                if (const FuncDecl* fd = env_.lookup_func(alt.op))
                    if (fd->type) return instantiate(fd->params, fd->type);
                if (const ConInfo* ci = env_.lookup_con(alt.op)) {
                    std::unordered_map<std::string, TyRef> sub;
                    for (const auto& p : ci->params) sub[p] = new_tvar();
                    std::vector<TyRef> result_args;
                    for (const auto& p : ci->params) result_args.push_back(sub.at(p));
                    TyRef result_ty = make_tycons(ci->type_name, std::move(result_args));
                    if (!ci->arg) return result_ty;
                    return make_fun_type(ast_to_tyref(*(*ci->arg), sub), result_ty);
                }
                error("unbound operator '" + alt.op + "'", e.loc);
            }();

            // Special case: :: is a constructor taking a product
            if (alt.op == "::") {
                TyRef tl = infer_expr(*alt.left,  vars);
                TyRef tr = infer_expr(*alt.right, vars);
                TyRef list_tl = make_list_type(tl);
                if (!unify(tr, list_tl))
                    error(":: : right operand must be list of same type as left", e.loc);
                // Unify the full constructor type
                TyRef pair_ty = make_prod_type(tl, deref(list_tl));
                TyRef ret = new_tvar();
                if (!unify(top, make_fun_type(pair_ty, ret)))
                    error(":: type mismatch", e.loc);
                return deref(list_tl);
            }

            TyRef tl  = infer_expr(*alt.left,  vars);
            TyRef tr  = infer_expr(*alt.right, vars);
            TyRef ret = new_tvar();
            // In Hope, infix operators take a product: top : (tl # tr) -> ret
            if (!unify(top, make_fun_type(make_prod_type(tl, tr), ret))) {
                std::vector<std::string> ctx;
                try { ctx.push_back("  " + print_expr(e)); } catch (...) {}
                try {
                    // Print "  (op) : op_type"
                    ctx.push_back("  (" + alt.op + ") : " + print_type(deref(top)));
                } catch (...) {}
                try { ctx.push_back("  " + print_expr(*alt.left)  + " : " + print_type(deref(tl))); } catch (...) {}
                try { ctx.push_back("  " + print_expr(*alt.right) + " : " + print_type(deref(tr))); } catch (...) {}
                error("argument has wrong type", e.loc, std::move(ctx));
            }
            return deref(ret);
        }

        // ---- ETuple ----
        else if constexpr (std::is_same_v<T, ETuple>) {
            if (alt.elems.empty())
                error("empty tuple expression", e.loc);
            if (alt.elems.size() == 1)
                return infer_expr(*alt.elems[0], vars);
            // Fold right: (a, b, c) → a # (b # c)
            TyRef acc = infer_expr(*alt.elems.back(), vars);
            for (int i = static_cast<int>(alt.elems.size()) - 2; i >= 0; --i)
                acc = make_prod_type(infer_expr(*alt.elems[i], vars), acc);
            return acc;
        }

        // ---- EList ----
        else if constexpr (std::is_same_v<T, EList>) {
            TyRef elem_ty = new_tvar();
            TyRef first_ty;  // type of first element (for error reporting)
            for (size_t i = 0; i < alt.elems.size(); ++i) {
                TyRef et = infer_expr(*alt.elems[i], vars);
                if (i == 0) first_ty = et;
                if (!unify(elem_ty, et)) {
                    // Format error like a :: type mismatch.
                    TyRef cons_ty; // alpha # list alpha -> list alpha
                    try {
                        if (const FuncDecl* fd = env_.lookup_func("::"))
                            if (fd->type) cons_ty = instantiate(fd->params, fd->type);
                    } catch (...) {}
                    std::vector<std::string> ctx;
                    try { ctx.push_back("  " + print_expr(e)); } catch (...) {}
                    if (cons_ty)
                        try { ctx.push_back("  (::) : " + print_type(deref(cons_ty))); } catch (...) {}
                    else
                        ctx.push_back("  (::) : alpha # list alpha -> list alpha");
                    // Show first element and the rest as a list.
                    if (i > 0 && first_ty)
                        try { ctx.push_back("  " + print_expr(*alt.elems[0]) + " : " + print_type(deref(first_ty))); } catch (...) {}
                    // Show the current element's type mismatching.
                    try {
                        // Build rest as a temporary EList for print_expr.
                        // Check if rest elements are all char literals → print as string.
                        bool rest_all_chars = true;
                        for (size_t j = i; j < alt.elems.size(); ++j) {
                            if (const auto* el = std::get_if<ELit>(&alt.elems[j]->data)) {
                                if (!std::holds_alternative<LitChar>(el->lit)) {
                                    rest_all_chars = false; break;
                                }
                            } else { rest_all_chars = false; break; }
                        }
                        std::string rest;
                        if (rest_all_chars && i < alt.elems.size()) {
                            rest = "\"";
                            for (size_t j = i; j < alt.elems.size(); ++j) {
                                if (const auto* el = std::get_if<ELit>(&alt.elems[j]->data)) {
                                    if (const auto* lc = std::get_if<LitChar>(&el->lit)) {
                                        const std::string& t = lc->text;
                                        if (t.size() >= 2) {
                                            std::string inner = t.substr(1, t.size() - 2);
                                            if (inner == "\\'") rest += "'";
                                            else if (inner == "\\\\") rest += "\\\\";
                                            else rest += inner;
                                        }
                                    }
                                }
                            }
                            rest += "\"";
                        } else {
                            rest = "[";
                            for (size_t j = i; j < alt.elems.size(); ++j) {
                                if (j > i) rest += ", ";
                                rest += print_expr(*alt.elems[j]);
                            }
                            rest += "]";
                        }
                        ctx.push_back("  " + rest + " : " + print_type(make_list_type(deref(et))));
                    } catch (...) {}
                    error("argument has wrong type", alt.elems[i]->loc, std::move(ctx));
                }
            }
            return make_list_type(deref(elem_ty));
        }

        // ---- ESection ----
        else if constexpr (std::is_same_v<T, ESection>) {
            // Look up the operator type.
            TyRef top = [&]() -> TyRef {
                auto it = vars.find(alt.op);
                if (it != vars.end()) return it->second;
                if (const FuncDecl* fd = env_.lookup_func(alt.op))
                    if (fd->type) return instantiate(fd->params, fd->type);
                if (const ConInfo* ci = env_.lookup_con(alt.op)) {
                    std::unordered_map<std::string, TyRef> sub;
                    for (const auto& p : ci->params) sub[p] = new_tvar();
                    std::vector<TyRef> result_args;
                    for (const auto& p : ci->params) result_args.push_back(sub.at(p));
                    TyRef result_ty = make_tycons(ci->type_name, std::move(result_args));
                    if (!ci->arg) return result_ty;
                    return make_fun_type(ast_to_tyref(*(*ci->arg), sub), result_ty);
                }
                error("unbound operator '" + alt.op + "' in section", e.loc);
            }();

            TyRef te = infer_expr(*alt.expr, vars);

            if (alt.is_left) {
                // Left section (e op): te is left arg; result is right_arg -> result
                // In Hope, op : te # right_arg -> result
                TyRef right_arg = new_tvar();
                TyRef result    = new_tvar();
                if (!unify(top, make_fun_type(make_prod_type(te, right_arg), result)))
                    error("type mismatch in left section", e.loc);
                return make_fun_type(deref(right_arg), deref(result));
            } else {
                // Right section (op e): te is right arg; result is left_arg -> result
                // In Hope, op : left_arg # te -> result
                TyRef left_arg = new_tvar();
                TyRef result   = new_tvar();
                if (!unify(top, make_fun_type(make_prod_type(left_arg, te), result)))
                    error("type mismatch in right section", e.loc);
                return make_fun_type(deref(left_arg), deref(result));
            }
        }

        // ---- ELambda ----
        else if constexpr (std::is_same_v<T, ELambda>) {
            return infer_lambda(alt.clauses, vars);
        }

        // ---- ELet ----
        else if constexpr (std::is_same_v<T, ELet>) {
            VarEnv local_vars = vars;
            for (const auto& bind : alt.binds) {
                VarEnv pat_vars;
                TyRef tp = infer_pattern(*bind.pat, pat_vars);
                TyRef tb = infer_expr(*bind.body, local_vars);
                if (!unify(tp, tb))
                    error("let binding: pattern type does not match body type", bind.loc);
                for (auto& [n, t] : pat_vars)
                    local_vars[n] = t;
            }
            return infer_expr(*alt.body, local_vars);
        }

        // ---- ELetRec ----
        else if constexpr (std::is_same_v<T, ELetRec>) {
            VarEnv local_vars = vars;
            // First pass: introduce fresh vars for each pattern variable,
            // and record the inferred pattern type so we can unify in pass 2.
            std::vector<TyRef> pat_types(alt.binds.size());
            for (size_t i = 0; i < alt.binds.size(); ++i) {
                VarEnv pat_vars;
                pat_types[i] = infer_pattern(*alt.binds[i].pat, pat_vars);
                for (auto& [n, t] : pat_vars)
                    local_vars[n] = t;
            }
            // Second pass: infer body types with all pattern vars in scope.
            for (size_t i = 0; i < alt.binds.size(); ++i) {
                TyRef tb = infer_expr(*alt.binds[i].body, local_vars);
                if (!unify(pat_types[i], tb))
                    error("letrec binding: type mismatch", alt.binds[i].loc);
            }
            return infer_expr(*alt.body, local_vars);
        }

        // ---- EWhere ----
        else if constexpr (std::is_same_v<T, EWhere>) {
            VarEnv local_vars = vars;
            for (const auto& bind : alt.binds) {
                VarEnv pat_vars;
                TyRef tp = infer_pattern(*bind.pat, pat_vars);
                TyRef tb = infer_expr(*bind.body, local_vars);
                if (!unify(tp, tb))
                    error("where binding: pattern type does not match body type", bind.loc);
                for (auto& [n, t] : pat_vars)
                    local_vars[n] = t;
            }
            return infer_expr(*alt.body, local_vars);
        }

        // ---- EWhereRec ----
        else if constexpr (std::is_same_v<T, EWhereRec>) {
            VarEnv local_vars = vars;
            std::vector<TyRef> pat_types(alt.binds.size());
            for (size_t i = 0; i < alt.binds.size(); ++i) {
                VarEnv pat_vars;
                pat_types[i] = infer_pattern(*alt.binds[i].pat, pat_vars);
                for (auto& [n, t] : pat_vars)
                    local_vars[n] = t;
            }
            for (size_t i = 0; i < alt.binds.size(); ++i) {
                TyRef tb = infer_expr(*alt.binds[i].body, local_vars);
                if (!unify(pat_types[i], tb))
                    error("whererec binding: type mismatch", alt.binds[i].loc);
            }
            return infer_expr(*alt.body, local_vars);
        }

        // ---- EIf ----
        else if constexpr (std::is_same_v<T, EIf>) {
            TyRef tc = infer_expr(*alt.cond,  vars);
            TyRef tt = infer_expr(*alt.then_, vars);
            TyRef te = infer_expr(*alt.else_, vars);

            // Condition must be truval (which is a synonym for bool).
            // We unify with bool directly (truval is its synonym).
            TyRef bool_ty = make_bool_type();
            if (!unify(tc, bool_ty))
                error("if condition must have type truval (bool)", e.loc);

            if (!unify(tt, te))
                error("if branches must have the same type", e.loc);

            return deref(tt);
        }

        // ---- EWrite ----
        else { // EWrite
            static_assert(std::is_same_v<T, EWrite>);
            // write expr: just check the inner expression; type is not constrained.
            return infer_expr(*alt.expr, vars);
        }
    }, e.data);
}

// ---------------------------------------------------------------------------
// Lambda inference
// ---------------------------------------------------------------------------

TyRef TypeChecker::infer_lambda(const std::vector<LambdaClause>& clauses,
                                 VarEnv& vars) {
    if (clauses.empty())
        error("lambda with no clauses", SourceLocation{});

    TyRef clause_ty; // unified type across all clauses

    for (const auto& clause : clauses) {
        VarEnv local_vars = vars;
        VarEnv pat_vars;

        // Infer each pattern's type, collecting bound names.
        std::vector<TyRef> pat_types;
        pat_types.reserve(clause.pats.size());
        for (const auto& pat : clause.pats) {
            pat_types.push_back(infer_pattern(*pat, pat_vars));
        }
        for (auto& [n, t] : pat_vars)
            local_vars[n] = t;

        // Infer body type.
        TyRef body_ty = infer_expr(*clause.body, local_vars);

        // Build the clause's full function type: p1 -> p2 -> ... -> body
        TyRef this_clause_ty = body_ty;
        for (int i = static_cast<int>(pat_types.size()) - 1; i >= 0; --i)
            this_clause_ty = make_fun_type(pat_types[i], this_clause_ty);

        if (!clause_ty) {
            clause_ty = this_clause_ty;
        } else {
            if (!unify(clause_ty, this_clause_ty))
                error("lambda clauses have incompatible types", clause.body->loc);
        }
    }

    return deref(clause_ty);
}

// ---------------------------------------------------------------------------
// Declaration processing
// ---------------------------------------------------------------------------

void TypeChecker::process_infix(const DInfix&) {
    // No type-checking needed for infix declarations.
}

void TypeChecker::process_typevar(const DTypeVar&) {
    // No type-checking needed.
}

void TypeChecker::process_data(const DData& d) {
    SourceLocation builtin{"<builtin>", 0, 0, 0};

    TypeDef td;
    td.name   = d.name;
    td.params = d.params;

    std::vector<ConInfo> cons;

    for (const auto& ctor : d.alts) {
        ConInfo ci;
        ci.name      = ctor.name;
        ci.type_name = d.name;
        ci.params    = d.params;
        ci.loc       = ctor.loc;

        if (ctor.arg) {
            // Deep-copy the arg TypePtr (no clone, so we reconstruct from the AST)
            // We can't clone a unique_ptr, so we store via a helper that converts
            // back to a fresh TypePtr from the visitor.
            // For now: store a nullptr and use a workaround via ast_to_tyref at
            // instantiation time.  Actually we CAN store the arg as-is because
            // we move it — but we have multiple constructors, so we'd need copies.
            // The cleanest approach: store the arg as-is but only one constructor
            // exists per ConInfo, so move is fine if we construct from the original.
            //
            // Since Constructor.arg is std::optional<TypePtr> (unique_ptr),
            // and ConInfo.arg is also std::optional<TypePtr>, we cannot copy.
            // We need to reconstruct the TypePtr from the AST Type node.
            // The Type node itself is stored by value in the variant, so we can
            // rebuild the TypePtr by visiting the Type::Data.

            // Rebuild a TypePtr from an existing Type node by cloning the tree.
            // We implement a small recursive clone here.
            std::function<TypePtr(const Type&)> clone_type = [&](const Type& t) -> TypePtr {
                return std::visit([&](const auto& v) -> TypePtr {
                    using V = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<V, TVar>) {
                        return make_type(TVar{v.name}, t.loc);
                    } else if constexpr (std::is_same_v<V, TCons>) {
                        std::vector<TypePtr> args;
                        args.reserve(v.args.size());
                        for (const auto& a : v.args)
                            args.push_back(clone_type(*a));
                        return make_type(TCons{v.name, std::move(args)}, t.loc);
                    } else if constexpr (std::is_same_v<V, TFun>) {
                        return make_type(TFun{clone_type(*v.dom), clone_type(*v.cod)}, t.loc);
                    } else if constexpr (std::is_same_v<V, TProd>) {
                        return make_type(TProd{clone_type(*v.left), clone_type(*v.right)}, t.loc);
                    } else { // TMu
                        static_assert(std::is_same_v<V, TMu>);
                        return make_type(TMu{v.var, clone_type(*v.body)}, t.loc);
                    }
                }, t.data);
            };

            ci.arg = clone_type(**ctor.arg);
        } else {
            ci.arg = std::nullopt;
        }

        cons.push_back(std::move(ci));
    }

    td.def = std::move(cons);
    env_.add_typedef(std::move(td));
}

void TypeChecker::process_type(const DType& d) {
    // Rebuild the synonym body as a fresh TypePtr.
    std::function<TypePtr(const Type&)> clone_type = [&](const Type& t) -> TypePtr {
        return std::visit([&](const auto& v) -> TypePtr {
            using V = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<V, TVar>) {
                return make_type(TVar{v.name}, t.loc);
            } else if constexpr (std::is_same_v<V, TCons>) {
                std::vector<TypePtr> args;
                for (const auto& a : v.args)
                    args.push_back(clone_type(*a));
                return make_type(TCons{v.name, std::move(args)}, t.loc);
            } else if constexpr (std::is_same_v<V, TFun>) {
                return make_type(TFun{clone_type(*v.dom), clone_type(*v.cod)}, t.loc);
            } else if constexpr (std::is_same_v<V, TProd>) {
                return make_type(TProd{clone_type(*v.left), clone_type(*v.right)}, t.loc);
            } else {
                static_assert(std::is_same_v<V, TMu>);
                return make_type(TMu{v.var, clone_type(*v.body)}, t.loc);
            }
        }, t.data);
    };

    TypeDef td;
    td.name   = d.name;
    td.params = d.params;
    td.def    = clone_type(*d.body);
    env_.add_typedef(std::move(td));
}

void TypeChecker::process_abstype(const DAbsType& d) {
    // Abstract type: extract name and params from the AST type expression.
    // DAbsType.type is the type expression being declared abstract, e.g.:
    //   abstype stream alpha;
    // which parses as TCons{"stream", [TVar{"alpha"}]}.
    std::visit([&](const auto& v) {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, TCons>) {
            TypeDef td;
            td.name = v.name;
            td.params.reserve(v.args.size());
            for (const auto& a : v.args) {
                if (auto* tv = std::get_if<TVar>(&a->data))
                    td.params.push_back(tv->name);
            }
            td.def = std::monostate{};
            env_.add_typedef(std::move(td));
        } else if constexpr (std::is_same_v<V, TVar>) {
            // abstype alpha — 0-arity abstract
            TypeDef td;
            td.name = v.name;
            td.def  = std::monostate{};
            env_.add_typedef(std::move(td));
        }
        // TFun, TProd, TMu — unusual for abstype; skip.
    }, d.type->data);
}

void TypeChecker::process_dec(const DDec& d) {
    // Collect the free type variables in the declared type.
    std::vector<std::string> params;
    collect_tvars(*d.type, params);

    // Clone the type for the FuncDecl.
    std::function<TypePtr(const Type&)> clone_type = [&](const Type& t) -> TypePtr {
        return std::visit([&](const auto& v) -> TypePtr {
            using V = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<V, TVar>) {
                return make_type(TVar{v.name}, t.loc);
            } else if constexpr (std::is_same_v<V, TCons>) {
                std::vector<TypePtr> args;
                for (const auto& a : v.args)
                    args.push_back(clone_type(*a));
                return make_type(TCons{v.name, std::move(args)}, t.loc);
            } else if constexpr (std::is_same_v<V, TFun>) {
                return make_type(TFun{clone_type(*v.dom), clone_type(*v.cod)}, t.loc);
            } else if constexpr (std::is_same_v<V, TProd>) {
                return make_type(TProd{clone_type(*v.left), clone_type(*v.right)}, t.loc);
            } else {
                static_assert(std::is_same_v<V, TMu>);
                return make_type(TMu{v.var, clone_type(*v.body)}, t.loc);
            }
        }, t.data);
    };

    FuncDecl fd;
    fd.names  = d.names;
    fd.params = std::move(params);
    fd.type   = clone_type(*d.type);
    fd.loc    = d.type->loc;

    env_.add_funcdecl(std::move(fd));
}

void TypeChecker::process_equation(const DEquation& eq, SourceLocation loc) {
    const std::string& fname = eq.lhs.func;

    // Build local environment with fresh var for recursive calls.
    VarEnv vars;
    TyRef self_var = new_tvar();
    vars[fname] = self_var;

    // Infer each LHS pattern.
    VarEnv pat_vars;
    std::vector<TyRef> pat_types;
    pat_types.reserve(eq.lhs.args.size());
    for (const auto& pat : eq.lhs.args) {
        pat_types.push_back(infer_pattern(*pat, pat_vars));
    }
    for (auto& [n, t] : pat_vars)
        vars[n] = t;

    // Infer RHS.
    TyRef rhs_ty = infer_expr(*eq.rhs, vars);

    // Build the equation's inferred type.
    // For infix equations (x op y), the operator takes a product: (x # y) -> ret.
    // For regular equations (f x y), build curried: x -> y -> ret.
    TyRef inferred = rhs_ty;
    if (eq.lhs.is_infix && pat_types.size() == 2) {
        // Infix: (arg1 # arg2) -> rhs
        inferred = make_fun_type(make_prod_type(pat_types[0], pat_types[1]), rhs_ty);
    } else {
        for (int i = static_cast<int>(pat_types.size()) - 1; i >= 0; --i)
            inferred = make_fun_type(pat_types[i], inferred);
    }

    // Unify with the self-reference variable (handles recursion).
    if (!unify(self_var, inferred))
        error("recursive type mismatch in '" + fname + "'", loc);

    // If a type was declared, check it.
    const FuncDecl* fd = env_.lookup_func(fname);
    if (fd && fd->type) {
        match_declared(fname, deref(inferred), *fd, loc);
    }
}

void TypeChecker::process_eval(const DEval& ev, SourceLocation loc) {
    (void)loc;
    VarEnv vars;
    infer_expr(*ev.expr, vars);
}

// ---------------------------------------------------------------------------
// match_declared
// ---------------------------------------------------------------------------

void TypeChecker::match_declared(const std::string&  name,
                                  TyRef               inferred,
                                  const FuncDecl&     decl,
                                  SourceLocation      loc) {
    // Instantiate the declared type with frozen (skolem) variables.
    TyRef frozen_decl = instantiate_frozen(decl.params, decl.type);

    // Try to unify inferred with the frozen declared type.
    if (!unify(inferred, frozen_decl)) {
        std::vector<std::string> ctx;
        try {
            // Print the declared type (from the AST, not the inferred graph).
            if (decl.type)
                ctx.push_back("  declared type: " + print_ast_type(*decl.type));
        } catch (...) {}
        try {
            ctx.push_back("  inferred type: " + print_type(inferred));
        } catch (...) {}
        error("'" + name + "': does not match declaration", loc, std::move(ctx));
    }
}

// ---------------------------------------------------------------------------
// error
// ---------------------------------------------------------------------------

[[noreturn]] void TypeChecker::error(const std::string& msg, SourceLocation loc) {
    throw TypeError(msg, std::move(loc));
}

[[noreturn]] void TypeChecker::error(const std::string& msg, SourceLocation loc,
                                     std::vector<std::string> context) {
    throw TypeError(msg, std::move(loc), std::move(context));
}

} // namespace hope
