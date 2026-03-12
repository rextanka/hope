#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace hope {

enum class Assoc { Left, Right };

struct OpInfo {
    int   prec;    // 1–9 (matching Hope's declared range)
    Assoc assoc;
};

// Binding powers used by the Pratt expression parser.
// For a left-associative operator at precedence p:
//   token_bp  = p * 2        (the "left binding power" — must exceed this to consume)
//   right_bp  = p * 2 + 1   (parse right operand at this minimum)
// For a right-associative operator:
//   token_bp  = p * 2
//   right_bp  = p * 2        (same, so the right sub-parse can grab the same precedence)
struct BindingPower {
    int token_bp;  // how tightly this operator binds to its left argument
    int right_bp;  // minimum bp for the right-hand operand
};

inline BindingPower binding_power(const OpInfo& op) {
    int base = op.prec * 2;
    return { base, op.assoc == Assoc::Left ? base + 1 : base - 1 };
}

// Tracks all currently-declared infix/infixr operators.
// Updated in place as infix declarations are parsed — subsequent expressions
// in the same file use the updated table immediately.
class OperatorTable {
public:
    OperatorTable() { reset_to_standard(); }

    void declare(std::string name, int prec, Assoc assoc) {
        ops_[std::move(name)] = { prec, assoc };
    }

    std::optional<OpInfo> lookup(const std::string& name) const {
        auto it = ops_.find(name);
        if (it == ops_.end()) return std::nullopt;
        return it->second;
    }

    // Populate with Standard.hop's built-in operator declarations.
    // Called on construction and on :clear.
    void reset_to_standard() {
        ops_.clear();
        // From Standard.hop (precedence and associativity as declared)
        ops_["->"]  = { 2, Assoc::Right };  // function type / application arrow
        ops_["#"]   = { 4, Assoc::Right };  // product type / pair
        ops_["X"]   = { 4, Assoc::Right };  // synonym for #
        ops_["o"]   = { 2, Assoc::Left  };  // function composition
        ops_["or"]  = { 1, Assoc::Right };  // boolean or
        ops_["and"] = { 2, Assoc::Left  };  // boolean and
        ops_["::"]  = { 5, Assoc::Right };  // list cons
        ops_["<>"]  = { 5, Assoc::Right };  // list concatenation
        ops_["="]   = { 3, Assoc::Left  };  // equality
        ops_["/="]  = { 3, Assoc::Left  };  // inequality
        ops_["<"]   = { 4, Assoc::Left  };  // less-than
        ops_["=<"]  = { 4, Assoc::Left  };  // less-or-equal
        ops_[">"]   = { 4, Assoc::Left  };  // greater-than
        ops_[">="]  = { 4, Assoc::Left  };  // greater-or-equal
        ops_["+"]   = { 5, Assoc::Left  };  // addition
        ops_["-"]   = { 5, Assoc::Left  };  // subtraction
        ops_["*"]   = { 6, Assoc::Left  };  // multiplication
        ops_["/"]   = { 6, Assoc::Left  };  // division (real)
        ops_["div"] = { 6, Assoc::Left  };  // integer division
        ops_["mod"] = { 6, Assoc::Left  };  // modulo
    }

private:
    std::unordered_map<std::string, OpInfo> ops_;
};

} // namespace hope
