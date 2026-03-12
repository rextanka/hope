#pragma once

// TypePrinter — render Hope types as human-readable strings.
//
// Two entry points:
//   print_type(TyRef)                   — inference-time TyNode graph → Hope type syntax
//   print_type(TyRef, OperatorTable&)   — same, but prints infix type constructors infix
//   print_ast_type(Type&)               — AST Type node → Hope type syntax

#include <string>
#include <unordered_set>

#include "ast/Ast.hpp"
#include "parser/OperatorTable.hpp"
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

// As above, but also renders infix type constructors (those present in ops)
// in infix form — e.g. `num OR alpha` instead of `OR num alpha`.
std::string print_type(TyRef t, const OperatorTable& ops);

// Print an AST Type node as Hope type syntax.
// Used for error messages and declared-type display.
std::string print_ast_type(const Type& t);

} // namespace hope
