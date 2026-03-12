# CI Setup

This document describes how the continuous integration pipeline for the C++20
Hope interpreter is configured.

## Workflow file

`.github/workflows/ci.yml` — runs on every push and pull request to `master`.

## Build matrix

| Runner OS     | Compiler       | Standard library |
|---------------|----------------|-----------------|
| ubuntu-24.04  | GCC 14         | libstdc++ 14    |
| ubuntu-24.04  | Clang 18       | libstdc++ 14    |
| macos-15      | Apple Clang 17 | libc++          |

All three legs build with `-DCMAKE_BUILD_TYPE=Release` and the C++20 standard
(`CMAKE_CXX_STANDARD 20`, extensions off).

## Toolchain installation

**Ubuntu legs** — the `Install compiler` step runs:

```bash
sudo apt-get update -qq
sudo apt-get install -y <cc> <cxx>
```

where `<cc>` and `<cxx>` come from the matrix (e.g. `gcc-14 g++-14` or
`clang-18 clang++-18`).  Both packages are present in the default ubuntu-24.04
apt sources; no extra PPA or LLVM repository is required.

**macOS leg** — the `Install compiler` step is skipped.  The runner ships
Xcode 16 with Apple Clang 17 as the default toolchain.

## Configure step

```bash
cmake -S cxx -B cxx/build_ci \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=<cc> \
      -DCMAKE_CXX_COMPILER=<cxx>
```

The source root is `cxx/` (contains the top-level `CMakeLists.txt`).
The build directory is `cxx/build_ci/`.

`HOPELIB` is set automatically by CMake to `<repo>/lib` (the standard-library
`.hop` files) and baked into the binary at compile time as a preprocessor
constant.  See `cxx/src/CMakeLists.txt`.

## Dependencies fetched at configure time

Google Test (v1.15.2) is downloaded via CMake `FetchContent`:

```cmake
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
```

v1.14.0 does not compile with GCC 14; v1.15.0+ is required.  No other
external dependencies are used; the project requires only a C++20-conformant
compiler, CMake ≥ 3.20, and the C++ standard library.

## Build step

```bash
cmake --build cxx/build_ci --parallel
```

Compile flags applied to `hope_lib` (defined in `src/CMakeLists.txt`):

| Compiler family   | Flags |
|-------------------|-------|
| GCC / Clang       | `-Wall -Wextra -Wpedantic -Wno-unused-parameter` |
| MSVC              | `/W4` |

## Test step

```bash
ctest --test-dir build_ci --output-on-failure --parallel 4
```

Run from the `cxx/` working directory.  Step timeout: 5 minutes.

### Unit tests (271 tests)

Compiled into `hope_tests` (links `hope_lib` + GTest).  Test cases:

| Suite | File | Coverage |
|-------|------|----------|
| LexerTest | `test/unit/LexerTest.cpp` | tokenisation, keywords, operators |
| ParserTest | `test/unit/ParserTest.cpp` | expressions, patterns, declarations, all lib/*.hop files, all test/*.in files |
| TypeTest | `test/unit/TypeTest.cpp` | HM unification, type inference, error reporting |
| EvalTest | `test/unit/EvalTest.cpp` | evaluator, pattern matching, lazy thunks |

### Integration tests (9 tests)

Each integration test runs the `hope` binary against a `.in` file and diffs
stdout+stderr against a committed `.out` file.

The test driver is `cxx/test/integration/run_hope_test.cmake`, invoked by
CTest via `cmake -P`.

| Test name | Input file | What it covers |
|-----------|-----------|----------------|
| `hope_tut` | `test/hope_tut.in` | Full Bailey tutorial — ~230 expressions |
| `primes` | `test/primes.in` | Lazy infinite list (`front_seq(25, primes)`) |
| `lambdas` | `test/lambdas.in` | Lambda expressions and closures |
| `sections` | `test/sections.in` | Operator sections `(op e)` / `(e op)` |
| `io` | `test/io.in` | `read`, `write`, `lines`, lazy file I/O |
| `type_errs` | `test/type_errs.in` | Type error messages and recovery |
| `maybe` | `test/maybe.in` | `maybe` stdlib module, `safe_head`, `safe_div` |
| `seq` | `test/seq.in` | `seq` stdlib module, `front_seq`, `gen_seq` |
| `arith` | `test/arith.in` | `arith` stdlib module, `fibs` (requires functor generation) |

### Environment variable: HOPEPATH

Integration tests pass the Hope library directory to the binary via:

```
HOPEPATH=<repo>/lib
```

This is set in `cxx/test/integration/CMakeLists.txt`:

```cmake
set(HOPEPATH  ${CMAKE_SOURCE_DIR}/../lib)
```

and forwarded to the binary through `cmake -E env "HOPEPATH=..."` inside
`run_hope_test.cmake`.

**Priority order** for library-directory resolution (see `src/main.cpp`):

1. `HOPEPATH` environment variable — highest priority
2. `HOPELIB` compile-time constant (set by CMake to `<repo>/lib`)
3. Executable-relative fallback (`../lib` next to the binary)

### Working directory for integration tests

The `hope` binary is run with its working directory set to `<repo>/src`
(Paterson's original C source directory).  This makes the `io.in` test's
relative path `../test/hope_tut.in` resolve correctly, since `test/` and
`src/` are siblings at the repo root.

### Expected outputs

Committed in `cxx/test/expected/*.out`.  These were generated by running each
`.in` file through the interpreter on a known-good build and recording stdout
+ stderr (merged).

The `.gitignore` root pattern `*.out` is overridden for this directory by:

```
!cxx/test/expected/*.out
```

To regenerate an expected file after an intentional interpreter change:

```bash
HOPEPATH=lib cxx/build_ci/src/hope -f test/<name>.in > cxx/test/expected/<name>.out 2>&1
```
