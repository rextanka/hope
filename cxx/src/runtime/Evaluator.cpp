// Evaluator.cpp — call-by-need tree-walking interpreter for Hope.
//
// See Evaluator.hpp for the public interface.

#include "runtime/Evaluator.hpp"
#include "runtime/RuntimeError.hpp"

#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "printer/ExprPrinter.hpp"
#include "printer/ValuePrinter.hpp"

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

void Evaluator::register_value(const std::string& name, ValRef val) {
    global_env_ = env_extend(global_env_, name, std::move(val));
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
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'f':  return '\f';
        case 'v':  return '\v';
        case 'x': {
            // Hex escape: \xNN
            int val = 0;
            for (size_t i = 2; i < inner.size(); ++i)
                val = val * 16 + (std::isdigit(static_cast<unsigned char>(inner[i]))
                                  ? inner[i] - '0'
                                  : std::tolower(static_cast<unsigned char>(inner[i])) - 'a' + 10);
            return static_cast<char>(val);
        }
        default:
            if (inner[1] >= '0' && inner[1] <= '7') {
                // Octal escape: \ddd
                int val = 0;
                for (size_t i = 1; i < inner.size() && inner[i] >= '0' && inner[i] <= '7'; ++i)
                    val = val * 8 + (inner[i] - '0');
                return static_cast<char>(val);
            }
            return inner[1];
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
                case 'a':  chars.push_back('\a'); break;
                case 'b':  chars.push_back('\b'); break;
                case 'f':  chars.push_back('\f'); break;
                case 'v':  chars.push_back('\v'); break;
                case 'x': {
                    // Hex escape: \xNN
                    int val = 0;
                    int count = 0;
                    while (i + 1 < inner.size() && count < 2 &&
                           std::isxdigit(static_cast<unsigned char>(inner[i + 1]))) {
                        ++i; ++count;
                        val = val * 16 + (std::isdigit(static_cast<unsigned char>(inner[i]))
                                          ? inner[i] - '0'
                                          : std::tolower(static_cast<unsigned char>(inner[i])) - 'a' + 10);
                    }
                    chars.push_back(static_cast<char>(val));
                    break;
                }
                default:
                    if (inner[i] >= '0' && inner[i] <= '7') {
                        // Octal escape: \ddd (up to 3 digits)
                        int val = inner[i] - '0';
                        int count = 1;
                        while (i + 1 < inner.size() && count < 3 &&
                               inner[i + 1] >= '0' && inner[i + 1] <= '7') {
                            ++i; ++count;
                            val = val * 8 + (inner[i] - '0');
                        }
                        chars.push_back(static_cast<char>(val));
                    } else {
                        chars.push_back(inner[i]);
                    }
                    break;
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
// build_repr_subst — build an ExprSubst from an environment for printing
// lambdas and sections.  We substitute free variables with their value
// representations so that closures print as e.g. "lambda y => 3 + y".
// ---------------------------------------------------------------------------

ExprSubst Evaluator::build_repr_subst(const Env& env) const {
    ExprSubst subst;
    if (!env) return subst;
    for (const EnvNode* node = env.get(); node; node = node->next.get()) {
        const std::string& fname = node->name;
        const ValRef&       fval  = node->val;
        if (subst.count(fname)) continue; // most recent binding already added
        if (!fval) continue;
        // We look through the value non-recursively (no forcing — we cannot
        // force here since we may be inside eval).  Only constant-like values
        // get a repr; functions get their repr string if available.
        const Value* raw = fval.get();
        if (!raw) continue;
        std::visit([&](const auto& vd) {
            using VT = std::decay_t<decltype(vd)>;
            if constexpr (std::is_same_v<VT, VNum>) {
                // Integer or float numeral.
                double n = vd.n;
                if (std::isfinite(n) && n == std::floor(n) &&
                    n >= -9007199254740992.0 && n <= 9007199254740992.0) {
                    subst[fname] = std::to_string(static_cast<long long>(n));
                } else {
                    std::ostringstream ss; ss << n;
                    subst[fname] = ss.str();
                }
            } else if constexpr (std::is_same_v<VT, VChar>) {
                std::string s = "'";
                char c = vd.c;
                if (c == '\'') s += "\\'";
                else if (c == '\\') s += "\\\\";
                else s += c;
                s += "'";
                subst[fname] = s;
            } else if constexpr (std::is_same_v<VT, VFun>) {
                if (vd.repr.has_value()) {
                    subst[fname] = *vd.repr;
                }
                // If no repr, leave variable unsubstituted (prints as its name).
            } else if constexpr (std::is_same_v<VT, VThunk>) {
                // Peek at the thunk's expression to infer a repr without forcing.
                if (vd.expr) {
                    const Expr& te = *vd.expr;
                    if (const auto* opref = std::get_if<EOpRef>(&te.data)) {
                        // e.g. (+) — its repr is "(+)"
                        subst[fname] = "(" + opref->op + ")";
                    } else if (const auto* evar = std::get_if<EVar>(&te.data)) {
                        // A variable thunk: use whatever we already have for it,
                        // or the variable name itself (for global functions etc.).
                        auto it2 = subst.find(evar->name);
                        if (it2 != subst.end())
                            subst[fname] = it2->second;
                        else
                            subst[fname] = evar->name; // e.g. "square"
                    } else if (std::holds_alternative<ELambda>(te.data) ||
                               std::holds_alternative<ESection>(te.data) ||
                               std::holds_alternative<ETuple>(te.data)   ||
                               std::holds_alternative<EList>(te.data)    ||
                               std::holds_alternative<ELit>(te.data)     ||
                               std::holds_alternative<EInfix>(te.data)   ||
                               std::holds_alternative<EApply>(te.data)) {
                        // Expression whose repr can be computed directly from
                        // the AST with the thunk's captured env.
                        ExprSubst inner_subst = build_repr_subst(vd.env);
                        subst[fname] = print_expr(te, inner_subst);
                    } else {
                        // For literals and other simple exprs: try to force safely.
                        try {
                            ValRef forced = const_cast<Evaluator*>(this)->force(fval);
                            std::visit([&](const auto& fvd) {
                                using FT = std::decay_t<decltype(fvd)>;
                                if constexpr (std::is_same_v<FT, VNum>) {
                                    double n = fvd.n;
                                    if (std::isfinite(n) && n == std::floor(n))
                                        subst[fname] = std::to_string(static_cast<long long>(n));
                                    else { std::ostringstream ss; ss << n; subst[fname] = ss.str(); }
                                } else if constexpr (std::is_same_v<FT, VChar>) {
                                    std::string s = "'";
                                    char c = fvd.c;
                                    if (c == '\'') s += "\\'";
                                    else if (c == '\\') s += "\\\\";
                                    else s += c;
                                    s += "'";
                                    subst[fname] = s;
                                } else if constexpr (std::is_same_v<FT, VFun>) {
                                    if (fvd.repr.has_value())
                                        subst[fname] = *fvd.repr;
                                }
                            }, forced->data);
                        } catch (...) {}
                    }
                }
            }
            // VCons, VPair, VHole — leave unsubstituted.
        }, raw->data);
    }
    return subst;
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
            // Look up the underlying function value, then wrap it in a new
            // VFun that carries a "(op)" repr for printing.
            ValRef base;
            if (auto v = env_lookup(env, d.op)) base = v;
            else if (auto v = env_lookup(global_env_, d.op)) base = v;
            else if (functions_.count(d.op)) base = make_function_val(d.op);
            else throw RuntimeError("unbound operator: " + d.op, e.loc);

            // Wrap in a VFun with the printed form "(-)" etc.
            std::string repr = "(" + d.op + ")";
            ValRef wrapped = base; // default: reuse
            if (auto* vf = std::get_if<VFun>(&base->data)) {
                (void)vf;
                // Re-wrap with repr.
                auto apply_fn = vf->apply;
                wrapped = make_fun(std::move(apply_fn), repr);
            }
            return wrapped;
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
                // Unary constructor used as infix (e.g. `data t == a CON b`).
                auto it_ctor = constructor_arity_.find(d.op);
                if (it_ctor != constructor_arity_.end() && it_ctor->second == 1) {
                    std::string cname = d.op;
                    return make_fun([cname](ValRef arg) -> ValRef {
                        return make_con1(cname, arg);
                    });
                }
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

            // Compute a printable representation by substituting known values
            // for free variables in the AST.
            ExprSubst subst = build_repr_subst(env);
            std::string repr = print_expr(e, subst);

            return make_fun([this, func_clauses = std::move(func_clauses), captured](ValRef arg) -> ValRef {
                return apply_clauses("<lambda>", func_clauses, arg, captured);
            }, std::move(repr));
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

            // Compute repr in lambda form: "lambda x' => e op x'" or
            // "lambda x' => x' op e".  Use env substitution for the expr part.
            ExprSubst subst = build_repr_subst(env);
            std::string e_str = print_expr(*d.expr, subst);
            std::string repr;
            if (d.is_left) {
                repr = "lambda x' => " + e_str + " " + d.op + " x'";
            } else {
                repr = "lambda x' => x' " + d.op + " " + e_str;
            }

            if (d.is_left) {
                // (e op): lambda y => op (e, y)
                return make_fun([this, op_val, e_val](ValRef y) -> ValRef {
                    ValRef pair = make_pair(e_val, y);
                    return apply(force(op_val), pair);
                }, std::move(repr));
            } else {
                // (op e): lambda x => op (x, e)
                return make_fun([this, op_val, e_val](ValRef x) -> ValRef {
                    ValRef pair = make_pair(x, e_val);
                    return apply(force(op_val), pair);
                }, std::move(repr));
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

        // EAnnotate — type annotation is erased at runtime; just eval the expr.
        if constexpr (std::is_same_v<T, EAnnotate>) {
            return eval(*d.expr, env);
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

// Multi-clause curried pattern dispatch with backtracking.
//
// When a function has N-argument clauses, matching argument k may succeed
// for some clauses but fail for others at argument k+1.  This struct
// tracks all STILL-LIVE clauses (those whose first k patterns matched) so
// that when argument k+1 fails for one clause, the next live clause is
// tried automatically — reproducing Hope's left-to-right, first-match
// semantics without committing early.
struct MultiCurried {
    Evaluator* ev;

    struct LiveClause {
        const Evaluator::FuncClause* clause;  // AST outlives runtime
        size_t                       next_pat;
        Env                          env;
    };

    std::vector<LiveClause> live;
    std::string             fname;

    ValRef operator()(ValRef arg) {
        std::vector<LiveClause> next_live;

        for (auto& lc : live) {
            if (lc.next_pat >= lc.clause->pats.size()) continue;

            Env cur_env = lc.env;
            if (!ev->match_pat(*lc.clause->pats[lc.next_pat], arg, cur_env))
                continue;   // this clause fails — try the next

            size_t n = lc.next_pat + 1;
            if (n == lc.clause->pats.size()) {
                // All patterns matched — execute body (first-match wins)
                return ev->eval_in(lc.clause->body, cur_env);
            }
            next_live.push_back({lc.clause, n, cur_env});
        }

        if (next_live.empty())
            throw RuntimeError("pattern match failure in function: " + fname,
                               SourceLocation{"<runtime>", 0, 0, 0});

        return make_fun(MultiCurried{ev, std::move(next_live), fname});
    }
};

} // anonymous namespace

ValRef Evaluator::apply_clauses(const std::string& name,
                                 const std::vector<FuncClause>& clauses,
                                 ValRef arg, Env closed_env) {
    // Collect all clauses whose first pattern matches `arg`.
    // Single-pattern clauses that match are executed immediately (first-match).
    // Multi-pattern clauses that match are deferred into a MultiCurried.
    std::vector<MultiCurried::LiveClause> live;

    for (const auto& clause : clauses) {
        if (clause.pats.empty()) {
            // Zero-argument clause — execute immediately
            return eval(*clause.body, closed_env);
        }

        // Infix-pair clauses: pats[0] and pats[1] match the left and right
        // sides of the single VPair argument.
        if (clause.is_infix_pair && clause.pats.size() >= 2) {
            ValRef forced = force(arg);
            const VPair* pr = std::get_if<VPair>(&forced->data);
            if (!pr) {
                if (auto* vc = std::get_if<VCons>(&forced->data)) {
                    if (vc->has_arg) {
                        ValRef ca = force(vc->arg);
                        pr = std::get_if<VPair>(&ca->data);
                    }
                }
            }
            if (!pr) continue;

            Env match_env = closed_env;
            if (!match(*clause.pats[0], pr->left,  match_env)) continue;
            if (!match(*clause.pats[1], pr->right, match_env)) continue;
            if (clause.pats.size() == 2) {
                return eval(*clause.body, match_env);
            }
            // Extra curried args after the infix pair
            live.push_back({&clause, 2, match_env});
            continue;
        }

        // Regular clause: try to match the first pattern.
        Env match_env = closed_env;
        if (!match(*clause.pats[0], arg, match_env)) continue;

        if (clause.pats.size() == 1) {
            // Fully matched — execute body immediately (first-match wins)
            return eval(*clause.body, match_env);
        }

        // More patterns needed — add to live set for backtracking dispatch
        live.push_back({&clause, 1, match_env});
    }

    if (live.empty())
        throw RuntimeError("non-exhaustive patterns in function: " + name,
                           SourceLocation{"<runtime>", 0, 0, 0});

    return make_fun(MultiCurried{this, std::move(live), name});
}

// ---------------------------------------------------------------------------
// match — pattern matching
// ---------------------------------------------------------------------------

bool Evaluator::match(const Pattern& p, ValRef v, Env& env) {
    return std::visit([&](const auto& pd) -> bool {
        using T = std::decay_t<decltype(pd)>;

        // PVar — bind the variable, unless the name is a known constructor,
        // in which case treat as a nullary PCons match.
        if constexpr (std::is_same_v<T, PVar>) {
            // Check if the name is a registered 0-arity constructor.
            auto it = constructor_arity_.find(pd.name);
            if (it != constructor_arity_.end() && it->second == 0) {
                // Treat as nullary PCons: force and check name.
                ValRef forced = force(v);
                auto* vc = std::get_if<VCons>(&forced->data);
                return vc && vc->name == pd.name && !vc->has_arg;
            }
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
            // We force to get the outer VPair, but we do NOT force the right
            // sub-expression until needed.  This preserves laziness: a pattern
            // like (x, y) matching against a lazy pair (x, thunk) should NOT
            // force the thunk — it should simply bind y to the thunk.
            ValRef cur = force(v);
            for (size_t i = 0; i < pats.size(); ++i) {
                if (i + 1 == pats.size()) {
                    // Last element: match directly (may be a thunk).
                    // Do NOT force cur here — let match() handle laziness.
                    return match(*pats[i], cur, env);
                }
                // Not last: need to destructure as a VPair.
                // Force cur to expose the VPair structure.
                cur = force(cur);
                auto* pr = std::get_if<VPair>(&cur->data);
                if (!pr) return false;
                if (!match(*pats[i], pr->left, env)) return false;
                // Advance to the right sub-pair WITHOUT forcing it.
                // This preserves laziness of the remaining elements.
                cur = pr->right;
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

    // User-defined infix constructor: the constructor has an arg which is a pair.
    // Also handle VPair directly for product-functor operators like # and ->
    // that are declared as abstype (not data constructors), where the value is
    // represented as a plain pair rather than a tagged VCons.
    if (auto* pr = std::get_if<VPair>(&forced->data)) {
        // Treat VPair as matching any infix pair pattern (a op b).
        if (!match(*pi.left, pr->left, env)) return false;
        return match(*pi.right, pr->right, env);
    }

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
    clause.body = d.rhs.get();

    // An infix equation (is_infix=true, args.size()>=2) is always called with
    // a single VPair{left, right} as its first argument.  The first two arg
    // patterns match the left and right sides of that pair.  Any additional
    // patterns (e.g., --- (f o g) x <= f(g x)) are curried extra arguments.
    //
    // Examples:
    //   --- [] <> ys <= ys           is_infix, args=[PCons{nil}, PVar{ys}]
    //   --- (x::xs) <> ys <= ...     is_infix, args=[PInfix{x,"::",xs}, PVar{ys}]
    //   --- (x::xs)@0 <= x           is_infix, args=[PInfix{x,"::",xs}, PLit{0}]
    //   --- (f o g) x <= f(g x)      is_infix, args=[PVar{f}, PVar{g}, PVar{x}]
    if (d.lhs.is_infix && d.lhs.args.size() >= 2) {
        for (const auto& arg_pat : d.lhs.args)
            clause.pats.push_back(arg_pat.get());
        clause.is_infix_pair = true;
    } else {
        for (const auto& arg_pat : d.lhs.args)
            clause.pats.push_back(arg_pat.get());
    }

    auto& func_def = functions_[fname];
    if (func_def.name.empty())
        func_def.name = fname;
    func_def.clauses.push_back(std::move(clause));

    // Update the global env with the appropriate value.
    // If all clauses have zero argument patterns, this is a constant definition
    // (e.g. --- primes <= sieve ...).  Register a thunk so it is evaluated
    // lazily but not treated as a function that needs an argument.
    // If any clause has patterns, register a VFun dispatcher.
    bool all_zero_arg = true;
    for (const auto& cl : func_def.clauses) {
        if (!cl.pats.empty()) { all_zero_arg = false; break; }
    }

    ValRef fval;
    if (all_zero_arg && func_def.clauses.size() == 1) {
        // Single 0-arg clause: register as a thunk over the body.
        fval = make_thunk(func_def.clauses[0].body, global_env_);
    } else {
        fval = make_function_val(fname);
    }
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

    // --- compare : alpha # alpha -> relation ---
    // Returns LESS, EQUAL, or GREATER.  Used by Standard.hop's comparison
    // equations (=, /=, <, >, =<, >=).
    constructor_arity_["LESS"]    = 0;
    constructor_arity_["EQUAL"]   = 0;
    constructor_arity_["GREATER"] = 0;

    register_builtin("compare", [this, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        ValRef fa = force(a), fb = force(b);

        // Numeric comparison
        if (auto* an = std::get_if<VNum>(&fa->data)) {
            if (auto* bn = std::get_if<VNum>(&fb->data)) {
                if (an->n < bn->n) return make_con0("LESS");
                if (an->n > bn->n) return make_con0("GREATER");
                return make_con0("EQUAL");
            }
        }
        // Character comparison
        if (auto* ac = std::get_if<VChar>(&fa->data)) {
            if (auto* bc = std::get_if<VChar>(&fb->data)) {
                if (ac->c < bc->c) return make_con0("LESS");
                if (ac->c > bc->c) return make_con0("GREATER");
                return make_con0("EQUAL");
            }
        }
        // Constructor comparison (by name — for relation / bool / user-defined nullary)
        if (auto* ac = std::get_if<VCons>(&fa->data)) {
            if (auto* bc = std::get_if<VCons>(&fb->data)) {
                int cmp = ac->name.compare(bc->name);
                if (cmp < 0) return make_con0("LESS");
                if (cmp > 0) return make_con0("GREATER");
                return make_con0("EQUAL");
            }
        }
        throw RuntimeError("compare: cannot compare these values",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

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

    register_builtin("sqrt",  [get_num](ValRef v) { return make_num(std::sqrt(get_num(v))); });
    register_builtin("exp",   [get_num](ValRef v) { return make_num(std::exp(get_num(v))); });
    register_builtin("log",   [get_num](ValRef v) { return make_num(std::log(get_num(v))); });
    register_builtin("log10", [get_num](ValRef v) { return make_num(std::log10(get_num(v))); });

    register_builtin("sin",   [get_num](ValRef v) { return make_num(std::sin(get_num(v))); });
    register_builtin("cos",   [get_num](ValRef v) { return make_num(std::cos(get_num(v))); });
    register_builtin("tan",   [get_num](ValRef v) { return make_num(std::tan(get_num(v))); });
    register_builtin("asin",  [get_num](ValRef v) { return make_num(std::asin(get_num(v))); });
    register_builtin("acos",  [get_num](ValRef v) { return make_num(std::acos(get_num(v))); });
    register_builtin("atan",  [get_num](ValRef v) { return make_num(std::atan(get_num(v))); });

    register_builtin("sinh",  [get_num](ValRef v) { return make_num(std::sinh(get_num(v))); });
    register_builtin("cosh",  [get_num](ValRef v) { return make_num(std::cosh(get_num(v))); });
    register_builtin("tanh",  [get_num](ValRef v) { return make_num(std::tanh(get_num(v))); });
    register_builtin("asinh", [get_num](ValRef v) { return make_num(std::asinh(get_num(v))); });
    register_builtin("acosh", [get_num](ValRef v) { return make_num(std::acosh(get_num(v))); });
    register_builtin("atanh", [get_num](ValRef v) { return make_num(std::atanh(get_num(v))); });

    register_builtin("erf",   [get_num](ValRef v) { return make_num(std::erf(get_num(v))); });
    register_builtin("erfc",  [get_num](ValRef v) { return make_num(std::erfc(get_num(v))); });

    // Binary math: atan2, hypot, pow take a pair
    register_builtin("atan2", [get_num, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        return make_num(std::atan2(get_num(a), get_num(b)));
    });
    register_builtin("hypot", [get_num, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        return make_num(std::hypot(get_num(a), get_num(b)));
    });
    register_builtin("pow",   [get_num, get_pair](ValRef v) -> ValRef {
        auto [a, b] = get_pair(v);
        return make_num(std::pow(get_num(a), get_num(b)));
    });

    register_builtin("floor",   [get_num](ValRef v) { return make_num(std::floor(get_num(v))); });
    register_builtin("ceil",    [get_num](ValRef v) { return make_num(std::ceil(get_num(v))); });
    register_builtin("ceiling", [get_num](ValRef v) { return make_num(std::ceil(get_num(v))); });

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
    register_builtin("num2str", [get_num](ValRef v) -> ValRef {
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

    // --- File I/O ---

    // read : list char -> list char
    // Reads the named file and returns its contents as a Hope list of chars.
    register_builtin("read", [this](ValRef v) -> ValRef {
        // Convert Hope string (list char) to C++ string filename
        std::string filename;
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
            filename += ch->c;
            cur = force(pr->right);
        }
        std::ifstream f(filename, std::ios::binary);
        if (!f.is_open()) {
            throw RuntimeError("read: cannot open file: " + filename,
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        std::string contents((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
        // Build a lazy Hope string (list of chars) from back to front
        ValRef result = make_nil();
        for (auto it = contents.rbegin(); it != contents.rend(); ++it) {
            result = make_cons(make_char(*it), result);
        }
        return result;
    });

    // --- input / argv: default empty lists ---
    // These are overridden by Session::set_input_stream / Session::set_argv
    // before any user program runs.
    global_env_ = env_extend(global_env_, "input", make_nil());
    global_env_ = env_extend(global_env_, "argv",  make_nil());

    // --- print / return / write_element / write_list stubs ---
    // print : alpha -> beta — force and pretty-print to stdout, return nil
    register_builtin("print", [this](ValRef v) -> ValRef {
        ValRef fv = force_full(v);
        std::cout << print_value(fv) << "\n";
        return make_nil();
    });

    // return : alpha — a polymorphic "bottom" / terminal value; we use nil
    global_env_ = env_extend(global_env_, "return", make_nil());

    // write_element : alpha -> list alpha -> beta
    // Prints the element then continues with write_list (via Hope definitions).
    // As a builtin stub: print element, ignore tail, return nil.
    register_builtin("write_element", [this](ValRef v) -> ValRef {
        ValRef fv = force(v);
        if (auto* pr = std::get_if<VPair>(&fv->data)) {
            ValRef elem = force_full(pr->left);
            std::cout << print_value(elem);
        }
        return make_nil();
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
