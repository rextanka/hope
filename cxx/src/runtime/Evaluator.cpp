// Evaluator.cpp — call-by-need tree-walking interpreter for Hope.
//
// See Evaluator.hpp for the public interface.

#include "runtime/Evaluator.hpp"
#include "runtime/RuntimeError.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

namespace hope {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Evaluator::Evaluator(TypeEnv& type_env)
    : type_env_(type_env), global_env_(make_env())
{
    // Seed the constructor arity registry with the built-in constructors
    // (nil, ::, true, false).  User-defined constructors are registered
    // as data declarations are processed.
    constructor_arity_["nil"]   = 0;
    constructor_arity_["::"]    = 1;
    constructor_arity_["true"]  = 0;
    constructor_arity_["false"] = 0;

    init_builtins();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Evaluator::register_builtin(const std::string& name,
                                  std::function<ValRef(ValRef)> fn) {
    auto v = make_fun(std::move(fn));
    global_env_ = env_extend(global_env_, name, v);
}

ValRef Evaluator::eval_top(const Expr& e) {
    return eval(e, global_env_);
}

void Evaluator::add_decl(Decl decl) {
    SourceLocation decl_loc = decl.loc;
    // For DEquation: heap-allocate the decl so that raw pointers into its
    // patterns and body remain stable even as more decls are added later.
    if (std::holds_alternative<DEquation>(decl.data)) {
        auto owned = std::make_unique<Decl>(std::move(decl));
        const Decl* ptr = owned.get();
        owned_decls_.push_back(std::move(owned));
        process_equation(std::get<DEquation>(ptr->data), ptr->loc);
        return;
    }

    std::visit([&](const auto& d) {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, DData>) {
            process_data(d);
        } else if constexpr (std::is_same_v<T, DDec>) {
            process_dec(d);
        } else if constexpr (std::is_same_v<T, DEquation>) {
            // handled above
        } else if constexpr (std::is_same_v<T, DUses>) {
            process_uses(d);
        } else if constexpr (std::is_same_v<T, DEval>) {
            process_eval(d, decl_loc);
        }
        // DInfix, DTypeVar, DType, DAbsType, DPrivate, DSave, DDisplay, DEdit
        // are either handled by the type checker / parser, or are no-ops here.
    }, decl.data);
}

void Evaluator::load_program(std::vector<Decl> decls) {
    for (auto& d : decls) {
        add_decl(std::move(d));
    }
}

// ---------------------------------------------------------------------------
// force / force_full
// ---------------------------------------------------------------------------

ValRef Evaluator::force(ValRef v) {
    while (true) {
        if (auto* th = std::get_if<VThunk>(&v->data)) {
            // Save before clobbering
            const Expr* expr = th->expr;
            Env env = th->env;
            // Black-hole: detect infinite loops
            v->data = VHole{};
            // Evaluate the thunk body
            ValRef result = eval(*expr, env);
            // Force through any nested thunks in the result
            result = force(result);
            // Update in place so all other holders see the forced value
            v->data = result->data;
            return v;
        } else if (std::holds_alternative<VHole>(v->data)) {
            throw RuntimeError("infinite loop detected (black hole)",
                               SourceLocation{"<runtime>", 0, 0, 0});
        } else {
            return v;
        }
    }
}

ValRef Evaluator::force_full(ValRef v) {
    v = force(v);
    std::visit([&](auto& d) {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, VPair>) {
            d.left  = force_full(d.left);
            d.right = force_full(d.right);
        } else if constexpr (std::is_same_v<T, VCons>) {
            if (d.has_arg) d.arg = force_full(d.arg);
        }
        // VNum, VChar, VFun: already in normal form
        // VThunk / VHole: handled by force() above
    }, v->data);
    return v;
}

// ---------------------------------------------------------------------------
// print_value
// ---------------------------------------------------------------------------

std::string Evaluator::print_value(ValRef v) {
    v = force(v);
    return std::visit([&](const auto& d) -> std::string {
        using T = std::decay_t<decltype(d)>;

        if constexpr (std::is_same_v<T, VNum>) {
            // Print integers without decimal point; floats with.
            double n = d.n;
            if (n == std::floor(n) && std::isfinite(n) &&
                n >= -1e15 && n <= 1e15) {
                return std::to_string(static_cast<long long>(n));
            }
            std::ostringstream ss;
            ss << n;
            return ss.str();
        }

        if constexpr (std::is_same_v<T, VChar>) {
            // Single-quote a character
            std::string s = "'";
            char c = d.c;
            if (c == '\'') s += "\\'";
            else if (c == '\\') s += "\\\\";
            else s += c;
            s += "'";
            return s;
        }

        if constexpr (std::is_same_v<T, VCons>) {
            // Nil / cons: check for list-of-char (print as string)
            if (d.name == "nil") {
                // Could be empty string or empty list
                return "nil";
            }
            if (d.name == "::") {
                // Check if this looks like a string (list of chars)
                ValRef arg_v = force(d.arg);
                if (auto* pair = std::get_if<VPair>(&arg_v->data)) {
                    ValRef head = force(pair->left);
                    if (std::holds_alternative<VChar>(head->data)) {
                        // Print as string
                        std::string s = "\"";
                        ValRef cur = v;
                        while (true) {
                            cur = force(cur);
                            auto* cons = std::get_if<VCons>(&cur->data);
                            if (!cons || cons->name == "nil") break;
                            if (cons->name != "::") break;
                            ValRef a = force(cons->arg);
                            auto* pr = std::get_if<VPair>(&a->data);
                            if (!pr) break;
                            ValRef h = force(pr->left);
                            auto* ch = std::get_if<VChar>(&h->data);
                            if (!ch) break;
                            char c = ch->c;
                            if (c == '"') s += "\\\"";
                            else if (c == '\\') s += "\\\\";
                            else if (c == '\n') s += "\\n";
                            else if (c == '\t') s += "\\t";
                            else s += c;
                            cur = pr->right;
                        }
                        s += "\"";
                        return s;
                    }
                }
                // Print as list
                std::string s = "[";
                ValRef cur = v;
                bool first = true;
                while (true) {
                    cur = force(cur);
                    auto* cons = std::get_if<VCons>(&cur->data);
                    if (!cons || cons->name == "nil") break;
                    if (cons->name != "::") {
                        // Malformed list
                        s += "...";
                        break;
                    }
                    if (!first) s += ", ";
                    first = false;
                    ValRef a = force(cons->arg);
                    auto* pr = std::get_if<VPair>(&a->data);
                    if (!pr) break;
                    s += print_value(pr->left);
                    cur = pr->right;
                }
                s += "]";
                return s;
            }
            // Other constructor
            if (d.has_arg) {
                return d.name + "(" + print_value(d.arg) + ")";
            }
            return d.name;
        }

        if constexpr (std::is_same_v<T, VPair>) {
            // Pairs print as (a, b) or (a, b, c) for nested right pairs.
            // We flatten right-nested pairs for display.
            std::string s = "(";
            ValRef cur = v;
            bool first = true;
            while (true) {
                cur = force(cur);
                auto* pr = std::get_if<VPair>(&cur->data);
                if (!pr) {
                    if (!first) s += ", ";
                    s += print_value(cur);
                    break;
                }
                if (!first) s += ", ";
                first = false;
                s += print_value(pr->left);
                cur = pr->right;
            }
            s += ")";
            return s;
        }

        if constexpr (std::is_same_v<T, VFun>) {
            return "<function>";
        }

        if constexpr (std::is_same_v<T, VThunk>) {
            return "<thunk>";
        }

        if constexpr (std::is_same_v<T, VHole>) {
            return "<black hole>";
        }

        return "<unknown>";
    }, v->data);
}

// ---------------------------------------------------------------------------
// Literal parsing helpers
// ---------------------------------------------------------------------------

double Evaluator::parse_num_literal(const std::string& text) {
    try {
        size_t pos = 0;
        double val = std::stod(text, &pos);
        return val;
    } catch (...) {
        throw RuntimeError("invalid numeric literal: " + text,
                           SourceLocation{"<runtime>", 0, 0, 0});
    }
}

char Evaluator::parse_char_literal(const std::string& text) {
    // text is like 'a' or '\n' — strip outer quotes, handle escapes.
    if (text.size() < 2) {
        throw RuntimeError("malformed char literal: " + text,
                           SourceLocation{"<runtime>", 0, 0, 0});
    }
    // Strip surrounding single quotes
    std::string inner = text.substr(1, text.size() - 2);
    if (inner.empty()) {
        throw RuntimeError("empty char literal",
                           SourceLocation{"<runtime>", 0, 0, 0});
    }
    if (inner[0] != '\\') return inner[0];
    if (inner.size() < 2) {
        throw RuntimeError("malformed char escape: " + text,
                           SourceLocation{"<runtime>", 0, 0, 0});
    }
    switch (inner[1]) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"':  return '"';
        case '0':  return '\0';
        default:   return inner[1];
    }
}

ValRef Evaluator::string_to_list(const std::string& text) {
    // text is like "hello" — strip outer double-quotes, handle escapes.
    if (text.size() < 2) {
        return make_nil();
    }
    // Strip surrounding double quotes
    std::string inner = text.substr(1, text.size() - 2);

    // Parse escape sequences and collect characters
    std::vector<char> chars;
    for (size_t i = 0; i < inner.size(); ++i) {
        if (inner[i] == '\\' && i + 1 < inner.size()) {
            ++i;
            switch (inner[i]) {
                case 'n':  chars.push_back('\n'); break;
                case 't':  chars.push_back('\t'); break;
                case 'r':  chars.push_back('\r'); break;
                case '\\': chars.push_back('\\'); break;
                case '\'': chars.push_back('\''); break;
                case '"':  chars.push_back('"');  break;
                case '0':  chars.push_back('\0'); break;
                default:   chars.push_back(inner[i]); break;
            }
        } else {
            chars.push_back(inner[i]);
        }
    }

    // Build the list from right to left
    ValRef result = make_nil();
    for (auto it = chars.rbegin(); it != chars.rend(); ++it) {
        result = make_cons(make_char(*it), result);
    }
    return result;
}

// ---------------------------------------------------------------------------
// eval — the main evaluator
// ---------------------------------------------------------------------------

ValRef Evaluator::eval(const Expr& e, Env env) {
    return std::visit([&](const auto& d) -> ValRef {
        using T = std::decay_t<decltype(d)>;

        // ----------------------------------------------------------------
        // ELit — literal values
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, ELit>) {
            return std::visit([&](const auto& lit) -> ValRef {
                using L = std::decay_t<decltype(lit)>;
                if constexpr (std::is_same_v<L, LitNum>) {
                    return make_num(parse_num_literal(lit.text));
                }
                if constexpr (std::is_same_v<L, LitFloat>) {
                    return make_num(parse_num_literal(lit.text));
                }
                if constexpr (std::is_same_v<L, LitChar>) {
                    return make_char(parse_char_literal(lit.text));
                }
                if constexpr (std::is_same_v<L, LitStr>) {
                    return string_to_list(lit.text);
                }
                return make_nil();
            }, d.lit);
        }

        // ----------------------------------------------------------------
        // EVar — variable lookup
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EVar>) {
            // 1. Local environment (most recent binding first)
            if (auto v = env_lookup(env, d.name)) return v;
            // 2. Global environment (built-ins, already-bound functions)
            if (auto v = env_lookup(global_env_, d.name)) return v;
            // 3. Functions registry (equations defined via DEquation)
            if (functions_.count(d.name)) {
                return make_function_val(d.name);
            }
            // 4. Nullary constructors from type env
            if (auto* ci = type_env_.lookup_con(d.name)) {
                if (!ci->arg.has_value()) return make_con0(d.name);
                // Unary constructor: return as a function
                std::string cname = d.name;
                return make_fun([cname](ValRef arg) -> ValRef {
                    return make_con1(cname, arg);
                });
            }
            throw RuntimeError("unbound variable: " + d.name, e.loc);
        }

        // ----------------------------------------------------------------
        // EOpRef — operator as a value: (+)
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EOpRef>) {
            if (auto v = env_lookup(env, d.op)) return v;
            if (auto v = env_lookup(global_env_, d.op)) return v;
            if (functions_.count(d.op)) return make_function_val(d.op);
            throw RuntimeError("unbound operator: " + d.op, e.loc);
        }

        // ----------------------------------------------------------------
        // EApply — function application: f x
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EApply>) {
            ValRef fun_val = eval(*d.func, env);
            // Argument is lazy: wrap in a thunk
            ValRef arg_thunk = make_thunk(d.arg.get(), env);
            return apply(force(fun_val), arg_thunk);
        }

        // ----------------------------------------------------------------
        // EInfix — infix application: x op y
        // The operator takes a product pair (x # y) as its argument.
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EInfix>) {
            ValRef op_val = [&]() -> ValRef {
                if (auto v = env_lookup(env, d.op)) return v;
                if (auto v = env_lookup(global_env_, d.op)) return v;
                if (functions_.count(d.op)) return make_function_val(d.op);
                throw RuntimeError("unbound operator: " + d.op, e.loc);
            }();
            ValRef left_thunk  = make_thunk(d.left.get(),  env);
            ValRef right_thunk = make_thunk(d.right.get(), env);
            ValRef pair_val = make_pair(left_thunk, right_thunk);
            return apply(force(op_val), pair_val);
        }

        // ----------------------------------------------------------------
        // ETuple — product types: (a, b, c) = right-nested pairs
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, ETuple>) {
            const auto& elems = d.elems;
            if (elems.empty()) {
                // Unit: represent as nil (or empty cons)
                return make_nil();
            }
            if (elems.size() == 1) {
                return make_thunk(elems[0].get(), env);
            }
            // Build right-nested pairs from the right
            // (a, b, c) = VPair{a, VPair{b, c}}
            ValRef result = make_thunk(elems.back().get(), env);
            for (int i = static_cast<int>(elems.size()) - 2; i >= 0; --i) {
                ValRef left = make_thunk(elems[i].get(), env);
                result = make_pair(left, result);
            }
            return result;
        }

        // ----------------------------------------------------------------
        // EList — list literals: [a, b, c] = a :: b :: c :: nil
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EList>) {
            const auto& elems = d.elems;
            ValRef result = make_nil();
            for (auto it = elems.rbegin(); it != elems.rend(); ++it) {
                ValRef elem_thunk = make_thunk((*it).get(), env);
                result = make_cons(elem_thunk, result);
            }
            return result;
        }

        // ----------------------------------------------------------------
        // ELambda — multi-clause anonymous function
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, ELambda>) {
            // Build a FuncClause list (raw pointers into the AST, safe since
            // the AST outlives the closure).
            std::vector<FuncClause> func_clauses;
            for (const auto& lc : d.clauses) {
                FuncClause fc;
                for (const auto& pp : lc.pats) fc.pats.push_back(pp.get());
                fc.body = lc.body.get();
                func_clauses.push_back(std::move(fc));
            }
            Env captured = env;
            return make_fun([this, func_clauses = std::move(func_clauses), captured](ValRef arg) -> ValRef {
                return apply_clauses("<lambda>", func_clauses, arg, captured);
            });
        }

        // ----------------------------------------------------------------
        // ESection — operator section
        //   Left section  (e op): lambda y => op(e, y)
        //   Right section (op e): lambda x => op(x, e)
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, ESection>) {
            ValRef op_val = [&]() -> ValRef {
                if (auto v = env_lookup(env, d.op)) return v;
                if (auto v = env_lookup(global_env_, d.op)) return v;
                if (functions_.count(d.op)) return make_function_val(d.op);
                throw RuntimeError("unbound operator in section: " + d.op, e.loc);
            }();
            ValRef e_val = make_thunk(d.expr.get(), env);

            if (d.is_left) {
                // (e op): lambda y => op (e, y)
                return make_fun([this, op_val, e_val](ValRef y) -> ValRef {
                    ValRef pair = make_pair(e_val, y);
                    return apply(force(op_val), pair);
                });
            } else {
                // (op e): lambda x => op (x, e)
                return make_fun([this, op_val, e_val](ValRef x) -> ValRef {
                    ValRef pair = make_pair(x, e_val);
                    return apply(force(op_val), pair);
                });
            }
        }

        // ----------------------------------------------------------------
        // EIf — conditional
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EIf>) {
            ValRef cond_val = force(eval(*d.cond, env));
            auto* cons = std::get_if<VCons>(&cond_val->data);
            if (!cons) {
                throw RuntimeError("condition is not a boolean", e.loc);
            }
            if (cons->name == "true") {
                return make_thunk(d.then_.get(), env);
            } else if (cons->name == "false") {
                return make_thunk(d.else_.get(), env);
            } else {
                throw RuntimeError("condition is not a boolean: " + cons->name,
                                   e.loc);
            }
        }

        // ----------------------------------------------------------------
        // ELet — non-recursive local binding
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, ELet>) {
            Env new_env = env;
            for (const auto& bind : d.binds) {
                ValRef val = make_thunk(bind.body.get(), new_env);
                if (!match(*bind.pat, val, new_env)) {
                    throw RuntimeError("let binding pattern match failed",
                                       bind.loc);
                }
            }
            return eval(*d.body, new_env);
        }

        // ----------------------------------------------------------------
        // ELetRec — recursive local binding
        // The trick: create a thunk whose captured environment contains
        // a self-referential binding (the thunk points to the env that
        // points back to the thunk).
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, ELetRec>) {
            Env new_env = env;
            for (const auto& bind : d.binds) {
                // Create a placeholder thunk
                auto placeholder = make_thunk(bind.body.get(), env);
                // For simple PVar binding: create the cycle
                if (auto* pv = std::get_if<PVar>(&bind.pat->data)) {
                    new_env = env_extend(new_env, pv->name, placeholder);
                    // Point the thunk's env back at new_env (enables recursion)
                    std::get<VThunk>(placeholder->data).env = new_env;
                } else {
                    // For complex patterns, evaluate eagerly and then match
                    // (true recursive pattern bindings are unusual in Hope)
                    ValRef val = make_thunk(bind.body.get(), new_env);
                    if (!match(*bind.pat, val, new_env)) {
                        throw RuntimeError("letrec binding pattern match failed",
                                           bind.loc);
                    }
                }
            }
            return eval(*d.body, new_env);
        }

        // ----------------------------------------------------------------
        // EWhere — non-recursive where clause (body first in AST)
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EWhere>) {
            Env new_env = env;
            for (const auto& bind : d.binds) {
                ValRef val = make_thunk(bind.body.get(), new_env);
                if (!match(*bind.pat, val, new_env)) {
                    throw RuntimeError("where binding pattern match failed",
                                       bind.loc);
                }
            }
            return eval(*d.body, new_env);
        }

        // ----------------------------------------------------------------
        // EWhereRec — recursive where clause
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EWhereRec>) {
            Env new_env = env;
            for (const auto& bind : d.binds) {
                auto placeholder = make_thunk(bind.body.get(), env);
                if (auto* pv = std::get_if<PVar>(&bind.pat->data)) {
                    new_env = env_extend(new_env, pv->name, placeholder);
                    std::get<VThunk>(placeholder->data).env = new_env;
                } else {
                    ValRef val = make_thunk(bind.body.get(), new_env);
                    if (!match(*bind.pat, val, new_env)) {
                        throw RuntimeError("whererec binding pattern match failed",
                                           bind.loc);
                    }
                }
            }
            return eval(*d.body, new_env);
        }

        // ----------------------------------------------------------------
        // EWrite — write expr (Hope's lazy I/O output)
        // ----------------------------------------------------------------
        if constexpr (std::is_same_v<T, EWrite>) {
            ValRef val = eval(*d.expr, env);
            val = force(val);
            // If it's a list of chars, print as raw characters
            // Otherwise, print the value representation
            auto* cons = std::get_if<VCons>(&val->data);
            if (cons && (cons->name == "::" || cons->name == "nil")) {
                // Walk the list and print chars
                ValRef cur = val;
                while (true) {
                    cur = force(cur);
                    auto* c = std::get_if<VCons>(&cur->data);
                    if (!c || c->name == "nil") break;
                    if (c->name != "::") break;
                    ValRef arg = force(c->arg);
                    auto* pr = std::get_if<VPair>(&arg->data);
                    if (!pr) break;
                    ValRef head = force(pr->left);
                    if (auto* ch = std::get_if<VChar>(&head->data)) {
                        std::cout << ch->c;
                    } else {
                        std::cout << print_value(head);
                    }
                    cur = pr->right;
                }
            } else {
                std::cout << print_value(val);
            }
            // write returns the unit value (nil in our representation)
            return make_nil();
        }

        throw RuntimeError("unknown expression type", e.loc);
    }, e.data);
}

// ---------------------------------------------------------------------------
// apply
// ---------------------------------------------------------------------------

ValRef Evaluator::apply(ValRef fun, ValRef arg) {
    // fun should already be forced (caller's responsibility)
    if (auto* vf = std::get_if<VFun>(&fun->data)) {
        return vf->apply(arg);
    }
    // If fun is still a thunk somehow (shouldn't happen if caller forced it)
    if (std::holds_alternative<VThunk>(fun->data)) {
        fun = force(fun);
        if (auto* vf = std::get_if<VFun>(&fun->data)) {
            return vf->apply(arg);
        }
    }
    throw RuntimeError("attempt to apply a non-function value",
                       SourceLocation{"<runtime>", 0, 0, 0});
}

// ---------------------------------------------------------------------------
// apply_clauses — multi-clause pattern dispatch
//
// Handles multi-argument functions via currying.  When a clause has N
// patterns (N > 1), matching the first pattern returns a VFun that waits
// for the second argument, and so on.  A helper struct CurriedApply
// captures the necessary state so that arbitrary arities work cleanly.
// ---------------------------------------------------------------------------

namespace {

// A single step in applying a curried, multi-pattern clause.
// Holds the clause being dispatched and the index of the NEXT pattern to
// match.  The environment accumulated so far is also captured.
struct CurriedApply {
    Evaluator*                  ev;
    const Evaluator::FuncClause* clause;   // raw pointer — AST outlives runtime
    size_t                      next_pat;  // index of next pattern to match
    Env                         env;       // accumulated bindings so far
    std::string                 fname;     // for error messages

    ValRef operator()(ValRef arg) {
        Env cur_env = env;
        // Don't force eagerly — let match_pat force lazily as needed.
        if (!ev->match_pat(*clause->pats[next_pat], arg, cur_env)) {
            throw RuntimeError("pattern match failure in function: " + fname,
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        size_t n = next_pat + 1;
        if (n == clause->pats.size()) {
            // All patterns matched — evaluate the body
            return ev->eval_in(clause->body, cur_env);
        }
        // More patterns remain — return another curried function
        CurriedApply next{ev, clause, n, cur_env, fname};
        return make_fun(std::move(next));
    }
};

} // anonymous namespace

ValRef Evaluator::apply_clauses(const std::string& name,
                                 const std::vector<FuncClause>& clauses,
                                 ValRef arg, Env closed_env) {
    for (const auto& clause : clauses) {
        if (clause.pats.empty()) {
            // Zero-argument clause
            return eval(*clause.body, closed_env);
        }

        // Try to match the first pattern.
        // Do NOT force here — let match() force lazily as needed.
        Env match_env = closed_env;
        if (!match(*clause.pats[0], arg, match_env)) {
            continue; // try next clause
        }

        if (clause.pats.size() == 1) {
            return eval(*clause.body, match_env);
        }

        // More patterns — return a curried function for the remaining patterns
        CurriedApply ca{this, &clause, 1, match_env, name};
        return make_fun(std::move(ca));
    }

    throw RuntimeError("non-exhaustive patterns in function: " + name,
                       SourceLocation{"<runtime>", 0, 0, 0});
}

// ---------------------------------------------------------------------------
// match — pattern matching
// ---------------------------------------------------------------------------

bool Evaluator::match(const Pattern& p, ValRef v, Env& env) {
    return std::visit([&](const auto& pd) -> bool {
        using T = std::decay_t<decltype(pd)>;

        // PVar — bind the variable
        if constexpr (std::is_same_v<T, PVar>) {
            env = env_extend(env, pd.name, v);
            return true;
        }

        // PWild — always matches, binds nothing
        if constexpr (std::is_same_v<T, PWild>) {
            return true;
        }

        // PLit — literal patterns
        if constexpr (std::is_same_v<T, PLit>) {
            return std::visit([&](const auto& lit) -> bool {
                using L = std::decay_t<decltype(lit)>;

                if constexpr (std::is_same_v<L, LitNum> || std::is_same_v<L, LitFloat>) {
                    double expected = parse_num_literal(lit.text);
                    ValRef forced = force(v);
                    auto* vn = std::get_if<VNum>(&forced->data);
                    return vn && vn->n == expected;
                }

                if constexpr (std::is_same_v<L, LitChar>) {
                    char expected = parse_char_literal(lit.text);
                    ValRef forced = force(v);
                    auto* vc = std::get_if<VChar>(&forced->data);
                    return vc && vc->c == expected;
                }

                if constexpr (std::is_same_v<L, LitStr>) {
                    // Match a string literal against a list of chars
                    ValRef expected_list = string_to_list(lit.text);
                    // Force both and compare recursively
                    ValRef lhs = force(v);
                    ValRef rhs = force(expected_list);
                    while (true) {
                        auto* lc = std::get_if<VCons>(&lhs->data);
                        auto* rc = std::get_if<VCons>(&rhs->data);
                        if (!lc || !rc) return lhs.get() == rhs.get();
                        if (lc->name != rc->name) return false;
                        if (lc->name == "nil") return true;
                        if (lc->name != "::") return false;
                        // Compare heads and advance tails
                        ValRef la = force(lc->arg);
                        ValRef ra = force(rc->arg);
                        auto* lpr = std::get_if<VPair>(&la->data);
                        auto* rpr = std::get_if<VPair>(&ra->data);
                        if (!lpr || !rpr) return false;
                        ValRef lh = force(lpr->left);
                        ValRef rh = force(rpr->left);
                        auto* lch = std::get_if<VChar>(&lh->data);
                        auto* rch = std::get_if<VChar>(&rh->data);
                        if (!lch || !rch) return false;
                        if (lch->c != rch->c) return false;
                        lhs = force(lpr->right);
                        rhs = force(rpr->right);
                    }
                }

                return false;
            }, pd.lit);
        }

        // PSucc — succ(p) matches VNum{n} where n > 0, binds n-1 to p
        if constexpr (std::is_same_v<T, PSucc>) {
            ValRef forced = force(v);
            auto* vn = std::get_if<VNum>(&forced->data);
            if (!vn || vn->n < 1.0) return false;
            ValRef inner_val = make_num(vn->n - 1.0);
            return match(*pd.inner, inner_val, env);
        }

        // PNPlusK — n+k pattern: matches VNum{m} where m >= k, binds var to m-k
        if constexpr (std::is_same_v<T, PNPlusK>) {
            ValRef forced = force(v);
            auto* vn = std::get_if<VNum>(&forced->data);
            if (!vn || vn->n < static_cast<double>(pd.k)) return false;
            ValRef bound = make_num(vn->n - pd.k);
            env = env_extend(env, pd.var, bound);
            return true;
        }

        // PCons — constructor patterns
        if constexpr (std::is_same_v<T, PCons>) {
            ValRef forced = force(v);
            auto* vc = std::get_if<VCons>(&forced->data);
            if (!vc || vc->name != pd.name) return false;
            if (!pd.arg.has_value()) {
                return !vc->has_arg;
            }
            if (!vc->has_arg) return false;
            return match(**pd.arg, vc->arg, env);
        }

        // PTuple — product patterns: (p1, p2, ...)
        if constexpr (std::is_same_v<T, PTuple>) {
            const auto& pats = pd.elems;
            if (pats.empty()) return true; // unit

            // Right-nested pairs: (a, b, c) = VPair{a, VPair{b, c}}
            ValRef cur = force(v);
            for (size_t i = 0; i < pats.size(); ++i) {
                if (i + 1 == pats.size()) {
                    // Last element: match directly
                    return match(*pats[i], cur, env);
                }
                auto* pr = std::get_if<VPair>(&cur->data);
                if (!pr) return false;
                if (!match(*pats[i], pr->left, env)) return false;
                cur = force(pr->right);
            }
            return true;
        }

        // PList — list patterns: [p1, p2, ...]
        if constexpr (std::is_same_v<T, PList>) {
            const auto& pats = pd.elems;
            ValRef cur = force(v);

            if (pats.empty()) {
                // Match nil
                auto* vc = std::get_if<VCons>(&cur->data);
                return vc && vc->name == "nil";
            }

            for (const auto& pat : pats) {
                auto* vc = std::get_if<VCons>(&cur->data);
                if (!vc || vc->name != "::") return false;
                // arg of :: is a VPair{head, tail}
                ValRef arg = force(vc->arg);
                auto* pr = std::get_if<VPair>(&arg->data);
                if (!pr) return false;
                if (!match(*pat, pr->left, env)) return false;
                cur = force(pr->right);
            }
            // After all elements, must be nil
            auto* vc = std::get_if<VCons>(&cur->data);
            return vc && vc->name == "nil";
        }

        // PInfix — infix patterns: x :: xs or user-defined infix constructors
        if constexpr (std::is_same_v<T, PInfix>) {
            return match_infix(pd, v, env);
        }

        return false;
    }, p.data);
}

bool Evaluator::match_infix(const PInfix& pi, ValRef v, Env& env) {
    ValRef forced = force(v);

    if (pi.op == "::") {
        // List cons pattern: left :: right
        auto* vc = std::get_if<VCons>(&forced->data);
        if (!vc || vc->name != "::") return false;
        // arg is VPair{head, tail}
        ValRef arg = force(vc->arg);
        auto* pr = std::get_if<VPair>(&arg->data);
        if (!pr) return false;
        if (!match(*pi.left, pr->left, env)) return false;
        return match(*pi.right, pr->right, env);
    }

    // User-defined infix constructor: the constructor has an arg which is a pair
    auto* vc = std::get_if<VCons>(&forced->data);
    if (!vc || vc->name != pi.op) return false;
    if (!vc->has_arg) return false;
    ValRef arg = force(vc->arg);
    auto* pr = std::get_if<VPair>(&arg->data);
    if (!pr) return false;
    if (!match(*pi.left, pr->left, env)) return false;
    return match(*pi.right, pr->right, env);
}

// ---------------------------------------------------------------------------
// Declaration processing
// ---------------------------------------------------------------------------

void Evaluator::process_data(const DData& d) {
    for (const auto& con : d.alts) {
        int arity = con.arg.has_value() ? 1 : 0;
        constructor_arity_[con.name] = arity;
        if (arity == 0) {
            // Register nullary constructor in global env
            global_env_ = env_extend(global_env_, con.name, make_con0(con.name));
        } else {
            // Register unary constructor as a function
            std::string cname = con.name;
            auto ctor_fun = make_fun([cname](ValRef arg) -> ValRef {
                return make_con1(cname, arg);
            });
            global_env_ = env_extend(global_env_, con.name, ctor_fun);
        }
    }
}

void Evaluator::process_equation(const DEquation& d, SourceLocation /*loc*/) {
    const std::string& fname = d.lhs.func;

    // Build a FuncClause from the equation's arg patterns and body.
    // Use raw pointers into the AST — the Decl is kept alive in owned_decls_.
    FuncClause clause;
    for (const auto& arg_pat : d.lhs.args)
        clause.pats.push_back(arg_pat.get());
    clause.body = d.rhs.get();

    auto& func_def = functions_[fname];
    if (func_def.name.empty())
        func_def.name = fname;
    func_def.clauses.push_back(std::move(clause));

    // Update the global env with the new function value.
    // (Previously registered versions are superseded by this new one.)
    ValRef fval = make_function_val(fname);
    // Remove old binding and add new one
    global_env_ = env_extend(global_env_, fname, fval);
}

void Evaluator::process_dec(const DDec& /*d*/) {
    // Type declarations are handled by the type checker.
    // The evaluator ignores them.
}

void Evaluator::process_uses(const DUses& /*d*/) {
    // Module loading is handled by ModuleLoader.
    // The evaluator itself ignores `uses` declarations.
}

void Evaluator::process_eval(const DEval& d, SourceLocation loc) {
    // Evaluate the expression and print the result.
    try {
        ValRef result = eval(*d.expr, global_env_);
        result = force_full(result);
        std::cout << print_value(result) << "\n";
    } catch (const RuntimeError& re) {
        std::cerr << "Runtime error: " << re.what()
                  << " at " << re.loc.to_string() << "\n";
    }
}

// ---------------------------------------------------------------------------
// make_function_val — build a VFun that dispatches to named function clauses
// ---------------------------------------------------------------------------

ValRef Evaluator::make_function_val(const std::string& name) {
    // Capture by name — look up clauses at call time (supports redefinition)
    return make_fun([this, name](ValRef arg) -> ValRef {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw RuntimeError("undefined function: " + name,
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        const FuncDef& fd = it->second;
        // Use current global_env_ so that recursive calls work
        return apply_clauses(name, fd.clauses, arg, global_env_);
    });
}

// ---------------------------------------------------------------------------
// init_builtins — register built-in functions
// ---------------------------------------------------------------------------

void Evaluator::init_builtins() {
    // Helper lambdas
    auto get_num = [this](ValRef v) -> double {
        ValRef f = force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return vn->n;
        throw RuntimeError("expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    };
    auto get_pair = [this](ValRef v) -> std::pair<ValRef, ValRef> {
        ValRef f = force(v);
        if (auto* pr = std::get_if<VPair>(&f->data)) {
            return {pr->left, pr->right};
        }
        throw RuntimeError("expected a pair argument for binary operator",
                           SourceLocation{"<runtime>", 0, 0, 0});
    };
    auto get_char = [this](ValRef v) -> char {
        ValRef f = force(v);
        if (auto* vc = std::get_if<VChar>(&f->data)) return vc->c;
        throw RuntimeError("expected a char",
                           SourceLocation{"<runtime>", 0, 0, 0});
    };

    // --- Arithmetic binary operators (take a pair) ---

    register_builtin("+", [get_pair, get_num](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        return make_num(get_num(a) + get_num(b));
    });

    register_builtin("-", [get_pair, get_num](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        return make_num(get_num(a) - get_num(b));
    });

    register_builtin("*", [get_pair, get_num](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        return make_num(get_num(a) * get_num(b));
    });

    register_builtin("/", [get_pair, get_num](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        double divisor = get_num(b);
        if (divisor == 0.0) {
            throw RuntimeError("division by zero",
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        return make_num(get_num(a) / divisor);
    });

    register_builtin("div", [get_pair, get_num](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        double da = get_num(a), db = get_num(b);
        if (db == 0.0) {
            throw RuntimeError("integer division by zero",
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        return make_num(std::floor(da / db));
    });

    register_builtin("mod", [get_pair, get_num](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        double da = get_num(a), db = get_num(b);
        if (db == 0.0) {
            throw RuntimeError("mod by zero",
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        double result = da - std::floor(da / db) * db;
        return make_num(result);
    });

    // --- Unary negation ---
    register_builtin("~", [get_num](ValRef v) -> ValRef {
        return make_num(-get_num(v));
    });
    register_builtin("neg", [get_num](ValRef v) -> ValRef {
        return make_num(-get_num(v));
    });

    // --- Comparison operators (polymorphic over num and char) ---

    // Helper to compare two forced values
    auto cmp_vals = [this, get_num, get_char](ValRef a, ValRef b,
                                              const std::string& op) -> bool {
        ValRef fa = force(a), fb = force(b);
        if (auto* an = std::get_if<VNum>(&fa->data)) {
            double bn = get_num(b);
            double av = an->n;
            if (op == "=")  return av == bn;
            if (op == "/=") return av != bn;
            if (op == "<")  return av < bn;
            if (op == ">")  return av > bn;
            if (op == "=<") return av <= bn;
            if (op == ">=") return av >= bn;
        }
        if (auto* ac = std::get_if<VChar>(&fa->data)) {
            char bc = get_char(b);
            char av = ac->c;
            if (op == "=")  return av == bc;
            if (op == "/=") return av != bc;
            if (op == "<")  return av < bc;
            if (op == ">")  return av > bc;
            if (op == "=<") return av <= bc;
            if (op == ">=") return av >= bc;
        }
        // Structural equality for constructors
        if (auto* ac = std::get_if<VCons>(&fa->data)) {
            if (auto* bc2 = std::get_if<VCons>(&fb->data)) {
                if (op == "=")  return ac->name == bc2->name;
                if (op == "/=") return ac->name != bc2->name;
            }
        }
        throw RuntimeError("cannot compare these values with " + op,
                           SourceLocation{"<runtime>", 0, 0, 0});
    };

    for (const char* op : {"=", "/=", "<", ">", "=<", ">="}) {
        std::string op_str(op);
        register_builtin(op_str, [get_pair, cmp_vals, op_str](ValRef v) -> ValRef {
            auto [a, b] = get_pair(v);
            return make_bool(cmp_vals(a, b, op_str));
        });
    }

    // --- Boolean operators (short-circuit via lazy args) ---

    register_builtin("and", [this, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        ValRef fa = force(a);
        auto* ca = std::get_if<VCons>(&fa->data);
        if (!ca) throw RuntimeError("'and' expects boolean",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        if (ca->name == "false") return make_bool(false);
        ValRef fb = force(b);
        auto* cb = std::get_if<VCons>(&fb->data);
        if (!cb) throw RuntimeError("'and' expects boolean",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        return make_bool(cb->name == "true");
    });

    register_builtin("or", [this, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        ValRef fa = force(a);
        auto* ca = std::get_if<VCons>(&fa->data);
        if (!ca) throw RuntimeError("'or' expects boolean",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        if (ca->name == "true") return make_bool(true);
        ValRef fb = force(b);
        auto* cb = std::get_if<VCons>(&fb->data);
        if (!cb) throw RuntimeError("'or' expects boolean",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        return make_bool(cb->name == "true");
    });

    register_builtin("not", [this](ValRef v) -> ValRef {
        ValRef f = force(v);
        auto* c = std::get_if<VCons>(&f->data);
        if (!c) throw RuntimeError("'not' expects boolean",
                                   SourceLocation{"<runtime>", 0, 0, 0});
        return make_bool(c->name == "false");
    });

    // --- Numeric functions ---

    register_builtin("succ", [get_num](ValRef v) -> ValRef {
        return make_num(get_num(v) + 1.0);
    });

    register_builtin("pred", [get_num](ValRef v) -> ValRef {
        return make_num(get_num(v) - 1.0);
    });

    register_builtin("abs", [get_num](ValRef v) -> ValRef {
        return make_num(std::abs(get_num(v)));
    });

    register_builtin("sqrt", [get_num](ValRef v) -> ValRef {
        return make_num(std::sqrt(get_num(v)));
    });

    register_builtin("floor", [get_num](ValRef v) -> ValRef {
        return make_num(std::floor(get_num(v)));
    });

    register_builtin("ceiling", [get_num](ValRef v) -> ValRef {
        return make_num(std::ceil(get_num(v)));
    });

    register_builtin("round", [get_num](ValRef v) -> ValRef {
        return make_num(std::round(get_num(v)));
    });

    register_builtin("trunc", [get_num](ValRef v) -> ValRef {
        return make_num(std::trunc(get_num(v)));
    });

    register_builtin("float", [get_num](ValRef v) -> ValRef {
        return make_num(get_num(v)); // already double
    });

    // --- Character functions ---

    register_builtin("ord", [get_char](ValRef v) -> ValRef {
        return make_num(static_cast<double>(static_cast<unsigned char>(get_char(v))));
    });

    register_builtin("chr", [get_num](ValRef v) -> ValRef {
        double n = get_num(v);
        return make_char(static_cast<char>(static_cast<int>(n)));
    });

    // --- String / list functions ---

    // num2str: num -> list char
    register_builtin("num2str", [this, get_num](ValRef v) -> ValRef {
        double n = get_num(v);
        std::string s;
        if (n == std::floor(n) && std::isfinite(n)) {
            s = std::to_string(static_cast<long long>(n));
        } else {
            std::ostringstream ss;
            ss << n;
            s = ss.str();
        }
        return string_to_list("\"" + s + "\"");
    });

    // str2num: list char -> num (returns 0 if invalid)
    register_builtin("str2num", [this](ValRef v) -> ValRef {
        // Collect chars from the list
        std::string s;
        ValRef cur = force(v);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ValRef arg = force(vc->arg);
            auto* pr = std::get_if<VPair>(&arg->data);
            if (!pr) break;
            ValRef head = force(pr->left);
            auto* ch = std::get_if<VChar>(&head->data);
            if (!ch) break;
            s += ch->c;
            cur = force(pr->right);
        }
        try {
            return make_num(std::stod(s));
        } catch (...) {
            return make_num(0.0);
        }
    });

    // --- Error function ---

    register_builtin("error", [this](ValRef v) -> ValRef {
        // Force the string (list of chars) and throw as RuntimeError
        std::string msg;
        ValRef cur = force(v);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ValRef arg = force(vc->arg);
            auto* pr = std::get_if<VPair>(&arg->data);
            if (!pr) break;
            ValRef head = force(pr->left);
            auto* ch = std::get_if<VChar>(&head->data);
            if (!ch) break;
            msg += ch->c;
            cur = force(pr->right);
        }
        throw RuntimeError("error: " + msg, SourceLocation{"<runtime>", 0, 0, 0});
    });

    // --- Identity / const ---

    register_builtin("id", [](ValRef v) -> ValRef { return v; });

    register_builtin("const", [](ValRef v) -> ValRef {
        return make_fun([v](ValRef /*ignored*/) -> ValRef { return v; });
    });

    // --- Function composition: (f o g) x = f (g x) ---

    register_builtin("o", [this, get_pair](ValRef v) -> ValRef {
        auto [f, g] = get_pair(v);
        ValRef ff = force(f);
        ValRef fg = force(g);
        return make_fun([this, ff, fg](ValRef x) -> ValRef {
            ValRef gx = apply(force(fg), x);
            return apply(force(ff), gx);
        });
    });

    // --- Pair accessors ---

    register_builtin("fst", [this](ValRef v) -> ValRef {
        ValRef f = force(v);
        if (auto* pr = std::get_if<VPair>(&f->data)) return pr->left;
        throw RuntimeError("fst: not a pair", SourceLocation{"<runtime>", 0, 0, 0});
    });

    register_builtin("snd", [this](ValRef v) -> ValRef {
        ValRef f = force(v);
        if (auto* pr = std::get_if<VPair>(&f->data)) return pr->right;
        throw RuntimeError("snd: not a pair", SourceLocation{"<runtime>", 0, 0, 0});
    });

    // --- List primitives ---

    // head : list alpha -> alpha
    register_builtin("head", [this](ValRef v) -> ValRef {
        ValRef f = force(v);
        auto* vc = std::get_if<VCons>(&f->data);
        if (!vc || vc->name != "::") {
            throw RuntimeError("head: empty list", SourceLocation{"<runtime>", 0, 0, 0});
        }
        ValRef arg = force(vc->arg);
        auto* pr = std::get_if<VPair>(&arg->data);
        if (!pr) throw RuntimeError("head: malformed cons", SourceLocation{"<runtime>", 0, 0, 0});
        return pr->left;
    });

    // tail : list alpha -> list alpha
    register_builtin("tail", [this](ValRef v) -> ValRef {
        ValRef f = force(v);
        auto* vc = std::get_if<VCons>(&f->data);
        if (!vc || vc->name != "::") {
            throw RuntimeError("tail: empty list", SourceLocation{"<runtime>", 0, 0, 0});
        }
        ValRef arg = force(vc->arg);
        auto* pr = std::get_if<VPair>(&arg->data);
        if (!pr) throw RuntimeError("tail: malformed cons", SourceLocation{"<runtime>", 0, 0, 0});
        return pr->right;
    });

    // null : list alpha -> bool
    register_builtin("null", [this](ValRef v) -> ValRef {
        ValRef f = force(v);
        auto* vc = std::get_if<VCons>(&f->data);
        return make_bool(vc && vc->name == "nil");
    });

    // List concatenation: <> takes a pair (list, list) -> list
    // Eagerly collects the left list and prepends to the right.
    register_builtin("<>", [this, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        std::vector<ValRef> elems;
        ValRef cur = force(a);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") throw RuntimeError("<>: expected a list",
                                                      SourceLocation{"<runtime>", 0, 0, 0});
            ValRef carg = force(vc->arg);
            auto* pr = std::get_if<VPair>(&carg->data);
            if (!pr) break;
            elems.push_back(pr->left);
            cur = force(pr->right);
        }
        ValRef result = b;
        for (auto it = elems.rbegin(); it != elems.rend(); ++it) {
            result = make_cons(*it, result);
        }
        return result;
    });

    // --- Built-in constructors as values ---
    // nil is already a VCons; register it as a value
    global_env_ = env_extend(global_env_, "nil", make_nil());

    // :: as a function: takes head, returns function taking tail
    register_builtin("::", [](ValRef h) -> ValRef {
        return make_fun([h](ValRef t) -> ValRef {
            return make_cons(h, t);
        });
    });

    // true and false
    global_env_ = env_extend(global_env_, "true",  make_bool(true));
    global_env_ = env_extend(global_env_, "false", make_bool(false));
}

} // namespace hope
