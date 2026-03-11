# Built-in Functions and I/O

## Files
- `src/builtin.c` — registration and implementation of built-in functions
- `src/number.c` — numeric type (integer, optional real)
- `src/char.c` — character type
- `src/char_array.c` — sparse character array (used for char pattern dispatch)
- `src/compare.c` — structural comparison (`compare`, `=`, `<`, etc.)
- `src/stream.c` — lazy I/O streams
- `src/output.c` — output buffering and `write` command
- `src/interrupt.c` — Ctrl-C signal handling

---

## How Built-ins Are Registered (`builtin.c`)

Built-in functions are **not** ordinary Hope functions — they are C functions attached directly to `Func` objects that were declared in `lib/Standard.hop`. The process:

1. `Standard.hop` is loaded first (via `mod_use("Standard")` in `mod_init()`), creating `Func` entries for `ord`, `chr`, `+`, `-`, etc. in the symbol table.
2. `init_builtins()` is called after Standard is loaded. It looks up each `Func` by name and installs a `UC_STRICT` code node pointing to the C implementation.

Three registration patterns:

```c
def_builtin(name, fn)   // fn: Cell* fn(Cell *arg)
                        // installs strict(builtin_expr(fn))
                        // arity = 1 (argument already in env->c_left)

def_1math(name, fn)     // fn: Num fn(Num x)
                        // installs strict(bu_1math_expr(fn))
                        // validates type is num -> num

def_2math(name, fn)     // fn: Num fn(Num x, Num y)
                        // installs strict(bu_2math_expr(fn))
                        // validates type is (num # num) -> num
```

All built-ins are **strict** — their argument is forced to WHNF before the C function is called. The `UC_STRICT` code node pushes a `FORCE_MARK` and forces `env->c_left`, then continues with the body (the `E_BUILTIN`/`E_BU_1MATH`/`E_BU_2MATH` expression).

---

## Built-in Function Inventory

### Conversions
| Hope name | C function | Description |
|-----------|-----------|-------------|
| `ord` | `ord()` | `char -> num`: ASCII ordinal |
| `chr` | `chr()` | `num -> char`: number to char (range-checked) |
| `num2str` | `num2str()` | `num -> list char`: number to string |
| `str2num` | `str2num()` | `list char -> num`: string to number |

### I/O
| Hope name | C function | Description |
|-----------|-----------|-------------|
| `read` | `open_stream()` | `list char -> list char`: open file lazily |
| `print` | `print_value()` | `alpha -> beta`: strict, prints value as side effect |
| `write_element` | `write_value()` | Internal: used by `write_list` in Standard |

### Error
| Hope name | C function | Description |
|-----------|-----------|-------------|
| `error` | `user_error()` | `list char -> alpha`: abort with message |

### Arithmetic (all `def_2math`, type `num # num -> num`)
`+`, `-`, `*`, `/`, `div`, `mod`

### Math library (all `def_1math` or `def_2math`, require `REALS` + `HAVE_LIBM`)
`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `cosh`, `sinh`, `tanh`, `acosh`, `asinh`, `atanh`, `ceil`, `floor`, `abs`, `exp`, `log`, `log10`, `sqrt`, `pow`, `erf`, `erfc`, `hypot`

---

## Comparison (`compare.c`)

The `compare`, `=`, `/=`, `<`, `=<`, `>`, `>=` functions are also built-ins but more complex: they must handle arbitrary algebraic data types structurally.

`compare(x, y)` returns a `relation` value (`LESS`/`EQUAL`/`GREATER`). The comparison rules:
- `num`: numeric order
- `char`: ASCII order
- Pairs: lexicographic
- Constructors: ordered by their `c_index` within their data type (the order they were defined with `++`)
- Lists: lexicographic character order for `list char`, otherwise constructor order
- Functions: runtime error (can't compare)

The comparison functions are registered via `init_cmps()` in `compare.c`.

---

## String Conversion Utilities

Two utility functions used pervasively:

```c
Cell *c2hope(const Byte *str);
```
Converts a NUL-terminated C string to a Hope `list char` value (a `C_CONS` chain).
Calls `chk_heap(NOCELL, 3*len+1)` first to ensure enough heap space.

```c
void hope2c(Byte *s, int n, Cell *arg);
```
Converts a Hope `list char` value to a NUL-terminated C string.
Errors if the string exceeds n-1 characters.

---

## Numeric Type (`number.c`)

The `Num` type is defined in `num.h`. By default (without `REALS`), it is `long` (C integer). With `REALS` defined, it becomes `double`.

The `succ` constructor has special handling: number literals in patterns are sugar for chains of `succ`, and the numeric comparison in `C_LCASE (LC_NUMERIC)` directly compares `c_num` to `Zero` (0 as a `Num`).

Key functions:
- `new_num(n)` — allocate a `C_NUM` cell
- `atoNUM(str)` — parse a number from a string

---

## Lazy I/O Streams (`stream.c`)

The `read` built-in opens a file and returns a lazy `list char`. Rather than reading the whole file into memory, it creates a `C_STREAM` cell:

```c
Cell *new_stream(FILE *f);
```

A `C_STREAM` cell holds a `FILE*`. When the interpreter encounters a `C_STREAM` as the current cell, it calls `read_stream(current)`:

```c
Cell *read_stream(Cell *current);
```

This reads the next character from the file:
- If a character is available: returns `c :: rest` where `rest` is a new `C_STREAM`
- If EOF: returns `nil`

This implements **lazy file reading** — the file is consumed on demand as the program processes the list.

The `input` special variable is initialised at the start of each interactive expression evaluation:
```c
Push(new_susp(expr, new_pair(new_stream(stdin), NULL_ENV)));
```
So `input` is always in position 0 of the environment, bound to a lazy stdin stream.

---

## Output (`output.c`)

Two output modes:

**Normal expression output** (`expr;`):
- The result is evaluated to full normal form
- `pr_value()` pretty-prints it
- The type is printed alongside

**`write` command** (`write expr;` or `write expr to "file";`):
- Calls `wr_expr(expr, filename)` in `module.c`
- The expression must have type `list T` (checked by `chk_list()`)
- Output is produced lazily: each element is evaluated and printed as it becomes available
- `write_value()` / `print_value()` are the strict built-ins used inside `write_list`
- If the list is a `list char`, characters are printed directly
- Otherwise each element is printed on a separate line

---

## Signal Handling (`interrupt.c`)

```c
void enable_interrupt(void);
void disable_interrupt(void);
```

Sets/clears a `SIGINT` handler. When Ctrl-C is pressed during evaluation, the signal handler calls `error(EXECERR, "interrupted")`, which longjmps back to the YACC loop and allows the REPL to continue.

---

## Implications for the C++20 Rewrite

### Built-ins
In C++20, built-in functions will be registered as:
```cpp
using BuiltinFn = std::function<Cell(Cell)>;
```
All arithmetic operations will support both `long long` and `double` (via `std::variant<long long, double>` in the `Num` type), eliminating the compile-time `REALS` flag.

### Strings
`c2hope` / `hope2c` become:
```cpp
Cell fromCppString(std::string_view s);
std::string toCppString(const Cell& hopeList);
```

### Streams
Lazy file I/O becomes a generator-style thunk: a `C_STREAM` equivalent holds a `std::shared_ptr<std::istream>` and a position. No `FILE*`.

### Comparisons
The structural comparison built-in (`compare`) will need careful handling of the constructor ordering — the order of `++` alternatives in `data` definitions must be preserved and associated with each constructor.

### Output
The `write` command's lazy streaming can use C++20 ranges or coroutines: the output loop evaluates the list one element at a time, printing immediately, which is already how it works in Paterson's implementation.

See [11-rewrite-roadmap.md](11-rewrite-roadmap.md).
