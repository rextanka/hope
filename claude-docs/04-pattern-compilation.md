# Pattern Matching Compilation

## Files
- `src/compile.c` — compiles pattern-match equations to decision trees
- `src/cases.h` / `src/cases.c` — the `UCase`/`LCase` decision tree node types
- `src/path.h` / `src/path.c` — paths through heap structures (used to navigate to matched values)

---

## Overview

Hope functions are defined by **multiple equations with patterns**, e.g.:

```hope
dec fact : num -> num;
--- fact 0     <= 1;
--- fact(n+1)  <= (n+1) * fact n;
```

Each equation is added to the function incrementally as it is parsed. Rather than interpreting patterns at runtime, the compiler builds a **decision tree** (`UCase`) that dispatches on the constructors/values in the arguments.

The key entry point is:

```c
UCase *comp_branch(UCase *old_body, Branch *branch);
```

Called once per equation. It takes the existing decision tree `old_body` (or `NULL` for the first equation) and the new `Branch`, and returns an updated tree that handles both.

---

## Decision Tree Nodes (`cases.h`)

### `UCase` — upper (dispatch) nodes

```c
enum { UC_CASE, UC_F_NOMATCH, UC_L_NOMATCH, UC_SUCCESS, UC_STRICT };

struct _UCase {
    short uc_class;
    union {
        struct { short references; short level; Path path; LCase *cases; } // UC_CASE
        Func  *uc_defun;  // UC_F_NOMATCH: no match, report function name
        Expr  *uc_who;    // UC_L_NOMATCH: no match in lambda
        Expr  *uc_who;    // UC_STRICT: strict evaluation marker
        struct { int size; Expr *body; }  // UC_SUCCESS: matched, run body
    };
};
```

| Class | Meaning |
|-------|---------|
| `UC_CASE` | Inspect argument at `(level, path)`, dispatch via `LCase` |
| `UC_F_NOMATCH` | No pattern matched — runtime error naming the function |
| `UC_L_NOMATCH` | No pattern matched in a lambda — runtime error |
| `UC_SUCCESS` | All patterns matched — evaluate `uc_body` |
| `UC_STRICT` | Force strict evaluation (used for built-ins) |

`uc_level` — which argument to inspect (0 = first, 1 = second, etc.)
`uc_path` — a path into the argument's heap structure (e.g. follow left branch, then predecessor, etc.)
`uc_references` — reference count for sharing UC_CASE nodes (avoids duplication when merging trees)

### `LCase` — lower (branch) nodes

```c
enum { LC_ALGEBRAIC, LC_NUMERIC, LC_CHARACTER };

struct _LCase {
    short lc_class;
    int   lc_arity;
    union {
        UCase **lc_limbs;        // LC_ALGEBRAIC, LC_NUMERIC: array of subtrees
        CharArray *lc_c_limbs;   // LC_CHARACTER: sparse map for 256 chars
    };
};
```

Each `LCase` has one subtree per possible case:
- `LC_ALGEBRAIC`: one slot per data constructor, indexed by `c_index`
- `LC_NUMERIC`: three slots indexed `LESS`/`EQUAL`/`GREATER` — for `< 0`, `= 0`, `> 0` (Paterson implements integers via the `succ` constructor, so matching a number n means "match GREATER n times then EQUAL")
- `LC_CHARACTER`: a `CharArray` with up to 256 slots, one per ASCII character

---

## Paths (`path.h`)

A `Path` is an encoded sequence of navigation steps used to locate a value deep within an argument:

| Step | Meaning |
|------|---------|
| `P_LEFT` | Take the left component of a pair `(a, b)` |
| `P_RIGHT` | Take the right component of a pair `(a, b)` |
| `P_PRED` | Follow `succ` — take the predecessor of a number |
| `P_STRIP` | Strip a constructor tag to get its argument |

For example, to reach `y` in the pattern `f (x, y :: ys)`:
- `y` is at path `RIGHT · HEAD` (right of the pair, then the head of the list cons)
- `ys` is at path `RIGHT · TAIL`

Paths are built using `p_push()` and reversed/stashed into compact form for storage in the `UCase` nodes.

---

## Compilation Algorithm (`compile.c`)

### Step 1: Generate match list

`scan_formals(level, formals)` walks the formal parameter list (the left-leaning `E_APPLY` chain) and calls `gen_matches()` for each pattern. This produces a flat list of `Match` records:

```c
typedef struct {
    short  level;    // which argument (0-based)
    Path   where;    // path within that argument
    ushort ncases;   // number of constructors (NUMCASE or CHARCASE for literals)
    ushort index;    // which constructor/value to match
} Match;
```

Each `Match` says: "at argument `level`, path `where`, I need to see constructor `index` out of `ncases` possibilities."

**Numeric literals** are expanded into chains of `NUMCASE/GREATER` steps plus a final `NUMCASE/EQUAL`:
```
match 3  →  [GREATER at path, GREATER at pred(path), GREATER at pred²(path), EQUAL at pred³(path)]
```

**p+k patterns** similarly generate k `GREATER` matches followed by the inner pattern.

**Pair patterns** split: left component and right component are matched independently.

### Step 2: Build a skinny tree

`gen_tree(matches, failure)` constructs a linear chain of `UC_CASE` nodes from the match list, with the body (`UC_SUCCESS`) at the leaf and `failure` at every side branch. This represents "this exact sequence of checks must succeed."

### Step 3: Merge into existing tree

`merge(old_tree)` merges the new skinny tree into the existing decision tree. The merge respects:

**Ordering**: checks are ordered first by argument level, then by path (left-to-right, outermost-to-innermost). This ensures a consistent, deterministic traversal order.

**Sharing**: when a `UC_CASE` node has `uc_references > 1`, it is copied before modification (copy-on-write). This prevents one equation's changes from corrupting another's subtree.

**Specificity**: `UC_SUCCESS` nodes carry `uc_size` — the number of non-variable pattern checks in the branch. When merging, a more specific pattern (larger `uc_size`) is preferred. This implements Hope's "best-fit" pattern matching: the most specific matching pattern wins, regardless of textual order.

```
merge(old):
  if old is NOMATCH: insert gen_tree(remaining matches, old)
  if old is SUCCESS:
    if new branch is more specific: insert new before old
    else: leave old in place (old pattern is more specific)
  if old is CASE:
    if next match is earlier than this node: insert new node above
    if next match is at same position: recurse into matching limb
    if next match is later: recurse into all limbs (map merge)
```

### Step 4: Compile sub-expressions

`comp_expr(expr)` recursively visits the body expression and compiles any `E_LAMBDA`/`E_EQN`/`E_PRESECT`/`E_POSTSECT` nodes it finds, building their decision trees as well.

---

## Incremental Compilation

Each equation is compiled as it is parsed:

```c
fn->f_code = comp_branch(fn->f_code, branch);
```

For the first equation: `fn->f_code` is initialised to `f_nomatch(fn)` (a tree that always fails), then `comp_branch` replaces it with the first match.

For subsequent equations: the new branch is merged into the existing tree. The tree grows incrementally and correctly handles overlapping patterns.

---

## Example

```hope
dec fib : num -> num;
--- fib 0     <= 1;
--- fib 1     <= 1;
--- fib(n+2)  <= fib n + fib(n+1);
```

After equation 1 (`fib 0`), the tree is:
```
UC_CASE(level=0, path=[], cases=NUMERIC[LESS:nomatch, EQUAL:SUCCESS(1), GREATER:nomatch])
```

After equation 2 (`fib 1`), the GREATER branch gets:
```
UC_CASE(level=0, path=[PRED], cases=NUMERIC[LESS:nomatch, EQUAL:SUCCESS(1), GREATER:nomatch])
```

After equation 3 (`fib(n+2)`), the second GREATER branch gets a SUCCESS with the recursive body. The variable `n` is bound to the value at path `[PRED, PRED]` (two predecessors deep).

---

## Runtime Use

The interpreter (`interpret.c`) executes a decision tree node by node:

- `UC_CASE`: evaluate the argument to WHNF, navigate to `(level, path)`, inspect the constructor tag, dispatch into the appropriate `lc_limbs[i]` subtree
- `UC_SUCCESS`: bind the matched variables (by paths stored during compilation) and evaluate `uc_body`
- `UC_F_NOMATCH` / `UC_L_NOMATCH`: report a runtime pattern-match failure error

See [05-runtime-krivine.md](05-runtime-krivine.md) for how the Krivine machine executes these trees.

---

## Implications for the C++20 Rewrite

The decision tree approach is efficient and well-suited for the rewrite. In C++20:

- `UCase` and `LCase` become `std::variant` types with named fields
- The `Path` encoding can use `std::vector<PathStep>` with a clear enum for steps
- The merge algorithm stays largely the same, but the copy-on-write is replaced by proper value semantics or `std::shared_ptr` with clone-on-mutation
- Source locations can be attached to `UC_SUCCESS` nodes for better error messages on match failure: "no pattern matched at file:line:col"
