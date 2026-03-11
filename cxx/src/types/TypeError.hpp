#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "lexer/SourceLocation.hpp"

namespace hope {

// Thrown by the type checker when a type error is detected.
// The REPL catches TypeError and displays it without terminating the session.
class TypeError : public std::runtime_error {
public:
    SourceLocation loc;

    // Optional context lines shown before the error message.
    // Each entry is a pre-formatted line (without trailing newline).
    std::vector<std::string> context;

    TypeError(std::string message, SourceLocation loc)
        : std::runtime_error(std::move(message)), loc(std::move(loc)) {}

    TypeError(std::string message, SourceLocation loc,
              std::vector<std::string> ctx)
        : std::runtime_error(std::move(message)), loc(std::move(loc)),
          context(std::move(ctx)) {}
};

} // namespace hope
