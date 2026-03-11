#pragma once

#include <stdexcept>
#include <string>

#include "lexer/SourceLocation.hpp"

namespace hope {

// Thrown by the evaluator when a runtime error is detected.
// The REPL catches RuntimeError and displays it without terminating the session.
class RuntimeError : public std::runtime_error {
public:
    SourceLocation loc;

    RuntimeError(std::string message, SourceLocation loc)
        : std::runtime_error(std::move(message)), loc(std::move(loc)) {}
};

} // namespace hope
