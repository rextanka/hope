#pragma once

// Session — processes a Hope program (batch or interactive mode).
//
// Owns the TypeEnv, TypeChecker, Evaluator, and OperatorTable.
// Handles loading the standard library, processing declarations, and
// printing evaluated expressions.

#include <iostream>
#include <set>
#include <string>

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

    // Set the library directory for resolving `uses` directives.
    void set_lib_dir(const std::string& dir) { lib_dir_ = dir; }

private:
    std::string      lib_dir_;
    OperatorTable    ops_;
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

    // Records for the `display;` command: each entry is a pre-formatted
    // string (one or more lines) representing a user declaration.
    std::vector<std::string> display_records_;

    // Process a single parsed declaration (takes ownership to allow move-into evaluator).
    void process_decl(Decl d, std::ostream& out);

    // Load a module by name, searching lib_dir_.
    void load_module(const std::string& name, std::ostream& out);

    // Format and write one evaluated expression result.
    void print_result(ValRef v, TyRef t, std::ostream& out);
};

} // namespace hope
