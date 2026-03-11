# Runtime and Krivine Machine

## Files
- `src/interpret.c` — the Krivine machine interpreter (core lazy evaluator)
- `src/eval.c` — top-level evaluation entry points
- `src/runtime.c` — heap/stack initialisation and garbage collector
- `src/memory.c` — low-level memory allocator (arenas for strings/structs)
- `src/heap.h` — `Cell` struct and cell class definitions
- `src/value.h` — data cell classes and constructors
- `src/stack.h` — stack operations (Push/Pop/Update)

---

## Memory Layout

The interpreter uses a single flat allocation arena divided into three zones:

```
base_memory ──┐
              │  String table (interned strings, newstring())
top_string  ──┤
              │  Temporary / struct area (AST nodes, type cells, etc.)
base_temp   ──┤
lim_temp    ──┤  ← this boundary moves to accommodate heap/stack split
              │
BaseHeap    ──┤  ← Heap (Cell objects, grows upward)
heap        ──┤  ← current heap top
heap_limit  ──┤  ← GC trigger threshold
              │
              │  Stack (StkElt objects, grows downward from TopStack)
stack_limit ──┤
stack       ──┤  ← current stack top
TopStack    ──┘  ← stack base (fixed)
top_memory
```

The heap-to-stack ratio is 5:1 (HEAP=5, STACK=1 in `runtime.c`). Type checking uses the heap directly without GC; execution uses `chk_heap()` to pre-allocate before each step.

---

## Heap Cells (`heap.h`, `value.h`)

All runtime values live as `Cell` objects on the heap. The `c_class` field (one byte) encodes both the **arity** (number of sub-cells) and a **tag** within that arity:

```
c_class = (arity << 4) | tag
```

GC uses the top bit (`GC_MARK = 0x40`) during mark phase.

### Cell classes

| Class | Arity | Description |
|-------|-------|-------------|
| `C_NUM` | 0 | Integer (or real) value. `c_num` field. |
| `C_CHAR` | 0 | Character value. `c_char` field. |
| `C_CONST` | 0 | Data constant (0-arg constructor). `c_cons` field. |
| `C_STREAM` | 0 | Lazy input stream (file pointer). `c_file` field. |
| `C_HOLE` | 0 | Black hole — detected as "infinite loop" |
| `C_CONS` | 1 | Data constructor applied to argument. `c_cons` + `c_arg`. |
| `C_SUSP` | 1 | Suspension (thunk): `(Expr, Env)`. `c_expr` + `c_env`. |
| `C_DIRS` | 1 | Path navigator: `(Path, Cell*)`. `c_path` + `c_val`. |
| `C_UCASE` | 1 | Decision tree node with environment. `c_code` + `c_env`. |
| `C_LCASE` | 1 | Branch table with environment. `c_lcase` + `c_env`. |
| `C_PAPP` | 1 | Partial application. `c_expr` + `c_env`, `c_arity`. |
| `C_PAIR` | 2 | Pair. `c_left` + `c_right`. Used for environments and pair values. |

**Type cells** (`C_TVAR`, `C_TSUB`, etc. from `type_value.h`) share the same `Cell` struct, distinguished by their class numbers. They live in the same heap but are never present during execution — only during type checking.

---

## The Environment

Environments are **linked lists of pairs** (`C_PAIR` cells). The environment at depth n is:

```
env = (v0, (v1, (v2, ... NULL)))
```

where `v0` is the most-recently-bound variable. Accessing variable at level k means following k `c_right` links then taking `c_left`.

The pair `(expr, env)` forms a **closure** (stored as `C_SUSP`).

---

## Krivine Machine (`interpret.c`)

The interpreter is a **call-by-need (lazy) Krivine machine** implemented as a tight `repeat`/`switch` loop in `run(Cell *current)`.

### Machine state
- `current` — the cell currently being evaluated
- `stack` — a downward-growing stack of `StkElt` (either a `Cell*` or an update frame pointer)

### Basic transitions

| Current cell | Action |
|-------------|--------|
| `C_SUSP (expr, env)` | **Enter** the suspension: set current = expr (with env), mark cell as `C_HOLE` |
| `C_PAPP (expr, env, arity > 0)` | **Pop argument** from stack; collect into env, decrement arity |
| `C_PAPP (expr, env, arity = 0)` | **Apply** fully: invoke decision tree or constructor |
| `C_NUM` / `C_CHAR` / `C_CONST` | **Value** reached: perform pending updates, then pop continuation |
| `C_CONS` | **Constructor**: perform updates; if FORCE_MARK, force the argument |
| `C_PAIR` | **Pair**: perform updates; if FORCE_MARK, force left then right |
| `C_UCASE (code, env)` | **Pattern match** dispatch: execute `UCase` decision tree node |
| `C_LCASE (lcase, env)` | **Branch** on inspected value's constructor index |
| `C_DIRS (path, val)` | **Navigate** path steps (LEFT, RIGHT, STRIP, PRED) |
| `C_STREAM` | **Lazy I/O**: read next character from stream |
| `C_HOLE` | **Error**: infinite loop detected |

### Call-by-need: update frames

When entering a `C_SUSP`, the cell is first marked `C_HOLE`. An **update frame** is pushed onto the stack pointing back to the original suspension cell. When a value is reached:

```c
// take(): perform all pending updates
Cell *take(Cell *current) {
    while (IsUpdate())
        *PopUpdate() = *current;  // in-place update (overwrite the thunk)
    return Pop();
}
```

This implements **memoisation**: once a thunk is evaluated, the result is written back to the heap cell, so future references to the same thunk find the value directly.

### FORCE_MARK

`FORCE_MARK` is a special sentinel value on the stack. When a value becomes current and `FORCE_MARK` is at the top of the stack, the value must be fully evaluated (forced to head normal form). This is used by:
- The `evaluate()` function (forces for printing)
- `C_PAIR`: forces both components recursively
- `UC_STRICT`: forces the argument before calling a strict built-in

### Suspension entry (C_SUSP)

Each expression class has a corresponding action:

| Expression | Action in interpreter |
|-----------|----------------------|
| `E_APPLY` | Push `(e_arg, env)` onto stack; continue with `(e_func, env)` |
| `E_PAIR` | Build a `C_PAIR` of two new suspensions |
| `E_RLET/E_RWHERE` | Build a **cyclic environment** (env points to itself); start body |
| `E_MU` | Build a cyclic environment for the recursive value |
| `E_DEFUN` | Create `C_PAPP` for the function, arity from `fn->f_arity` |
| `E_LAMBDA` | Create `C_PAPP` capturing current env |
| `E_NUM` | Allocate `C_NUM` |
| `E_CHAR` | Allocate `C_CHAR` |
| `E_CONS` | 0-arg: `C_CONST`. n-arg: `C_PAPP` |
| `E_PARAM` | Navigate env chain by `e_level`, then follow path via `C_DIRS` |
| `E_BUILTIN` | Call C function directly with `env->c_left` as argument |
| `E_BU_1MATH` | Apply unary math function to `env->c_left->c_num` |
| `E_BU_2MATH` | Apply binary math function to pair in env |
| `E_RETURN` | Return from `run()` immediately |

### Decision tree execution (C_UCASE / C_LCASE)

```
C_UCASE (UC_CASE):
  1. Navigate to the inspected argument: follow env chain by uc_level,
     then navigate the path via C_DIRS
  2. Push the argument (as C_DIRS) onto stack
  3. Push a C_LCASE continuation (to dispatch on the result)
  4. EnterUpdate(the argument) — forces it to WHNF, memoises

C_LCASE (LC_ALGEBRAIC):
  1. Pop the (now updated) argument
  2. Look up the constructor index in lc_limbs
  3. Continue with new_ucase(selected_subtree, env)

C_LCASE (LC_NUMERIC):
  1. Pop the argument
  2. Compare c_num to Zero: dispatch LESS/EQUAL/GREATER
  3. Continue with selected subtree

C_UCASE (UC_SUCCESS):
  1. The pattern matched — evaluate the body expression
  2. current = new_susp(code->uc_body, env)

C_UCASE (UC_F_NOMATCH / UC_L_NOMATCH):
  1. Print the failing arguments (for diagnostics)
  2. Report runtime error "no match found"
```

### Letrec / cyclic structures

`E_RLET` and `E_MU` create **cyclic heap structures** to represent recursive values:

```c
// RLET: letrec p == e in body
env = new_pair(new_susp(e, NULL_ENV), outer_env);
env->c_left->c_env = env;  // closure points back to env
current = new_susp(body, env);
```

The suspension's environment points to itself. When the suspension is entered, it will find its own value in the environment — this is how Paterson implements recursive definitions without a fixed-point combinator.

---

## Garbage Collector (`runtime.c`)

The GC is **mark-sweep**:

1. **Mark**: set `GC_MARK` bit on every cell from `BaseHeap` to `heap`.
2. **Unmark reachable**: call `reach(cell)` on:
   - `expr_type` (current inferred type, needed for error messages)
   - `current` (the cell being evaluated)
   - Every cell on the stack (both normal values and update frame targets)
3. **Sweep**: add all still-marked cells to the free list.

`reach(cell)` follows all `Cell*` links recursively:
- Arity 0: no children (stop)
- Arity 1: one child (`c_sub`)
- Arity 2: two children (`c_sub1`, `c_sub2`)

The GC is triggered by `chk_heap(current, n)` when fewer than n free cells are available. If the GC recovers fewer than `MIN_RECOVERED = 100` cells, it reports "out of memory."

The **preserved zone**: the Standard module's heap cells are allocated before `start_stack()` is called. These cells are before `BaseHeap` (actually, BaseHeap is set to `top_string` after the standard module is loaded, so they are in the struct area and never GC'd). The `preserve()` call in `main.c` marks the current heap top as permanent.

---

## Stack Operations (`stack.h`)

```c
Push(cell)          // push a Cell* onto the stack
Pop()               // pop and return top Cell*
PushUpdate(cell)    // push an update frame pointing to cell
IsUpdate()          // true if top of stack is an update frame
PopUpdate()         // pop and return the update frame pointer
FORCE_MARK          // sentinel: signals "force to HNF"
```

The stack grows downward. Stack overflow is detected by `chk_stack(n)`.

---

## Entry Points

```c
void interpret(Expr *action, Expr *expr);
```
Called to evaluate an interactive expression. Sets up the environment with `input` bound to a lazy stdin stream, then runs the machine.

```c
Cell *evaluate(Cell *value);
```
Forces a cell to head normal form (for pretty-printing). Pushes a FORCE_MARK and an `e_return` sentinel, then runs the machine until E_RETURN is reached.

---

## Implications for the C++20 Rewrite

The Krivine machine is the right approach to keep. In C++20:

- `Cell` becomes `std::variant<Num, Char, Const, Cons, Susp, Pair, Papp, Dirs, UCase, LCase, Stream, Hole>` stored in a pool allocator
- The `run()` loop becomes `std::visit`-based dispatch or a switch on a tag enum (both viable; tagged enum + switch is likely faster)
- Update frames are stored on a separate `std::vector<Cell**>` (update stack) alongside the argument stack `std::vector<std::shared_ptr<Cell>>`
- **Cyclic structures** (letrec, mu) require reference-counting or a tracing GC; `std::shared_ptr` handles cycles poorly. A simple arena + mark-sweep (like Paterson's) or a conservative GC (Boehm is excluded — see plan) is better. The rewrite will use a custom arena with mark-sweep, similar to the original
- FORCE_MARK becomes a `std::monostate` variant in the stack type
- The `C_HOLE` class (black hole detection for infinite loops) must be preserved — it is essential for detecting `ones == 1 :: ones`-style infinite loops that aren't written with whererec

See [11-rewrite-roadmap.md](11-rewrite-roadmap.md).
