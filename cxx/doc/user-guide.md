# Hope Interpreter — User Guide

This guide covers building the C++20 Hope interpreter, setting it up, and
using it interactively and in batch mode.

---

## Building

### Requirements

- CMake ≥ 3.20
- A C++20-conformant compiler: GCC 14+, Clang 18+, Apple Clang 17+
- Internet access at configure time (Google Test is fetched automatically)

### Build steps

```bash
cmake -S cxx -B cxx/build -DCMAKE_BUILD_TYPE=Release
cmake --build cxx/build --parallel
```

The `hope` binary is placed at `cxx/build/src/hope`.

---

## Environment variables

### `HOPEPATH`

The interpreter searches this directory for standard-library `.hop` modules
(`Standard.hop`, `list.hop`, `seq.hop`, etc.).

```bash
export HOPEPATH=/path/to/hope/lib
```

If `HOPEPATH` is not set the binary falls back to `HOPELIB`, a path baked in
at compile time that points to `<repo>/lib`.  A freshly-built binary therefore
works without setting `HOPEPATH` as long as you run it from within the repo.

**Summary of resolution order:**

| Priority | Source |
|----------|--------|
| 1 (highest) | `HOPEPATH` environment variable |
| 2 | `HOPELIB` compile-time constant (default: `<repo>/lib`) |
| 3 | `<executable-dir>/../lib` (relative fallback) |

### `HOPE_EDITOR` and `EDITOR`

`HOPE_EDITOR` sets the editor launched by the `edit;` statement and the
`:edit` REPL command.  It takes precedence over the generic `EDITOR`
environment variable, which the interpreter also honours as a fallback.

If neither variable is set the interpreter auto-detects VS Code (`code` or
`code-insiders`) from `PATH`.  If VS Code is not found either, the command
prints a message and returns to the prompt without launching an editor.

**Editor selection order:**

| Priority | Source |
|----------|--------|
| 1 (highest) | `HOPE_EDITOR` environment variable |
| 2 | `EDITOR` environment variable |
| 3 | Auto-detect `code` / `code-insiders` in PATH |
| 4 | Error message (no editor found) |

**Examples:**

```bash
export HOPE_EDITOR="code --wait"   # VS Code (--wait added automatically if omitted)
export HOPE_EDITOR=nano            # nano in the terminal
export HOPE_EDITOR=vim             # Vim in the terminal
export HOPE_EDITOR="subl --wait"   # Sublime Text
```

`--wait` is appended automatically when `code` or `code-insiders` appear in
`HOPE_EDITOR` without it — so `export HOPE_EDITOR=code` is equivalent to
`export HOPE_EDITOR="code --wait"`.  For all other editors the value is used
as-is; add `--wait` (or the editor's equivalent blocking flag) yourself if the
command would otherwise return immediately.

---

## Running the interpreter

### Batch mode

Evaluate a Hope source file and print results to stdout:

```bash
hope -f myprogram.hop
```

### Interactive mode (REPL)

Run without arguments when stdin is a terminal:

```bash
hope
```

---

## Line editing

When running interactively on a POSIX system (macOS, Linux, WSL), the REPL
provides full line editing with history.  When stdin is not a terminal (piped
input, batch scripts, Windows native) the interpreter falls back to simple
line input — everything still works, just without the editing features.

### Key bindings

| Key | Action |
|-----|--------|
| `Left` / `Ctrl-B` | Move cursor left one character |
| `Right` / `Ctrl-F` | Move cursor right one character |
| `Home` / `Ctrl-A` | Move cursor to start of line |
| `End` / `Ctrl-E` | Move cursor to end of line |
| `Up` / `Ctrl-P` | Recall previous history entry |
| `Down` / `Ctrl-N` | Recall next history entry (or blank line) |
| `Backspace` | Delete character before cursor |
| `Delete` / `Ctrl-D` | Delete character at cursor; EOF on empty line |
| `Ctrl-U` | Clear entire line |
| `Ctrl-K` | Kill (delete) from cursor to end of line |
| `Ctrl-W` | Delete previous word |
| `Ctrl-L` | Clear screen and redraw current line |
| `Ctrl-C` | Discard current line and show a fresh prompt |
| `Enter` | Submit the line |

### History

History entries are complete Hope **statements** (everything up to and
including the terminating `;`), not individual physical lines.  This means
that a multi-line declaration recalled from history arrives already formatted
across multiple lines.  Meta-commands (`:load`, `:type`, etc.) are also
added to history.

Consecutive duplicate entries are suppressed so that repeatedly evaluating
the same expression does not fill the history with identical lines.

History is **in-memory only** for this release — it is not persisted between
sessions.

### Copy and paste

The REPL enables **bracketed paste mode** on terminals that support it
(macOS Terminal, iTerm2, most Linux terminals).  Pasted text is inserted at
the cursor position without being misinterpreted as key sequences.

---

## REPL commands

The REPL accepts Hope expressions and declarations, plus two classes of
commands: **meta-commands** (prefixed with `:`) and **Hope-level commands**
(terminated with `;` like any other statement).

### Meta-commands

| Command | What it does |
|---------|-------------|
| `:load <file.hop>` | Load and execute a `.hop` file into the current session.  All declarations in the file become available immediately. |
| `:reload` | Re-run the most recently loaded file (useful after editing). |
| `:edit [module]` | Open a file in the configured editor (see [Editor configuration](#hope_editor-and-editor)), then reload it automatically when the editor exits.  Without a module name, edits the last loaded file; if no file has been loaded, opens a temp file containing the current session definitions. |
| `:type <expr>` | Evaluate *expr* and display its value together with its inferred type. |
| `:display` | List all user declarations entered in the current session. |
| `:clear` | Clear session history (removes display records and reload state; standard library remains; definitions stay in scope). |
| `:quit` / `:exit` / `:q` | Exit the interpreter. |
| `:help` / `:?` | Print a summary of available meta-commands. |

### Hope-level commands

These are regular Hope statements entered at the prompt:

| Command | What it does |
|---------|-------------|
| `expr;` | Evaluate *expr* and print `>> value : type`. |
| `write expr;` | Stream the value of *expr* (a list) to stdout without the `>> … : type` wrapper.  Characters are printed directly; other elements are printed one per line.  Each element is output as soon as it is computed (lazy streaming). |
| `write expr to "file";` | Like `write expr;` but output goes to *file* (created or overwritten). |
| `display;` | Display all definitions entered in the current session (Hope-level equivalent of `:display`). |
| `save` *ModuleName*`;` | Write the current session definitions to *ModuleName*`.hop` in the current directory. |
| `uses` *ModuleName*`;` | Load a library module and make its definitions available. |
| `exit;` | Exit the interpreter (equivalent to `:quit`). |
| `edit;` | Open the last loaded file (or the current session definitions in a temp file) in the configured editor, then reload automatically.  Equivalent to `:edit` at the meta-command prompt.  `edit` *ModuleName*`;` opens a specific module. |

### Evaluating expressions

Type any Hope expression followed by `;` to evaluate it and print the result
with its type:

```
hope> 1 + 1;
>> 2 : num

hope> map (+ 1) [1, 2, 3];
>> [2, 3, 4] : list num
```

### Declarations

Enter function and data declarations directly at the prompt:

```
hope> dec double : num -> num;
hope> --- double n <= n * 2;
hope> double 7;
>> 14 : num
```

Multi-line input is accepted; the interpreter waits for the closing `;`.

### Loading files

```
hope> :load examples/factorial.hop
hope> factorial 10;
>> 3628800 : num
```

### Inspecting types

`:type` evaluates the expression and prints the result with its type:

```
hope> :type 1 + 1;
>> 2 : num
```

For a function value, the type is printed without a `>> ` prefix if the
value has no printable form:

```
hope> :type map;
map : (alpha -> beta) -> list alpha -> list beta
```

### Lazy output with `write`

`write expr;` streams characters to stdout without buffering the entire
result first, and without the `>> value : type` wrapper.  Use it for lazy
or infinite output:

```
hope> write "hello\n";
hello
hope> uses seq;
hope> write (front_seq(5, gen_seq (+ 1) 1));
[1, 2, 3, 4, 5]
```

To save output to a file, use `to "filename"`:

```
hope> write "hello world" to "output.txt";
hope> write (front_seq(10, primes)) to "primes.txt";
```

The file is created (or overwritten) and receives the same output that
`write expr;` would have printed to the terminal.

### Saving and reloading a session

```
hope> dec sq : num -> num;
hope> --- sq n <= n * n;
hope> save mysession;
Saved to mysession.hop.
hope> :quit

$ hope
hope> uses mysession;
hope> sq 7;
>> 49 : num
```

---

## Standard library modules

Modules are loaded with `uses <ModuleName>;` at the top of a file or at the
REPL prompt.  Standard modules live in `<HOPEPATH>/`.

| Module | Key exports |
|--------|-------------|
| `Standard` | Loaded automatically. `bool`, `list`, `num`, `char`, arithmetic, comparison, `map`, `filter`, `foldl`, `foldr`, `append`, `length`, `reverse`, `nth`, `hd`, `tl`, `null`, `not`, … |
| `list` | Additional list utilities: `||` (zip), `front`, `after`, `@` (index), `iterate`, `partition`, `span`, `front_with`, `after_with` |
| `lists` | Extended list operations: `member`, `many`, `repeat`, `--` (difference), `last`, `init`, `rotl`, `rotr` |
| `arith` | Extended arithmetic: `fibs`, `primes` (via lazy sequences) |
| `seq` | Lazy sequences: `seq alpha`, `gen_seq`, `front_seq`, `filter_seq`, `map_seq` |
| `maybe` | Option type: `maybe alpha` (`No` / `Yes x`), `unit_maybe`, `bind_maybe`, `AND`, `OR`, `-->`, `flatten` |
| `tree` | Binary trees: `tree alpha` (`Tip` / `Branch`), `fold_tree`, `flatten`, `show_tree` |
| `sort` | Sorting: `sort` (merge sort), `uniq` |
| `set` | Ordered sets (abstract type): `{}` / `empty`, `&` (insert), `U` (union), `choose`, `card` |
| `lines` | Line-oriented I/O: `lines`, `unlines` |
| `fold` | Higher-order folds: `fold`, `unfold`, `rec` |
| `functions` | Function combinators: `o` (compose), `id`, `const`, `curry`, `uncurry` |
| `products` | Pair operations |
| `sums` | Sum type: `alpha OR beta` (`Left` / `Right`), `\/` (case analysis), `either` |
| `range` | Numeric ranges: `..` notation |
| `words` | Word splitting/joining: `words`, `unwords` |
| `ctype` | Character classification: `isalpha`, `isdigit`, `isspace`, `toupper`, `tolower` |
| `diag` | Diagonal enumeration of pairs: `//` (zip as pairs), `diagonal` |
| `case` | Case combinator: `case x f` applies `f` to `x` |
| `y` | Fixed-point combinators: `Ycurry`, `Yturing` |
| `void` | The empty type: `data void == Nothing` |

### Abstract types

Some modules declare abstract types (using `abstype`) whose internal
representation is hidden.  For example, `set.hop` declares `abstype set pos`,
meaning `set alpha` is opaque — callers can only use the exported interface
(`{}`, `&`, `U`, `choose`, `card`), not the underlying `list alpha`
representation.  This opacity is enforced: the representation type does not
escape the module.

---

## Language features

### Reserved words

The following identifiers cannot be used as variable or function names:

```
abstype data dec display edit else exit if in
infix infixr lambda let letrec private save then
to type typevar uses where whererec write
```

Synonyms accepted by this interpreter: `\` for `lambda`, `use` for `uses`,
`infixrl` for `infixr`.

The `to` keyword is reserved for `write expr to "file"` file output.
`nonop` (prefix operator-as-value) is also reserved.

---

### Data types

```hope
data shape == circle(num) ++ rect(num # num);

dec area : shape -> num;
--- area (circle r)   <= 3.14159 * r * r;
--- area (rect(w, h)) <= w * h;

area (circle 5);
area (rect(4, 6));
```

### Functors (auto-generated `map`)

For every data or type declaration with one or more type parameters, the
interpreter automatically generates a functor — a mapping function with the
same name as the type.  For a 1-parameter type the functor takes one function
argument; for an n-parameter type it takes n function arguments (one per type
variable).

For example, `data tree alpha == Tip ++ Branch(alpha # tree alpha # tree alpha)`
generates:

```hope
--- tree f_0 Tip <= Tip;
--- tree f_0 (Branch(x, l, r)) <= Branch(f_0 x, tree f_0 l, tree f_0 r);
```

For a 2-parameter type like `data alpha OR beta == Left alpha ++ Right beta`
(from `sums.hop`):

```hope
--- OR f_0 f_1 (Left x)  <= Left  (f_0 x);
--- OR f_0 f_1 (Right y) <= Right (f_1 y);
```

The standard `map` function for `list alpha` is the list functor.  User-defined
types get the same treatment for free:

```hope
data maybe alpha == No ++ Yes alpha;

tree (+ 1) (Branch(5, Tip, Tip));
! >> Branch(6, Tip, Tip) : tree num

maybe (+ 1) (Yes 42);
! >> Yes 43 : maybe num

uses sums;
OR (+ 1) not (Left 3);
! >> Left 4 : num OR bool
```

### Infix operators

Operators are declared with `infix` or `infixr`:

```hope
infixr <> : 5;
dec <> : list alpha # list alpha -> list alpha;
--- [] <> ys <= ys;
--- (x::xs) <> ys <= x :: (xs <> ys);
```

Operator symbols can be used as values by wrapping them in parentheses:
`(<>)` is the append function as a value.

### Type annotations

An expression can be given an explicit type with `(expr : type)`.  The type
is checked against the inferred type; a mismatch is a type error.  The most
common use case is disambiguating the empty list, whose element type cannot
be inferred from context alone:

```hope
hd ([] : list num);   ! forces [] to have type list num
```

Annotations work with any expression, including infix expressions:

```hope
(1 + 1 : num);        ! >> 2 : num
(42 : bool);          ! type error: num does not match bool
```

### Lazy evaluation

The interpreter uses call-by-need evaluation.  Infinite data structures are
supported and are only computed as far as needed:

```hope
uses seq;
front_seq(10, primes);
! >> [2, 3, 5, 7, 11, 13, 17, 19, 23, 29] : list num

uses list;
front(5, iterate (* 2) 1);
! >> [1, 2, 4, 8, 16] : list num
```

### Local bindings and irrefutable patterns

Hope provides four forms of local binding:

| Form | Description |
|------|-------------|
| `let pat == e1 in e2` | Non-recursive binding: evaluate `e1`, bind `pat`, then evaluate `e2` |
| `letrec pat == e1 in e2` | Recursive binding: variables in `pat` may appear in `e1` |
| `e2 where pat == e1` | Same as `let`, written postfix |
| `e2 whererec pat == e1` | Same as `letrec`, written postfix |

The binding pattern `pat` in all four forms must be **irrefutable** — it may only
be a variable or a tuple of irrefutable patterns.  Constructor patterns (e.g.
`x::xs`), literal patterns, and list patterns are not permitted in binding
position; they belong in function equations.

This restriction mirrors Paterson's grammar rule for `let`/`where` (his `tuple`
non-terminal), and ensures that bindings always succeed.

**Irrefutable semantics — lazy binding**

Because Hope uses call-by-need evaluation, a tuple binding is fully lazy: the
right-hand side is not forced until one of the bound variables is first accessed.
This is Paterson's key extension over Imperial HOPE.

```hope
! Tuple binding: (a, b) pattern in where clause
dec span : (alpha -> bool) # list alpha -> list alpha # list alpha;
--- span cond (x::xs) <=
        if cond x
        then (x::f, a)
        where (f, a) == span cond xs
        else ([], x::xs);
--- span cond [] <= ([], []);
```

```hope
! Lazy binding: the second component is never forced
dec fst : alpha # beta -> alpha;
--- fst(x, y) <= x;

let (a, b) == (42, error "never used") in a;
! >> 42 : num
! 'b' is never accessed so error "never used" is never evaluated
```

```hope
! Nested tuples: (x, y, z) is right-nested (x, (y, z))
let (x, y, z) == (1, 2, 3) in x + y + z;
! >> 6 : num
```

**Parse error on non-irrefutable patterns**

Placing a constructor pattern in binding position is a parse error:

```hope
let (x::xs) == [1,2,3] in x;
! Parse error: expected variable or tuple pattern in let/where binding
```

See also: `ref_man.md` §4.3 for the formal grammar of `let`/`where` and the
definition of irrefutable patterns.

---

## Examples

### Fibonacci

```hope
dec fib : num -> num;
--- fib 0 <= 0;
--- fib 1 <= 1;
--- fib n <= fib (n - 1) + fib (n - 2);

fib 10;
```

### Infinite lists (lazy evaluation)

```hope
uses seq;

front_seq(10, primes);
```

Output: `[2, 3, 5, 7, 11, 13, 17, 19, 23, 29]`

### Pattern matching with algebraic types

```hope
data shape == circle(num) ++ rect(num # num);

dec area : shape -> num;
--- area (circle r)    <= 3.14159 * r * r;
--- area (rect(w, h))  <= w * h;

area (circle 5);
area (rect(4, 6));
```

### Sets (abstract type)

```hope
uses set;

card (1 & (2 & (3 & {})));      !  >> 3 : num
(1 & (2 & {})) U (2 & (3 & {}));  !  >> [1, 2, 3] : set num
```

### Using the `maybe` module

`maybe.hop` provides the `maybe alpha` option type and monadic combinators:

```hope
uses maybe;

unit_maybe 42;
! >> Yes 42 : maybe num

bind_maybe (Yes 3) (unit_maybe o (+ 1));
! >> Yes 4 : maybe num

bind_maybe No (unit_maybe o (+ 1));
! >> No : maybe num

Yes 5 OR 0;    ! OR extracts, defaulting to second arg if No
! >> 5 : num

No OR 0;
! >> 0 : num
```

### Sorting

```hope
uses sort;

sort [3, 1, 4, 1, 5, 9, 2, 6];
! >> [1, 1, 2, 3, 4, 5, 6, 9] : list num

uniq (sort [3, 1, 4, 1, 5, 9, 2, 6]);
! >> [1, 2, 3, 4, 5, 6, 9] : list num
```

---

## Error recovery

A parse error or type error at the REPL does not terminate the session.  The
interpreter reports the error and returns to the prompt:

```
hope> 1 + "hello";
line 1: type error - argument has wrong type
  1 + "hello"
  (+) : num # num -> num
  "hello" : list char
hope>
```

---

## Command-line flags

| Flag | Meaning |
|------|---------|
| `-f <file>` | Run `<file>` in batch mode instead of starting the REPL |

Additional positional arguments after all flags are collected as the Hope
`argv` variable (a `list (list char)` of command-line strings).

---

## Known limitations

No known limitations at this time.  All features described in Paterson's
reference manual are implemented.
