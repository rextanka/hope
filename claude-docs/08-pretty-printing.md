# Pretty Printers

## Files
- `src/pr_value.c` — print runtime values (fully evaluated `Cell*`)
- `src/pr_type.c` — print `Type*` objects (from declarations)
- `src/pr_ty_value.c` — print type inference `Cell*` objects
- `src/pr_expr.c` — print `Expr*` AST nodes (for error messages)
- `src/print.h` — precedence constants

---

## Precedence-based Printing

All printers use a **precedence context** to decide whether to add parentheses. The pattern is:

```c
void pr_something(FILE *f, Something *node, int context) {
    int prec = precedence_of(node);
    if (prec < context)
        fprintf(f, "(");
    // ... print content recursively, passing prec as context ...
    if (prec < context)
        fprintf(f, ")");
}
```

Precedence constants (from `print.h`):
- `PREC_BODY` — weakest (body of lambda, top level)
- `PREC_INFIX(n)` — precedence n operator context
- `PREC_APPLY` — function application (tight)
- `PREC_ARG` — argument of application (tightest)

The printer knows about infix operators and uses them when a constructor/function has been declared as an operator.

---

## Value Printer (`pr_value.c`)

Entry point: `pr_value(FILE *f, Cell *value)`

Handles fully-evaluated heap cells. The value is pushed onto the GC stack before printing to prevent it being collected.

| Cell class | Printed as |
|-----------|-----------|
| `C_NUM` | Number in `NUMfmt` (e.g. `42` or `3.14`) |
| `C_CHAR` | `'a'` with C-style escaping for non-printable characters |
| `C_CONST` | Constructor name (e.g. `true`, `nil`) |
| `C_CONS` | Constructor applied to argument; uses infix form if operator |
| `C_PAIR` | `(left, right)` — or as a list if it's a cons-cell chain |
| `C_PAPP` | Partial application — printed with as many args as available |

**List detection**: before printing a `C_CONS` cell, the printer checks `is_vlist()` — whether the value is a chain of `::` constructors ending in `nil`. If so, it prints as `[a, b, c]`. If additionally all elements are characters (`is_vstring()`), it prints as `"hello"`.

**Infix values**: if a constructor or function name is a declared infix operator, it is printed infix: `1 :: 2 :: nil` rather than `cons(1, cons(2, nil))`.

**Pattern match failure output**: `pr_f_match(defun, env)` and `pr_l_match(func, env)` are called when a pattern match fails at runtime. They print the function/lambda name and the actual argument values, using `evaluate()` to force the relevant parts of the environment.

---

## Type Printer (`pr_type.c`)

Entry point: `pr_type(FILE *f, Type *type)` and `pr_qtype(FILE *f, QType *qtype)`

Prints a `Type*` (the static type representation from declarations):

| Type class | Printed as |
|-----------|-----------|
| `TY_VAR` | Type variable name (e.g. `alpha`) |
| `TY_MU` | `mu s => body` |
| `TY_CONS` | Type constructor applied to args; infix if declared as operator |

For `TY_CONS`, the printer checks if the `DefType` is `function` (prints as `arg -> result`), `product` (prints as `a # b`), or `list` (prints as `list alpha`). For user-defined types declared with infix, the infix form is used.

`pr_alt(f, cons)` prints a constructor in `data` definition format: `Name type1 ++ Name2 type2`.

---

## Type Cell Printer (`pr_ty_value.c`)

Entry point: `pr_ty_value(FILE *f, Cell *type_cell)`

Prints a type inference `Cell*` (the dynamic type representation):

| Cell class | Printed as |
|-----------|-----------|
| `C_TVAR` | Fresh variable like `_a`, `_b`, ... |
| `C_FROZEN` | Frozen variable (during instance checking) |
| `C_TSUB` | Type constructor applied to arg |
| `C_TREF` | Dereferenced (follow the chain) |
| `C_TCONS` | Type constructor with two args |
| `C_TLIST` | List of type args (part of a multi-arg constructor) |

This printer is used in error messages: when a type error is found, `pr_ty_value` prints the inferred type and `pr_qtype` prints the declared type.

---

## Expression Printer (`pr_expr.c`)

Used for error messages (not for output of evaluated values). Prints `Expr*` nodes in a form close to the original source.

Two entry points:
- `pr_expr(f, expr)` — prints a source-level expression (for displaying declarations)
- `pr_c_expr(f, expr, level, context)` — prints a compiled expression at a given environment level

The expression printer reconstructs `let`/`where`/`letrec`/`whererec` from the underlying `E_LET`/`E_WHERE`/`E_RLET`/`E_RWHERE` nodes, and reconstructs lambda forms from `E_LAMBDA`/`E_EQN`/`E_PRESECT`/`E_POSTSECT`.

---

## How the REPL Displays Results

After evaluating an interactive expression:

1. `evaluate(cell)` forces it to head normal form
2. `pr_value(stdout, cell)` prints the value
3. `pr_ty_value(stdout, expr_type)` prints the inferred type

Example output:
```
[1, 2, 3, 4, 5] : list num
```

---

## Implications for the C++20 Rewrite

The C++20 rewrite will consolidate these printers into a single `Printer` class with:

```cpp
class Printer {
public:
    std::string formatValue(const Cell& cell, int precedence = PREC_BODY);
    std::string formatType(const Type& type);
    std::string formatExpr(const Expr& expr);
};
```

Using `std::format` (C++20) for numeric formatting and `std::string` accumulation instead of `fprintf` directly to `FILE*`. This makes the output testable (return strings rather than print to file) and allows the REPL to format results without coupling to `stdout`.

The precedence system and infix detection remain the same. The list/string detection heuristic (`is_vlist`, `is_vstring`) is preserved.

Error messages will be improved by:
1. Always including the source location of the expression that failed
2. Printing the "expected type" and "found type" side-by-side in aligned format
3. Highlighting the specific sub-expression that caused the mismatch

See [11-rewrite-roadmap.md](11-rewrite-roadmap.md).
