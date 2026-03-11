#pragma once

// Builtins — extra built-in functions that can be registered into an Evaluator.
//
// The core arithmetic and comparison operators are already registered by
// Evaluator::init_builtins().  This module registers any additional functions
// that are part of the Standard Hope library but are more naturally implemented
// in C++ than in Hope itself.

namespace hope {

class Evaluator;

class Builtins {
public:
    // Register all additional built-in functions into the given evaluator.
    static void init(Evaluator& ev);
};

} // namespace hope
