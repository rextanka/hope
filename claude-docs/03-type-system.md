# Type System

## Files
- `src/type_check.c` — Hindley-Milner type inference and checking
- `src/type_value.c` — runtime type cell representation and operations (unification, instantiation)
- `src/deftype.h` / `src/deftype.c` — type definitions: data types, type synonyms, abstract types
- `src/functor_type.c` — computes the auto-generated functor (map) type for a data type
- `src/polarity.c` — analyses positive/negative occurrences of type variables
- `src/bad_rectype.c` — validates that recursive type definitions are well-formed
- `src/remember_type.c` — memoises type lookups during recursive type checking

---

## Two-Level Type Representation

The type system uses **two different representations** for types, at different stages:

### 1. `Type` — Static type terms (pre-inference)

Used in source declarations (`dec`, `data`, `type`). This is a tree of type terms built by the parser.

```c
struct _Type {
    TypeClass ty_class;  // TY_VAR, TY_CONS, or TY_MU
    union {
        struct { TVar ty_var; bool ty_mu_bound; int ty_index;
                 bool ty_pos; bool ty_neg; }  // TY_VAR
        struct { DefType *ty_deftype; TypeList *ty_args; }  // TY_CONS
        struct { TVar ty_muvar; Type *ty_body; }            // TY_MU
    };
};
```

- `TY_VAR`: a type variable. `ty_index` is a de Bruijn index (for mu-bound vars) or a parameter position (for data type parameters) or a variable number (for value declarations).
- `TY_CONS`: an applied type constructor. `ty_deftype` points to the `DefType`, `ty_args` is the argument list.
- `TY_MU`: a recursive type `mu v => T`. This is a Paterson extension — regular types.

A **qualified type** (`QType`) wraps a `Type` with a count of free type variables:
```c
struct _QType {
    unsigned char qt_ntvars;  // number of free type variables
    Type *qt_type;            // the type
};
```

This is what `dec` declarations store: `f : alpha # alpha -> bool` has `qt_ntvars = 1` (one polymorphic variable) and `qt_type = TY_CONS(#, [TY_VAR(0), TY_VAR(0)]) -> TY_CONS(bool, [])`.

### 2. `Cell` — Dynamic type cells (inference)

Used during type inference. Cells live on the shared heap (same heap as runtime values). Type cells are distinguished by their `CellClass`:

| Class | Meaning |
|-------|---------|
| `C_TVAR` | Unification variable (free type variable during inference) |
| `C_VOID` | Uninitialised type variable |
| `C_FROZEN` | Type variable that must not be unified (prevents over-generalisation) |
| `C_TSUB` | Type constructor applied to argument(s): `TyCons(arg)` |
| `C_TREF` | Indirection (after unification): points to unified type |
| `C_TLIST` | Cons cell for a list of type arguments |
| `C_TCONS` | Curried type constructor with two arguments |

This is essentially a **union-find** (disjoint-set) structure for type variables. `C_TREF` cells represent the "forwarded" result after a unification step.

---

## DefType — Type Constructor Definitions

```c
struct _DefType {
    String  dt_name;
    char    dt_arity;           // number of type parameters
    char    dt_syn_depth;       // synonym expansion depth (0 = data type)
    bool    dt_private;
    bool    dt_tupled;
    TypeList *dt_varlist;       // type parameter variables
    union {
        Cons    *dt_cons;       // for data types: list of constructors
        Type    *dt_type;       // for type synonyms: the RHS type
    };
};
```

Macros distinguish the three kinds:
- `IsDataType(dt)` — `dt_syn_depth == 0 && dt_cons != NULL`
- `IsAbsType(dt)` — `dt_syn_depth == 0 && dt_cons == NULL`
- `IsSynType(dt)` — `dt_syn_depth > 0`

The globally pre-declared `DefType`s are:
- `product` — the `#` type constructor (pairs)
- `function` — the `->` type constructor
- `list` — `list alpha`
- `num` — numeric type
- `truval` / `bool` — Boolean type
- `character` — `char` type

---

## Type Inference Algorithm (`type_check.c`)

The algorithm is **Algorithm W** (Hindley-Milner), implemented as a recursive descent over `Expr` nodes. The main function is `ty_expr(expr)` which returns a `Cell*` representing the inferred type.

### Typing rules (from comments in source)

**Variables** (`E_VAR`): Look up in `local_var[]` array:
```
Γ, x:t ⊢ x : t
```

**Application** (`E_APPLY`):
```
Γ ⊢ e1 : t2 → t      Γ ⊢ e2 : t2
------------------------------------
Γ ⊢ e1 e2 : t
```
Implemented as: infer type of function, infer type of argument, unify function type with `arg_type → fresh_var`, return the result type.

**Let** (`E_LET`):
```
Γ' ⊢ pat : t1        Γ, Γ' ⊢ val : t2        Γ ⊢ exp : t1
--------------------------------------------------------------
Γ ⊢ let pat == exp in val : t2
```

**Letrec** (`E_RLET`):
```
Γ' ⊢ pat : t1        Γ, Γ' ⊢ val : t2        Γ, Γ' ⊢ exp : t1
-----------------------------------------------------------------
Γ ⊢ letrec pat == exp in val : t2
```
The difference: in `letrec`, `exp` (the right-hand side) is typed in the same context as `val`, allowing recursive references.

**If-then-else** (`E_IF`):
- Condition must unify with `bool`/`truval`
- Both branches must unify with each other
- Result is the unified branch type

**Pairs** (`E_PAIR`):
```
Γ ⊢ e1 : t1     Γ ⊢ e2 : t2
-----------------------------
Γ ⊢ (e1, e2) : t1 # t2
```

**Lambda / multi-clause function**: delegates to `ty_list()` which types all branches and unifies them.

**Mu-expression** (`E_MU`):
```
Γ' ⊢ pat : t      Γ, Γ' ⊢ exp : t
------------------------------------
Γ ⊢ mu pat => exp : t
```
The pattern and body must have the same type; this creates a recursive value.

### Variable scoping

Type variables for pattern-bound identifiers are managed through two arrays:
- `local_var[]`: types of the local variables of the current scope
- `variables[level]`: variables at each lambda nesting level

`new_vars(n)` pushes a new scope with n fresh type variables. `del_vars()` pops it. This implements the **stack of binding environments** needed for multi-argument functions.

### Error reporting

When a type error is found, the current source uses `start_err_line()` + `fprintf(errout, ...)` to print:
- The declared type (if any)
- The inferred type
- A short error description

This is one of the weakest parts of the implementation — no source locations, terse messages. The rewrite will improve this significantly.

---

## Unification (`type_value.c`)

```c
Bool unify(Cell *type1, Cell *type2);
```

Standard Robinson unification on `Cell*` nodes:
1. `deref()` both cells (follow `C_TREF` chains).
2. If both are `C_TVAR`: make one point to the other.
3. If one is `C_TVAR`: bind it to the other (occurs check implied by `C_FROZEN` for let-bound variables).
4. If both are `C_TSUB` or `C_TCONS`: unify the constructors and recursively unify arguments.
5. Return `FALSE` on failure.

```c
Cell *deref(Cell *type);
```
Follows `C_TREF` chains (path compression). Essential for efficient unification.

```c
Cell *copy_type(Type *type, Natural ntvars, Bool frozen);
```
Creates a fresh `Cell*` copy of a `Type*`, with `ntvars` fresh unification variables substituted for the type's free variables. If `frozen = TRUE`, uses `C_FROZEN` cells that resist unification — used when checking that an inferred type is an instance of a declared type.

```c
Bool instance(Type *type, Natural ntvars, Cell *inf_type);
```
Checks that `inf_type` is an instance of the polymorphic type `(type, ntvars)`. Creates a frozen copy of the declared type and tries to unify.

---

## Type Checking Entry Points

| Function | Called from | Purpose |
|----------|------------|---------|
| `chk_func(branch, fn)` | `def_value()` | Type-check a new equation against declared type |
| `chk_expr(expr)` | `eval_expr()` | Type-check an interactive expression; sets `expr_type` |
| `chk_list(expr)` | `wr_expr()` | Type-check a `write` expression (must be `list(T)`) |
| `ty_instance(t1, n1, t2, n2)` | type comparison utilities | Check if one type scheme is an instance of another |
| `nr_branch(branch)` | `def_value()` | Name-resolve a branch (converts E_VAR to E_PARAM etc.) |

`chk_expr()` wraps `ty_expr()` with a special setup: the implicit `input` variable is given type `list char`, representing the lazy stream of stdin characters.

---

## Type Definitions Processing

### `decl_type(deftype, conslist)` (for `data`)
1. Registers the `DefType` in the module's type table
2. For each constructor in `conslist`: registers the `Cons` in the value table with its type
3. Creates an auto-generated functor function (see below)

### `type_syn(deftype, type)` (for `type`)
1. Registers the synonym definition
2. Validates depth constraints (max 50 synonym expansion steps)
3. Handles the regular type case (`mu s => ...`)

### `abstype(deftype)` (for `abstype`)
Registers the type name with no constructors visible. The implementation may be supplied later by `data` or `type`.

---

## Regular Types (`bad_rectype.c`, `remember_type.c`)

Paterson's system supports **regular (possibly infinite) types** — a significant extension over Imperial HOPE and standard ML. These arise from recursive `type` synonyms:

```hope
type seq alpha == alpha # seq alpha;
```

This would be an infinite type in standard HM, but Paterson's system handles it by:
1. Detecting when type expansion would cycle (`bad_rectype.c`)
2. Caching previously-expanded types to detect cycles (`remember_type.c`)
3. Using `mu` notation to represent the canonical form: `mu s => alpha # s`

`bad_rectype.c` validates that recursive type definitions have a "data constructor barrier" — a non-synonym type must appear between recursive occurrences. Without this, the type system would be non-terminating.

---

## Functor Types (`functor_type.c`, `polarity.c`)

For every `data` or `type` definition, an auto-generated **functor** (map) function is created with the same name as the type constructor. The type of this functor depends on how the type parameter is used:

| Polarity of type var | Functor type |
|---------------------|-------------|
| Positive only | `(alpha → beta) → T alpha → T beta` |
| Negative only | `(alpha → beta) → T beta → T alpha` |
| Both | `(alpha → alpha) → T alpha → T alpha` |
| Neither (unused) | `T alpha → T beta` (or similar) |

`polarity.c` analyses which type variables appear positively (covariant positions: output) vs. negatively (contravariant positions: input to a function). This determines the functor's type signature.

The `abstype` form allows explicit polarity annotation via the keywords `pos`, `neg`, `none`.

---

## Implications for the C++20 Rewrite

The type system is the most algorithmically complex part of the interpreter. The rewrite will:

1. **Separate** the static `Type` representation (for AST) and the inference `Cell` representation into clearly distinct types using `std::variant`
2. Use `std::shared_ptr<TypeCell>` for unification cells with automatic memory management
3. Track **source locations** through type inference to give precise error messages like:
   ```
   hope:3:15: error: type mismatch
     expected: num
     found:    list alpha
     in expression: x :: 1
   ```
4. Implement `std::expected<Type, TypeError>` for clean error propagation
5. Regular types (`mu`) will be a first-class variant in the type cell representation

See [11-rewrite-roadmap.md](11-rewrite-roadmap.md).
