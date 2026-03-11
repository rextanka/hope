// ModuleLoader.cpp — loads .hop files and registers their declarations.

#include "modules/ModuleLoader.hpp"
#include "runtime/Evaluator.hpp"
#include "runtime/RuntimeError.hpp"
#include "parser/OperatorTable.hpp"
#include "parser/Parser.hpp"
#include "lexer/Lexer.hpp"

#include <cstdlib>      // getenv
#include <filesystem>
#include <fstream>
#include <sstream>

namespace hope {

namespace {

// Resolve a module name to an absolute path in lib_dir.
std::string resolve_module(const std::string& lib_dir,
                           const std::string& module_name) {
    // Try <lib_dir>/<Name>.hop  (exact case)
    std::string path1 = lib_dir + "/" + module_name + ".hop";
    if (std::filesystem::exists(path1)) return path1;

    // Try with lower-case first letter
    if (!module_name.empty()) {
        std::string lower_name = module_name;
        lower_name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower_name[0])));
        std::string path2 = lib_dir + "/" + lower_name + ".hop";
        if (std::filesystem::exists(path2)) return path2;
    }

    return ""; // not found
}

} // anonymous namespace

ModuleLoader::ModuleLoader(Evaluator& eval, OperatorTable& ops)
    : eval_(eval), ops_(ops)
{
    // Determine the library directory.
    // Priority: environment variable > compile-time constant.
    const char* env_dir = std::getenv("HOPE_LIB_DIR");
    if (env_dir && *env_dir) {
        lib_dir_ = env_dir;
    } else {
#ifdef HOPELIB
        lib_dir_ = HOPELIB;
#else
        lib_dir_ = ".";
#endif
    }
}

void ModuleLoader::load(const std::string& module_name) {
    std::string path = resolve_module(lib_dir_, module_name);
    if (path.empty()) {
        throw RuntimeError("module not found: " + module_name +
                           " (searched in: " + lib_dir_ + ")",
                           SourceLocation{"<module-loader>", 0, 0, 0});
    }
    load_file(path);
}

void ModuleLoader::load_file(const std::string& path) {
    // Normalise to absolute path to prevent double-loading via symlinks etc.
    std::string abs_path;
    try {
        abs_path = std::filesystem::absolute(path).string();
    } catch (...) {
        abs_path = path;
    }

    if (loaded_.count(abs_path)) return; // already loaded
    loaded_.insert(abs_path);

    // Read the file
    std::ifstream file(abs_path);
    if (!file.is_open()) {
        throw RuntimeError("cannot open file: " + path,
                           SourceLocation{"<module-loader>", 0, 0, 0});
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    // Lex and parse: Parser(Lexer) owns the lexer internally.
    // We seed its OperatorTable with our accumulated operator declarations.
    Lexer lexer(source, abs_path);
    Parser parser(std::move(lexer));
    // Copy accumulated operator declarations into the parser's table
    parser.op_table() = ops_;

    auto decls = parser.parse_program();

    // Merge any new operator declarations back into our table
    ops_ = parser.op_table();

    // Process declarations (move into the evaluator to transfer ownership)
    eval_.load_program(std::move(decls));
}

} // namespace hope
