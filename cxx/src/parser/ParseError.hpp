#pragma once

#include <stdexcept>
#include <string>
#include "lexer/SourceLocation.hpp"

namespace hope {

// Thrown by the parser when it encounters a token sequence it cannot accept.
// The REPL catches ParseError and displays it without terminating the session.
class ParseError : public std::runtime_error {
public:
    SourceLocation loc;

    ParseError(std::string message, SourceLocation loc)
        : std::runtime_error(std::move(message)), loc(std::move(loc)) {}
};

} // namespace hope
