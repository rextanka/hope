// Session.cpp — Hope interpreter session (batch/file mode).

#include "repl/Session.hpp"
#include "repl/LineEditor.hpp"

#include <chrono>
#include <cstdlib>
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

// Recursively produce a (pattern, expression) pair for mapping functions over
// a value of type `tau`.
//
//   type_params : the type variables being mapped, e.g. ["alpha", "beta"]
//                 param[i] is mapped by function variable "__f_<i>"
//   type_name   : the type being defined (for self-recursive references)
//   ctr         : counter used to generate unique value variables (__x0, __x1, ...)
//
// For a 1-param type the single function is named "__f_0".
// For 2-param types they are "__f_0" and "__f_1".
struct PatExprPair { PatPtr pat; ExprPtr expr; };

PatExprPair make_functor_pe(const Type& tau,
                             const std::vector<std::string>& type_params,
                             const std::string& type_name,
                             int& ctr)
{
    return std::visit([&](const auto& v) -> PatExprPair {
        using V = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<V, TVar>) {
            std::string var = "__x" + std::to_string(ctr++);
            PatPtr  p = make_pat(PVar{var}, FUNCTOR_LOC);
            ExprPtr e;
            // Find which function argument to apply (if any).
            auto it = std::find(type_params.begin(), type_params.end(), v.name);
            if (it != type_params.end()) {
                std::string fn = "__f_" + std::to_string(
                    static_cast<int>(it - type_params.begin()));
                e = make_expr(
                    EApply{make_expr(EVar{fn},  FUNCTOR_LOC),
                           make_expr(EVar{var}, FUNCTOR_LOC)},
                    FUNCTOR_LOC);
            } else {
                e = make_expr(EVar{var}, FUNCTOR_LOC);
            }
            return {std::move(p), std::move(e)};
        }

        if constexpr (std::is_same_v<V, TProd>) {
            auto [pl, el] = make_functor_pe(*v.left,  type_params, type_name, ctr);
            auto [pr, er] = make_functor_pe(*v.right, type_params, type_name, ctr);
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
                // Self-reference: apply T __f_0 __f_1 ... recursively.
                ExprPtr app = make_expr(EVar{type_name}, FUNCTOR_LOC);
                for (size_t i = 0; i < type_params.size(); ++i) {
                    std::string fn = "__f_" + std::to_string(i);
                    app = make_expr(
                        EApply{std::move(app),
                               make_expr(EVar{fn}, FUNCTOR_LOC)},
                        FUNCTOR_LOC);
                }
                e = make_expr(
                    EApply{std::move(app), make_expr(EVar{var}, FUNCTOR_LOC)},
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
            return make_functor_pe(*v.body, type_params, type_name, ctr);
        }

        // Unreachable — all five Type alternatives covered above.
        return {make_pat(PWild{}, FUNCTOR_LOC), make_expr(EVar{"_"}, FUNCTOR_LOC)};
    }, tau.data);
}

// Build the functor type declaration for an n-parameter type.
//
// For n=1 (params = ["alpha"]):
//   dec T : (alpha -> __r_0) -> T alpha -> T __r_0;
//
// For n=2 (params = ["alpha", "beta"]):
//   dec T : (alpha -> __r_0) -> (beta -> __r_1) -> T alpha beta -> T __r_0 __r_1;
Decl make_functor_dec(const std::string& type_name,
                       const std::vector<std::string>& params)
{
    // Build T params (source)
    std::vector<TypePtr> src_args;
    for (const auto& p : params)
        src_args.push_back(make_type(TVar{p}, FUNCTOR_LOC));
    auto t_src = make_type(TCons{type_name, std::move(src_args)}, FUNCTOR_LOC);

    // Build T __r_0 __r_1 ... (target)
    std::vector<TypePtr> tgt_args;
    for (size_t i = 0; i < params.size(); ++i)
        tgt_args.push_back(make_type(TVar{"__r_" + std::to_string(i)}, FUNCTOR_LOC));
    auto t_tgt = make_type(TCons{type_name, std::move(tgt_args)}, FUNCTOR_LOC);

    // Build (T src -> T tgt) — the result of applying all function args
    TypePtr result_ty = make_type(TFun{std::move(t_src), std::move(t_tgt)}, FUNCTOR_LOC);

    // Prepend (params[i] -> __r_i) -> ... from right to left
    for (int i = static_cast<int>(params.size()) - 1; i >= 0; --i) {
        auto f_ty = make_type(
            TFun{make_type(TVar{params[i]}, FUNCTOR_LOC),
                 make_type(TVar{"__r_" + std::to_string(i)}, FUNCTOR_LOC)},
            FUNCTOR_LOC);
        result_ty = make_type(TFun{std::move(f_ty), std::move(result_ty)}, FUNCTOR_LOC);
    }

    DDec dec;
    dec.names = {type_name};
    dec.type  = std::move(result_ty);
    return Decl{std::move(dec), FUNCTOR_LOC};
}

// Generate functor declarations for a data type.
// Returns [DDec, DEquation, DEquation, ...] — one equation per constructor.
std::vector<Decl> functor_decls_for_data(const DData& d)
{
    if (d.params.empty()) return {};              // nullary types have no functor
    const std::string& type_name = d.name;

    std::vector<Decl> result;
    result.push_back(make_functor_dec(type_name, d.params));

    int ctr = 0;
    for (const auto& con : d.alts) {
        DEquation eq;
        eq.lhs.func     = type_name;
        eq.lhs.is_infix = false;
        // One argument pattern per type parameter: __f_0, __f_1, ...
        for (size_t i = 0; i < d.params.size(); ++i)
            eq.lhs.args.push_back(make_pat(PVar{"__f_" + std::to_string(i)}, FUNCTOR_LOC));

        if (!con.arg.has_value()) {
            // Nullary constructor C: --- T __f_0 ... C <= C
            eq.lhs.args.push_back(
                make_pat(PCons{con.name, std::nullopt}, FUNCTOR_LOC));
            eq.rhs = make_expr(EVar{con.name}, FUNCTOR_LOC);
        } else {
            // Unary constructor C(tau): --- T __f_0 ... (C pat) <= C expr
            auto [arg_pat, arg_expr] =
                make_functor_pe(**con.arg, d.params, type_name, ctr);
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
    if (d.params.empty() || !d.body) return {};
    const std::string& type_name = d.name;

    std::vector<Decl> result;
    result.push_back(make_functor_dec(type_name, d.params));

    int ctr = 0;
    auto [body_pat, body_expr] =
        make_functor_pe(*d.body, d.params, type_name, ctr);

    DEquation eq;
    eq.lhs.func     = type_name;
    eq.lhs.is_infix = false;
    for (size_t i = 0; i < d.params.size(); ++i)
        eq.lhs.args.push_back(make_pat(PVar{"__f_" + std::to_string(i)}, FUNCTOR_LOC));
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

    // Track the last user-loaded file (not standard library / module loads).
    if (!in_silent_load_)
        last_loaded_file_ = abs_path;

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
    // Seed the parser with constructors from prior data declarations so that
    // lowercase constructor names (e.g. `node`, `onode`) are recognised in
    // patterns even when defined in a previous run_string call.
    for (const auto& name : known_constructors_)
        parser.register_constructor(name);

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

    // Merge any new operator/constructor declarations back into our accumulated state.
    ops_ = parser.op_table();
    for (const auto& name : parser.known_constructors())
        known_constructors_.insert(name);
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

    // Handle DPrivate: everything declared after this point in a module load
    // is private and will be hidden from external callers after the load.
    if (std::holds_alternative<DPrivate>(d.data)) {
        if (in_silent_load_) in_private_section_ = true;
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

    // Handle DSave: write display_records_ to module_name.hop.
    if (auto* save_alt = std::get_if<DSave>(&d.data)) {
        std::string filename = save_alt->module_name + ".hop";
        std::ofstream f(filename);
        if (!f) {
            if (!in_silent_load_)
                out << "Error: cannot write to " << filename << "\n";
            return;
        }
        for (const auto& rec : display_records_)
            f << rec << "\n";
        if (!in_silent_load_)
            out << "Saved to " << filename << ".\n";
        return;
    }

    // Handle DEdit: open editor on named module (or last loaded file / temp).
    if (auto* edit_alt = std::get_if<DEdit>(&d.data)) {
        if (!in_silent_load_)
            run_edit(edit_alt->module_name, out);
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
        // Track private function names so we can hide them after module load.
        if (in_silent_load_ && in_private_section_) {
            const auto& dec = std::get<DDec>(d.data);
            for (const auto& n : dec.names)
                module_private_names_.push_back(n);
        }
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
    // Save and clear module_abstypes_ / module_private_names_ / in_private_section_
    // so that each module's private section is tracked independently.
    auto outer_abstypes      = std::move(module_abstypes_);
    auto outer_private_names = std::move(module_private_names_);
    bool outer_private_flag  = in_private_section_;
    module_abstypes_.clear();
    module_private_names_.clear();
    in_private_section_ = false;

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

    // Hide private function declarations: remove their FuncDecl entries from
    // TypeEnv so that code outside the module cannot reference them by name.
    // The evaluator closures still hold references to the private functions,
    // so they continue to work when called indirectly from public functions.
    for (const auto& name : module_private_names_) {
        type_env_.remove_funcdecl(name);
    }

    // Restore the parent module's accumulators.
    module_abstypes_      = std::move(outer_abstypes);
    module_private_names_ = std::move(outer_private_names);
    in_private_section_   = outer_private_flag;
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
        type_str = print_type(t, ops_);
    } catch (...) {
        type_str = "_";
    }

    out << ">> " << val_str << " : " << type_str << "\n";
}

// ---------------------------------------------------------------------------
// run_edit
// ---------------------------------------------------------------------------

void Session::run_edit(const std::string& module_name, std::ostream& out) {
    std::string filepath;
    bool is_temp = false;

    if (!module_name.empty()) {
        // Locate the named module file.
        std::string candidate = module_name + ".hop";
        if (std::filesystem::exists(candidate)) {
            filepath = candidate;
        } else if (!lib_dir_.empty()) {
            std::string lib_candidate = lib_dir_ + "/" + candidate;
            if (std::filesystem::exists(lib_candidate))
                filepath = lib_candidate;
        }
        if (filepath.empty()) {
            out << "edit: module '" << module_name << "' not found.\n";
            return;
        }
    } else if (!last_loaded_file_.empty()) {
        // No module named: edit the last loaded file.
        filepath = last_loaded_file_;
    } else {
        // No file context: write current definitions to a temp file.
        filepath = "hope_edit_" + std::to_string(
                       std::hash<std::string>{}(std::to_string(
                           std::chrono::steady_clock::now().time_since_epoch().count()))
                       % 100000) + ".hop";
        is_temp = true;
        std::ofstream f(filepath);
        for (const auto& rec : display_records_)
            f << rec << "\n";
    }

    // Determine editor.
    // Priority: HOPE_EDITOR > EDITOR > auto-detect VS Code.
    const char* hope_editor_env = std::getenv("HOPE_EDITOR");
    const char* editor_env      = std::getenv("EDITOR");
    std::string editor;

    if (hope_editor_env && *hope_editor_env) {
        // User explicitly chose an editor for Hope.
        editor = hope_editor_env;
    } else if (editor_env && *editor_env) {
        // Fall back to the shell default editor.
        editor = editor_env;
    } else {
        // Auto-detect VS Code.
#if defined(_WIN32)
        if (std::system("where code >nul 2>&1") == 0) editor = "code";
#else
        if (std::system("command -v code >/dev/null 2>&1") == 0)         editor = "code";
        else if (std::system("command -v code-insiders >/dev/null 2>&1") == 0) editor = "code-insiders";
#endif
    }

    if (editor.empty()) {
        out << "edit: no editor configured.\n"
            << "  Set HOPE_EDITOR (or EDITOR) to the command for your preferred editor, e.g.:\n"
            << "    export HOPE_EDITOR=\"code --wait\"    # VS Code\n"
            << "    export HOPE_EDITOR=nano              # terminal editor\n"
            << "    export HOPE_EDITOR=vim\n"
            << "  See the user guide (cxx/doc/user-guide.md) for details.\n";
        if (is_temp) {
            try { std::filesystem::remove(filepath); } catch (...) {}
        }
        return;
    }

    // VS Code and code-insiders need --wait to block until the tab is closed.
    if ((editor.find("code") != std::string::npos ||
         editor.find("code-insiders") != std::string::npos) &&
        editor.find("--wait") == std::string::npos) {
        editor += " --wait";
    }

    // Launch editor (blocks until exit for terminal editors and code --wait).
    std::string cmd = editor + " \"" + filepath + "\"";
    std::system(cmd.c_str());

    // Reload: remove from loaded set so run_file accepts it.
    try {
        loaded_files_.erase(std::filesystem::absolute(filepath).string());
    } catch (...) {
        loaded_files_.erase(filepath);
    }
    last_loaded_file_ = filepath;
    if (!run_file(filepath, out))
        out << "edit: could not reload '" << filepath << "'\n";

    // Clean up temp file.
    if (is_temp) {
        try { std::filesystem::remove(filepath); } catch (...) {}
    }
}

// ---------------------------------------------------------------------------
// run_interactive
// ---------------------------------------------------------------------------

void Session::run_interactive(std::istream& in, std::ostream& out) {
    // Version banner.
    out << "Hope C++20 interpreter  (type :quit to exit)\n";

    // Input accumulator: Hope declarations can span multiple lines.
    // We accumulate until a top-level semicolon terminates the statement.
    std::string accum;
    int depth = 0;           // paren/bracket nesting depth
    bool in_string  = false; // inside a string literal
    bool in_char    = false; // inside a char literal
    bool in_comment = false; // inside a ! line comment

    // Statement accumulator for history: physical lines of the current
    // in-progress statement, joined so a complete statement is one entry.
    std::string stmt_for_history;

    // Use LineEditor (raw TTY with history/editing) only when reading from the
    // real stdin.  Test code and piped input inject a different istream, so we
    // fall back to plain std::getline in that case.
    const bool use_editor = (&in == &std::cin);
    LineEditor editor;

    auto prompt_str = [&]() -> std::string {
        return accum.empty() ? "hope> " : "...   ";
    };

    // Read one physical line, displaying the prompt.
    auto read_line = [&]() -> std::optional<std::string> {
        if (use_editor) {
            return editor.read_line(prompt_str());
        } else {
            out << prompt_str() << std::flush;
            std::string line;
            if (!std::getline(in, line)) return std::nullopt;
            return line;
        }
    };

    // Process whatever is in `accum` as a snippet.
    auto flush_accum = [&](bool add_to_history) {
        if (accum.empty()) return;
        if (use_editor && add_to_history && !stmt_for_history.empty())
            editor.add_history(stmt_for_history);
        stmt_for_history.clear();
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

    while (true) {
        auto maybe_line = read_line();
        if (!maybe_line) break;  // EOF / Ctrl-D
        std::string line = std::move(*maybe_line);
        // Check for a meta-command on its own line (starts with ':').
        std::string trimmed = trim(line);
        if (accum.empty() && !trimmed.empty() && trimmed[0] == ':') {
            // Meta-commands go into history so the user can recall them.
            if (use_editor) editor.add_history(trimmed);
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
                if (last_loaded_file_.empty()) {
                    out << "No file loaded yet.\n";
                } else {
                    try {
                        std::string abs = std::filesystem::absolute(last_loaded_file_).string();
                        loaded_files_.erase(abs);
                    } catch (...) {
                        loaded_files_.erase(last_loaded_file_);
                    }
                    if (!run_file(last_loaded_file_, out)) {
                        out << "Error: could not open " << last_loaded_file_ << "\n";
                    }
                }
            } else if (trimmed.substr(0, 5) == ":edit") {
                std::string mod = trim(trimmed.substr(5));
                run_edit(mod, out);
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
                    << "  :load <file>      load a .hop file\n"
                    << "  :reload           reload last loaded file\n"
                    << "  :edit [module]    open file in $EDITOR then reload\n"
                    << "  :type <expr>      show type of expression\n"
                    << "  :display          list session definitions\n"
                    << "  :clear            clear session history\n"
                    << "  :quit / :exit     exit the REPL\n";
            } else {
                out << "Unknown command: " << trimmed
                    << "  (type :help for help)\n";
            }
            continue;
        }

        // Accumulate the physical line for history tracking.
        if (!stmt_for_history.empty()) stmt_for_history += '\n';
        stmt_for_history += line;

        // Append the line to the accumulator, scanning for statement endings.
        // A statement ends at a top-level ';' (not inside a string/comment).
        bool statement_completed = false;
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
                flush_accum(/*add_to_history=*/true);
                stmt_for_history.clear();
                statement_completed = true;
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
        }
        (void)statement_completed;
    }

    // Process any trailing input without a final semicolon.
    if (!accum.empty()) {
        flush_accum(/*add_to_history=*/false);
    }
}

} // namespace hope
