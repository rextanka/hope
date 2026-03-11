#pragma once

#include <string>

namespace hope {

// The location of a token or AST node in source text.
// line and column are 1-based. length is in source characters.
struct SourceLocation {
    std::string file;
    int         line   = 1;
    int         column = 1;
    int         length = 0;

    std::string to_string() const {
        return file + ":" + std::to_string(line) + ":" + std::to_string(column);
    }

    bool operator==(const SourceLocation&) const = default;
};

} // namespace hope
