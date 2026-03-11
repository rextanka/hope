// ExprPrinter.cpp — render Hope AST expressions as source text.

#include "printer/ExprPrinter.hpp"

#include <sstream>
#include <unordered_set>

namespace hope {

namespace {

// Operator precedence levels for parenthesization decisions.
// (These match Hope's infix/infixr declarations and built-ins.)
static int op_prec(const std::string& op) {
    if (op == "::")          return 5;  // right-assoc cons
    if (op == "+" || op == "-")   return 6;
    if (op == "*" || op == "/" || op == "div" || op == "mod") return 7;
    if (op == "<>" )         return 4;  // append
    if (op == "=" || op == "/=" || op == "<" ||
        op == ">" || op == "=<" || op == ">=") return 4;
    if (op == "and")         return 3;
    if (op == "or")          return 2;
    return 5; // default
}

// Returns true if the operator is right-associative.
static bool op_right_assoc(const std::string& op) {
    return op == "::" || op == "o";
}

// Forward declare.
std::string pe(const Expr& e, const ExprSubst& subst, int min_prec, bool parens_needed);
std::string pp(const Pattern& p, const std::unordered_set<std::string>& infix_vars = {});

// Collect all names used as infix operators in the expression body.
// This lets us detect that a PVar pattern is used as an infix op.
void collect_infix_vars(const Expr& e, std::unordered_set<std::string>& out) {
    std::visit([&](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, EInfix>) {
            out.insert(alt.op);
            collect_infix_vars(*alt.left,  out);
            collect_infix_vars(*alt.right, out);
        } else if constexpr (std::is_same_v<T, EApply>) {
            collect_infix_vars(*alt.func, out);
            collect_infix_vars(*alt.arg,  out);
        } else if constexpr (std::is_same_v<T, ELambda>) {
            for (const auto& c : alt.clauses)
                collect_infix_vars(*c.body, out);
        } else if constexpr (std::is_same_v<T, ESection>) {
            collect_infix_vars(*alt.expr, out);
        } else if constexpr (std::is_same_v<T, ETuple>) {
            for (const auto& el : alt.elems) collect_infix_vars(*el, out);
        } else if constexpr (std::is_same_v<T, EList>) {
            for (const auto& el : alt.elems) collect_infix_vars(*el, out);
        } else if constexpr (std::is_same_v<T, EIf>) {
            collect_infix_vars(*alt.cond,  out);
            collect_infix_vars(*alt.then_, out);
            collect_infix_vars(*alt.else_, out);
        }
        // EVar, ELit, EOpRef: no sub-expressions.
    }, e.data);
}

std::string literal_str(const Literal& lit) {
    return std::visit([](const auto& l) -> std::string {
        return l.text;
    }, lit);
}

std::string pp(const Pattern& p, const std::unordered_set<std::string>& infix_vars) {
    return std::visit([&](const auto& alt) -> std::string {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, PVar>) {
            // If this variable is used as an infix operator in the body,
            // it must have been written as (op) in the source — wrap in parens.
            if (infix_vars.count(alt.name))
                return "(" + alt.name + ")";
            return alt.name;
        }
        if constexpr (std::is_same_v<T, PWild>) return "_";
        if constexpr (std::is_same_v<T, PLit>)  return literal_str(alt.lit);
        if constexpr (std::is_same_v<T, PSucc>) return "succ(" + pp(*alt.inner) + ")";
        if constexpr (std::is_same_v<T, PNPlusK>) return alt.var + "+" + std::to_string(alt.k);
        if constexpr (std::is_same_v<T, PCons>) {
            if (!alt.arg.has_value()) return alt.name;
            std::string arg_str = pp(*(*alt.arg));
            // Check if arg needs parens (has spaces and not already grouped).
            bool needs_p = !arg_str.empty() &&
                           arg_str.find(' ') != std::string::npos &&
                           arg_str.front() != '(' && arg_str.front() != '[';
            if (needs_p) return alt.name + " (" + arg_str + ")";
            return alt.name + " " + arg_str;
        }
        if constexpr (std::is_same_v<T, PTuple>) {
            if (alt.elems.empty()) return "()";
            std::string s = "(";
            for (size_t i = 0; i < alt.elems.size(); ++i) {
                if (i > 0) s += ", ";
                s += pp(*alt.elems[i]);
            }
            s += ")";
            return s;
        }
        if constexpr (std::is_same_v<T, PList>) {
            std::string s = "[";
            for (size_t i = 0; i < alt.elems.size(); ++i) {
                if (i > 0) s += ", ";
                s += pp(*alt.elems[i]);
            }
            return s + "]";
        }
        if constexpr (std::is_same_v<T, PInfix>) {
            return pp(*alt.left) + " " + alt.op + " " + pp(*alt.right);
        }
        return "?";
    }, p.data);
}

// Check if a string looks like an infix operator (non-alphanumeric symbol
// or a known infix keyword).  Used when a variable substitution produces
// an operator name so that EApply(f, (l,r)) can be re-written as l op r.
static bool is_infix_op(const std::string& name) {
    if (name.empty()) return false;
    // Known alphanumeric infix keywords.
    if (name == "div" || name == "mod" || name == "and" || name == "or" ||
        name == "o")
        return true;
    // If the first character is non-alphanumeric (and not underscore/quote),
    // it's a symbolic operator.
    char c = name[0];
    return !(std::isalnum((unsigned char)c) || c == '_' || c == '\'');
}

// Strip outer parens from an operator representation, e.g. "(+)" -> "+".
static std::string strip_op_parens(const std::string& s) {
    if (s.size() >= 3 && s.front() == '(' && s.back() == ')')
        return s.substr(1, s.size() - 2);
    return s;
}

// Try to split a string of the form "(a, b)" into two parts "a" and "b".
// Returns true on success.  Used when printing (op)(a, b) as a op b.
static bool split_pair_repr(const std::string& s, std::string& left, std::string& right) {
    if (s.size() < 5 || s.front() != '(' || s.back() != ')') return false;
    std::string inner = s.substr(1, s.size() - 2);
    // Find the top-level comma (depth 0 in nested parens/brackets).
    int depth = 0;
    for (size_t i = 0; i < inner.size(); ++i) {
        char c = inner[i];
        if (c == '(' || c == '[') ++depth;
        else if (c == ')' || c == ']') --depth;
        else if (c == ',' && depth == 0) {
            left  = inner.substr(0, i);
            right = inner.substr(i + 1);
            // Trim leading/trailing spaces.
            while (!left.empty()  && left.front()  == ' ') left.erase(left.begin());
            while (!left.empty()  && left.back()   == ' ') left.pop_back();
            while (!right.empty() && right.front() == ' ') right.erase(right.begin());
            while (!right.empty() && right.back()  == ' ') right.pop_back();
            return true;
        }
    }
    return false;
}

std::string pe(const Expr& e, const ExprSubst& subst, int min_prec, bool need_parens) {
    std::string result;

    std::visit([&](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;

        if constexpr (std::is_same_v<T, EVar>) {
            auto it = subst.find(alt.name);
            if (it != subst.end()) {
                result = it->second;
            } else {
                result = alt.name;
            }
        }

        else if constexpr (std::is_same_v<T, ELit>) {
            result = literal_str(alt.lit);
        }

        else if constexpr (std::is_same_v<T, EOpRef>) {
            // Operator used as value: (op)
            result = "(" + alt.op + ")";
        }

        else if constexpr (std::is_same_v<T, EApply>) {
            // Detect the pattern: EApply(EVar f, ...) where f is substituted
            // to an infix operator, and the argument is a 2-tuple (ETuple or a
            // variable bound to a tuple repr).  Print as l op r.
            bool printed_as_infix = false;
            if (const auto* fv = std::get_if<EVar>(&alt.func->data)) {
                auto it = subst.find(fv->name);
                if (it != subst.end()) {
                    std::string op_name = strip_op_parens(it->second);
                    if (is_infix_op(op_name)) {
                        if (const auto* tup = std::get_if<ETuple>(&alt.arg->data)) {
                            if (tup->elems.size() == 2) {
                                int prec = op_prec(op_name);
                                std::string ls = pe(*tup->elems[0], subst, prec + 1, prec < min_prec);
                                std::string rs = pe(*tup->elems[1], subst, prec,     prec < min_prec);
                                result = ls + " " + op_name + " " + rs;
                                printed_as_infix = true;
                            }
                        } else {
                            // arg is not an ETuple — check if its repr is a pair "(a, b)".
                            std::string arg_repr = pe(*alt.arg, subst, 0, false);
                            std::string lr, rr;
                            if (split_pair_repr(arg_repr, lr, rr)) {
                                result = lr + " " + op_name + " " + rr;
                                printed_as_infix = true;
                            }
                        }
                    }
                }
            }
            // Also detect EApply(EOpRef op, ETuple[l, r])
            if (!printed_as_infix) {
                if (const auto* opref = std::get_if<EOpRef>(&alt.func->data)) {
                    if (const auto* tup = std::get_if<ETuple>(&alt.arg->data)) {
                        if (tup->elems.size() == 2) {
                            int prec = op_prec(opref->op);
                            std::string ls = pe(*tup->elems[0], subst, prec + 1, prec < min_prec);
                            std::string rs = pe(*tup->elems[1], subst, prec,     prec < min_prec);
                            result = ls + " " + opref->op + " " + rs;
                            printed_as_infix = true;
                        }
                    } else {
                        // EApply(EOpRef op, x) where x may be a variable bound to
                        // a tuple repr, e.g. subst[x] = "(1, 2)".
                        std::string arg_repr = pe(*alt.arg, subst, 0, false);
                        std::string lr, rr;
                        if (split_pair_repr(arg_repr, lr, rr)) {
                            result = lr + " " + opref->op + " " + rr;
                            printed_as_infix = true;
                        }
                    }
                }
            }
            if (!printed_as_infix) {
                std::string func_str = pe(*alt.func, subst, 10, false);
                std::string arg_str  = pe(*alt.arg,  subst, 10, true);
                result = func_str + " " + arg_str;
            }
        }

        else if constexpr (std::is_same_v<T, EInfix>) {
            // The operator may itself be a variable bound in subst (e.g. a
            // user-declared infix variable `op`).
            std::string op_name = alt.op;
            auto it = subst.find(op_name);
            if (it != subst.end())
                op_name = strip_op_parens(it->second);
            int prec = op_prec(op_name);
            bool ra  = op_right_assoc(op_name);
            std::string left_str  = pe(*alt.left,  subst, prec + (ra ? 0 : 1), prec < min_prec);
            std::string right_str = pe(*alt.right, subst, prec + (ra ? 1 : 0), prec < min_prec);
            result = left_str + " " + op_name + " " + right_str;
        }

        else if constexpr (std::is_same_v<T, ETuple>) {
            if (alt.elems.empty()) {
                result = "()";
            } else if (alt.elems.size() == 1) {
                result = pe(*alt.elems[0], subst, 0, true);
            } else {
                result = "(";
                for (size_t i = 0; i < alt.elems.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += pe(*alt.elems[i], subst, 0, false);
                }
                result += ")";
            }
        }

        else if constexpr (std::is_same_v<T, EList>) {
            // If all elements are char literals, print as a string.
            bool all_chars = !alt.elems.empty();
            for (const auto& ep : alt.elems) {
                // Check for char literal or substituted char.
                bool is_char = false;
                if (const auto* el = std::get_if<ELit>(&ep->data)) {
                    if (std::holds_alternative<LitChar>(el->lit)) is_char = true;
                }
                if (!is_char) { all_chars = false; break; }
            }
            if (all_chars) {
                // Print as string literal.
                result = "\"";
                for (const auto& ep : alt.elems) {
                    if (const auto* el = std::get_if<ELit>(&ep->data)) {
                        if (const auto* lc = std::get_if<LitChar>(&el->lit)) {
                            // lc->text is like 'a' — strip the outer quotes and unescape.
                            const std::string& t = lc->text;
                            if (t.size() >= 2) {
                                std::string inner = t.substr(1, t.size() - 2);
                                // Re-escape for string context.
                                if (inner == "\\'") result += "'";
                                else if (inner == "\\\\") result += "\\\\";
                                else result += inner;
                            }
                        }
                    }
                }
                result += "\"";
            } else {
                result = "[";
                for (size_t i = 0; i < alt.elems.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += pe(*alt.elems[i], subst, 0, false);
                }
                result += "]";
            }
        }

        else if constexpr (std::is_same_v<T, ESection>) {
            // Sections are printed in section form: (e op) or (op e).
            std::string e_str = pe(*alt.expr, subst, 0, false);
            if (alt.is_left) {
                result = "(" + e_str + " " + alt.op + ")";
            } else {
                result = "(" + alt.op + " " + e_str + ")";
            }
        }

        else if constexpr (std::is_same_v<T, ELambda>) {
            // Print: lambda pat => body | pat => body | ...
            // Compute which pattern variable names are used as infix operators
            // in the body so we can print them as (op).
            std::unordered_set<std::string> infix_vars;
            for (const auto& clause : alt.clauses)
                collect_infix_vars(*clause.body, infix_vars);

            auto print_clause_pat = [&](const LambdaClause& clause) -> std::string {
                if (clause.pats.size() == 1) {
                    return pp(*clause.pats[0], infix_vars);
                }
                std::string s = "(";
                for (size_t i = 0; i < clause.pats.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += pp(*clause.pats[i], infix_vars);
                }
                s += ")";
                return s;
            };

            if (alt.clauses.empty()) {
                result = "lambda ???";
            } else if (alt.clauses.size() == 1) {
                const auto& clause = alt.clauses[0];
                result = "lambda " + print_clause_pat(clause)
                       + " => " + pe(*clause.body, subst, 0, false);
            } else {
                // Multi-clause lambda
                std::string s;
                for (size_t ci = 0; ci < alt.clauses.size(); ++ci) {
                    if (ci > 0) s += " | ";
                    const auto& clause = alt.clauses[ci];
                    s += "lambda " + print_clause_pat(clause)
                       + " => " + pe(*clause.body, subst, 0, false);
                }
                result = s;
            }
        }

        else if constexpr (std::is_same_v<T, EIf>) {
            result = "if " + pe(*alt.cond, subst, 0, false)
                   + " then " + pe(*alt.then_, subst, 0, false)
                   + " else " + pe(*alt.else_, subst, 0, false);
        }

        else if constexpr (std::is_same_v<T, ELet>) {
            result = "let ... in ...";  // simplified
        }

        else if constexpr (std::is_same_v<T, ELetRec>) {
            result = "letrec ... in ...";  // simplified
        }

        else if constexpr (std::is_same_v<T, EWhere>) {
            result = "... where ...";  // simplified
        }

        else if constexpr (std::is_same_v<T, EWhereRec>) {
            result = "... whererec ...";  // simplified
        }

        else if constexpr (std::is_same_v<T, EWrite>) {
            result = "write " + pe(*alt.expr, subst, 0, false);
        }

        else {
            result = "?";
        }
    }, e.data);

    // Wrap in parens if needed and result contains spaces.
    if (need_parens && !result.empty()) {
        char first = result.front();
        bool already_grouped = (first == '(' || first == '[' || first == '"' || first == '\'');
        if (!already_grouped && result.find(' ') != std::string::npos) {
            result = "(" + result + ")";
        }
    }

    return result;
}

} // anonymous namespace

std::string print_expr(const Expr& e, const ExprSubst& subst) {
    return pe(e, subst, 0, false);
}

std::string print_pattern(const Pattern& p) {
    return pp(p);
}

} // namespace hope
