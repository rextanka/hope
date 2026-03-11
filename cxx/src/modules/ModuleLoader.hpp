#pragma once

// ModuleLoader — loads .hop source files and registers their declarations
// with an Evaluator.
//
// The loader searches for modules in a configurable library directory
// (defaulting to the HOPE_LIB_DIR compile-time constant, or the
// HOPE_LIB_DIR environment variable).
//
// Already-loaded modules are tracked to prevent double-loading.

#include <set>
#include <string>

namespace hope {

class Evaluator;
class OperatorTable;

class ModuleLoader {
public:
    ModuleLoader(Evaluator& eval, OperatorTable& ops);

    // Load a module by name (e.g., "Standard").
    // Searches lib_dir_ for <name>.hop.
    // Throws RuntimeError if the file is not found or cannot be parsed.
    void load(const std::string& module_name);

    // Load a .hop file by absolute or relative path.
    // Throws RuntimeError if the file cannot be opened.
    void load_file(const std::string& path);

    // Set the library directory used when searching for modules by name.
    void set_lib_dir(const std::string& dir) { lib_dir_ = dir; }

    // Return the current library directory.
    const std::string& lib_dir() const { return lib_dir_; }

private:
    Evaluator&      eval_;
    OperatorTable&  ops_;
    std::string     lib_dir_;
    std::set<std::string> loaded_;  // tracks absolute paths already loaded
};

} // namespace hope
