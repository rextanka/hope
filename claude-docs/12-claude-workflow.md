# Working with Claude on This Project

## Purpose

This document describes how to work efficiently with Claude Code on the Hope C++20 rewrite. It covers: how to restore context at the start of each session, how to structure work for maximum productivity, the test strategy, the test framework setup, and the git check-in strategy.

---

## Context Across Sessions

### What persists automatically

Claude Code maintains a persistent memory file at:

```
~/.claude/projects/-Users-nthompson-src-cxx-hope/memory/MEMORY.md
```

This file is loaded automatically at the start of every session. It contains:
- Project location and goals
- C++20 rewrite constraints (zero deps, lazy semantics, REPL goals)
- Key language facts
- Key source files
- User preferences

Do not delete or move this file.

### What does NOT persist

Claude's context window is cleared between sessions. The following are **not** automatically restored:
- Which files were read last session
- Decisions made mid-session that weren't written to a file
- The current state of any partially-written code

### How to start a new session efficiently

Open a new Claude Code session and say:

```
Read claude-docs/MEMORY.md and then let's continue with Phase N of the rewrite.
```

Or, if picking up mid-phase:

```
Read claude-docs/12-claude-workflow.md and claude-docs/11-rewrite-roadmap.md,
then read cxx/src/<current-component>/. We were implementing X and got as far as Y.
```

The documents in `claude-docs/` are the authoritative context. They were written specifically to be the reference Claude reads before doing any work. The more specific you are about which document and which section is relevant, the less time Claude spends re-reading irrelevant material.

### Golden rule for session starts

**Point Claude at the relevant `claude-docs/` document before asking it to write code.** For example:
- Writing the lexer? Ask Claude to read `claude-docs/01-lexer-parser.md` first.
- Writing the type checker? `claude-docs/03-type-system.md` first.
- Writing the Krivine machine? `claude-docs/05-runtime-krivine.md` first.

This costs one extra message but saves many corrections later.

### Keeping MEMORY.md current

As the rewrite progresses, update MEMORY.md to reflect the current state. A good cadence is to ask Claude to update it at the end of each phase:

```
Update MEMORY.md to reflect that Phase 1 is complete and Phase 2 is starting.
Note any decisions we made about X and Y.
```

---

## Effective Prompting Strategies

### One component at a time

Never ask Claude to "write the whole interpreter". Always ask for one self-contained unit:

```
Implement cxx/src/lexer/Token.hpp and cxx/src/lexer/Lexer.hpp/.cpp
as described in claude-docs/01-lexer-parser.md. Write only the lexer;
do not start on the parser.
```

Claude works best when the scope is bounded by a single file or a tightly related pair of files.

### Describe the contract, not the implementation

Give Claude the **interface** specification, not implementation instructions:

```
The Lexer class must:
- Accept a string_view of source text and a filename
- Return tokens via a next() method that returns std::optional<Token>
- Track line and column in each Token's SourceLocation
- Recognize all reserved words listed in claude-docs/01-lexer-parser.md
- Treat '!' to end-of-line as a comment (discard)
```

Then let Claude figure out the implementation details. This produces more natural code than micro-managing every loop.

### Ask for tests at the same time as the implementation

When asking Claude to implement a component, always include:

```
Also write the corresponding Google Test file at cxx/test/unit/lexer/LexerTest.cpp.
```

Getting tests written alongside the implementation is far easier than retrofitting them afterward. Claude has full context of what it just wrote.

### Verification step after each component

After Claude writes a component, ask it to review its own work:

```
Review what you just wrote. Check that:
1. All reserved words from ref_man.tex are handled
2. Operator tokens use the graphic-char definition from the reference manual
3. Source locations are tracked correctly for multi-line inputs
4. The lexer handles the edge case of (-1) vs (-) correctly
```

This catches obvious omissions before compilation.

### Use the existing C source as a specification

The C source is the ground truth for behaviour. When in doubt about how something should work, ask:

```
Read src/yylex.c lines 45-120 and describe exactly how it handles operator tokenization.
Then implement the equivalent in cxx/src/lexer/Lexer.cpp.
```

Do not ask Claude to guess at semantics — point it at the original.

---

## Test Strategy

### Principle: Test at three levels

```
Level 1: Unit tests     — one component, no interpreter
Level 2: Component tests — pipeline stage (e.g. parse → type-check)
Level 3: Integration tests — full interpreter, compare stdout to .out file
```

All three levels use Google Test (see setup below). Integration tests additionally use CTest's `add_test` mechanism to run the Hope interpreter as a subprocess.

### Level 1: Unit Tests

Each `cxx/src/<component>/` directory has a corresponding `cxx/test/unit/<component>/` directory. Tests cover:

#### Lexer (`LexerTest.cpp`)

```cpp
TEST(LexerTest, ReservedWords) {
    Lexer lex("dec foo : num;", "<test>");
    EXPECT_EQ(lex.next()->kind, Token::Kind::KW_DEC);
    EXPECT_EQ(lex.next()->kind, Token::Kind::IDENT);
    EXPECT_EQ(lex.next()->text, "foo");
    // ...
}

TEST(LexerTest, OperatorTokens) {
    Lexer lex("x <> y", "<test>");
    // Verify <> is a single OPERATOR token, not < followed by >
}

TEST(LexerTest, SourceLocations) {
    Lexer lex("a\nb", "<test>");
    auto t1 = lex.next(); EXPECT_EQ(t1->loc.line, 1);
    lex.next(); // newline
    auto t2 = lex.next(); EXPECT_EQ(t2->loc.line, 2);
}

TEST(LexerTest, SectionAmbiguity) {
    // (-1) must lex as LEFT_PAREN OPERATOR NUMLIT RIGHT_PAREN
    // not as LEFT_PAREN NEGNUM RIGHT_PAREN
    Lexer lex("(-1)", "<test>");
    EXPECT_EQ(lex.next()->kind, Token::Kind::LPAREN);
    EXPECT_EQ(lex.next()->kind, Token::Kind::OPERATOR);
    EXPECT_EQ(lex.next()->kind, Token::Kind::NUMLIT);
    EXPECT_EQ(lex.next()->kind, Token::Kind::RPAREN);
}

TEST(LexerTest, Comments) {
    // ! to end of line is a comment
    Lexer lex("x ! this is a comment\ny", "<test>");
    EXPECT_EQ(lex.next()->text, "x");
    EXPECT_EQ(lex.next()->text, "y");
    EXPECT_EQ(lex.next(), std::nullopt);
}
```

#### Parser (`ParserTest.cpp`)

Test that expressions parse to the expected AST shape. Use a helper that prints the AST to a string and compare:

```cpp
std::string parseExpr(std::string_view src) {
    Lexer lex(src, "<test>");
    Parser p(lex);
    auto expr = p.expression();
    return AstPrinter{}.format(expr);
}

TEST(ParserTest, InfixApplication) {
    EXPECT_EQ(parseExpr("1 + 2 * 3"), "(+ 1 (* 2 3))");
}

TEST(ParserTest, OperatorSection_Left) {
    EXPECT_EQ(parseExpr("(3-)"), "(section-left - 3)");
}

TEST(ParserTest, OperatorSection_Right) {
    EXPECT_EQ(parseExpr("(-1)"), "(section-right - 1)");
}

TEST(ParserTest, LambdaMultiClause) {
    EXPECT_EQ(parseExpr("lambda 0 => 0 | (succ n) => n"),
              "(lambda ((0 -> 0) | ((succ n) -> n)))");
}
```

#### Type Checker (`TypeCheckTest.cpp`)

```cpp
TEST(TypeCheckTest, Literal_Num) {
    EXPECT_EQ(inferType("42"), "num");
}

TEST(TypeCheckTest, Literal_Char) {
    EXPECT_EQ(inferType("'a'"), "char");
}

TEST(TypeCheckTest, Literal_String) {
    EXPECT_EQ(inferType("\"hello\""), "list char");
}

TEST(TypeCheckTest, PolymorphicId) {
    EXPECT_EQ(inferType("lambda x => x"), "alpha -> alpha");
}

TEST(TypeCheckTest, TypeError_OrdOnNum) {
    // ord : char -> num; applying to num is a type error
    auto result = typecheck("ord 23");
    EXPECT_TRUE(result.has_errors());
    EXPECT_THAT(result.errors(), Contains(HasSubstr("cannot unify")));
}

TEST(TypeCheckTest, TypeError_MixedList) {
    auto result = typecheck("[1, 'a']");
    EXPECT_TRUE(result.has_errors());
}

TEST(TypeCheckTest, RegularType_Y) {
    // y.hop uses mu types; must not reject them
    loadModule("../lib/y.hop");
    EXPECT_FALSE(last_errors().has_errors());
}
```

#### Runtime / Krivine Machine (`EvalTest.cpp`)

```cpp
TEST(EvalTest, Arithmetic) {
    EXPECT_EQ(eval("1 + 2"), "3 : num");
}

TEST(EvalTest, PatternMatch_Succ) {
    defineAndEval(
        "dec fact: num -> num;"
        "--- fact(0) <= 1;"
        "--- fact(succ(n)) <= succ(n) * fact(n);",
        "fact(5)"
    );
    EXPECT_EQ(last_result(), "120 : num");
}

TEST(EvalTest, LazyThunk_Memoization) {
    // A thunk forced twice should execute its body only once.
    // Use a counter built-in to verify.
    // This is a white-box test using an internal hook.
}

TEST(EvalTest, BlackHole_Detection) {
    // Cyclic definition must throw RuntimeError, not diverge.
    EXPECT_THROW(eval("let x <= x in x"), RuntimeError);
}

TEST(EvalTest, InfiniteList_Front) {
    // front(3, iterate (+1) 0) = [0,1,2]
    EXPECT_EQ(eval("uses list; front(3, iterate (+1) 0)"), "[0, 1, 2] : list num");
}
```

### Level 2: Component / Pipeline Tests

These tests wire two stages together and verify the handoff. Located in `cxx/test/component/`.

```cpp
// test/component/ParseAndType.cpp
TEST(ParseAndType, BaileyExamples) {
    // Parse + type-check every expression from hope_tut.in
    // Verify no type errors and correct inferred types.
    auto session = Session::fresh();
    session.load("../test/inputs/hope_tut.in");
    EXPECT_FALSE(session.has_errors());
}

TEST(ParseAndType, TypeErrors_All_Rejected) {
    auto session = Session::fresh();
    session.load("../test/inputs/type_errs.in");
    EXPECT_EQ(session.error_count(), 4); // exactly 4 type errors
}
```

### Level 3: Integration Tests

These run the full `hope` binary as a subprocess and compare stdout+stderr to expected output. Located in `cxx/test/integration/`.

The test infrastructure is straightforward CMake:

```cmake
# cxx/test/CMakeLists.txt

function(add_hope_test name input expected)
    add_test(
        NAME "integration_${name}"
        COMMAND ${CMAKE_COMMAND}
            -DHOPE=$<TARGET_FILE:hope>
            -DINPUT=${input}
            -DEXPECTED=${expected}
            -DHOPEPATH=${CMAKE_SOURCE_DIR}/lib
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_hope_test.cmake
    )
endfunction()

add_hope_test(hope_tut   inputs/hope_tut.in   expected/hope_tut.out)
add_hope_test(primes     inputs/primes.in      expected/primes.out)
add_hope_test(lambdas    inputs/lambdas.in     expected/lambdas.out)
add_hope_test(sections   inputs/sections.in    expected/sections.out)
add_hope_test(io         inputs/io.in          expected/io.out)
add_hope_test(type_errs  inputs/type_errs.in   expected/type_errs.out)
```

```cmake
# cxx/test/run_hope_test.cmake
execute_process(
    COMMAND ${HOPE} -f ${INPUT}
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE  actual_err
    ENVIRONMENT "HOPEPATH=${HOPEPATH}"
)
string(APPEND actual ${actual_err})
file(READ ${EXPECTED} expected)
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Output mismatch for ${INPUT}")
endif()
```

The `expected/*.out` files are committed to the repository. They are generated once by running Paterson's C interpreter and saving the output. They are the contract.

**The `primes` integration test is the canonical lazy semantics gate.** It must pass before Phase 3 is considered complete.

---

## Test Framework: Google Test

### Rationale

Google Test (gtest) is appropriate despite the "zero external dependencies" rule because:

1. It is a **build-time test dependency only** — it is never linked into the `hope` binary itself
2. It is the dominant C++ unit test framework, with excellent CMake integration
3. It provides `EXPECT_THAT` + matchers, death tests, and parameterized tests that are genuinely useful here
4. It is fetched by CMake at configure time — no system package required

The zero-dependency constraint applies to the `hope` runtime. The test suite is a separate CMake target.

### Setup (`cxx/CMakeLists.txt` additions)

```cmake
# Fetch GoogleTest at configure time — no system install needed
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0  # pin to a specific release
)
# Do not install gtest alongside the project
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(test)
```

### Test executable layout

```cmake
# cxx/test/CMakeLists.txt

# Unit + component test binary
file(GLOB_RECURSE UNIT_SOURCES unit/**/*.cpp component/**/*.cpp)
add_executable(hope_tests ${UNIT_SOURCES})
target_link_libraries(hope_tests
    PRIVATE hope_lib   # the interpreter as a library target (not main.cpp)
    PRIVATE GTest::gtest_main
)
target_include_directories(hope_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
gtest_discover_tests(hope_tests)

# Integration tests (CMake script-based, hope binary as subprocess)
add_subdirectory(integration)
```

**Key point**: the interpreter must be buildable as both a library (`hope_lib`) and an executable (`hope`). `main.cpp` just calls `hope_lib` entry points. This is the standard split that makes unit testing an interpreter possible.

```cmake
# In cxx/src/CMakeLists.txt
file(GLOB_RECURSE LIB_SOURCES *.cpp)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(hope_lib STATIC ${LIB_SOURCES})
target_include_directories(hope_lib PUBLIC .)

add_executable(hope main.cpp)
target_link_libraries(hope PRIVATE hope_lib)
```

### Running tests

```bash
# Build everything
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build

# Run all tests with output on failure
ctest --test-dir build --output-on-failure

# Run just unit tests
ctest --test-dir build -R "^hope_tests"

# Run just the integration tests
ctest --test-dir build -R "^integration_"

# Run the primes integration test specifically
ctest --test-dir build -R "^integration_primes" --verbose
```

### Test tagging convention

Use gtest's `DISABLED_` prefix to mark tests for features not yet implemented, rather than deleting them:

```cpp
TEST(EvalTest, DISABLED_InputStream) {
    // requires io.in to work; implement after write/read builtins are done
}
```

This keeps the tests visible and tracked without failing the build.

---

## Git Check-in Strategy

### Branch structure

Branch names are prefixed with a `yyyymmddhhmm` timestamp — the datetime when the branch was created. This makes branches sort chronologically in `git branch -a` and provides an unambiguous audit trail.

```
master
  └── 202603101732-phase-1-lexer-parser
  └── 202603181400-phase-2-type-system
  └── 202603251000-phase-3-runtime
  └── 202604011200-phase-4-repl
  └── 202604081000-phase-5-polish
```

To create a branch:

```bash
# Get the current timestamp
STAMP=$(date +%Y%m%d%H%M)
git checkout -b ${STAMP}-phase-1-lexer-parser
```

Each phase branch is created from master, worked on until all tests for that phase pass, then merged to master via a pull request.

**Never commit directly to master.** Every merge to master must have a passing `ctest` run.

### Commit granularity

Commit early and often within a phase branch. A good commit is:
- **One logical unit**: a new file, a new test, a bug fix
- **Always compiles** (even if tests fail — mark failing tests `DISABLED_`)
- **Has a message describing what changed and why**, not just what files changed

Examples of good commit messages:
```
Add Lexer class with token stream and source location tracking

Implement all reserved words, identifier tokens, operator tokens,
character and string literals, numeric literals, and line comments.
Source locations include file, line, column, and token length.
Does not yet handle the lexer ambiguity between (-1) section and
negative number literal — marked TODO.
```

```
Add LexerTest: 23 unit tests, all passing

Covers reserved words, operators, string/char escapes, comments,
source location tracking, and the section/negative-number ambiguity.
The (-1) ambiguity test is DISABLED pending parser context.
```

```
Fix: (-1) lexes as section (OPERATOR NUMLIT), not negative number

The context for the parenthesised section is resolved during lexing
by tracking whether we are inside a parenthesised expression following
an operator. Matches Paterson's behaviour per src/yylex.c:214.
```

### What to commit vs not commit

**Always commit:**
- All `cxx/src/**` source files
- All `cxx/test/**` test files
- `cxx/CMakeLists.txt` and all sub-`CMakeLists.txt`
- `cxx/locale/en.txt`
- `claude-docs/` updates
- `cxx/test/expected/*.out` files (the integration test contracts)

**Never commit:**
- `build/` directory (add to `.gitignore`)
- CMake-generated files (`CMakeCache.txt`, `cmake_install.cmake`, etc.)
- Compiled binaries
- `.out` files generated during test runs (the committed ones are in `cxx/test/expected/`)

### `.gitignore` additions needed

```gitignore
# C++20 rewrite build artifacts
cxx/build/
cxx/.cache/
cxx/compile_commands.json
*.o
*.a
*.so
```

### Phase merge checklist

Before merging a phase branch to master:

- [ ] `cmake --build build` completes without warnings (`-Wall -Wextra`)
- [ ] `ctest --test-dir build --output-on-failure` — zero failures, zero DISABLED_ tests that should now pass
- [ ] All new source files have corresponding test files
- [ ] `claude-docs/MEMORY.md` updated to reflect phase completion
- [ ] Integration test `.out` files updated if interpreter output changed
- [ ] PR description lists what was implemented and references the relevant `claude-docs/` document

### Generating expected `.out` files

Before the C++20 rewrite can be tested, the expected outputs must be generated from Paterson's C interpreter and committed. Do this once, right before starting Phase 1:

```bash
cd src
make
cd ..
mkdir -p cxx/test/expected
for f in test/*.in; do
    STEM=$(basename "$f" .in)
    HOPEPATH=./lib ./src/hope -f "$f" > cxx/test/expected/$STEM.out 2>&1
done
git add cxx/test/expected/
git commit -m "Add integration test expected outputs from Paterson C interpreter"
```

These files become the immutable specification for the C++20 interpreter's output.

---

## Session Template

To start a productive session, copy and adapt this opening message:

```
Context:
- Project: Hope C++20 rewrite in cxx/ subdirectory
- Read claude-docs/12-claude-workflow.md for working conventions
- Read claude-docs/11-rewrite-roadmap.md for the overall plan
- Current phase: Phase N — [component name]
- Current branch: yyyymmddhhmm-phase-N-component-name

Relevant docs for this session:
- claude-docs/NN-<topic>.md (the architectural doc for this component)

Current state:
- [What files exist so far]
- [What is working / what test is failing]

Today's goal:
- [Specific file(s) to create or modify]
- [Test file(s) to create or extend]
- [Specific behaviour to implement]

Please read the relevant claude-docs file before writing any code.
```

---

## Common Pitfalls

**Pitfall 1: Asking Claude to write "the interpreter"**
Ask for one file at a time. Asking for too much produces code that compiles but has subtle semantic errors that take longer to fix than writing it incrementally.

**Pitfall 2: Not reading the original C source**
Paterson's C code is the specification. When Claude's implementation behaves differently from the C interpreter on a test case, the first step is always: `Read src/<relevant-file>.c and compare to what we wrote.` Do not try to reason about the correct behaviour from first principles.

**Pitfall 3: Skipping the library syntax check**
After any change to the type system or module loader, run all `lib/*.hop` files through the interpreter. These files exercise almost every type system feature and catch regressions quickly.

**Pitfall 4: Testing only terminating programs**
`primes.in` is the only test that validates lazy semantics. Add tests for `iterate`, `from`, and `many` in `EvalTest.cpp` as soon as the runtime is standing. If these diverge, the memoization or update mechanism is broken.

**Pitfall 5: Letting context drift**
After a long session of iteration and debugging, ask Claude to summarise any decisions made and update `MEMORY.md` before ending the session. This takes 30 seconds and prevents the next session from re-litigating the same decisions.
