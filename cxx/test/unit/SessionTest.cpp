// SessionTest.cpp — integration-style tests for Session (Phase 10).
//
// These tests exercise the full session pipeline: stdlib loading, user
// definitions via run_string, and round-trip save/load via save.

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "repl/Session.hpp"

namespace hope {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr const char* kLibDir =
#ifdef HOPE_LIB_DIR
    HOPE_LIB_DIR;
#else
    "";
#endif

// RAII: change directory on construction, restore on destruction.
struct ScopedChdir {
    std::filesystem::path saved;
    explicit ScopedChdir(const std::filesystem::path& dir) {
        saved = std::filesystem::current_path();
        std::filesystem::current_path(dir);
    }
    ~ScopedChdir() {
        std::error_code ec;
        std::filesystem::current_path(saved, ec);
    }
};

// ---------------------------------------------------------------------------
// Sieve of Eratosthenes — save / load round-trip
//
// The test injects a finite Sieve of Eratosthenes into a Hope session, uses
// save to write the definitions to a file, then loads that file in a fresh
// session and re-evaluates the sieve — verifying identical results.
//
// The sieve uses `list` (front, filter) and `range` (..) from the standard
// library.  `save` only persists user declarations (dec + equations); the
// reload session must re-declare those modules itself.
// ---------------------------------------------------------------------------

// Sieve definitions, self-contained (no uses).
// filter and front come from list.hop; .. from range.hop — both must be in scope.
static const char* kSieveDefs = R"(
dec nonmultiple : num -> num -> bool;
--- nonmultiple p n <= n mod p /= 0;
dec sieve : list num -> list num;
--- sieve [] <= [];
--- sieve (p :: rest) <= p :: sieve (filter (nonmultiple p) rest);
)";

// Evaluate the first 10 primes via the sieve over 2..200.
static const char* kSieveExpr = "front (10, sieve (2 .. 200));\n";

// Expected result line (Paterson hope output format).
static const char* kExpected = "[2, 3, 5, 7, 11, 13, 17, 19, 23, 29]";

TEST(SessionSave, SieveRoundTrip) {
    if (std::string(kLibDir).empty()) {
        GTEST_SKIP() << "HOPE_LIB_DIR not set";
    }

    // Work in a temporary directory so save writes there.
    auto tmpdir = std::filesystem::temp_directory_path() / "hope_session_test";
    std::filesystem::create_directories(tmpdir);
    // Cleanup on scope exit.
    struct RmDir {
        std::filesystem::path p;
        ~RmDir() { std::error_code ec; std::filesystem::remove_all(p, ec); }
    } cleanup{tmpdir};

    std::string saved_file = (tmpdir / "sieve_save.hop").string();
    std::string eval_line_original;

    // -------------------------------------------------------------------------
    // Phase 1: define sieve in a session, evaluate it, then save.
    // -------------------------------------------------------------------------
    {
        ScopedChdir cd(tmpdir);

        Session s;
        s.set_lib_dir(kLibDir);
        ASSERT_TRUE(s.load_standard(kLibDir)) << "stdlib not found at " << kLibDir;

        std::ostringstream out;
        // Load list and range modules so filter, front, and .. are available.
        s.run_string("uses list; uses range;", "<test-uses>", out);
        // Inject sieve definitions.
        s.run_string(kSieveDefs, "<test-defs>", out);
        // Evaluate the sieve expression.
        s.run_string(kSieveExpr, "<test-eval>", out);

        std::string output = out.str();
        ASSERT_FALSE(output.empty()) << "sieve produced no output";
        EXPECT_NE(output.find(kExpected), std::string::npos)
            << "expected primes not found in output:\n" << output;

        // Extract the >> result line for later comparison.
        {
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.rfind(">> ", 0) == 0) { eval_line_original = line; break; }
            }
        }
        ASSERT_FALSE(eval_line_original.empty()) << "no >> line in output:\n" << output;

        // Save the session (writes nonmultiple + sieve defs to sieve_save.hop).
        std::ostringstream save_out;
        s.run_string("save sieve_save;", "<test-save>", save_out);
        EXPECT_NE(save_out.str().find("Saved"), std::string::npos)
            << "save command did not print confirmation: " << save_out.str();
    }

    // The saved file must exist and be non-empty.
    ASSERT_TRUE(std::filesystem::exists(saved_file))
        << "sieve_save.hop was not created in " << tmpdir;
    ASSERT_GT(std::filesystem::file_size(saved_file), 0u)
        << "sieve_save.hop is empty";

    // -------------------------------------------------------------------------
    // Phase 2: load the saved file in a fresh session, re-evaluate the sieve.
    // -------------------------------------------------------------------------
    {
        Session s2;
        s2.set_lib_dir(kLibDir);
        ASSERT_TRUE(s2.load_standard(kLibDir));

        std::ostringstream out2;
        // Re-import list/range so filter, front, .. are available for the saved defs.
        s2.run_string("uses list; uses range;", "<test2-uses>", out2);
        // Load the saved sieve definitions.
        ASSERT_TRUE(s2.run_file(saved_file, out2))
            << "run_file failed for " << saved_file;
        // Evaluate the same sieve expression.
        s2.run_string(kSieveExpr, "<test2-eval>", out2);

        std::string output2 = out2.str();
        EXPECT_NE(output2.find(kExpected), std::string::npos)
            << "expected primes not found after reload:\n" << output2;

        // Extract the >> result line and compare.
        std::string eval_line_reloaded;
        {
            std::istringstream iss(output2);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.rfind(">> ", 0) == 0) { eval_line_reloaded = line; break; }
            }
        }
        ASSERT_FALSE(eval_line_reloaded.empty()) << "no >> line after reload:\n" << output2;

        EXPECT_EQ(eval_line_original, eval_line_reloaded)
            << "original:  " << eval_line_original
            << "\nreloaded: " << eval_line_reloaded;
    }
}

} // namespace
} // namespace hope
