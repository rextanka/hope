// SessionTest.cpp — integration-style tests for Session.
//
// Tests cover:
//   SessionSave    — save command round-trip (sieve of Eratosthenes)
//   SessionRunFile — run_file idempotency (same file loaded only once)
//   SessionRunString — error recovery, write vs eval output, display; command,
//                      write-to-file, type annotations, n+k patterns, functors,
//                      private function hiding
//   SessionInteractive — :display, :type, :clear, :load, :reload, :help,
//                         unknown command, multi-line input accumulation

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

// Build a Session with stdlib loaded.  Skip the test if stdlib is unavailable.
// Returns false (and marks skip) if the lib dir is empty or load fails.
struct LiveSession {
    Session s;
    bool ok = false;

    LiveSession() {
        if (std::string(kLibDir).empty()) return;
        s.set_lib_dir(kLibDir);
        ok = s.load_standard(kLibDir);
    }

    // Run code and return combined output.
    std::string run(const std::string& code,
                    const std::string& name = "<test>") {
        std::ostringstream out;
        s.run_string(code, name, out);
        return out.str();
    }

    // Run the interactive REPL with the given input lines and return output.
    std::string interactive(const std::string& input) {
        std::istringstream in(input);
        std::ostringstream out;
        s.run_interactive(in, out);
        return out.str();
    }
};

// Write text to a temp file and return its path.
std::filesystem::path write_temp_file(const std::filesystem::path& dir,
                                      const std::string& name,
                                      const std::string& contents) {
    auto p = dir / name;
    std::ofstream f(p);
    f << contents;
    return p;
}

// Extract all lines that start with ">> " from output.
std::vector<std::string> eval_lines(const std::string& out) {
    std::vector<std::string> result;
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line))
        if (line.rfind(">> ", 0) == 0) result.push_back(line);
    return result;
}

// Count occurrences of needle in haystack.
int count_occurrences(const std::string& haystack, const std::string& needle) {
    int n = 0;
    for (size_t pos = 0; (pos = haystack.find(needle, pos)) != std::string::npos; pos += needle.size())
        ++n;
    return n;
}

// ---------------------------------------------------------------------------
// SessionSave — save / load round-trip (Sieve of Eratosthenes)
// ---------------------------------------------------------------------------

static const char* kSieveDefs = R"(
dec nonmultiple : num -> num -> bool;
--- nonmultiple p n <= n mod p /= 0;
dec sieve : list num -> list num;
--- sieve [] <= [];
--- sieve (p :: rest) <= p :: sieve (filter (nonmultiple p) rest);
)";
static const char* kSieveExpr   = "front (10, sieve (2 .. 200));\n";
static const char* kSieveResult = "[2, 3, 5, 7, 11, 13, 17, 19, 23, 29]";

TEST(SessionSave, SieveRoundTrip) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    auto tmpdir = std::filesystem::temp_directory_path() / "hope_sieve_test";
    std::filesystem::create_directories(tmpdir);
    struct RmDir { std::filesystem::path p;
                   ~RmDir() { std::error_code ec; std::filesystem::remove_all(p, ec); } } cleanup{tmpdir};

    std::string saved_file = (tmpdir / "sieve_save.hop").string();
    std::string eval_line_original;

    // Phase 1: define, evaluate, save.
    {
        ScopedChdir cd(tmpdir);
        LiveSession ls;
        ASSERT_TRUE(ls.ok) << "stdlib not found";

        ls.run("uses list; uses range;");
        ls.run(kSieveDefs);
        std::string out = ls.run(kSieveExpr);

        ASSERT_NE(out.find(kSieveResult), std::string::npos)
            << "sieve output:\n" << out;

        auto lines = eval_lines(out);
        ASSERT_EQ(lines.size(), 1u);
        eval_line_original = lines[0];

        std::string save_out = ls.run("save sieve_save;");
        EXPECT_NE(save_out.find("Saved"), std::string::npos);
    }

    ASSERT_TRUE(std::filesystem::exists(saved_file));
    ASSERT_GT(std::filesystem::file_size(saved_file), 0u);

    // Phase 2: fresh session, load saved file, re-evaluate.
    {
        LiveSession ls;
        ASSERT_TRUE(ls.ok);
        ls.run("uses list; uses range;");

        std::ostringstream out2;
        ASSERT_TRUE(ls.s.run_file(saved_file, out2));
        ls.s.run_string(kSieveExpr, "<test2>", out2);

        std::string output2 = out2.str();
        EXPECT_NE(output2.find(kSieveResult), std::string::npos)
            << "reloaded output:\n" << output2;

        auto lines = eval_lines(output2);
        ASSERT_GE(lines.size(), 1u);
        EXPECT_EQ(eval_line_original, lines.back());
    }
}

// ---------------------------------------------------------------------------
// SessionRunFile — run_file idempotency
//
// Loading the same file twice must execute it only once.  The loaded_files_
// set guards against double-execution.
// ---------------------------------------------------------------------------

TEST(SessionRunFile, Idempotency) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    auto tmpdir = std::filesystem::temp_directory_path() / "hope_idem_test";
    std::filesystem::create_directories(tmpdir);
    struct RmDir { std::filesystem::path p;
                   ~RmDir() { std::error_code ec; std::filesystem::remove_all(p, ec); } } cleanup{tmpdir};

    // A .hop file that evaluates one expression.
    auto f = write_temp_file(tmpdir, "once.hop", "1 + 1;\n");

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::ostringstream out;
    ls.s.run_file(f.string(), out);
    ls.s.run_file(f.string(), out);  // second load — should be a no-op

    std::string combined = out.str();
    // ">> 2 : num" should appear exactly once.
    EXPECT_EQ(count_occurrences(combined, ">> 2 : num"), 1)
        << "file was executed more than once:\n" << combined;
}

// ---------------------------------------------------------------------------
// SessionRunString — error recovery
//
// A type error in one run_string call must not prevent subsequent evaluations
// from succeeding.
// ---------------------------------------------------------------------------

TEST(SessionRunString, TypeErrorRecovery) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    // This should produce a type error (can't add num and bool).
    std::string err_out = ls.run("1 + true;");
    EXPECT_NE(err_out.find("type error"), std::string::npos)
        << "expected type error, got:\n" << err_out;

    // The session should continue normally.
    std::string ok_out = ls.run("2 + 2;");
    EXPECT_NE(ok_out.find(">> 4 : num"), std::string::npos)
        << "session did not recover after type error:\n" << ok_out;
}

// A runtime error must also be recoverable.
TEST(SessionRunString, RuntimeErrorRecovery) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string err_out = ls.run("error \"boom\";");
    EXPECT_NE(err_out.find("boom"), std::string::npos)
        << "expected runtime error, got:\n" << err_out;

    std::string ok_out = ls.run("3 + 3;");
    EXPECT_NE(ok_out.find(">> 6 : num"), std::string::npos)
        << "session did not recover after runtime error:\n" << ok_out;
}

// ---------------------------------------------------------------------------
// SessionRunString — write vs eval output format
//
// expr;        → ">> value : type\n"
// write expr;  → raw characters, no ">> " prefix
// ---------------------------------------------------------------------------

TEST(SessionRunString, EvalOutputFormat) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.run("\"hello\";");
    EXPECT_NE(out.find(">> \"hello\" : list char"), std::string::npos)
        << "unexpected eval output:\n" << out;
}

TEST(SessionRunString, WriteOutputFormat) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    // write streams chars to stdout (std::cout), not to the Session output
    // stream.  What we can assert via run_string is that no ">> value : type"
    // line appears in the session output — i.e. the result is suppressed.
    std::string out = ls.run("write \"hello\";");
    EXPECT_EQ(out.find(">>"), std::string::npos)
        << "write produced a >> line in session output:\n" << out;

    // By contrast, a plain eval must produce a >> line.
    std::string eval_out = ls.run("\"hello\";");
    EXPECT_NE(eval_out.find(">>"), std::string::npos)
        << "plain eval did not produce a >> line:\n" << eval_out;
}

// ---------------------------------------------------------------------------
// SessionRunString — Hope-level display; command
//
// display; (as a Hope declaration, not the REPL :display meta-command) prints
// all recorded user declarations to the output stream.
// ---------------------------------------------------------------------------

TEST(SessionRunString, HopeDisplayCommand) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    ls.run("dec double : num -> num;");
    ls.run("--- double x <= x * 2;");

    std::string out = ls.run("display;");
    EXPECT_NE(out.find("dec double"), std::string::npos)
        << "display; did not show dec:\n" << out;
    EXPECT_NE(out.find("--- double"), std::string::npos)
        << "display; did not show equation:\n" << out;
}

// ---------------------------------------------------------------------------
// SessionInteractive — :display meta-command
// ---------------------------------------------------------------------------

TEST(SessionInteractive, Display) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    // Define something, then use the :display meta-command.
    std::string out = ls.interactive(
        "dec triple : num -> num;\n"
        "--- triple x <= x * 3;\n"
        ":display\n"
        ":quit\n"
    );

    EXPECT_NE(out.find("dec triple"), std::string::npos)
        << ":display did not show dec:\n" << out;
    EXPECT_NE(out.find("--- triple"), std::string::npos)
        << ":display did not show equation:\n" << out;
}

// ---------------------------------------------------------------------------
// SessionInteractive — :type meta-command
// ---------------------------------------------------------------------------

TEST(SessionInteractive, TypeSimple) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(":type 1 + 1\n:quit\n");
    // :type evaluates the expression and shows ">> value : type".
    EXPECT_NE(out.find(": num"), std::string::npos)
        << ":type did not show num:\n" << out;
}

TEST(SessionInteractive, TypeFunction) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    // Define a function, then :type it.
    std::string out = ls.interactive(
        "dec sq : num -> num;\n"
        "--- sq x <= x * x;\n"
        ":type sq 5\n"
        ":quit\n"
    );
    EXPECT_NE(out.find(">> 25 : num"), std::string::npos)
        << ":type sq 5 output:\n" << out;
}

TEST(SessionInteractive, TypeNoExpr) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(":type\n:quit\n");
    EXPECT_NE(out.find("Usage"), std::string::npos)
        << ":type (no expr) should print usage:\n" << out;
}

// ---------------------------------------------------------------------------
// SessionInteractive — :clear meta-command
// ---------------------------------------------------------------------------

TEST(SessionInteractive, Clear) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(
        "dec halve : num -> num;\n"
        "--- halve x <= x div 2;\n"
        ":clear\n"
        ":display\n"   // should show nothing (display_records_ is empty)
        ":quit\n"
    );

    EXPECT_NE(out.find("Session cleared"), std::string::npos)
        << ":clear did not print confirmation:\n" << out;

    // After :clear, :display should not show the halve definition.
    // Find the :display output (comes after "Session cleared").
    size_t cleared_pos = out.find("Session cleared");
    ASSERT_NE(cleared_pos, std::string::npos);
    std::string after_clear = out.substr(cleared_pos);
    EXPECT_EQ(after_clear.find("halve"), std::string::npos)
        << ":display after :clear still shows definitions:\n" << after_clear;
}

// ---------------------------------------------------------------------------
// SessionInteractive — :load and :reload meta-commands
// ---------------------------------------------------------------------------

TEST(SessionInteractive, LoadAndReload) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    auto tmpdir = std::filesystem::temp_directory_path() / "hope_load_test";
    std::filesystem::create_directories(tmpdir);
    struct RmDir { std::filesystem::path p;
                   ~RmDir() { std::error_code ec; std::filesystem::remove_all(p, ec); } } cleanup{tmpdir};

    auto f = write_temp_file(tmpdir, "greet.hop",
                             "dec greet : num -> num;\n"
                             "--- greet x <= x + 100;\n");

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string load_cmd  = ":load " + f.string() + "\n";
    std::string out = ls.interactive(
        load_cmd +
        "greet 7;\n"
        ":reload\n"
        "greet 7;\n"
        ":quit\n"
    );

    // greet 7 should produce 107 both times.
    EXPECT_EQ(count_occurrences(out, ">> 107 : num"), 2)
        << ":load/:reload output:\n" << out;
}

TEST(SessionInteractive, LoadMissingFile) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(
        ":load /no/such/file.hop\n"
        ":quit\n"
    );
    EXPECT_NE(out.find("Error"), std::string::npos)
        << "missing :load should produce Error:\n" << out;
}

TEST(SessionInteractive, ReloadNoFile) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(":reload\n:quit\n");
    EXPECT_NE(out.find("No file loaded"), std::string::npos)
        << ":reload before :load should say 'No file loaded':\n" << out;
}

// ---------------------------------------------------------------------------
// SessionInteractive — :help and unknown command
// ---------------------------------------------------------------------------

TEST(SessionInteractive, Help) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(":help\n:quit\n");
    EXPECT_NE(out.find("Meta-commands"), std::string::npos)
        << ":help output:\n" << out;
    EXPECT_NE(out.find(":load"), std::string::npos);
    EXPECT_NE(out.find(":quit"), std::string::npos);
}

TEST(SessionInteractive, UnknownCommand) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(":frobnicate\n:quit\n");
    EXPECT_NE(out.find("Unknown command"), std::string::npos)
        << "expected 'Unknown command' for :frobnicate:\n" << out;
}

// ---------------------------------------------------------------------------
// SessionInteractive — multi-line input accumulation
//
// A definition that spans multiple lines is accumulated until the top-level
// semicolon is reached.
// ---------------------------------------------------------------------------

TEST(SessionInteractive, MultiLineDefinition) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    // The equation body is on a separate line from the head.
    std::string out = ls.interactive(
        "dec add3 : num -> num;\n"
        "--- add3 x <=\n"
        "  x + 3;\n"
        "add3 10;\n"
        ":quit\n"
    );

    EXPECT_NE(out.find(">> 13 : num"), std::string::npos)
        << "multi-line definition output:\n" << out;
}

TEST(SessionInteractive, MultiLineSemicolonInString) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";

    // A semicolon inside a string literal must not flush the accumulator early.
    LiveSession ls;
    ASSERT_TRUE(ls.ok);

    std::string out = ls.interactive(
        "\"hello; world\";\n"
        ":quit\n"
    );
    EXPECT_NE(out.find("\"hello; world\""), std::string::npos)
        << "semicolon inside string literal flushed accumulator early:\n" << out;
}

// ---------------------------------------------------------------------------
// Type annotations on expressions: (expr : type)
// ---------------------------------------------------------------------------

TEST(SessionRunString, TypeAnnotationBasic) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    std::ostringstream out;
    ls.s.run_string("(42 : num);", "<test>", out);
    EXPECT_NE(out.str().find("42"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("num"), std::string::npos) << out.str();
}

TEST(SessionRunString, TypeAnnotationEmptyList) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    std::ostringstream out;
    ls.s.run_string("([] : list num);", "<test>", out);
    EXPECT_NE(out.str().find("[]"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("list num"), std::string::npos) << out.str();
}

TEST(SessionRunString, TypeAnnotationInfixExpr) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    std::ostringstream out;
    ls.s.run_string("(1 + 1 : num);", "<test>", out);
    EXPECT_NE(out.str().find("2"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("num"), std::string::npos) << out.str();
}

TEST(SessionRunString, TypeAnnotationMismatch) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    std::ostringstream out;
    // (42 : bool) must produce a type error.
    ls.s.run_string("(42 : bool);", "<test>", out);
    EXPECT_NE(out.str().find("type error"), std::string::npos) << out.str();
}

// ---------------------------------------------------------------------------
// Private function hiding
// ---------------------------------------------------------------------------

TEST(SessionRunString, PrivateFunctionHiding) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    // Write a module with a private section into a temp dir.
    auto tmpdir = std::filesystem::temp_directory_path() / "hope_priv_test";
    std::filesystem::create_directories(tmpdir);
    {
        std::ofstream f(tmpdir / "privmod.hop");
        f << "dec visible : num -> num;\n";
        f << "--- visible n <= hidden n;\n";
        f << "private;\n";
        f << "dec hidden : num -> num;\n";
        f << "--- hidden n <= n * 2;\n";
    }
    // Copy Standard.hop so the session can load stdlib from tmpdir.
    std::filesystem::copy_file(
        std::filesystem::path(kLibDir) / "Standard.hop",
        tmpdir / "Standard.hop",
        std::filesystem::copy_options::overwrite_existing);

    Session s;
    s.set_lib_dir(tmpdir.string());
    bool loaded = s.load_standard(tmpdir.string());
    ASSERT_TRUE(loaded);

    std::ostringstream out;
    s.run_string("uses privmod;", "<test>", out);
    out.str("");
    s.run_string("visible 7;", "<test>", out);
    EXPECT_NE(out.str().find("14"), std::string::npos) << out.str();
    out.str("");
    s.run_string("hidden 7;", "<test>", out);
    EXPECT_NE(out.str().find("type error"), std::string::npos) << out.str();
}

// ---------------------------------------------------------------------------
// Multi-parameter functors
// ---------------------------------------------------------------------------

TEST(SessionRunString, BiParameterFunctor) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    std::ostringstream out;
    ls.s.run_string(
        "data pair alpha beta == MkPair(alpha # beta);\n"
        "pair (+1) not (MkPair(3, true));",
        "<test>", out);
    EXPECT_NE(out.str().find("MkPair"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("4"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("false"), std::string::npos) << out.str();
}

// ---------------------------------------------------------------------------
// n+k patterns
// ---------------------------------------------------------------------------

TEST(SessionRunString, WriteToFile) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    auto out_path = std::filesystem::temp_directory_path() / "hope_write_test.txt";
    std::filesystem::remove(out_path);   // ensure clean slate
    std::ostringstream out;
    ls.s.run_string(
        "write \"hello from hope\" to \"" + out_path.string() + "\";",
        "<test>", out);
    // The REPL output should be silent (write suppresses >> result : type).
    EXPECT_EQ(out.str(), "") << out.str();
    // The file should have been created and contain the written text.
    std::ifstream f(out_path);
    ASSERT_TRUE(f.is_open()) << "file not created: " << out_path;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "hello from hope");
}

TEST(SessionRunString, NPlusKPattern) {
    if (std::string(kLibDir).empty()) GTEST_SKIP() << "HOPE_LIB_DIR not set";
    LiveSession ls;
    ASSERT_TRUE(ls.ok);
    std::ostringstream out;
    ls.s.run_string(
        "dec fib : num -> num;\n"
        "--- fib 0 <= 1;\n"
        "--- fib 1 <= 1;\n"
        "--- fib(n+2) <= fib n + fib(n+1);\n"
        "fib 10;",
        "<test>", out);
    EXPECT_NE(out.str().find("89"), std::string::npos) << out.str();
}

} // namespace
} // namespace hope
