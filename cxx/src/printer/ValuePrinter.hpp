#pragma once

// ValuePrinter — render Hope runtime values as human-readable strings.
//
// Uses the evaluator to force thunks on demand (lazy evaluation).
// An optional type_hint guides formatting (e.g. distinguishing
// list char from list num for string printing).

#include <string>

#include "runtime/Evaluator.hpp"
#include "runtime/Value.hpp"
#include "types/Type.hpp"

namespace hope {

// Print a runtime value in Hope syntax.
// Forces thunks as needed via the evaluator.
// type_hint may be null; if provided it is used to decide string vs list
// printing for char lists.
std::string print_value(ValRef v, Evaluator& ev, TyRef type_hint = nullptr);

} // namespace hope
