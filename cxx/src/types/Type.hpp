#pragma once

// Inference-time type representation used during Hindley-Milner type inference.
//
// This is NOT the AST type (see ast/Ast.hpp) — it is the mutable, shared
// representation used during unification.  The key idea is union-find:
// a TyVar node can be "instantiated" by setting its binding field, which
// effectively links it to another node.
//
// All nodes are heap-allocated and shared via shared_ptr (TyRef) so that
// two type references can share the same mutable cell.

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace hope {

struct TyNode;
using TyRef = std::shared_ptr<TyNode>;

// An unification variable.  If binding is set the variable has been
// instantiated; callers should follow the chain via TypeChecker::deref().
struct TyVar {
    int id;                       // unique id, used for printing and occurs check
    std::optional<TyRef> binding; // non-empty when instantiated
};

// A skolem (frozen) variable introduced when checking a declared type against
// an inferred type.  Frozen variables cannot be instantiated during unification,
// so they act as opaque constants that the inferred type must match exactly.
struct TyFrozen {
    int id; // unique id for printing / equality
};

// A type constructor applied to zero or more arguments.
// Common names: "->", "#", "list", "num", "char", "bool", "truval"
struct TyCons {
    std::string         name;
    std::vector<TyRef>  args;
};

using TyNodeData = std::variant<TyVar, TyFrozen, TyCons>;

struct TyNode {
    TyNodeData data;

    // Convenience constructors used by the factory helpers below.
    explicit TyNode(TyNodeData d) : data(std::move(d)) {}
};

// ---------------------------------------------------------------------------
// Factory helpers
// ---------------------------------------------------------------------------

inline TyRef make_tyvar(int id) {
    return std::make_shared<TyNode>(TyVar{id, std::nullopt});
}

inline TyRef make_frozen(int id) {
    return std::make_shared<TyNode>(TyFrozen{id});
}

inline TyRef make_tycons(std::string name, std::vector<TyRef> args) {
    return std::make_shared<TyNode>(TyCons{std::move(name), std::move(args)});
}

inline TyRef make_fun_type(TyRef from, TyRef to) {
    return make_tycons("->", {std::move(from), std::move(to)});
}

inline TyRef make_prod_type(TyRef l, TyRef r) {
    return make_tycons("#", {std::move(l), std::move(r)});
}

inline TyRef make_list_type(TyRef elem) {
    return make_tycons("list", {std::move(elem)});
}

inline TyRef make_num_type() {
    return make_tycons("num", {});
}

inline TyRef make_char_type() {
    return make_tycons("char", {});
}

inline TyRef make_bool_type() {
    return make_tycons("bool", {});
}

} // namespace hope
