#pragma once

// Runtime value representation for call-by-need (lazy) evaluation.
//
// Values are heap-allocated and reference-counted via shared_ptr.
// Thunks (VThunk) enable lazy evaluation: the expression is only evaluated
// when the value is forced.  VHole detects infinite loops (black holes).

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "ast/Ast.hpp"

namespace hope {

struct Value;
using ValRef = std::shared_ptr<Value>;

// ---------------------------------------------------------------------------
// Environment: persistent singly-linked list of named bindings.
//
// env_extend is O(1): it prepends a new node to the front of the list.
// env_lookup is O(n): it walks from the most-recent binding to the oldest.
// Closures share their parent environment without copying.
// ---------------------------------------------------------------------------

struct EnvNode {
    std::string              name;
    ValRef                   val;
    std::shared_ptr<EnvNode> next;
};

// Env is a (nullable) pointer to the head of the binding chain.
using Env = std::shared_ptr<EnvNode>;

// ---------------------------------------------------------------------------
// Value alternatives
// ---------------------------------------------------------------------------

struct VNum   { double n; };                  // numeric value (int or float)

struct VChar  { char c; };                    // character

struct VCons  {                               // constructor applied to 0 or 1 arguments
    std::string name;
    bool        has_arg;
    ValRef      arg;                          // valid iff has_arg == true
};

struct VPair  { ValRef left; ValRef right; }; // product pair (a # b)

struct VFun   {                               // function value (closure or built-in)
    std::function<ValRef(ValRef)> apply;
    std::optional<std::string> repr;          // printable representation (if known)
};

struct VThunk {                               // unevaluated lazy closure
    const Expr* expr;                         // raw pointer — AST outlives runtime
    Env         env;
};

struct VHole  {};                             // black hole: detected infinite loop

struct VProj  {                               // lazy pair projection (irrefutable binding)
    ValRef source;                            // the pair (a thunk until forced)
    bool   take_left;                         // true → .left, false → .right
};

using ValueData = std::variant<VNum, VChar, VCons, VPair, VFun, VThunk, VHole, VProj>;

struct Value {
    ValueData data;
    explicit Value(ValueData d) : data(std::move(d)) {}
};

// ---------------------------------------------------------------------------
// Factory helpers
// ---------------------------------------------------------------------------

inline ValRef make_num(double n) {
    return std::make_shared<Value>(VNum{n});
}

inline ValRef make_char(char c) {
    return std::make_shared<Value>(VChar{c});
}

inline ValRef make_bool(bool b) {
    return std::make_shared<Value>(VCons{b ? "true" : "false", false, {}});
}

inline ValRef make_nil() {
    return std::make_shared<Value>(VCons{"nil", false, {}});
}

inline ValRef make_cons(ValRef h, ValRef t) {
    auto pair = std::make_shared<Value>(VPair{h, t});
    return std::make_shared<Value>(VCons{"::", true, pair});
}

inline ValRef make_pair(ValRef l, ValRef r) {
    return std::make_shared<Value>(VPair{l, r});
}

inline ValRef make_con0(std::string name) {
    return std::make_shared<Value>(VCons{std::move(name), false, {}});
}

inline ValRef make_con1(std::string name, ValRef arg) {
    return std::make_shared<Value>(VCons{std::move(name), true, std::move(arg)});
}

inline ValRef make_fun(std::function<ValRef(ValRef)> f,
                       std::optional<std::string> repr = std::nullopt) {
    return std::make_shared<Value>(VFun{std::move(f), std::move(repr)});
}

inline ValRef make_thunk(const Expr* e, Env env) {
    return std::make_shared<Value>(VThunk{e, std::move(env)});
}

// ---------------------------------------------------------------------------
// Environment helpers
// ---------------------------------------------------------------------------

// Create an empty environment.
inline Env make_env() {
    return nullptr;
}

// Return a new environment with name->val prepended to base.  O(1).
inline Env env_extend(Env base, std::string name, ValRef val) {
    auto node = std::make_shared<EnvNode>();
    node->name = std::move(name);
    node->val  = std::move(val);
    node->next = std::move(base);
    return node;
}

// Search from the most-recently-bound name.  Returns nullptr if not found.
inline ValRef env_lookup(const Env& env, const std::string& name) {
    for (const EnvNode* n = env.get(); n; n = n->next.get()) {
        if (n->name == name) return n->val;
    }
    return nullptr;
}

} // namespace hope
