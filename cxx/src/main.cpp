#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "repl/Session.hpp"

int main(int argc, char* argv[]) {
    // Determine the library directory.
    // Priority: HOPEPATH environment variable > executable-relative ../lib.
    std::string lib_dir;
    if (const char* hopepath = std::getenv("HOPEPATH")) {
        lib_dir = hopepath;
    } else {
        try {
            auto exe = std::filesystem::canonical(argv[0]);
            lib_dir = (exe.parent_path().parent_path() / "lib").string();
        } catch (...) {
            lib_dir = ".";
        }
    }

    // Parse arguments: hope -f <file.hop>
    std::string input_file;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-f" && i + 1 < argc) {
            input_file = argv[++i];
        }
    }

    if (input_file.empty()) {
        std::cerr << "Usage: hope -f <file.hop>\n";
        return 1;
    }

    hope::Session session;
    session.set_lib_dir(lib_dir);

    // Load the standard library first.
    if (!session.load_standard(lib_dir)) {
        std::cerr << "Warning: Standard.hop not found in " << lib_dir << "\n";
    }

    // Run the input file, printing results to stdout.
    if (!session.run_file(input_file, std::cout)) {
        std::cerr << "Error: could not open " << input_file << "\n";
        return 1;
    }

    return 0;
}
