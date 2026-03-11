#pragma once

// ExprPrinter — render Hope AST expressions as source text.
//
// Used primarily to print lambda/closure values for REPL output.
// A substitution map can be provided to replace free variable names
// with their already-computed string representations (e.g., for
// printing closures where some variables have been bound to values).

#include <string>
#include <unordered_map>

#include "ast/Ast.hpp"

namespace hope {

// Substitution map: variable name → string to print in its place.
using ExprSubst = std::unordered_map<std::string, std::string>;

// Print an expression as Hope source text.
// Free variables found in `subst` are replaced with their mapped strings.
std::string print_expr(const Expr& e, const ExprSubst& subst = {});

// Print a pattern as Hope source text.
std::string print_pattern(const Pattern& p);

} // namespace hope
