# Hope — A Functional Programming Language

> *A lazy interpreter for the Hope functional programming language,*
> *together with a modern C++20 reimplementation.*

---

## Contents

1. [What this repository is](#1-what-this-repository-is)
2. [The history and influence of Hope](#2-the-history-and-influence-of-hope)
3. [The C++20 reimplementation](#3-the-c20-reimplementation)
4. [Ross Paterson's original C interpreter](#4-ross-patersons-original-c-interpreter)
5. [Conclusions and acknowledgements](#5-conclusions-and-acknowledgements)

---

## 1  What this repository is

This repository preserves two things:

- **Ross Paterson's lazy Hope interpreter**, written in C around 1990–1993 and
  originally hosted at City University London.  The files vanished from the web
  around 2013; this copy was retrieved from the Internet Archive and patched to
  build on modern macOS and Linux.  The original text is archived as
  [`readme`](readme) at the repository root.

- **A complete C++20 reimplementation** of the interpreter, written from scratch
  in the `cxx/` subdirectory.  It is faithful to Paterson's semantics and
  language definition while providing a substantially improved interactive
  experience.

Both implementations run Roger Bailey's Imperial College Hope tutorial
(`doc/hope_tut.src`, also available as [`cxx/doc/hope_tut.md`](cxx/doc/hope_tut.md))
and the full standard library inherited from Paterson's distribution.

---

## 2  The history and influence of Hope

### Origins at Edinburgh

Hope was designed at the University of Edinburgh in the late 1970s by
**Rod Burstall**, **David MacQueen**, and **Don Sannella**, building on earlier
work in the logic-programming and algebraic-specification communities.  The
language took its name — appropriately — from Hope Park, the area of Edinburgh
where the Department of Computer Science was located.

The original Hope paper, *HOPE: An Experimental Applicative Language*
(Burstall, MacQueen, Sannella, 1980), introduced several ideas that became
foundational in functional programming:

- **Algebraic data types** with pattern matching — the basis of `data`
  declarations in Hope, ML, Haskell, and virtually every subsequent typed
  functional language.
- **Polymorphic type inference** in the Hindley–Milner tradition, making strong
  static typing practical without requiring the programmer to annotate every
  expression.
- **A clean separation** between the language's equational semantics and its
  operational implementation, enabling formal reasoning about programs.

### Imperial College and teaching

**Roger Bailey** at Imperial College London developed Hope into a teaching
language during the early 1980s.  His tutorial — reproduced here in
[`doc/hope_tut.src`](doc/hope_tut.src) — introduced generations of Imperial
students to functional programming through a carefully graded sequence from
simple recursion to higher-order functions and polymorphism.  The tutorial
remains one of the clearest introductions to functional programming ever
written.

The Imperial implementation was a *strict* (call-by-value) interpreter.
Sadly, the original Imperial source code has not been located; this repository
contains only Paterson's later lazy implementation.

### The Alvey Programme and the Flagship project

In the mid-1980s, Hope became a research vehicle in the UK's
[Alvey Programme](https://en.wikipedia.org/wiki/Alvey), a government-funded
initiative in advanced information technology.  The **Flagship project**,
a collaboration between ICL, Imperial College, and several universities, aimed
to build a parallel computer whose architecture was natively suited to
functional programming.  Hope and its derivatives were used both to program
the machine and to reason formally about the programs running on it.

**John Darlington** and colleagues at Imperial developed a systematic
programme of **program transformation** for Hope: starting from a clear,
obviously-correct specification written in Hope, mechanical transformation
rules — grounded in the lambda calculus — could derive an efficient
implementation that was provably equivalent to the original.  This work
influenced the later development of program fusion and deforestation in
Haskell, and demonstrated that equational reasoning, far from being merely
theoretical, could be applied industrially.

### Influence on subsequent languages

Hope's direct influence on later languages is substantial:

| Language | Influence from Hope |
|---|---|
| **Standard ML** (1983–90) | Shared Edinburgh parentage; algebraic types, pattern matching, module system concepts |
| **Miranda** (1985) | David Turner adopted Hope-style algebraic types and polymorphism; added lazy evaluation |
| **Haskell** (1990–) | Synthesised Miranda's laziness with a Hope/ML-style type system; became the dominant research FP language |
| **Erlang** (1986–) | Pattern matching on algebraic terms; equational style of function definition |
| **Scala**, **F#**, **Rust**, **Swift** | Algebraic data types and exhaustive pattern matching in mainstream languages, tracing directly back to Hope |

The `data` declaration, the `::` list constructor, and the `#` product type
operator that appear throughout Hope programs are all recognisable to any
Haskell or ML programmer.  The intellectual lineage is direct.

### Functional programming and scalable systems

The transformation-based reasoning that Hope's design encouraged turns out to
be practically important for modern distributed systems.  Referential
transparency — the property that a function's result depends only on its
arguments, never on hidden state — makes it straightforward to parallelise,
replicate, and cache computations.  The map/reduce pattern, ubiquitous in
cloud data processing from Google's MapReduce to Apache Spark, is a direct
descendant of the `map` and `reduce` higher-order functions Bailey introduces
in his tutorial.

Lambda calculus, which underpins Hope's equational semantics, gives a
mathematical basis for reasoning about the correctness of these transformations.
Darlington's work on Hope established that such reasoning can be made
systematic and machine-assisted — a line of work that connects directly to
modern automated verification of distributed protocols.

### Strict versus lazy evaluation

Both implementations in this repository use **lazy (call-by-need) evaluation**,
following Paterson's design.  It is worth understanding what this means and
how it differs from the strict Imperial interpreter that is not preserved here.

**Strict (call-by-value) evaluation**, used in the original Imperial Hope
interpreter and in Standard ML, evaluates all function arguments before the
function body is entered.  The semantics are predictable and close to
conventional programming languages.  Termination behaviour matches intuition:
if an argument does not terminate, the function call does not terminate.

**Lazy (call-by-need) evaluation**, the strategy chosen by Paterson and
retained in the C++20 reimplementation, defers evaluation of an argument
until — and only until — its value is actually needed, and memoises the result
so the computation is performed at most once.  This enables:

- **Infinite data structures**: a list such as all prime numbers or all
  Fibonacci numbers can be defined without an explicit bound, and a consumer
  simply takes as many elements as it needs.
- **Separating generation from consumption**: the sieve of Eratosthenes, for
  instance, is written as a clean recursive specification over an infinite
  stream.
- **More general termination behaviour**: a function can return a useful partial
  result even if some sub-computation does not terminate.

The trade-off is that space usage is harder to predict (unevaluated thunks
accumulate on the heap) and that reasoning about performance requires more
care.  Hope's type system is compatible with both strategies; the same program
text will produce the same result under either regime for programs that
terminate, though a lazy evaluator may succeed on inputs that would cause a
strict evaluator to loop.

Paterson's interpreter supports **regular types** (potentially infinite types
described by fixed-point equations) and **auto-generated functors** (a
`map`-like function for every type constructor), both of which depend on or
are most natural in a lazy setting.

---

## 3  The C++20 reimplementation

### Goals

The reimplementation in `cxx/` was created to provide a version of Paterson's
interpreter that:

- Builds without modification on contemporary toolchains (GCC 10+, Clang 12+,
  MSVC 19.29+, CMake 3.20+).
- Requires **zero external dependencies** beyond the C++ standard library.
- Is faithful to **Paterson's lazy semantics** and the full Hope language as
  described in `doc/ref_man.tex` (also available as
  [`cxx/doc/ref_man.md`](cxx/doc/ref_man.md)).
- Provides a substantially **improved interactive REPL** to make the language
  easier to learn and explore.
- Comes with a comprehensive **test suite** (unit tests via Google Test,
  integration tests comparing output against Paterson's interpreter).

### Source layout

```
cxx/
  CMakeLists.txt          top-level build
  CMakePresets.json       preset configurations (release, strict, CI, ...)
  src/
    main.cpp              entry point
    lexer/
      Lexer.cpp/.hpp      hand-written lexer with source locations
      Token.hpp           token types
      SourceLocation.hpp  file/line/column tracking
    parser/
      Parser.cpp/.hpp     recursive-descent parser
      OperatorTable.hpp   infix precedence/associativity table
      ParseError.hpp      structured parse errors
    ast/
      Ast.hpp             std::variant-based AST node types
    types/
      TypeChecker.cpp/.hpp  Hindley-Milner type inference
      TypeEnv.cpp/.hpp      type environment (scoping)
      Type.hpp              type representation
      TypeError.hpp         structured type errors
    runtime/
      Evaluator.cpp/.hpp  call-by-need tree-walking evaluator (Krivine-style)
      Value.hpp           runtime value representation
      RuntimeError.hpp    runtime error types
    builtins/
      Builtins.cpp/.hpp   built-in functions (arithmetic, I/O, math library)
    modules/
      ModuleLoader.cpp/.hpp  .hop file loading, private/abstype enforcement
    printer/
      ExprPrinter.cpp/.hpp   expression pretty-printer
      TypePrinter.cpp/.hpp   type pretty-printer
      ValuePrinter.cpp/.hpp  value pretty-printer
    repl/
      Session.cpp/.hpp    interactive session state, REPL loop
  test/
    unit/
      LexerTest.cpp       lexer unit tests
      ParserTest.cpp      parser unit tests
      TypeTest.cpp        type inference unit tests
      EvalTest.cpp        evaluator unit tests
      SessionTest.cpp     REPL/session integration tests
    integration/          CTest-driven subprocess tests against .out files
  doc/
    ref_man.md            Markdown conversion of Paterson's reference manual
    hope_tut.md           Markdown conversion of Bailey's Imperial tutorial
    user-guide.md         User guide for the C++20 interpreter
    ci-setup.md           CI configuration notes
  lib -> ../lib           symlink to the shared Hope standard library
```

### The standard library

The `lib/` directory (shared between both implementations) contains 23 Hope
modules:

| Module | Contents |
|---|---|
| `Standard.hop` | Core types: `bool`, `num`, `char`, `list`, `#`, `->`, `relation`; arithmetic, comparison, I/O primitives |
| `list.hop` / `lists.hop` | List operations: `reverse`, `filter`, `take`, `drop`, `zip`, `<>`, ... |
| `maybe.hop` | The `maybe` option type |
| `seq.hop` | Lazy sequences (infinite lists) |
| `arith.hop` | Fibonacci numbers, prime sieve (exercises lazy evaluation) |
| `range.hop` | Integer ranges with `..` notation |
| `sort.hop` | Sorting algorithms |
| `set.hop` | Sets as ordered lists |
| `tree.hop` | Binary search trees |
| `fold.hop` | Generic fold operations |
| `words.hop` | String / word processing |
| `functions.hop` | Function combinators (`o`, `id`, `const`, `flip`, ...) |
| `products.hop` | Product type utilities |
| `sums.hop` | Sum type utilities |
| `diag.hop` | Diagonal enumeration of pairs |
| `y.hop` | Fixed-point combinators |
| `void.hop` | The empty type |
| `ctype.hop` | Character classification |
| `lines.hop` | Line-by-line I/O |
| `case.hop` | Case-insensitive character operations |

### The improved REPL

While the language semantics are faithful to Paterson's implementation, the
interactive experience has been substantially improved.

**Meta-commands** (prefixed with `:`):

| Command | Effect |
|---|---|
| `:load <file.hop>` | Load a Hope source file into the session |
| `:reload` | Reload the last loaded file |
| `:save <Module>` | Save current session definitions to `Module.hop` |
| `:type <expr>` | Evaluate and display the type of an expression |
| `:display` | List all definitions in the current session |
| `:clear` | Reset the session to the initial state |
| `:help` or `:?` | Show available commands |
| `:quit` or `:exit` | Exit the interpreter |

**Hope-level session commands** (as in Paterson's original):

| Command | Effect |
|---|---|
| `expr;` | Evaluate and display value and type |
| `write expr;` | Lazy streaming output (characters printed as computed) |
| `write expr to "file";` | Write output to a named file |
| `display;` | Display current definitions |
| `save ModuleName;` | Save session to `ModuleName.hop` |
| `uses ModuleName;` | Import a module |
| `exit;` | Exit |

Error messages include **source locations** (file, line, column) and
distinguish clearly between parse errors, type errors, and runtime errors.
A REPL error does not terminate the session.

### Building

```bash
cmake --preset release          # configure
cmake --build --preset release  # build
ctest --preset release          # run tests (285 tests)
```

The `hope` binary is placed in `cxx/build_release/`.

### Documentation

- [`cxx/doc/user-guide.md`](cxx/doc/user-guide.md) — full user guide
- [`cxx/doc/ref_man.md`](cxx/doc/ref_man.md) — language reference (from Paterson's LaTeX original)
- [`cxx/doc/hope_tut.md`](cxx/doc/hope_tut.md) — Bailey's tutorial (from the Imperial LaTeX original)

### Source analysis documents

Before writing a line of the reimplementation, Paterson's original C source
was analysed in depth.  The results are in `claude-docs/`:

| Document | Contents |
|---|---|
| [`00-overview.md`](claude-docs/00-overview.md) | Architecture, data flow, file inventory, build system |
| [`01-lexer-parser.md`](claude-docs/01-lexer-parser.md) | `yylex.c`, `yyparse.y`, operator tables, AST construction |
| [`02-ast-expressions.md`](claude-docs/02-ast-expressions.md) | `expr.c/h`, `functors.c/h`, `value.c/h`, pattern representation |
| [`03-type-system.md`](claude-docs/03-type-system.md) | Hindley–Milner inference, `type_check.c`, regular types |
| [`04-pattern-compilation.md`](claude-docs/04-pattern-compilation.md) | `compile.c`, `cases.c`, decision trees |
| [`05-runtime-krivine.md`](claude-docs/05-runtime-krivine.md) | `interpret.c` (Krivine machine), GC, memory layout |
| [`06-builtins-io.md`](claude-docs/06-builtins-io.md) | `builtin.c`, number/char/stream/output |
| [`07-module-system.md`](claude-docs/07-module-system.md) | `module.c`, source/path handling |
| [`08-pretty-printing.md`](claude-docs/08-pretty-printing.md) | `pr_expr.c`, `pr_type.c`, `pr_value.c` |
| [`09-standard-library.md`](claude-docs/09-standard-library.md) | Annotated walkthrough of all 23 `.hop` library modules |
| [`10-test-suite.md`](claude-docs/10-test-suite.md) | Test infrastructure and coverage |
| [`11-rewrite-roadmap.md`](claude-docs/11-rewrite-roadmap.md) | Design notes for the C++20 rewrite |
| [`12-claude-workflow.md`](claude-docs/12-claude-workflow.md) | Notes on the analysis process |

---

## 4  Ross Paterson's original C interpreter

### History

Ross Paterson wrote this interpreter at the Department of Computing, City
University London, around 1990–1993.  It was distributed freely from his
departmental web page and formed the basis for practical Hope programming
long after Imperial's original strict interpreter had ceased to be maintained.

The original page at `http://www.soi.city.ac.uk/~ross/Hope/` disappeared
around 2013.  It can still be viewed via the Internet Archive:

- https://web.archive.org/web/20131022235656/http://www.soi.city.ac.uk/~ross/Hope/

This copy was retrieved from that archive.

### Differences from the original strict Imperial interpreter

Paterson's interpreter diverges from the original Imperial Hope in several
deliberate ways, all of which extend the language rather than restrict it.
The original strict Imperial source code has not been located; the following
is based on the language definition in Bailey's tutorial and Paterson's own
documentation.

| Feature | Imperial (strict) | Paterson (lazy) |
|---|---|---|
| Evaluation strategy | Call-by-value | Call-by-need (lazy) |
| Infinite data structures | Not supported | First-class; used throughout `lib/` |
| `input` variable | Not present | Lazy list of stdin characters |
| `letrec` / `whererec` | Not present | Supported |
| Irrefutable patterns | Not present | Supported (variables and pairs) |
| Operator sections | Not present | `(e op)` and `(op e)` |
| Regular types | Not present | `mu s => alpha # s`; printed by system |
| Auto-generated functors | Not present | Every `data`/`type` gets a `map` |
| Curried type/data constructors | Not present | Supported |
| `abstype` | Not present | Supported |
| `private` | Not present | Supported |
| Multi-argument functions | Not present | Supported (as tuples) |
| `nonop` prefix | Present | Present (retained for compatibility) |

The `nonop` prefix, used to pass an infix operator as a value (e.g.
`reduce(l, nonop +, 0)`), is a standard feature of both implementations.
The C++20 interpreter additionally accepts plain parenthesised operators
`(+)` in the Haskell style as a compatibility extension.

### Building the original interpreter

The original interpreter uses autoconf and YACC/Bison.  On a modern macOS or
Debian-like Linux:

**macOS (Homebrew):**
```bash
brew install bison autoconf
export PATH="/opt/homebrew/opt/bison/bin:$PATH"
./configure
make
make check
```

**Debian / Ubuntu:**
```bash
sudo apt-get install gcc make bison autoconf byacc gawk
./configure
make
make check
```

The `make check` target runs the test suite in `test/` against committed
expected outputs.

### Source layout of the original interpreter

```
src/         C source for the interpreter
  interpret.c   Krivine-style lazy evaluator (the heart of the system)
  type_check.c  Hindley-Milner type inference
  yyparse.y     YACC grammar
  compile.c     Pattern match compilation to decision trees
  module.c      Module loading and scoping
  runtime.c     Heap and garbage collection
  builtin.c     Built-in functions
  ...
doc/         Reference manual and tutorial (LaTeX)
lib/         Hope standard library modules
test/        Test inputs and expected outputs
sh/          Miscellaneous shell scripts
```

---

## 5  Conclusions and acknowledgements

### Keeping old code alive

Hope is not widely taught today.  Its direct descendants — Haskell, ML, and
the functional subsets of Scala, Rust, and Swift — are the languages that
programmers reach for when they want algebraic data types, strong type
inference, and equational reasoning.  But there is value in being able to
run the original.

Old source code is fragile.  Interpreters written for SunOS 4 and VAX
BSD Unix stop building when the compiler changes, when the YACC dialect
changes, when `getline` becomes a POSIX standard function and the name
clashes.  Without active maintenance, such code quietly disappears.
Paterson's interpreter very nearly did.

This repository exists because it is worth preserving working software that
embodies important ideas — both as a historical artefact and as a teaching
tool.  Bailey's tutorial, run against a working interpreter, remains an
excellent introduction to functional programming.  The standard library modules
(`arith.hop` and its lazy Fibonacci sequence and prime sieve, `sort.hop` and
its tree-sort, `words.hop` and its string processing) are small, readable, and
instructive.

The C++20 reimplementation extends this goal: to make Hope accessible on any
contemporary machine, without dependencies, without a build system from 1993,
and with the kind of interactive feedback that learners now expect.

### Acknowledgements

- **Rod Burstall**, **David MacQueen**, and **Don Sannella** — designers of Hope
  at Edinburgh.
- **Roger Bailey** — Imperial College; the tutorial that introduced Hope to
  thousands of students.
- **John Darlington** and the Imperial functional programming group — program
  transformation, the Alvey Flagship project, and the formal foundations of
  equational reasoning in Hope.
- **Ross Paterson** — the lazy C interpreter that is the basis of this
  repository.  His implementation extended the language in several important
  directions and kept Hope usable long after the original Imperial code had
  ceased to be maintained.
- **Djordje Baturin** — whose fork (`dmbaturin/hope` on GitHub) provided an
  early reference point for getting the code to build on modern Linux.
- All contributors to the Alvey Programme and to the broader UK functional
  programming community of the 1980s and 1990s.

### Licence

Ross Paterson's original interpreter is distributed under the **GNU General
Public License, version 2 or later**.  Roger Bailey's Hope Tutorial is
included with permission and is not covered by the GPL.  The C++20
reimplementation in `cxx/` is released under the same GPL v2+ terms.

---

*This repository is maintained at
[github.com/rextanka/hope](https://github.com/rextanka/hope).*
