#pragma once

#include <stdexcept>
#include <string>

#include "lexer/SourceLocation.hpp"

namespace hope {

// Thrown by the type checker when a type error is detected.
// The REPL catches TypeError and displays it without terminating the session.
class TypeError : public std::runtime_error {
public:
    SourceLocation loc;

    TypeError(std::string message, SourceLocation loc)
        : std::runtime_error(std::move(message)), loc(std::move(loc)) {}
};

} // namespace hope
