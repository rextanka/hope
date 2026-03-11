#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>    // isatty, STDIN_FILENO

#include "repl/Session.hpp"

int main(int argc, char* argv[]) {
    // Determine the library directory.
    // Priority:
    //   1. HOPEPATH environment variable
    //   2. HOPELIB compile-time constant (set by CMake to CMAKE_SOURCE_DIR/../lib)
    //   3. Executable-relative fallback
    std::string lib_dir;
    if (const char* hopepath = std::getenv("HOPEPATH")) {
        lib_dir = hopepath;
    } else {
#ifdef HOPELIB
        lib_dir = HOPELIB;
#else
        try {
            auto exe = std::filesystem::canonical(argv[0]);
            lib_dir = (exe.parent_path().parent_path() / "lib").string();
        } catch (...) {
            lib_dir = ".";
        }
#endif
    }

    // Parse arguments: hope [-f <file.hop>]
    std::string input_file;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-f" && i + 1 < argc) {
            input_file = argv[++i];
        }
    }

    hope::Session session;
    session.set_lib_dir(lib_dir);

    // Collect non-flag arguments as Hope's argv.
    std::vector<std::string> hope_args;
    bool skip_next = false;
    for (int i = 1; i < argc; ++i) {
        if (skip_next) { skip_next = false; continue; }
        if (std::string(argv[i]) == "-f") { skip_next = true; continue; }
        hope_args.push_back(argv[i]);
    }
    session.set_argv(hope_args);

    // Load the standard library first.
    if (!session.load_standard(lib_dir)) {
        std::cerr << "Warning: Standard.hop not found in " << lib_dir << "\n";
    }

    if (!input_file.empty()) {
        // Batch mode: run the file.
        if (!session.run_file(input_file, std::cout)) {
            std::cerr << "Error: could not open " << input_file << "\n";
            return 1;
        }
    } else {
        // Interactive mode: REPL if stdin is a TTY, or pipe/redirect otherwise.
        session.run_interactive(std::cin, std::cout);
    }

    return 0;
}
