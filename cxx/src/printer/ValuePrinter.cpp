// ValuePrinter.cpp — render Hope runtime values as strings.

#include "printer/ValuePrinter.hpp"

#include <cmath>
#include <sstream>
#include <string>

namespace hope {

// ---------------------------------------------------------------------------
// Internal context — threading depth limit through the recursion.
// ---------------------------------------------------------------------------

namespace {

struct PrintCtx {
    Evaluator& ev;
    int depth = 0;
    static constexpr int kMaxDepth = 50;
    static constexpr int kMaxElems = 10000;
};

// Forward declaration.
std::string pv(ValRef v, TyRef hint, PrintCtx& ctx, bool need_parens);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Escape a character for use inside a char literal: 'c'
std::string escape_char_in_char_lit(char c) {
    switch (c) {
        case '\'': return "\\'";
        case '\\': return "\\\\";
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\0': return "\\0";
        default:
            if (static_cast<unsigned char>(c) < 32) {
                // Non-printable: hex escape would be ideal but Hope uses '\n' style.
                // Just fall through and emit as-is for robustness.
            }
            return std::string(1, c);
    }
}

// Escape a character for use inside a string literal: "..."
std::string escape_char_in_str_lit(char c) {
    switch (c) {
        case '"':  return "\\\"";
        case '\\': return "\\\\";
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\0': return "\\0";
        default:
            return std::string(1, c);
    }
}

// Is this type hint a list-char type?
bool is_list_char_hint(TyRef hint) {
    if (!hint) return false;
    if (auto* tc = std::get_if<TyCons>(&hint->data)) {
        if (tc->name == "list" && tc->args.size() == 1) {
            TyRef elem = tc->args[0];
            if (elem) {
                if (auto* ec = std::get_if<TyCons>(&elem->data)) {
                    return ec->name == "char" && ec->args.empty();
                }
            }
        }
    }
    return false;
}

// Get list element type from a list hint (or null).
TyRef list_elem_hint(TyRef hint) {
    if (!hint) return nullptr;
    if (auto* tc = std::get_if<TyCons>(&hint->data)) {
        if (tc->name == "list" && tc->args.size() == 1) {
            return tc->args[0];
        }
    }
    return nullptr;
}

// Print a number value.
std::string print_num(double n) {
    // Whole numbers within a safe integer range: print without decimal.
    if (std::isfinite(n) && n == std::floor(n) &&
        n >= -9007199254740992.0 && n <= 9007199254740992.0) {
        // Print as integer.
        std::ostringstream ss;
        ss << static_cast<long long>(n);
        return ss.str();
    }
    // Fractional or out-of-range: print as double.
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

// Collect a list (cons cells) into elements.
// Returns true if the list terminated normally (nil).
// Elements are appended to `out`; limit is applied.
bool collect_list(ValRef v, Evaluator& ev, std::vector<ValRef>& out, int limit) {
    int count = 0;
    while (true) {
        v = ev.force(v);
        if (!v) return false;
        if (auto* vc = std::get_if<VCons>(&v->data)) {
            if (vc->name == "nil") return true;
            if (vc->name == "::") {
                if (count >= limit) return false; // truncated
                ValRef arg = ev.force(vc->arg);
                if (!arg) return false;
                if (auto* pr = std::get_if<VPair>(&arg->data)) {
                    out.push_back(pr->left);
                    v = pr->right;
                    ++count;
                    continue;
                }
                return false;
            }
        }
        return false;
    }
}

// Collect a nested-pair tuple into a flat vector.
void collect_tuple(ValRef v, Evaluator& ev, std::vector<ValRef>& out) {
    while (true) {
        v = ev.force(v);
        if (!v) return;
        if (auto* pr = std::get_if<VPair>(&v->data)) {
            out.push_back(pr->left);
            v = pr->right;
        } else {
            out.push_back(v);
            return;
        }
    }
}

// Main recursive printer.
// need_parens: if true, wrap the result in parens if it would otherwise be
// ambiguous as a constructor argument (contains spaces, etc.)
std::string pv(ValRef v, TyRef hint, PrintCtx& ctx, bool need_parens) {
    if (ctx.depth > PrintCtx::kMaxDepth) return "...";

    v = ctx.ev.force(v);
    if (!v) return "<null>";

    ++ctx.depth;
    std::string result;

    if (auto* vn = std::get_if<VNum>(&v->data)) {
        result = print_num(vn->n);

    } else if (auto* vc_c = std::get_if<VChar>(&v->data)) {
        result = "'" + escape_char_in_char_lit(vc_c->c) + "'";

    } else if (auto* vc = std::get_if<VCons>(&v->data)) {

        if (vc->name == "nil") {
            // Empty list. Use string syntax if hint says list char.
            if (is_list_char_hint(hint)) {
                result = "\"\"";
            } else {
                result = "[]";
            }

        } else if (vc->name == "::") {
            // Non-empty list — collect all elements.
            std::vector<ValRef> elems;
            bool terminated = collect_list(v, ctx.ev, elems, PrintCtx::kMaxElems);

            // Check whether this is a char list (string).
            bool is_string = is_list_char_hint(hint);
            if (!is_string && !elems.empty()) {
                ValRef first = ctx.ev.force(elems[0]);
                if (first && std::holds_alternative<VChar>(first->data))
                    is_string = true;
            }

            if (is_string) {
                // Print as string literal.
                // Re-collect to get the raw VChar values (they may be thunks).
                result = "\"";
                for (const ValRef& e : elems) {
                    ValRef fe = ctx.ev.force(e);
                    if (!fe) { result += "?"; continue; }
                    if (auto* ch = std::get_if<VChar>(&fe->data)) {
                        result += escape_char_in_str_lit(ch->c);
                    } else {
                        result += "?";
                    }
                }
                if (!terminated) result += "...";
                result += "\"";
            } else {
                // Print as list.
                TyRef elem_hint = list_elem_hint(hint);
                result = "[";
                for (size_t i = 0; i < elems.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += pv(elems[i], elem_hint, ctx, false);
                }
                if (!terminated) {
                    if (!elems.empty()) result += ", ...";
                    else result += "...";
                }
                result += "]";
            }

        } else if (!vc->has_arg) {
            // Nullary constructor.
            result = vc->name;

        } else {
            // Unary constructor with argument.
            std::string name = vc->name;
            ValRef arg = ctx.ev.force(vc->arg);

            // Determine whether the argument needs parentheses.
            bool arg_needs_parens = false;
            if (arg) {
                if (auto* ac = std::get_if<VCons>(&arg->data)) {
                    // Constructor with an arg needs parens (ambiguous application).
                    if (ac->has_arg) arg_needs_parens = true;
                    // "::" (cons list) — no parens needed for the list itself.
                    // VPair → tuple — needs parens.
                } else if (std::holds_alternative<VPair>(arg->data)) {
                    arg_needs_parens = true;
                }
            }

            std::string arg_str = pv(arg, nullptr, ctx, arg_needs_parens);
            if (arg_needs_parens) {
                result = name + " " + arg_str; // pv already wrapped in parens
            } else {
                result = name + " " + arg_str;
            }

            // If the whole thing needs parens as a ctor arg itself, that is
            // handled by the caller (need_parens parameter).
        }

    } else if (std::holds_alternative<VPair>(v->data)) {
        // Tuple: collect nested pairs into a flat list.
        std::vector<ValRef> elems;
        collect_tuple(v, ctx.ev, elems);

        std::string s = "(";
        for (size_t i = 0; i < elems.size(); ++i) {
            if (i > 0) s += ", ";
            s += pv(elems[i], nullptr, ctx, false);
        }
        s += ")";
        result = s;

    } else if (auto* vf = std::get_if<VFun>(&v->data)) {
        if (vf->repr.has_value()) {
            result = *vf->repr;
        } else {
            result = "<function>";
        }

    } else if (std::holds_alternative<VHole>(v->data)) {
        result = "<loop>";

    } else if (std::holds_alternative<VThunk>(v->data)) {
        // Should have been forced by this point; defensive fallback.
        result = "<thunk>";

    } else {
        result = "<unknown>";
    }

    --ctx.depth;

    // Wrap in parens if the caller needs it and the result would be ambiguous.
    if (need_parens) {
        // Only wrap if the result contains a space (i.e., has sub-structure)
        // or starts with a letter/digit but has a space in it.
        // A simpler rule: wrap if it contains a space and is not already
        // wrapped in brackets or parens.
        if (!result.empty()) {
            char first = result.front();
            bool already_grouped = (first == '(' || first == '[' || first == '"' || first == '\'');
            if (!already_grouped && result.find(' ') != std::string::npos) {
                result = "(" + result + ")";
            }
        }
    }

    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string print_value(ValRef v, Evaluator& ev, TyRef type_hint) {
    PrintCtx ctx{ev};
    return pv(v, type_hint, ctx, false);
}

} // namespace hope
