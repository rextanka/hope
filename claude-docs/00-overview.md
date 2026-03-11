# Hope Interpreter: Architecture Overview

## What This Is

This repository contains Ross Paterson's **Lazy HOPE** interpreter, originally written at City University London, 1990–1993. It is a complete interpreter for the HOPE functional programming language — a strict, ML-influenced language developed at Edinburgh and Imperial College in the late 1970s and early 1980s — but with **lazy (call-by-need) evaluation** rather than the strict semantics of the original Imperial College implementation.

The interpreter was rescued from oblivion around 2013 (when the original City University web pages went offline), minimally patched for modern Linux, and placed on GitHub. It requires gcc, make, autoconf, bison/yacc, and gawk to build.

Roger Bailey's Hope Tutorial (`doc/hope_tut.src`) is included in the repository; the reference manual (`doc/ref_man.tex`) was written by Paterson himself.

---

## Directory Layout

```
hope/
  src/          C source for the interpreter (36 .c files + headers)
  lib/          Standard library (23 .hop modules, including Standard.hop)
  doc/          LaTeX reference manual + Bailey's tutorial
  test/         6 test input files with expected outputs
  sh/           Build helper scripts
  claude-docs/  This documentation
  cxx/          C++20 rewrite (planned)
  configure.in  Autoconf template
  Makefile      Top-level build orchestration
```

---

## Build System

The build uses the classic autoconf/yacc/make stack:

1. `autoconf` processes `configure.in` to produce a `configure` script.
2. `configure` sets paths (install prefix, library location) and probes for tools.
3. `src/Makefile.in` becomes `src/Makefile` with paths substituted.
4. The parser is **not** committed to the repo as C source. It is regenerated from `src/yyparse.y` via a pipeline:
   - `src/op.h` defines all binary operator tokens.
   - `src/Mult-op.awk` generates `op.sed` from `op.h`.
   - `op.sed` is applied to `yyparse.y` to replicate grammar rules for each binary operator.
   - `src/Assoc.sed` adds operator associativity rules.
   - The transformed grammar is fed to `yacc`/`bison` to produce `yyparse.c` + `yyparse.h`.
5. All 36 C source files plus `yyparse.c` are compiled and linked into the `hope` executable.
6. `make check` runs all six test cases, comparing output against `test/*.out`.
7. `make check` also loads every `lib/*.hop` module to verify they parse and type-check.

**Key problem for the rewrite:** autoconf + yacc are not available on all modern systems without extra packages, and bison 3+ introduces incompatibilities. The C++20 rewrite eliminates all of this.

---

## Execution Pipeline

A Hope session proceeds through these stages:

```
stdin / file
    │
    ▼
[Lexer]          src/yylex.c
    │  token stream
    ▼
[Parser]         src/yyparse.y  (YACC grammar → yyparse.c)
    │  AST (Expr, Pat, TypeExpr nodes)
    ▼
[Module loader]  src/module.c
    │  resolves `uses`, loads .hop files, manages scopes
    ▼
[Type checker]   src/type_check.c + type_value.c + deftype.c
    │  infers / checks types, instantiates type schemes
    ▼
[Pattern compiler] src/compile.c + cases.c
    │  compiles multi-equation definitions to decision trees
    ▼
[Evaluator]      src/interpret.c  (Krivine machine)
    │  lazy call-by-need reduction
    ▼
[Pretty printer] src/pr_value.c + pr_type.c
    │  formatted output
    ▼
stdout
```

Each stage is driven by the YACC parser: `yyparse()` is the outer loop. The parser calls type-checking and evaluation directly as it reduces productions — there is no explicit separate pass that walks the AST end-to-end.

---

## Source File Inventory

### Entry point
| File | Purpose |
|------|---------|
| `main.c` | Argument parsing, initialisation, calls `yyparse()` |

### Lexer & Parser
| File | Purpose |
|------|---------|
| `yylex.c` | Hand-written lexer; produces tokens for YACC |
| `yyparse.y` | YACC grammar; builds AST and drives the entire pipeline |
| `op.h` | Defines all binary operator token names and precedences |
| `Assoc.sed` | Injects `%left`/`%right` declarations for each operator |
| `Mult-op.awk` | Generates `op.sed` from `op.h` |

### AST Representation
| File | Purpose |
|------|---------|
| `expr.c` / `expr.h` | Expression and pattern node types + constructors |
| `value.c` / `value.h` | Runtime value representation |
| `functors.c` / `functors.h` | Functor (map function) data for each data type |
| `cons.h` | Constructor tag types |

### Type System
| File | Purpose |
|------|---------|
| `type_check.c` | Hindley-Milner type inference and unification |
| `type_value.c` | Type value representation: mono-types, poly-types, type vars |
| `deftype.c` | Algebraic data type and type abbreviation definitions |
| `functor_type.c` | Computes the type of auto-generated functor (map) functions |
| `polarity.c` | Analyses polarity of type variable occurrences (pos/neg/both) |
| `bad_rectype.c` | Validates that recursive type definitions are well-formed |
| `remember_type.c` | Memoises type lookups |

### Pattern Matching Compilation
| File | Purpose |
|------|---------|
| `compile.c` | Compiles multi-equation function definitions to decision trees |
| `cases.c` | Handles `case` sub-expressions during compilation |

### Lazy Evaluation Runtime
| File | Purpose |
|------|---------|
| `interpret.c` | **Core evaluator.** Krivine machine with call-by-need, update frames, thunks |
| `eval.c` | Top-level entry point for evaluating an expression |
| `runtime.c` | Heap allocation, stack management, garbage collector |
| `memory.c` | Low-level allocator: free-list, arena, GC roots |
| `path.c` | Navigates paths through heap graph nodes |

### Built-in Functions & I/O
| File | Purpose |
|------|---------|
| `builtin.c` | All built-in functions: arithmetic, comparisons, type conversions, I/O |
| `number.c` | Numeric type implementation (integer + optional float) |
| `char.c` | Character type |
| `char_array.c` | Character arrays / string representation |
| `compare.c` | Structural comparison (`compare`, `=`, `<`, etc.) |
| `stream.c` | Lazy stream / lazy I/O handling |
| `output.c` | Output buffering and formatting |
| `interrupt.c` | Signal handling (Ctrl-C) |

### Module System
| File | Purpose |
|------|---------|
| `module.c` | Module loading, `uses`, `private`, scope management |
| `source.c` | Source file reading, listing generation |

### Utilities
| File | Purpose |
|------|---------|
| `newstring.c` | Interned string table |
| `table.c` | Hash table |
| `set.c` | Simple set operations |
| `error.h` | Error macro definitions |

### Pretty Printers
| File | Purpose |
|------|---------|
| `pr_expr.c` | Expression pretty-printer |
| `pr_type.c` | Type pretty-printer |
| `pr_value.c` | Value pretty-printer |
| `pr_ty_value.c` | Typed value pretty-printer |

---

## Data Flow in More Detail

### Startup
```
main()
  init_memory()      allocate heap + stack
  init_strings()     initialise interned string table
  init_lex()         reset lexer state
  init_source()      attach stdin or -f file
  mod_init()         load the Standard module (lib/Standard.hop)
  preserve()         mark Standard module's heap as permanent (GC root)
  yyparse()          main parse/execute loop
```

### Per-definition flow (inside yyparse)
The YACC grammar triggers actions when it recognises:
- A `dec` declaration → registers the identifier and its declared type in the module's symbol table
- A `data` or `type` definition → calls into `deftype.c` to build the type representation, registers constructors
- An equation (`--- f pat <= expr`) → calls `compile.c` to compile the pattern into a decision tree, registers the resulting code in the module
- A `uses Module;` directive → calls `module.c` to locate and load `Module.hop`
- An expression (interactive evaluation) → calls `type_check.c` to infer the type, then `eval.c` + `interpret.c` to evaluate it, then `pr_value.c` to print the result

### Memory model
The runtime uses a single flat heap divided into:
- **Heap cells**: fixed-size tagged nodes (application, constructor, integer, thunk, etc.)
- **Stack**: a separate region for the Krivine machine's continuation stack
- **Preserved zone**: the Standard module's cells, never GC'd

Garbage collection is a simple mark-and-sweep or free-list compaction. The ratio of heap to stack is configurable (default 5:1 in the source).

---

## Why We're Rewriting

The existing interpreter is functionally correct and historically important, but:

| Problem | Impact |
|---------|--------|
| autoconf + yacc/bison required at build time | Hard to build on modern systems without extra packages |
| Error messages are terse and location-free | Poor learning experience; makes the tutorial frustrating |
| No REPL `:load`, `:type`, `:reload` commands | Limited interactivity |
| `getline` name clash with POSIX (required a rename to `hope_getline`) | Portability issue |
| No locale/i18n support for UI strings | English-only |
| Global C state throughout | Hard to test, extend, or embed |
| C89/C90 style (no stdint, no const everywhere, `global` typedef) | Does not compile cleanly with modern C standards |

The `cxx/` rewrite targets C++20, CMake, zero external dependencies, a proper REPL, rich error messages with source locations, and externalised strings for localisation.

---

## Cross-references

- Lexer & parser details: [01-lexer-parser.md](01-lexer-parser.md)
- AST and expression nodes: [02-ast-expressions.md](02-ast-expressions.md)
- Type system: [03-type-system.md](03-type-system.md)
- Pattern compilation: [04-pattern-compilation.md](04-pattern-compilation.md)
- Runtime and Krivine machine: [05-runtime-krivine.md](05-runtime-krivine.md)
- Built-ins and I/O: [06-builtins-io.md](06-builtins-io.md)
- Module system: [07-module-system.md](07-module-system.md)
- Pretty printers: [08-pretty-printing.md](08-pretty-printing.md)
- Standard library: [09-standard-library.md](09-standard-library.md)
- Test suite: [10-test-suite.md](10-test-suite.md)
- C++20 rewrite roadmap: [11-rewrite-roadmap.md](11-rewrite-roadmap.md)
