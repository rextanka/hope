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
// Functor generation helpers (anonymous namespace)
// ---------------------------------------------------------------------------

namespace {

// Synthetic source location for auto-generated functor code.
static const SourceLocation FUNCTOR_LOC{"<functor>", 0, 0, 0};

// Recursively produce a (pattern, expression) pair for mapping a function f
// over a value of type `tau`.
//
//   type_param : the type variable being mapped (e.g. "alpha")
//   type_name  : the type being defined (for self-recursive references)
//   ctr        : counter used to generate unique variable names (__x0, __x1, ...)
//
// Examples:
//   tau = TVar{"alpha"}            → (PVar{"__x0"}, EApply{EVar{"__f"}, EVar{"__x0"}})
//   tau = TProd{TVar{"alpha"},     → (PTuple{[__x0, __x1]},
//               TCons{"list",...}}      ETuple{[__f __x0, list __f __x1]})
struct PatExprPair { PatPtr pat; ExprPtr expr; };

PatExprPair make_functor_pe(const Type& tau,
                             const std::string& type_param,
                             const std::string& type_name,
                             int& ctr)
{
    return std::visit([&](const auto& v) -> PatExprPair {
        using V = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<V, TVar>) {
            std::string var = "__x" + std::to_string(ctr++);
            PatPtr  p = make_pat(PVar{var}, FUNCTOR_LOC);
            ExprPtr e;
            if (v.name == type_param) {
                // Apply f to this position
                e = make_expr(
                    EApply{make_expr(EVar{"__f"}, FUNCTOR_LOC),
                           make_expr(EVar{var},   FUNCTOR_LOC)},
                    FUNCTOR_LOC);
            } else {
                e = make_expr(EVar{var}, FUNCTOR_LOC);
            }
            return {std::move(p), std::move(e)};
        }

        if constexpr (std::is_same_v<V, TProd>) {
            auto [pl, el] = make_functor_pe(*v.left,  type_param, type_name, ctr);
            auto [pr, er] = make_functor_pe(*v.right, type_param, type_name, ctr);
            std::vector<PatPtr>  pats;  pats.push_back(std::move(pl));  pats.push_back(std::move(pr));
            std::vector<ExprPtr> exprs; exprs.push_back(std::move(el)); exprs.push_back(std::move(er));
            return {
                make_pat(PTuple{std::move(pats)},   FUNCTOR_LOC),
                make_expr(ETuple{std::move(exprs)},  FUNCTOR_LOC)
            };
        }

        if constexpr (std::is_same_v<V, TCons>) {
            std::string var = "__x" + std::to_string(ctr++);
            PatPtr  p = make_pat(PVar{var}, FUNCTOR_LOC);
            ExprPtr e;
            if (v.name == type_name) {
                // Self-reference: apply T __f recursively
                e = make_expr(
                    EApply{
                        make_expr(EApply{make_expr(EVar{type_name}, FUNCTOR_LOC),
                                         make_expr(EVar{"__f"}, FUNCTOR_LOC)},
                                  FUNCTOR_LOC),
                        make_expr(EVar{var}, FUNCTOR_LOC)},
                    FUNCTOR_LOC);
            } else {
                // Other type constructor: identity
                e = make_expr(EVar{var}, FUNCTOR_LOC);
            }
            return {std::move(p), std::move(e)};
        }

        if constexpr (std::is_same_v<V, TFun>) {
            // Function type: identity (not mapped)
            std::string var = "__x" + std::to_string(ctr++);
            return {make_pat(PVar{var}, FUNCTOR_LOC),
                    make_expr(EVar{var}, FUNCTOR_LOC)};
        }

        // TMu: unfold one level and recurse
        if constexpr (std::is_same_v<V, TMu>) {
            return make_functor_pe(*v.body, type_param, type_name, ctr);
        }

        // Unreachable — all five Type alternatives covered above.
        return {make_pat(PWild{}, FUNCTOR_LOC), make_expr(EVar{"_"}, FUNCTOR_LOC)};
    }, tau.data);
}

// Build "dec T : (alpha -> beta) -> T alpha -> T beta;"
Decl make_functor_dec(const std::string& type_name,
                       const std::string& type_param)
{
    // Build the type:  (alpha -> beta) -> T alpha -> T beta
    // T alpha
    std::vector<TypePtr> a_args;
    a_args.push_back(make_type(TVar{type_param}, FUNCTOR_LOC));
    auto t_a = make_type(TCons{type_name, std::move(a_args)}, FUNCTOR_LOC);
    // T beta  (using __beta to avoid clashing with user's own "beta" typevar)
    std::vector<TypePtr> b_args;
    b_args.push_back(make_type(TVar{"__beta"}, FUNCTOR_LOC));
    auto t_b = make_type(TCons{type_name, std::move(b_args)}, FUNCTOR_LOC);
    // alpha -> __beta
    auto f_ty = make_type(TFun{make_type(TVar{type_param}, FUNCTOR_LOC),
                                make_type(TVar{"__beta"}, FUNCTOR_LOC)}, FUNCTOR_LOC);
    // (alpha -> __beta) -> T alpha -> T __beta
    auto functor_ty = make_type(
        TFun{std::move(f_ty),
             make_type(TFun{std::move(t_a), std::move(t_b)}, FUNCTOR_LOC)},
        FUNCTOR_LOC);
    DDec dec;
    dec.names = {type_name};
    dec.type  = std::move(functor_ty);
    return Decl{std::move(dec), FUNCTOR_LOC};
}

// Generate functor declarations for a data type.
// Returns [DDec, DEquation, DEquation, ...] — one equation per constructor.
std::vector<Decl> functor_decls_for_data(const DData& d)
{
    if (d.params.size() != 1) return {};          // only 1-param types get functors
    const std::string& type_name  = d.name;
    const std::string& type_param = d.params[0];

    std::vector<Decl> result;
    result.push_back(make_functor_dec(type_name, type_param));

    int ctr = 0;
    for (const auto& con : d.alts) {
        DEquation eq;
        eq.lhs.func     = type_name;
        eq.lhs.is_infix = false;
        // First argument pattern: the mapping function
        eq.lhs.args.push_back(make_pat(PVar{"__f"}, FUNCTOR_LOC));

        if (!con.arg.has_value()) {
            // Nullary constructor C: --- T __f C <= C
            eq.lhs.args.push_back(
                make_pat(PCons{con.name, std::nullopt}, FUNCTOR_LOC));
            eq.rhs = make_expr(EVar{con.name}, FUNCTOR_LOC);
        } else {
            // Unary constructor C(tau): --- T __f (C pat) <= C expr
            auto [arg_pat, arg_expr] =
                make_functor_pe(**con.arg, type_param, type_name, ctr);
            std::optional<PatPtr> opt_arg =
                std::make_optional(std::move(arg_pat));
            eq.lhs.args.push_back(
                make_pat(PCons{con.name, std::move(opt_arg)}, FUNCTOR_LOC));
            eq.rhs = make_expr(
                EApply{make_expr(EVar{con.name}, FUNCTOR_LOC),
                       std::move(arg_expr)},
                FUNCTOR_LOC);
        }
        result.push_back(Decl{std::move(eq), FUNCTOR_LOC});
    }
    return result;
}

// Generate functor declarations for a type synonym.
// Returns [DDec, DEquation].
std::vector<Decl> functor_decls_for_type(const DType& d)
{
    if (d.params.size() != 1 || !d.body) return {};
    const std::string& type_name  = d.name;
    const std::string& type_param = d.params[0];

    std::vector<Decl> result;
    result.push_back(make_functor_dec(type_name, type_param));

    int ctr = 0;
    auto [body_pat, body_expr] =
        make_functor_pe(*d.body, type_param, type_name, ctr);

    DEquation eq;
    eq.lhs.func     = type_name;
    eq.lhs.is_infix = false;
    eq.lhs.args.push_back(make_pat(PVar{"__f"}, FUNCTOR_LOC));
    eq.lhs.args.push_back(std::move(body_pat));
    eq.rhs = std::move(body_expr);
    result.push_back(Decl{std::move(eq), FUNCTOR_LOC});
    return result;
}

} // anonymous namespace

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
        // Re-sync the parser's operator table after each declaration.
        // This propagates operators from `uses`-loaded modules back into the
        // outer parser so that subsequent declarations in this file can use them.
        parser.op_table() = ops_;
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
        // When loading a module, record abstract type declarations so that
        // their names can be "sealed" back to abstract after the module load
        // completes (the module's private section may define a type synonym
        // for the representation, which must not escape the module).
        if (in_silent_load_ && std::holds_alternative<DAbsType>(d.data)) {
            const auto& abs = std::get<DAbsType>(d.data);
            std::visit([&](const auto& v) {
                using V = std::decay_t<decltype(v)>;
                AbstypeRecord rec;
                if constexpr (std::is_same_v<V, TCons>) {
                    rec.name = v.name;
                    for (const auto& a : v.args) {
                        if (auto* tv = std::get_if<TVar>(&a->data))
                            rec.params.push_back(tv->name);
                    }
                    module_abstypes_.push_back(std::move(rec));
                } else if constexpr (std::is_same_v<V, TVar>) {
                    rec.name = v.name;
                    module_abstypes_.push_back(std::move(rec));
                }
            }, abs.type->data);
        }
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
    // For DData and DType, also type-check and generate functors.
    bool is_data = std::holds_alternative<DData>(d.data);
    bool is_type = std::holds_alternative<DType>(d.data);

    // Build functor declarations BEFORE moving d into the evaluator.
    std::vector<Decl> functors;
    if (is_data) {
        functors = functor_decls_for_data(std::get<DData>(d.data));
    } else if (is_type) {
        functors = functor_decls_for_type(std::get<DType>(d.data));
    }

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

    // Register the auto-generated functor: type-check then eval.
    // Done silently (no display recording, no output).
    for (auto& fd : functors) {
        try { type_checker_.check_decl(fd); } catch (...) {}
        if (std::holds_alternative<DEquation>(fd.data)) {
            try { evaluator_.add_decl(std::move(fd)); } catch (...) {}
        }
    }
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
    // Save and clear module_abstypes_ so that abstypes declared by THIS module
    // (and not by any parent module still being loaded) are tracked separately.
    auto outer_abstypes = std::move(module_abstypes_);
    module_abstypes_.clear();

    bool old_flag = in_silent_load_;
    in_silent_load_ = true;
    std::ostringstream sink;
    run_file(path, sink);
    in_silent_load_ = old_flag;

    // Seal abstract types: if the module's private section defined a type
    // synonym `type T alpha == rep` for an `abstype T alpha` declared in the
    // public section, that synonym has now been registered in TypeEnv.  Overwrite
    // it back to abstract so the representation does not leak outside the module.
    for (const auto& rec : module_abstypes_) {
        TypeDef td;
        td.name   = rec.name;
        td.params = rec.params;
        td.def    = std::monostate{};
        type_env_.add_typedef(std::move(td));
    }

    // Restore the parent module's accumulator.
    module_abstypes_ = std::move(outer_abstypes);
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
