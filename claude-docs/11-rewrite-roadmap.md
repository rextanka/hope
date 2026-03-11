# C++20 Rewrite Roadmap

## Overview

The C++20 rewrite lives in `cxx/` at the repository root. It is a clean-room reimplementation of Paterson's lazy HOPE interpreter, preserving the language semantics exactly while replacing every piece of the infrastructure with modern, portable, dependency-free C++20.

The existing interpreter (36 C source files, ~8 000 lines) was written in the early 1990s and depends on autoconf, yacc/bison, and various POSIX-isms. The rewrite targets:
- Correctness: pass all 6 existing tests, all library files, and Bailey's tutorial
- Portability: compile with GCC 10+, Clang 12+, MSVC 19.29+ using only the C++ standard library
- Maintainability: clear separation of concerns, no global mutable state
- Diagnostics: rich error messages with source locations for all errors
- Usability: a proper interactive REPL

---

## Dependency Policy: Zero External Dependencies

The rewrite must compile with only:
- A C++20-conforming compiler
- CMake 3.20+
- The C++ standard library (including `<format>`, `<filesystem>`, `<variant>`, `<expected>` where available)

No Boehm GC, PEGTL, Boost, LLVM, readline, or external parser generators.

---

## What to Keep vs Redesign

### Keep (semantics must be preserved exactly)
| What | Why |
|------|-----|
| Lazy (call-by-need) evaluation | Paterson's key contribution; required for `primes`, `seq`, `iterate` |
| Krivine machine semantics | Standard lazy evaluator; thunks memoize on first force |
| Hindley-Milner type inference | HM with union-find unification |
| Best-fit pattern matching | Most specific pattern wins (not first-match) |
| Regular types (`mu s => ...`) | Required for `y.hop`, `fold.hop`, `seq.hop` |
| Functor auto-generation | Auto-generated `map` for every `data`/`type` |
| Module system (`uses`, `private`) | Encapsulation mechanism |
| `input` / `write` lazy I/O | Lazy character streams |
| All Standard library semantics | Exact same types and functions |

### Redesign (implementation changed, semantics preserved)
| What | Old | New |
|------|-----|-----|
| Parser | YACC grammar + op.sed + Assoc.sed pipeline | Hand-written recursive-descent + Pratt |
| Build system | autoconf + make | CMake 3.20+ |
| AST nodes | Tagged C unions (`Expr`, `UCase`, `LCase`) | `std::variant<...>` |
| Runtime values | Tagged `Cell*` heap | `std::variant<...>` arena cells |
| Type inference cells | Tagged `Cell*` (shared heap) | Separate inference nodes |
| GC | Mark-sweep over flat arena | Mark-compact arena, `std::vector`-backed |
| Interned strings | Manual hash table | `std::unordered_map<std::string, ...>` |
| Symbol tables | Global C arrays | Per-`Module` `std::unordered_map` |
| Module limit | Hard 32-module limit | `std::vector<Module>`, unlimited |
| Error reporting | `printf` to stderr, no locations | Structured diagnostics with file:line:col |
| REPL | Basic `gets()`-style loop | Proper REPL with history and meta-commands |
| Numeric formatting | `printf("%g")` | `std::format("{}", ...)` |
| Path resolution | Manual `getenv("HOPEPATH")` + `strcat` | `std::filesystem` |
| Signals | `signal()` + `setjmp/longjmp` | C++ exceptions (for errors); `std::atomic` (for interrupts) |

---

## C++20 Features to Exploit

| Feature | Use |
|---------|-----|
| `std::variant<...>` | AST nodes (`Expr`), runtime values (`Cell`), types (`Type`) |
| `std::visit` + overloaded lambdas | Dispatch on variant alternatives (replaces switch on `c_class`) |
| `std::string_view` | Zero-copy lexer tokens, symbol table keys |
| `std::format` | Numeric output, error message formatting |
| `std::filesystem` | Module path resolution |
| `std::unordered_map` | Symbol tables, intern table |
| `std::optional<T>` | Nullable results (lookup returns `std::optional`) |
| `std::expected<T, E>` (C++23) or hand-rolled | Error propagation without exceptions in hot paths |
| `std::shared_ptr` | Shared heap nodes (memoized thunks, shared types) |
| `std::unique_ptr` | Exclusive ownership (AST nodes, module objects) |
| Concepts | Type constraints on generic functions |
| Ranges / views | List traversal, lazy iteration helpers |
| `constexpr` | Precedence tables, operator metadata |

---

## Directory Layout

```
cxx/
  CMakeLists.txt              Top-level CMake: compiles, runs ctest
  src/
    main.cpp                  Entry point: parse args, init, launch REPL or run file
    repl/
      Repl.hpp / Repl.cpp     Interactive REPL loop, meta-command dispatch
      Session.hpp / Session.cpp Session state (current module, history)
    lexer/
      Lexer.hpp / Lexer.cpp   Hand-written lexer with source locations
      Token.hpp               Token types and values
      SourceLocation.hpp      File + line + column + length
    parser/
      Parser.hpp / Parser.cpp Recursive-descent + Pratt parser
      ParseError.hpp          Parse error type with source location
    ast/
      Expr.hpp                std::variant<...> AST node types
      Pattern.hpp             Pattern node types
      Decl.hpp                Declaration types (Dec, Data, Type, Infix, ...)
      AstPrinter.hpp          Pretty-print AST (for :display and error messages)
    types/
      Type.hpp                Static type representation (std::variant)
      TypeCheck.hpp / TypeCheck.cpp  HM inference: Algorithm W, unification
      Unifier.hpp             Union-find data structure for type variables
      TypeError.hpp           Type error types with source locations
      FunctorType.hpp         Auto-generated map/functor types
      Polarity.hpp            Polarity analysis for functor generation
      RegularTypes.hpp        mu-type expansion and cycle checking
    compiler/
      Compiler.hpp / Compiler.cpp  Pattern match compilation to decision trees
      DecisionTree.hpp        UCase/LCase decision tree nodes
      Overlap.hpp             Pattern overlap and exhaustiveness checking
    runtime/
      Cell.hpp                Runtime value: std::variant<Num, Char, Cons, Pair, ...>
      Arena.hpp               GC arena: mark-compact, std::vector-backed
      Thunk.hpp               Lazy thunk with memoization
      Interpreter.hpp / Interpreter.cpp  Krivine machine
      Evaluator.hpp / Evaluator.cpp      Top-level force/evaluate
      RuntimeError.hpp        Pattern match failure, stack overflow, etc.
    builtins/
      Builtins.hpp / Builtins.cpp  Built-in function registration
      Arithmetic.hpp          Numeric operations
      Strings.hpp             String/character built-ins (ord, chr, num2str, str2num)
      IO.hpp                  read, write, input stream, argv
      Compare.hpp             compare, =, /=, <, =<, >, >=
    modules/
      Module.hpp / Module.cpp Module struct: tables, uses, private
      ModuleLoader.hpp / ModuleLoader.cpp  Load, resolve, dep-order, private
      PathResolver.hpp        HOPEPATH / std::filesystem path search
    printer/
      Printer.hpp / Printer.cpp  Format values, types, exprs (returns std::string)
      Precedence.hpp          Precedence constants
    errors/
      Diagnostic.hpp          Unified diagnostic type: location + message + hints
      DiagnosticEngine.hpp    Collect, format, and emit diagnostics
  locale/
    en.txt                    All user-facing strings, keyed by ID
  lib/                        Symlink or copy of ../lib (Standard.hop, etc.)
  test/                       CTest test cases
    CMakeLists.txt            ctest definitions
    inputs/                   Symlink or copy of ../test/*.in
    expected/                 Expected output files (committed)
    run_test.cmake            Helper script for comparing output
```

---

## Build System

```cmake
# cxx/CMakeLists.txt (sketch)
cmake_minimum_required(VERSION 3.20)
project(hope CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# All sources under src/
file(GLOB_RECURSE HOPE_SOURCES src/*.cpp)
add_executable(hope ${HOPE_SOURCES})
target_include_directories(hope PRIVATE src)

# Compiler warnings
target_compile_options(hope PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Path to standard library
set(HOPELIB "${CMAKE_SOURCE_DIR}/lib" CACHE PATH "Path to Hope standard library")
target_compile_definitions(hope PRIVATE HOPELIB="${HOPELIB}")

# Tests
enable_testing()
add_subdirectory(test)
```

---

## Lexer Design

The hand-written lexer replaces `yylex.c`. It must handle:

- **Identifier tokens**: alphanumeric + `_`, starting with a letter
- **Operator tokens**: sequences of graphic characters (`!`, `#`, `$`, `%`, `&`, `*`, `+`, `-`, `.`, `/`, `:`, `<`, `=`, `>`, `?`, `@`, `\`, `^`, `|`, `~`)
- **String literals**: `"..."` with C-style escapes
- **Character literals**: `'...'` with C-style escapes
- **Comments**: `!` to end of line
- **Reserved words**: `dec`, `data`, `type`, `typevar`, `abstype`, `infix`, `infixr`, `lambda`, `\`, `let`, `letrec`, `where`, `whererec`, `if`, `then`, `else`, `uses`, `private`, `save`, `write`, `display`, `edit`, `exit`
- **Numeric literals**: integers (used as `succ(succ(...0...))` internally) and optionally reals
- **Source location tracking**: every token carries its `SourceLocation`

```cpp
struct SourceLocation {
    std::string_view file;
    int line;
    int column;
    int length;
};

struct Token {
    enum class Kind { ... };
    Kind kind;
    std::string_view text;   // points into source buffer
    SourceLocation loc;
    // For operator tokens: user-defined precedence and associativity
    // (looked up from current operator table during parse)
    std::optional<int> prec;
    std::optional<Assoc> assoc;
};
```

The key challenge inherited from Paterson's lexer: operator precedence is **user-defined** and changes as `infix`/`infixr` declarations are processed. The Pratt parser handles this dynamically — the lexer provides the token type; the parser looks up the binding power on the fly from the current operator table.

---

## Parser Design

Replace YACC + op.sed + Assoc.sed with a hand-written **recursive-descent + Pratt expression parser**.

### Top-level declarations (recursive descent)

```
program     ::= decl* EOF
decl        ::= infix_decl | data_decl | type_decl | typevar_decl
              | abstype_decl | dec_decl | equation | uses_decl
              | command
infix_decl  ::= ('infix' | 'infixr') NAME ':' NUM ';'
data_decl   ::= 'data' type_lhs '==' cons_list ';'
type_decl   ::= 'type' type_lhs '==' type ';'
dec_decl    ::= 'dec' name ':' type ';'
equation    ::= '---' pat '<=' expr ';'
```

### Expressions (Pratt / top-down operator precedence)

Pratt parsing handles arbitrary user-defined infix operators with their declared precedences. The binding power table is queried from the current `Module`'s operator table on each token.

```
expr        ::= pratt_expr(0)
pratt_expr(p) ::= prefix_expr (infix_op pratt_expr(p') )*
prefix_expr ::= 'if' expr 'then' expr 'else' expr
              | 'let' bindings 'in' expr
              | 'letrec' bindings 'in' expr
              | 'lambda' lam_body
              | 'write' expr
              | application
application ::= atom atom*
atom        ::= NAME | NUM | CHAR | STRING | '(' expr ')' | '[' list ']'
              | '(' op_section ')'
```

The `private;` sentinel is handled as a special declaration in the module loader, not a parser production.

---

## Type System Design

### Type Representation

```cpp
// Static types (from declarations)
struct TyVar  { std::string name; };
struct TyCons { DefType* def; std::vector<Type> args; };
struct TyMu   { std::string var; std::shared_ptr<Type> body; };
using Type = std::variant<TyVar, TyCons, TyMu>;

// Quantified type (with universally quantified variables)
struct QType { std::vector<std::string> vars; Type body; };
```

### Inference Variables

During type checking, inference uses a union-find structure over type variable nodes:

```cpp
struct InfVar {
    int id;
    std::shared_ptr<InfNode> rep; // union-find root
};
struct InfCons { DefType* def; std::vector<InfType> args; };
struct InfMu   { std::string var; std::shared_ptr<InfType> body; };
using InfType = std::variant<InfVar, InfCons, InfMu>;
```

Unification (`unify(t1, t2)`) follows the paths-and-occur-check algorithm from the original, but implemented as a proper recursive function over `InfType` variants rather than the C `Cell*` walk.

### Algorithm W

```cpp
InfType typecheck(const Expr& expr, TypeEnv& env);
void    unify(InfType& t1, InfType& t2, SourceLocation loc);
InfType instantiate(const QType& qt);
QType   generalise(const InfType& t, const TypeEnv& env);
```

Errors are reported via `DiagnosticEngine::error(loc, message)` and collected; type checking continues (with a placeholder error type) to find as many errors as possible in a single pass.

---

## Runtime Design

### Cell Representation

```cpp
// All runtime values
struct NumCell  { double value; };  // or int64_t for integer-only builds
struct CharCell { char32_t ch; };
struct ConsCell { const Cons* constructor; Cell* arg; };
struct PairCell { Cell* left; Cell* right; };
struct PAppCell { Cell* func; Cell* arg; int remaining; };
struct SuspCell { const Expr* body; Env* env; };  // unevaluated thunk
struct HoleCell {};  // black hole (loop detection)
struct StreamCell { std::function<std::pair<Cell*, Cell*>()> step; };

using CellData = std::variant<
    NumCell, CharCell, ConsCell, PairCell, PAppCell, SuspCell, HoleCell, StreamCell
>;

struct Cell {
    CellData data;
    bool gc_mark = false;
};
```

### Thunk Memoization

When a `SuspCell` is forced:
1. Replace it with `HoleCell` (black hole)
2. Evaluate `body` under `env`
3. Replace `HoleCell` with the result value
4. Return the result

This is the standard call-by-need update. In C++20, the cell is updated in place because all cells live in the arena:

```cpp
Cell* force(Cell* cell) {
    while (std::holds_alternative<SuspCell>(cell->data)) {
        auto& susp = std::get<SuspCell>(cell->data);
        auto body = susp.body;
        auto env = susp.env;
        cell->data = HoleCell{};           // black hole
        Cell* result = eval(body, env);
        cell->data = result->data;         // in-place update (copy data)
        cell->gc_mark = result->gc_mark;
    }
    if (std::holds_alternative<HoleCell>(cell->data))
        throw RuntimeError("infinite loop detected");
    return cell;
}
```

### Arena GC

The arena is a `std::vector<Cell>` that never reallocates (pre-reserved to a fixed size, configurable at startup). GC is triggered when the arena is near full:

```cpp
class Arena {
    std::vector<Cell> cells;
    size_t next_free = 0;
    std::vector<Cell*> roots; // GC roots (stack, globals)

    Cell* alloc();    // returns &cells[next_free++], triggers GC if needed
    void  gc();       // mark from roots, compact live cells
};
```

Mark phase: traverse reachable cells from all roots, set `gc_mark`.
Compact phase: slide live cells to the front, update all pointers.

Alternatively, use a **free-list** over a fixed `std::array<Cell, N>` for cache locality and simpler pointer arithmetic.

---

## Krivine Machine Design

The Krivine machine runs as a loop over an explicit stack. The C++ version uses a `std::vector<StackFrame>` instead of Paterson's hand-managed stack array:

```cpp
struct ApplyFrame  { Cell* arg; };
struct UpdateFrame { Cell* to_update; };
struct CaseFrame   { const UCase* cases; Env* env; };
struct LCaseFrame  { const LCase* cases; Env* env; };

using StackFrame = std::variant<ApplyFrame, UpdateFrame, CaseFrame, LCaseFrame>;

Cell* run(Cell* control, std::vector<StackFrame>& stack) {
    while (true) {
        // Unwind applications
        while (auto* papp = get_if<PAppCell>(&control->data)) {
            stack.push_back(ApplyFrame{papp->arg});
            control = papp->func;
        }
        // Force suspension
        if (auto* susp = get_if<SuspCell>(&control->data)) {
            stack.push_back(UpdateFrame{control});
            // enter the body
            control = eval(susp->body, susp->env);
            continue;
        }
        // Dispatch on top of stack
        if (stack.empty()) return control;
        auto frame = stack.back(); stack.pop_back();
        std::visit(overloaded{
            [&](ApplyFrame& f) { /* apply control to f.arg */ },
            [&](UpdateFrame& f) { f.to_update->data = control->data; },
            [&](CaseFrame& f) { /* pattern match control against f.cases */ },
            [&](LCaseFrame& f) { /* lazy case: pass control to f.cases */ },
        }, frame);
    }
}
```

The `overloaded` helper (standard C++20 pattern):
```cpp
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
```

---

## Error Reporting Design

### Diagnostic Structure

```cpp
enum class DiagnosticKind { Error, Warning, Note };

struct Diagnostic {
    DiagnosticKind kind;
    SourceLocation primary_loc;
    std::string message_id;       // key into locale/en.txt
    std::vector<std::string> args; // substituted into message template
    std::vector<std::string> hints;
    std::optional<SourceLocation> secondary_loc;
    std::optional<std::string> secondary_note;
};
```

### Locale File (`locale/en.txt`)

```
# Syntax errors
E_UNEXPECTED_TOKEN     = unexpected token '{0}', expected {1}
E_UNCLOSED_STRING      = unterminated string literal
E_UNCLOSED_COMMENT     = unterminated comment
E_MISSING_SEMICOLON    = missing ';' after {0}

# Type errors
E_UNIFY_MISMATCH       = cannot unify '{0}' with '{1}'
E_UNIFY_OCCURS         = type variable '{0}' occurs in '{1}' (infinite type)
E_UNDEFINED_NAME       = undefined name '{0}'
E_UNDEFINED_NAME_HINT  = did you mean '{0}'?
E_ARITY_MISMATCH       = '{0}' applied to {1} arguments, but expects {2}

# Pattern errors
E_OVERLAP_PATTERN      = pattern '{0}' is unreachable (shadowed by earlier case)
E_NONEXHAUSTIVE        = non-exhaustive patterns in '{0}' (missing: {1})

# Runtime errors
E_PATTERN_FAILURE      = pattern match failure in '{0}' with argument {1}
E_INFINITE_LOOP        = infinite loop (black hole) detected in '{0}'
E_DIVISION_BY_ZERO     = division by zero

# Module errors
E_MODULE_NOT_FOUND     = module '{0}' not found on HOPEPATH
E_CYCLIC_IMPORT        = cyclic module dependency: {0}

# REPL messages
REPL_PROMPT            = hope>
REPL_CONT_PROMPT       =     >
REPL_RESULT            = {0} : {1}
REPL_GOODBYE           = Goodbye.
```

All user-facing text is loaded from `locale/en.txt` at startup. The locale file is a simple `key = value` format with `{N}` positional substitution. To add another language, ship an alternative `XX.txt` and point to it with a `HOPE_LOCALE` environment variable.

---

## REPL Design

```cpp
class Repl {
    Session session;        // current module, loaded modules, history
    Lexer   lexer;
    Parser  parser;
    DiagnosticEngine diag;

    void run();             // main loop
    void eval_line(std::string_view input);
    void run_meta(std::string_view cmd, std::string_view arg);
};
```

### Meta-commands

| Command | Action |
|---------|--------|
| `:load <file>` | Load and execute `<file>` into the current session |
| `:reload` | Reload the last loaded file |
| `:type <expr>` | Infer and display the type of `<expr>` without evaluating |
| `:display` | Show all current session definitions |
| `:save <name>` | Save session definitions to `<name>.hop` |
| `:clear` | Reset the session (keep Standard loaded) |
| `:quit` / `:exit` | Exit the REPL |

### Expression evaluation output

```
hope> 1 + 2;
3 : num

hope> [1,2,3];
[1, 2, 3] : list num

hope> "hello";
"hello" : list char
```

### Error recovery

A REPL error does not terminate the session:
```
hope> ord 23;
Error: cannot unify 'char' with 'num'
  --> <repl>:1:1
   |
 1 | ord 23;
   | ^^^^^^
   = note: ord : char -> num
hope>
```

---

## Pattern Compilation

The decision tree compiler is largely the same as Paterson's, but with a cleaner representation:

```cpp
// Decision tree nodes
struct UMatch { // Constructor case
    std::vector<std::pair<const Cons*, DecisionTree>> branches;
    std::optional<DecisionTree> default_branch;
};
struct LMatch { // Lazy case (irrefutable)
    std::vector<std::pair<Pattern, DecisionTree>> branches;
};
struct Leaf   { const Expr* body; Env* env; };
struct Fail   {};

using DecisionTree = std::variant<UMatch, LMatch, Leaf, Fail>;
```

Pattern specificity and overlap checking are preserved from `cases.c`. The incremental merge algorithm (`add_branch`) is reimplemented as a standalone function.

---

## Module System Design

```cpp
class Module {
    std::string name;
    std::unordered_map<std::string, Op>      operators;
    std::unordered_map<std::string, DefType*> types;
    std::unordered_map<std::string, Func*>    functions;
    std::unordered_map<std::string, Cons*>    constructors;
    std::vector<std::string>                  uses_list;
    bool is_private_section = false;
    Module* public_interface = nullptr; // non-null during private section
};

class ModuleLoader {
    std::vector<std::filesystem::path> search_path;
    std::vector<std::unique_ptr<Module>> loaded;

    Module* load(std::string_view name);
    std::optional<std::filesystem::path> find(std::string_view name) const;
    void check_cycles(std::string_view name, const std::vector<std::string>& stack);
};
```

Path resolution uses `std::filesystem::path` throughout. HOPEPATH is parsed from the environment using `std::getenv`, split on `:` (or `;` on Windows with `#ifdef _WIN32`).

---

## Migration Path

### Phase 1: Lexer + Parser (week 1–2)
- Hand-write the lexer with source location tracking
- Hand-write the recursive-descent parser for declarations
- Hand-write the Pratt parser for expressions
- Target: parse all `test/*.in` and `lib/*.hop` files without errors
- Validation: compare AST dumps against Paterson's `pr_expr` output

### Phase 2: Type System (week 3–4)
- Implement `Type`, `QType`, inference variables, unification
- Implement Algorithm W for expressions
- Implement regular type expansion and cycle checking
- Implement functor type generation and polarity analysis
- Target: type-check all `lib/*.hop` files correctly
- Validation: compare inferred types against Paterson's output

### Phase 3: Runtime (week 5–6)
- Implement the `Cell` variant and arena allocator
- Implement the Krivine machine
- Implement pattern match compilation
- Implement built-in functions
- Target: evaluate all `test/*.in` files correctly
- Key milestone: `primes.in` must produce correct output (validates lazy semantics)

### Phase 4: Module System + REPL (week 7)
- Implement module loading and scoping
- Implement the `private` shadow module mechanism
- Implement the REPL loop with meta-commands
- Implement the diagnostic engine and locale file loading
- Target: fully interactive REPL matching Paterson's behaviour plus `:type`, `:load`, `:reload`

### Phase 5: Polish (week 8)
- Improve error messages (source locations, hints, "did you mean?")
- Add `ctest` test suite with committed expected outputs
- Add `CMakePresets.json` for common build configurations
- Add CI configuration (GitHub Actions)
- Write brief user documentation

---

## Known Risks

| Risk | Mitigation |
|------|-----------|
| Regular type termination (μ-types can be infinite) | Port `bad_rectype.c` cycle-checking algorithm exactly |
| Lazy I/O semantics under the Krivine machine | Study `stream.c` and `output.c` carefully before reimplementing |
| Numeric semantics (int vs float) | The original uses double throughout; replicate exactly |
| `private` module scoping | The shadow-module mechanism is subtle; test with `set.hop` |
| Functor polarity analysis | Negative positions must not generate the `map` component; port `polarity.c` |
| Operator precedence during parsing | Pratt parser must query live operator table as declarations are processed |
| Pattern match best-fit ordering | Port the specificity comparison from `cases.c` verbatim and test with overlapping patterns |

---

## References

- `src/interpret.c` — Krivine machine (the single most important file to understand)
- `src/type_check.c` — HM type inference
- `src/yyparse.y` — Full language grammar (use as specification for the recursive-descent parser)
- `src/compile.c`, `src/cases.c` — Pattern compilation
- `src/module.c` — Module system
- `doc/ref_man.tex` — Paterson's reference manual (authoritative language specification)
- `doc/hope_tut.src` — Roger Bailey's tutorial (acceptance test suite)
- `claude-docs/05-runtime-krivine.md` — Detailed Krivine machine documentation
- `claude-docs/03-type-system.md` — Type system documentation
- `claude-docs/04-pattern-compilation.md` — Pattern compilation documentation
