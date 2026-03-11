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
// set_argv / set_input_stream
// ---------------------------------------------------------------------------

void Session::set_argv(const std::vector<std::string>& args) {
    // Build a Hope value: list (list char) — each arg as a list of chars.
    ValRef result = make_nil();
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        // Build the arg string as list char (back to front).
        ValRef str = make_nil();
        for (auto ci = it->rbegin(); ci != it->rend(); ++ci) {
            str = make_cons(make_char(*ci), str);
        }
        result = make_cons(str, result);
    }
    evaluator_.register_value("argv", result);
}

void Session::set_input_stream(std::istream& in) {
    input_stream_ = &in;
    // Build a lazy list of stdin chars.  We use a shared_ptr to the stream
    // and build the list eagerly here (since the stream must be available).
    // For true laziness this would be a thunk chain; a full eager read is
    // acceptable for file-redirect use cases.
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    ValRef list = make_nil();
    for (auto it = content.rbegin(); it != content.rend(); ++it) {
        list = make_cons(make_char(*it), list);
    }
    evaluator_.register_value("input", list);
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

// ---------------------------------------------------------------------------
// run_interactive
// ---------------------------------------------------------------------------

void Session::run_interactive(std::istream& in, std::ostream& out) {
    // Version banner.
    out << "Hope C++20 interpreter  (type :quit to exit)\n";

    // Keep a last-loaded file path for :reload.
    std::string last_loaded_file;

    // Input accumulator: Hope declarations can span multiple lines.
    // We accumulate until a top-level semicolon terminates the statement.
    std::string accum;
    int depth = 0;           // paren/bracket nesting depth
    bool in_string  = false; // inside a string literal
    bool in_char    = false; // inside a char literal
    bool in_comment = false; // inside a ! line comment

    auto prompt = [&]() {
        if (accum.empty()) out << "hope> " << std::flush;
        else               out << "...   " << std::flush;
    };

    // Process whatever is in `accum` as a snippet.
    auto flush_accum = [&]() {
        if (accum.empty()) return;
        std::string code = std::move(accum);
        accum.clear();
        depth = 0; in_string = false; in_char = false; in_comment = false;
        run_string(code, "<interactive>", out);
    };

    // Trim leading/trailing whitespace from a string view for meta-command
    // matching.
    auto trim = [](const std::string& s) -> std::string {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return {};
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    };

    prompt();
    std::string line;
    while (std::getline(in, line)) {
        // Check for a meta-command on its own line (starts with ':').
        std::string trimmed = trim(line);
        if (accum.empty() && !trimmed.empty() && trimmed[0] == ':') {
            if (trimmed == ":quit" || trimmed == ":exit" || trimmed == ":q") {
                out << "Goodbye.\n";
                return;
            } else if (trimmed == ":clear") {
                // Reset session by creating a fresh Session is heavy; instead
                // just clear display records and note to user.
                display_records_.clear();
                loaded_files_.clear();
                loaded_modules_.clear();
                out << "Session cleared (definitions remain; type :quit to fully restart).\n";
            } else if (trimmed == ":display") {
                out << "\n";
                for (const auto& rec : display_records_) out << rec << "\n";
            } else if (trimmed.substr(0, 5) == ":load") {
                std::string path = trim(trimmed.substr(5));
                if (path.empty()) {
                    out << "Usage: :load <file.hop>\n";
                } else {
                    last_loaded_file = path;
                    // Allow re-loading (remove from loaded set first).
                    try {
                        std::string abs = std::filesystem::absolute(path).string();
                        loaded_files_.erase(abs);
                    } catch (...) {
                        loaded_files_.erase(path);
                    }
                    if (!run_file(path, out)) {
                        out << "Error: could not open " << path << "\n";
                    }
                }
            } else if (trimmed == ":reload") {
                if (last_loaded_file.empty()) {
                    out << "No file loaded yet.\n";
                } else {
                    try {
                        std::string abs = std::filesystem::absolute(last_loaded_file).string();
                        loaded_files_.erase(abs);
                    } catch (...) {
                        loaded_files_.erase(last_loaded_file);
                    }
                    if (!run_file(last_loaded_file, out)) {
                        out << "Error: could not open " << last_loaded_file << "\n";
                    }
                }
            } else if (trimmed.substr(0, 5) == ":type") {
                std::string expr_src = trim(trimmed.substr(5));
                if (expr_src.empty()) {
                    out << "Usage: :type <expr>\n";
                } else {
                    // Wrap in a temporary eval so the parser sees it.
                    run_string("!type query\n" + expr_src + ";", "<:type>", out);
                }
            } else if (trimmed == ":help" || trimmed == ":?") {
                out << "Meta-commands:\n"
                    << "  :load <file>  load a .hop file\n"
                    << "  :reload       reload last loaded file\n"
                    << "  :type <expr>  show type of expression\n"
                    << "  :display      list session definitions\n"
                    << "  :clear        clear session history\n"
                    << "  :quit / :exit exit the REPL\n";
            } else {
                out << "Unknown command: " << trimmed
                    << "  (type :help for help)\n";
            }
            prompt();
            continue;
        }

        // Append the line to the accumulator, scanning for statement endings.
        // A statement ends at a top-level ';' (not inside a string/comment).
        for (char ch : line) {
            if (in_comment) {
                // Nothing after '!' on the same line is significant.
                accum += ch;
                continue;
            }
            if (in_string) {
                accum += ch;
                if (ch == '"') in_string = false;
                continue;
            }
            if (in_char) {
                accum += ch;
                if (ch == '\'') in_char = false;
                continue;
            }
            if (ch == '!') { in_comment = true; accum += ch; continue; }
            if (ch == '"') { in_string  = true;  accum += ch; continue; }
            if (ch == '\'') { in_char   = true;  accum += ch; continue; }
            if (ch == '(' || ch == '[') { ++depth; accum += ch; continue; }
            if (ch == ')' || ch == ']') {
                if (depth > 0) --depth;
                accum += ch;
                continue;
            }
            if (ch == ';' && depth == 0) {
                accum += ch;
                flush_accum();
                prompt();
                continue;
            }
            accum += ch;
        }
        // End-of-line resets comment and char state.
        in_comment = false;
        in_char    = false;
        // If the line ended mid-statement, add a newline and keep going.
        if (!accum.empty()) {
            accum += '\n';
            prompt();
        } else {
            prompt();
        }
    }

    // Process any trailing input without a final semicolon.
    if (!accum.empty()) {
        flush_accum();
    }
}

} // namespace hope
