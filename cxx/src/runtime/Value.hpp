#pragma once

// Runtime value representation for call-by-need (lazy) evaluation.
//
// Values are heap-allocated and reference-counted via shared_ptr.
// Thunks (VThunk) enable lazy evaluation: the expression is only evaluated
// when the value is forced.  VHole detects infinite loops (black holes).

#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "ast/Ast.hpp"

namespace hope {

struct Value;
using ValRef = std::shared_ptr<Value>;

// ---------------------------------------------------------------------------
// Environment: immutable linked-list of named bindings, vector-based.
// ---------------------------------------------------------------------------

struct EnvFrame {
    std::string name;
    ValRef      val;
};
using Env = std::shared_ptr<std::vector<EnvFrame>>;

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
};

struct VThunk {                               // unevaluated lazy closure
    const Expr* expr;                         // raw pointer — AST outlives runtime
    Env         env;
};

struct VHole  {};                             // black hole: detected infinite loop

using ValueData = std::variant<VNum, VChar, VCons, VPair, VFun, VThunk, VHole>;

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

inline ValRef make_fun(std::function<ValRef(ValRef)> f) {
    return std::make_shared<Value>(VFun{std::move(f)});
}

inline ValRef make_thunk(const Expr* e, Env env) {
    return std::make_shared<Value>(VThunk{e, std::move(env)});
}

// ---------------------------------------------------------------------------
// Environment helpers
// ---------------------------------------------------------------------------

inline Env make_env(std::vector<EnvFrame> frames = {}) {
    return std::make_shared<std::vector<EnvFrame>>(std::move(frames));
}

inline Env env_extend(Env base, std::string name, ValRef val) {
    auto frames = *base; // copy the vector
    frames.push_back({std::move(name), std::move(val)});
    return std::make_shared<std::vector<EnvFrame>>(std::move(frames));
}

// Search from back (most recent binding first).
inline ValRef env_lookup(const Env& env, const std::string& name) {
    const auto& frames = *env;
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        if (it->name == name) return it->val;
    }
    return nullptr;
}

} // namespace hope
