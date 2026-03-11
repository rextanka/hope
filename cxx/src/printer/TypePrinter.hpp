#pragma once

// TypePrinter — render Hope types as human-readable strings.
//
// Two entry points:
//   print_type(TyRef)   — inference-time TyNode graph → Hope type syntax
//   print_ast_type(Type&) — AST Type node → Hope type syntax

#include <string>

#include "ast/Ast.hpp"
#include "types/Type.hpp"

namespace hope {

// Print an inference-time TyRef as Hope type syntax.
// Unbound type variables are named alpha, beta, gamma, delta, epsilon, zeta,
// eta, theta, then t0, t1, t2, ...  The mapping is stable within a single
// call: the first unbound variable encountered gets "alpha", the second
// "beta", etc.
//
// A null TyRef is printed as "_" (unknown type).
// Cycles in the type graph are rendered as "...".
std::string print_type(TyRef t);

// Print an AST Type node as Hope type syntax.
// Used for error messages and declared-type display.
std::string print_ast_type(const Type& t);

} // namespace hope
