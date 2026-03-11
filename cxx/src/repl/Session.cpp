// Session.cpp — Hope interpreter session (batch/file mode).

#include "repl/Session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "lexer/Lexer.hpp"
#include "parser/ParseError.hpp"
#include "parser/Parser.hpp"
#include "printer/ExprPrinter.hpp"
#include "printer/TypePrinter.hpp"
#include "printer/ValuePrinter.hpp"
#include "runtime/RuntimeError.hpp"
#include "types/TypeError.hpp"

namespace hope {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Session::Session()
    : type_checker_(type_env_)
    , evaluator_(type_env_)
    , loader_(evaluator_, ops_)
{
}

// ---------------------------------------------------------------------------
// load_standard
// ---------------------------------------------------------------------------

bool Session::load_standard(const std::string& lib_dir) {
    lib_dir_ = lib_dir;
    loader_.set_lib_dir(lib_dir);

    std::string path = lib_dir + "/Standard.hop";
    if (!std::filesystem::exists(path)) {
        // Try lower-case first letter.
        std::string lower = lib_dir + "/standard.hop";
        if (std::filesystem::exists(lower))
            path = lower;
        else
            return false;
    }

    // Suppress output while loading the standard library.
    bool old_flag = in_silent_load_;
    in_silent_load_ = true;
    std::ostringstream sink;
    bool ok = run_file(path, sink);
    in_silent_load_ = old_flag;
    return ok;
}

// ---------------------------------------------------------------------------
// run_file
// ---------------------------------------------------------------------------

bool Session::run_file(const std::string& filepath, std::ostream& out) {
    // Canonicalize path for double-load prevention.
    std::string abs_path;
    try {
        abs_path = std::filesystem::absolute(filepath).string();
    } catch (...) {
        abs_path = filepath;
    }

    if (loaded_files_.count(abs_path)) return true; // already loaded
    loaded_files_.insert(abs_path);

    std::ifstream file(abs_path);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    run_string(ss.str(), abs_path, out);
    return true;
}

// ---------------------------------------------------------------------------
// run_string
// ---------------------------------------------------------------------------

void Session::run_string(const std::string& code,
                         const std::string& source_name,
                         std::ostream& out) {
    Lexer lex(code, source_name);
    Parser parser(std::move(lex));
    parser.op_table() = ops_;

    while (true) {
        std::optional<Decl> decl;
        try {
            decl = parser.parse_decl();
        } catch (const ParseError& e) {
            if (!in_silent_load_) {
                std::cerr << "Parse error at " << e.loc.to_string()
                          << ": " << e.what() << "\n";
            }
            break;
        }
        if (!decl) break;

        process_decl(std::move(*decl), out);
    }

    // Merge any new operator declarations back into our accumulated table.
    ops_ = parser.op_table();
}

// ---------------------------------------------------------------------------
// process_decl
// ---------------------------------------------------------------------------

void Session::process_decl(Decl d, std::ostream& out) {
    // We need to inspect the decl before (and in some cases after) moving it,
    // so pull out the relevant data first.

    // Handle DEval specially: we need both the expr (for type inference) and
    // then eval, but we cannot copy. Keep a raw ptr to the expr before moving.
    if (auto* eval_alt = std::get_if<DEval>(&d.data)) {
        const Expr* expr_ptr = eval_alt->expr.get();

        // Infer type before we do anything else.
        TyRef inferred_type = nullptr;
        if (expr_ptr) {
            try {
                inferred_type = type_checker_.infer_top_expr(*expr_ptr);
            } catch (const TypeError& te) {
                if (!in_silent_load_) {
                    // Print context lines first, then the error.
                    for (const auto& line : te.context) {
                        out << line << "\n";
                    }
                    out << "line " << te.loc.line << ": type error - " << te.what() << "\n";
                }
                return;
            } catch (...) {
                // Type inference failure: proceed without type annotation.
            }
        }

        if (expr_ptr) {
            // A top-level write expression streams output but does not print
            // a >> result : type line.
            bool is_write_cmd = std::holds_alternative<EWrite>(expr_ptr->data);
            try {
                ValRef val = evaluator_.eval_top(*expr_ptr);
                if (!in_silent_load_ && !is_write_cmd) {
                    print_result(val, inferred_type, out);
                }
            } catch (const RuntimeError& re) {
                if (!in_silent_load_) {
                    out << "Runtime error: " << re.what();
                    if (re.loc.line > 0)
                        out << " at " << re.loc.to_string();
                    out << "\n";
                }
            } catch (const std::exception& e) {
                if (!in_silent_load_) {
                    out << "Error: " << e.what() << "\n";
                }
            }
        }
        return;
    }

    // Handle DUses: load the module(s) (don't pass to evaluator).
    if (auto* uses_alt = std::get_if<DUses>(&d.data)) {
        for (const std::string& name : uses_alt->module_names) {
            load_module(name, out);
        }
        return;
    }

    // Handle DInfix: update our operator table.
    if (auto* infix_alt = std::get_if<DInfix>(&d.data)) {
        Assoc assoc = infix_alt->right_assoc ? Assoc::Right : Assoc::Left;
        ops_.declare(infix_alt->name, infix_alt->prec, assoc);
        // DInfix has no unique_ptr members, so evaluator ignores it anyway.
        // Still pass it so the evaluator can record it if needed.
        try { evaluator_.add_decl(std::move(d)); } catch (...) {}
        return;
    }

    // Handle DDisplay: print all recorded user declarations.
    if (std::holds_alternative<DDisplay>(d.data)) {
        if (!in_silent_load_) {
            out << "\n";
            for (const auto& rec : display_records_) {
                out << rec << "\n";
            }
        }
        return;
    }

    // Handle type-checker-only declarations (no eval involvement needed).
    if (std::holds_alternative<DAbsType>(d.data) ||
        std::holds_alternative<DTypeVar>(d.data)) {
        try { type_checker_.check_decl(d); } catch (...) {}
        return;
    }

    // Handle DDec: type-check and record for display.
    if (std::holds_alternative<DDec>(d.data)) {
        try { type_checker_.check_decl(d); } catch (...) {}
        if (!in_silent_load_) {
            const auto& dec = std::get<DDec>(d.data);
            if (dec.type) {
                std::string rec = "dec ";
                for (size_t i = 0; i < dec.names.size(); ++i) {
                    if (i > 0) rec += ", ";
                    rec += dec.names[i];
                }
                rec += " : ";
                rec += print_ast_type(*dec.type);
                rec += ";";
                display_records_.push_back(std::move(rec));
            }
        }
        return;
    }

    // Record DEquation for display before it gets moved into the evaluator.
    // Also type-check the equation (after recording, before moving).
    if (std::holds_alternative<DEquation>(d.data)) {
        if (!in_silent_load_) {
            const auto& eq = std::get<DEquation>(d.data);
            std::string rec = "--- " + eq.lhs.func;
            for (const auto& pat : eq.lhs.args) {
                rec += " ";
                rec += print_pattern(*pat);
            }
            rec += " <= ";
            if (eq.rhs) rec += print_expr(*eq.rhs);
            rec += ";";
            display_records_.push_back(std::move(rec));
        }
        // Type-check before moving.
        try {
            type_checker_.check_decl(d);
        } catch (const TypeError& te) {
            if (!in_silent_load_) {
                for (const auto& line : te.context) out << line << "\n";
                out << "line " << te.loc.line << ": type error - " << te.what() << "\n";
            }
            // Don't add to evaluator if type check fails (allows recovery).
            // But DO add so the evaluator has it for future equations in the same session.
        } catch (...) {}
        try { evaluator_.add_decl(std::move(d)); } catch (...) {}
        return;
    }

    // All other declarations: pass to the type checker (optionally) and
    // evaluator.  Since add_decl takes by value, we move d.
    // For DData and DType, also type-check.
    bool is_data = std::holds_alternative<DData>(d.data);
    bool is_type = std::holds_alternative<DType>(d.data);

    if (is_data || is_type) {
        // Type-check first (before move).
        try {
            type_checker_.check_decl(d);
        } catch (const TypeError& te) {
            if (!in_silent_load_) {
                for (const auto& line : te.context) out << line << "\n";
                out << "line " << te.loc.line << ": type error - " << te.what() << "\n";
            }
        } catch (...) {}
    }

    // Move into the evaluator.
    try { evaluator_.add_decl(std::move(d)); } catch (...) {}
}

// ---------------------------------------------------------------------------
// load_module
// ---------------------------------------------------------------------------

void Session::load_module(const std::string& name, std::ostream& out) {
    if (loaded_modules_.count(name)) return;
    loaded_modules_.insert(name);

    // Resolve the module file.
    std::string path = lib_dir_ + "/" + name + ".hop";
    if (!std::filesystem::exists(path)) {
        if (!name.empty()) {
            std::string lower = name;
            lower[0] = static_cast<char>(
                std::tolower(static_cast<unsigned char>(lower[0])));
            std::string path2 = lib_dir_ + "/" + lower + ".hop";
            if (std::filesystem::exists(path2)) {
                path = path2;
            } else {
                if (!in_silent_load_) {
                    std::cerr << "Module not found: " << name
                              << " (searched in: " << lib_dir_ << ")\n";
                }
                return;
            }
        }
    }

    // Load the module silently.
    bool old_flag = in_silent_load_;
    in_silent_load_ = true;
    std::ostringstream sink;
    run_file(path, sink);
    in_silent_load_ = old_flag;
}

// ---------------------------------------------------------------------------
// print_result
// ---------------------------------------------------------------------------

void Session::print_result(ValRef v, TyRef t, std::ostream& out) {
    // Do NOT force_full here — the value may be an infinite lazy structure.
    // The value printer calls force() on demand as it traverses the value.
    // Just force to head-normal form first.
    try {
        v = evaluator_.force(v);
    } catch (const RuntimeError& re) {
        out << "Runtime error (forcing): " << re.what() << "\n";
        return;
    } catch (...) {
        out << "Error forcing value\n";
        return;
    }

    std::string val_str;
    try {
        val_str = print_value(v, evaluator_, t);
    } catch (...) {
        val_str = "<error printing value>";
    }

    std::string type_str;
    try {
        type_str = print_type(t);
    } catch (...) {
        type_str = "_";
    }

    out << ">> " << val_str << " : " << type_str << "\n";
}

} // namespace hope
