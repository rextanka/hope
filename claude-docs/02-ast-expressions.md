# AST and Expression Representation

## Files
- `src/expr.h` / `src/expr.c` — expression and pattern node types, constructors, branch/equation handling
- `src/cons.h` — data constructor representation
- `src/structs.h` — forward declarations of all key structs
- `src/path.h` / `src/path.c` — path navigation within heap graph nodes

---

## Overview

There is a single `Expr` struct used for **both** AST nodes (before compilation) and compiled expression forms (after name resolution). The same struct represents expressions in patterns, function bodies, lambda forms, and let/where bindings. Different `e_class` values distinguish these uses.

The union `e_union` is used for all node payloads; macros like `e_num`, `e_left`, `e_func` provide named access to specific fields.

---

## Expression Classes (`expr.h`)

```
E_NUM        — integer (or float) literal
E_CHAR       — character literal
E_CONS       — data constructor constant (e.g. nil, true, succ)
E_PAIR       — pair constructor (,) — both patterns and expressions
E_APPLY      — function application f(x) — both patterns and expressions

E_VAR        — variable in a pattern (identifier not yet resolved)
E_PLUS       — p+k pattern (matches succ^k of a pattern)

E_DEFUN      — resolved reference to a declared function or constant
E_LAMBDA     — anonymous function (lambda expression)
E_PARAM      — resolved variable reference in an expression body
E_MU         — mu-expression (recursive infinite-type expression)

E_IF         — if-then-else (structurally like E_APPLY)
E_WHERE      — where clause (structurally like E_APPLY)
E_LET        — let...in (structurally like E_APPLY)
E_RWHERE     — recursive where (whererec)
E_RLET       — recursive let (letrec)
E_EQN        — the equation part of a let/where (structurally like E_LAMBDA)
E_PRESECT    — (e op) operator section (structurally like E_LAMBDA)
E_POSTSECT   — (op e) operator section (structurally like E_LAMBDA)

E_BUILTIN    — pointer to a C function implementing a built-in
E_BU_1MATH   — pointer to a unary numeric C function
E_BU_2MATH   — pointer to a binary numeric C function
E_RETURN     — special: causes immediate exit from evaluation
```

### Key design points

**Shared patterns/expression space.** Patterns are parsed as expressions. The type-checker and name-resolver later re-interpret `E_VAR` (as a pattern variable if it is not a declared constructor) versus `E_DEFUN`/`E_CONS` (if it is). After name resolution, `E_VAR` only appears as a pattern variable and `E_PARAM` as the corresponding expression-side binding.

**let/where desugaring.** `let pat == body in subexpr` is internally represented as `(lambda pat => subexpr)(body)`, i.e.:
```
E_LET
  e_func → E_EQN (like E_LAMBDA, contains the branches)
  e_arg  → body expression
```
The `E_LET`/`E_WHERE`/`E_RLET`/`E_RWHERE` classes are just markers overlaid on an `E_APPLY`-shaped node to help the pretty-printer reconstruct the original syntax.

**if-then-else desugaring.** `if e1 then e2 else e3` desugars to `if_then_else(e1)(e2)(e3)`, where `if_then_else` is a Standard module function. Structurally an `E_IF` node is an `E_APPLY` node with a special tag for the pretty-printer.

**Operator sections.** `(e op)` → `lambda x' => e op x'`, `(op e)` → `lambda x' => x' op e`. The bound variable uses the special non-user-accessible string `"x'"`.

**String literals.** Desugared immediately by `text_expr()` to a list of character nodes: `"ab"` becomes `apply_expr(e_cons, pair_expr(char_expr('a'), apply_expr(e_cons, pair_expr(char_expr('b'), e_nil))))`.

---

## Expr Struct Layout

```c
struct _Expr {
    ExprClass e_class;     // one of E_NUM, E_APPLY, etc.
    char      e_misc_num;  // overloaded field, meaning depends on e_class:
                           //   E_VAR   → variable index (after name res.)
                           //   E_PARAM → de Bruijn level (nesting depth)
                           //   E_APPLY → number of variables (in branch)
                           //   E_LAMBDA → arity
    union {
        Num      eu_num;          // E_NUM
        Char     eu_char;         // E_CHAR
        Cons     *eu_const;       // E_CONS
        struct { String eu_vname; Path eu_dirs; } e_v;        // E_VAR
        struct { Expr *eu_patt; Path eu_where; } e_p;         // E_PARAM
        struct { Expr *eu_left; Expr *eu_right; } e_pair;     // E_PAIR, E_MU
        struct { Expr *eu_func; Expr *eu_arg; } e_apply;      // E_APPLY
        struct { int eu_incr; Expr *eu_rest; } e_plus;        // E_PLUS
        Func     *eu_defun;                                    // E_DEFUN
        struct { Branch *eu_branch; UCase *eu_code; } e_lambda; // E_LAMBDA
        Function *eu_fn;          // E_BUILTIN
        Unary    *eu_1math;       // E_BU_1MATH
        Binary   *eu_2math;       // E_BU_2MATH
    } e_union;
};
```

The macro layer (`e_num`, `e_left`, `e_func`, etc.) hides the union accesses.

---

## Branch Struct

A `Branch` is one equation in a multi-equation function definition, or one case in a lambda:

```c
struct _Branch {
    Expr   *br_formals;  // formal parameters as a left-leaning APPLY tree
    Expr   *br_expr;     // right-hand side (body)
    Branch *br_next;     // next branch (NULL if last)
};
```

The formals are stored as a left-leaning chain of `E_APPLY` nodes:
```
f a b c   →   ((NULL · a) · b) · c
```
where `·` denotes E_APPLY. The function position (`e_func`) of the outermost node is `NULL` for a lambda, or the function name node for a defined function.

---

## Func Struct

A `Func` represents a declared or implicitly declared value identifier:

```c
struct _Func {
    TabElt  f_linkage;          // name in hash table
    short   f_arity;
    SBool   f_explicit_dec;     // true if declared with `dec`
    SBool   f_explicit_def;     // true if at least one equation given
    union {
        QType   *fu_qtype;      // declared type (if f_explicit_dec)
        DefType *fu_tycons;     // associated type constructor (if implicit)
    };
    Branch  *f_branch;          // linked list of branches (equations)
    UCase   *f_code;            // compiled decision tree (see compile.c)
};
```

Every declared identifier maps to a `Func` in the module's hash table. When a new equation is added via `def_value()`, `comp_branch()` incrementally extends `f_code` (the compiled decision tree).

---

## Cons Struct (Data Constructors)

```c
struct _Cons {
    String c_name;       // interned name
    Type   *c_type;      // the type this constructor produces
    unsigned char c_nargs;   // number of arguments (0 for constants)
    unsigned char c_index;   // index among siblings in the data type
    unsigned char c_ntvars;  // number of type variables in the type
    SBool  c_tupled;     // TRUE if argument is a tuple (pair type)
    Cons   *c_next;      // next constructor in same data type
};
```

The special constructors `nil`, `cons`, `succ`, `true`, `false` are globally accessible. `succ` receives special treatment: numeric literals are syntactic sugar for applications of `succ` to `0`.

---

## Name Resolution

After parsing, `E_VAR` nodes are resolved:
- If the name is a declared data constructor (`cons_lookup`), the node becomes `E_CONS`.
- If the name is a declared function (`fn_lookup`), the node becomes `E_DEFUN`.
- Otherwise (in a pattern) it remains `E_VAR` as a pattern variable.
- In an expression body, pattern variables are replaced with `E_PARAM` nodes that carry a **de Bruijn level** (depth of lambda nesting where the variable was bound) and a **path** into the argument structure.

The path describes how to extract the bound value from the argument. For example, in `f (x, y)`, `x` has path `LEFT` (first element of the pair) and `y` has path `RIGHT`. For nested constructors like `f (x :: xs)`, the path would be `HEAD` (for `x`) and `TAIL` (for `xs`).

---

## Key Global Expressions

```c
extern Expr *e_true;   // E_CONS for 'true'
extern Expr *e_false;  // E_CONS for 'false'
extern Expr *e_cons;   // E_CONS for '::' (list cons)
extern Expr *e_nil;    // E_CONS for 'nil'
extern Func *f_id;     // the 'id' function
```

---

## Construction Functions (expr.c)

| Function | Returns | Description |
|----------|---------|-------------|
| `char_expr(c)` | `Expr*` | Character literal node |
| `text_expr(text, n)` | `Expr*` | String → list of chars (desugared) |
| `num_expr(n)` | `Expr*` | Numeric literal node |
| `cons_expr(constr)` | `Expr*` | Data constructor reference |
| `id_expr(name)` | `Expr*` | Unresolved identifier (E_VAR initially) |
| `dir_expr(path)` | `Expr*` | E_PARAM (variable reference with path) |
| `pair_expr(left, right)` | `Expr*` | Pair constructor |
| `apply_expr(func, arg)` | `Expr*` | Application node |
| `func_expr(branches)` | `Expr*` | Lambda expression |
| `ite_expr(if, then, else)` | `Expr*` | if-then-else (desugared to apply) |
| `let_expr(pat, body, sub, rec)` | `Expr*` | let/letrec expression |
| `where_expr(sub, pat, body, rec)` | `Expr*` | where/whererec expression |
| `mu_expr(muvar, body)` | `Expr*` | mu-expression |
| `presection(op, e)` | `Expr*` | `(e op)` operator section |
| `postsection(op, e)` | `Expr*` | `(op e)` operator section |
| `builtin_expr(fn)` | `Expr*` | Built-in C function wrapper |
| `bu_1math_expr(fn)` | `Expr*` | Unary numeric built-in |
| `bu_2math_expr(fn)` | `Expr*` | Binary numeric built-in |

---

## Key Semantic Functions

### `decl_value(name, qtype)` — declares a value identifier

- Checks for duplicate declarations
- Creates a new `Func` entry in the current module's table
- Called from the parser when it sees a `dec` production

### `def_value(formals, body)` — adds an equation to a function definition

1. Extracts the function name from the left-leaning `APPLY` chain (the head must be `E_VAR`)
2. Looks up the `Func` in the current module
3. Calls `nr_branch()` (name resolution + type checking for the new branch)
4. Calls `chk_func()` (type checks the branch against the declared type)
5. Appends the branch to `fn->f_branch`
6. Calls `comp_branch(fn->f_code, branch)` to incrementally extend the compiled decision tree

---

## Implications for the C++20 Rewrite

In the rewrite, `Expr` becomes a `std::variant` over a set of node types:

```cpp
using Expr = std::variant<
    NumLit,        // was E_NUM
    CharLit,       // was E_CHAR
    ConsRef,       // was E_CONS
    Pair,          // was E_PAIR
    Apply,         // was E_APPLY
    Var,           // was E_VAR
    PlusPattern,   // was E_PLUS
    FunRef,        // was E_DEFUN
    Lambda,        // was E_LAMBDA
    Param,         // was E_PARAM
    MuExpr,        // was E_MU
    IfExpr,        // was E_IF
    LetExpr,       // was E_LET / E_RLET
    WhereExpr,     // was E_WHERE / E_RWHERE
    BuiltinExpr    // was E_BUILTIN
>;
```

This eliminates the unsafe union and the overloaded `e_misc_num` field. Each variant carries only the fields it needs, with clear named members. Source location (file/line/column) will be stored in each node.

See [11-rewrite-roadmap.md](11-rewrite-roadmap.md) for the full plan.
