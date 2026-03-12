# Hope Interpreter — User Guide

This guide covers building the C++20 Hope interpreter, setting it up, and
using it interactively and in batch mode.  It is updated alongside each
implementation phase.

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

## REPL commands

The REPL accepts Hope expressions and declarations, plus the special commands
below.  All commands begin with `:`.

| Command | What it does |
|---------|-------------|
| `:load <file.hop>` | Load and execute a `.hop` file into the current session.  All declarations in the file become available immediately. |
| `:reload` | Re-run the most recently loaded file (useful after editing). |
| `:type <expr>` | Print the inferred type of an expression without evaluating it. |
| `:display` | List all declarations in the current session. |
| `:clear` | Reset the session (remove all user declarations; standard library remains loaded). |
| `:quit` / `:exit` | Exit the interpreter. |

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

```
hope> :type map;
map : (alpha -> beta) -> list alpha -> list beta
```

### Lazy output with `write`

The `write` command streams characters to stdout without buffering the entire
result first.  This is how infinite lists are printed one element at a time:

```
hope> write (hd [1,2,3]);
1
hope> uses seq;
hope> write (front_seq(5, gen_seq (+ 1) 1));
[1, 2, 3, 4, 5]
```

---

## Standard library modules

Modules are loaded with `uses <ModuleName>;` at the top of a file or at the
REPL prompt.  Standard modules live in `<HOPEPATH>/`.

| Module | Key bindings |
|--------|-------------|
| `Standard` | Loaded automatically. `bool`, `list`, `num`, `char`, arithmetic, comparison, `map`, `filter`, `foldl`, `foldr`, `append`, `length`, `reverse`, `nth`, `hd`, `tl`, `null`, `not`, … |
| `list` | Additional list utilities |
| `arith` | Extended arithmetic: `div`, `mod`, `gcd`, `lcm`, `even`, `odd`, integer powers |
| `seq` | Lazy sequences: `seq alpha`, `gen_seq`, `front_seq`, `filter_seq`, `map_seq`, `primes` |
| `maybe` | Option type: `maybe alpha` (`No` / `Yes x`), `safe_head`, `safe_div` |
| `tree` | Binary search trees: `tree alpha`, `insert`, `member`, `flatten` |
| `sort` | Sorting: `sort`, `msort`, `qsort` |
| `set` | Sets as sorted lists: `union`, `inter`, `diff` |
| `lines` | Line-oriented I/O: `lines`, `unlines` |
| `fold` | Higher-order folds |
| `functions` | Function combinators: `o` (compose), `id`, `const`, `curry`, `uncurry` |
| `products` | Pair operations |
| `sums` | Sum type utilities |
| `range` | Numeric ranges |

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

### Using the `maybe` module

```hope
uses maybe;

safe_head [1, 2, 3];   !  >> Yes 1 : maybe num
safe_head ([] : list num);  !  >> No : maybe num
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
