#pragma once

// Session — processes a Hope program (batch or interactive mode).
//
// Owns the TypeEnv, TypeChecker, Evaluator, and OperatorTable.
// Handles loading the standard library, processing declarations, and
// printing evaluated expressions.

#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "modules/ModuleLoader.hpp"
#include "parser/OperatorTable.hpp"
#include "runtime/Evaluator.hpp"
#include "types/TypeChecker.hpp"
#include "types/TypeEnv.hpp"

namespace hope {

class Session {
public:
    Session();

    // Load the standard library (Standard.hop) from the given path.
    // Must be called before processing any user code.
    // Returns true on success, false if Standard.hop is not found.
    bool load_standard(const std::string& lib_dir);

    // Process a .hop file. For each DEval, output is written to `out`.
    // Returns true on success, false if the file is not found.
    bool run_file(const std::string& filepath, std::ostream& out);

    // Process a string of Hope code, writing DEval results to `out`.
    void run_string(const std::string& code, const std::string& source_name,
                    std::ostream& out);

    // Run an interactive REPL loop reading from `in`, writing to `out`.
    // Handles meta-commands (:load, :type, :display, :clear, :quit/:exit).
    // Returns when the user exits.
    void run_interactive(std::istream& in, std::ostream& out);

    // Set the library directory for resolving `uses` directives.
    void set_lib_dir(const std::string& dir) { lib_dir_ = dir; }

    // Set command-line arguments available as `argv` in Hope programs.
    // Must be called before load_standard / run_file.
    void set_argv(const std::vector<std::string>& args);

    // Set the input stream used for the `input` builtin (lazy stdin list).
    // Defaults to std::cin.  Must be called before run_file.
    void set_input_stream(std::istream& in);

private:
    std::string      lib_dir_;
    OperatorTable    ops_;
    // Constructor names accumulated across run_string calls so that each new
    // parser can recognise lowercase constructors from prior data declarations.
    std::unordered_set<std::string> known_constructors_;
    TypeEnv          type_env_;
    TypeChecker      type_checker_;
    Evaluator        evaluator_;
    ModuleLoader     loader_;

    // Tracks files already loaded (by canonical path) to prevent loops.
    std::set<std::string> loaded_files_;

    // Tracks modules already loaded (by module name) to prevent loops.
    std::set<std::string> loaded_modules_;

    // Are we currently loading silently (stdlib / uses module)?
    bool in_silent_load_ = false;

    // Are we past the `private;` separator in the current module load?
    bool in_private_section_ = false;

    // Function names declared in the private section of the current module load.
    std::vector<std::string> module_private_names_;

    // Last file loaded via :load or run_file (for :reload and edit;).
    std::string last_loaded_file_;

    // Input stream for the `input` builtin (default: std::cin).
    std::istream* input_stream_ = &std::cin;

    // Records for the `display;` command: each entry is a pre-formatted
    // string (one or more lines) representing a user declaration.
    std::vector<std::string> display_records_;

    // Abstract type declarations accumulated while loading a module.
    // Used to "seal" abstract types after the module load completes:
    // a module's private section may define `type T alpha == ...` to give
    // the representation of `abstype T alpha`, but that synonym must not
    // be visible outside the module.
    struct AbstypeRecord {
        std::string              name;
        std::vector<std::string> params;
    };
    std::vector<AbstypeRecord> module_abstypes_;

    // Process a single parsed declaration (takes ownership to allow move-into evaluator).
    void process_decl(Decl d, std::ostream& out);

    // Load a module by name, searching lib_dir_.
    void load_module(const std::string& name, std::ostream& out);

    // Open module_name (or a temp file of current defs if empty) in $EDITOR,
    // then reload the file into the session on editor exit.
    void run_edit(const std::string& module_name, std::ostream& out);

    // Format and write one evaluated expression result.
    void print_result(ValRef v, TyRef t, std::ostream& out);
};

} // namespace hope
