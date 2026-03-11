// TypePrinter.cpp — render Hope types as strings.

#include "printer/TypePrinter.hpp"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace hope {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

// Names assigned to unbound type variables, in order of first encounter.
// The first 8 are the Greek letters used in Hope; then t0, t1, ...
static const char* kTyVarNames[] = {
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta"
};
static constexpr int kNamedCount = 8;

struct PrintState {
    // Maps TyVar id → assigned name string.
    std::unordered_map<int, std::string> var_names;
    int next_idx = 0;

    // Set of TyNode* currently being printed (cycle detection).
    std::set<TyNode*> visiting;

    const std::string& name_for(int id) {
        auto it = var_names.find(id);
        if (it != var_names.end()) return it->second;
        // Assign a new name.
        std::string name;
        if (next_idx < kNamedCount) {
            name = kTyVarNames[next_idx];
        } else {
            name = "t" + std::to_string(next_idx - kNamedCount);
        }
        ++next_idx;
        var_names[id] = std::move(name);
        return var_names[id];
    }
};

// Forward declarations.
std::string print_ty(TyRef t, PrintState& st);
std::string atom_str(TyRef t, PrintState& st);

// Fully dereference a TyVar chain.
TyRef deref(TyRef t) {
    while (t) {
        if (auto* tv = std::get_if<TyVar>(&t->data)) {
            if (tv->binding.has_value()) {
                t = *tv->binding;
                continue;
            }
        }
        break;
    }
    return t;
}

// Return true if the printed type would need parens when used as a type arg.
// A type needs parens as an arg if it's not simply an identifier (nullary cons
// or tvar) — i.e., if it contains spaces or operator characters.
bool needs_parens_as_arg(TyRef t, PrintState& st) {
    t = deref(t);
    if (!t) return false;

    if (auto* tv = std::get_if<TyVar>(&t->data)) {
        (void)tv; // unbound var → simple name, no parens
        return false;
    }
    if (auto* tf = std::get_if<TyFrozen>(&t->data)) {
        (void)tf;
        return false;
    }
    if (auto* tc = std::get_if<TyCons>(&t->data)) {
        if (tc->args.empty()) return false; // nullary constructor
        // Any applied constructor needs parens as an arg.
        return true;
    }
    return false;
}

std::string atom_str(TyRef t, PrintState& st) {
    if (needs_parens_as_arg(t, st)) {
        return "(" + print_ty(t, st) + ")";
    }
    return print_ty(t, st);
}

std::string print_ty(TyRef t, PrintState& st) {
    t = deref(t);

    if (!t) return "_";

    // Cycle check.
    if (st.visiting.count(t.get())) return "...";

    if (auto* tv = std::get_if<TyVar>(&t->data)) {
        // Unbound variable.
        return st.name_for(tv->id);
    }

    if (auto* tf = std::get_if<TyFrozen>(&t->data)) {
        // Skolem variable — print as t<id> with special prefix.
        return "frozen" + std::to_string(tf->id);
    }

    if (auto* tc = std::get_if<TyCons>(&t->data)) {
        st.visiting.insert(t.get());

        std::string result;

        const std::string& name = tc->name;
        const std::vector<TyRef>& args = tc->args;

        // Arrow type: dom -> cod  (right-associative, prec 2)
        if (name == "->" && args.size() == 2) {
            TyRef dom = deref(args[0]);
            TyRef cod = deref(args[1]);

            std::string dom_str = print_ty(dom, st);
            // If dom is itself an arrow type, wrap in parens.
            bool dom_needs_parens = false;
            if (dom) {
                if (auto* dc = std::get_if<TyCons>(&dom->data)) {
                    if (dc->name == "->" && dc->args.size() == 2)
                        dom_needs_parens = true;
                }
            }
            if (dom_needs_parens) dom_str = "(" + dom_str + ")";

            std::string cod_str = print_ty(cod, st);
            result = dom_str + " -> " + cod_str;

        // Product type: left # right  (right-associative, prec 4)
        } else if (name == "#" && args.size() == 2) {
            TyRef left = deref(args[0]);
            TyRef right = deref(args[1]);

            std::string left_str = print_ty(left, st);
            // Arrow types have lower prec than #, so they need parens on left.
            bool left_needs_parens = false;
            if (left) {
                if (auto* lc = std::get_if<TyCons>(&left->data)) {
                    if (lc->name == "->") left_needs_parens = true;
                }
            }
            if (left_needs_parens) left_str = "(" + left_str + ")";

            std::string right_str = print_ty(right, st);
            result = left_str + " # " + right_str;

        // List type: list elem
        } else if (name == "list" && args.size() == 1) {
            result = "list " + atom_str(args[0], st);

        // Truval is a synonym for bool; always display as bool.
        } else if (name == "truval" && args.empty()) {
            result = "bool";

        // Nullary constructor: just the name.
        } else if (args.empty()) {
            result = name;

        // Multi-arg constructor: name arg1 arg2 ...
        } else {
            result = name;
            for (const TyRef& arg : args) {
                result += " " + atom_str(arg, st);
            }
        }

        st.visiting.erase(t.get());
        return result;
    }

    return "_";
}

// ---------------------------------------------------------------------------
// AST type printer
// ---------------------------------------------------------------------------

std::string print_ast_ty(const Type& t);
std::string ast_atom_str(const Type& t);

bool ast_needs_parens(const Type& t) {
    // Atoms: TVar (nullary tvar), TCons with no args.
    if (std::holds_alternative<TVar>(t.data)) return false;
    if (auto* tc = std::get_if<TCons>(&t.data)) {
        return !tc->args.empty();
    }
    return true;
}

std::string ast_atom_str(const Type& t) {
    if (ast_needs_parens(t)) return "(" + print_ast_ty(t) + ")";
    return print_ast_ty(t);
}

std::string print_ast_ty(const Type& t) {
    return std::visit([&](const auto& d) -> std::string {
        using T = std::decay_t<decltype(d)>;

        if constexpr (std::is_same_v<T, TVar>) {
            return d.name;
        }

        if constexpr (std::is_same_v<T, TCons>) {
            if (d.name == "truval" && d.args.empty()) return "bool";
            if (d.args.empty()) return d.name;
            std::string r = d.name;
            for (const auto& arg : d.args)
                r += " " + ast_atom_str(*arg);
            return r;
        }

        if constexpr (std::is_same_v<T, TFun>) {
            std::string dom = print_ast_ty(*d.dom);
            // If dom is itself a function type, wrap it.
            if (std::holds_alternative<TFun>(d.dom->data))
                dom = "(" + dom + ")";
            return dom + " -> " + print_ast_ty(*d.cod);
        }

        if constexpr (std::is_same_v<T, TProd>) {
            std::string left = print_ast_ty(*d.left);
            // Arrow on the left of # needs parens.
            if (std::holds_alternative<TFun>(d.left->data))
                left = "(" + left + ")";
            return left + " # " + print_ast_ty(*d.right);
        }

        if constexpr (std::is_same_v<T, TMu>) {
            return "mu " + d.var + " => " + print_ast_ty(*d.body);
        }

        return "_";
    }, t.data);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string print_type(TyRef t) {
    PrintState st;
    return print_ty(t, st);
}

std::string print_ast_type(const Type& t) {
    return print_ast_ty(t);
}

} // namespace hope
