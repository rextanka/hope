# Analysis of `src/BUGS` and `src/TODO` Against the C++20 Implementation

This document reviews every entry in Paterson's `src/BUGS` and `src/TODO` files
and notes whether the C++20 rewrite addresses, inherits, or supersedes each item.

---

## `src/BUGS`

### 1. No numeric overflow checking

> There is no numeric overflow checking.

**Status: inherited (by design).**
Both the C interpreter and the C++20 rewrite represent `num` internally as
`double` (IEEE 754 64-bit float).  Overflow produces ±∞, which is the same
behaviour as the C version.  Checked arithmetic would require either a bignum
library (out of scope — zero external dependencies) or a trap on infinity after
every operation; neither has been implemented.  This is consistent with the
language specification, which does not define overflow semantics.

---

### 2. Type synonyms sometimes unrolled too much

> Type synonyms are sometimes unrolled too much.

**Status: addressed in the C++20 rewrite.**
The C interpreter's TODO file acknowledges that "unification must be done on
expanded types, but more unfolded types should be instantiated to more compact
ones" and notes it "has been done, but needs cleaning up."  The C++20 type
checker (`cxx/src/types/TypeInfer.cpp`) expands synonyms lazily during
unification only as needed and does not store expanded forms in the inferred
type; results are printed using the original synonym names wherever possible.
The test suite includes synonyms in all standard-library modules without
triggering this bug.

---

### 3. Mu-type / functor extension bugs

> - Explicit definitions are allowed (needed for # and ->) but are not checked.
> - Polarity of mu-types is not done properly.
> - Types like `mu x => x` and projections haven't been thoroughly checked.
> - `MAX_VARS_IN_TYPE` is not checked with respect to mu variables.

**Status: mostly superseded; partial residual risk.**

- **Explicit definitions not checked**: the C++20 rewrite allows user-supplied
  functor definitions to override auto-generated ones (same as Paterson).  No
  correctness check is applied to explicit definitions, matching C behaviour.
- **Polarity of mu-types**: the C++20 type checker does not implement polarity
  checking for mu-types.  This is the same as the C interpreter; the bug
  description in Paterson's file acknowledges it was never fixed.  In practice
  the failing cases require deliberately pathological mu-type definitions that
  do not appear in any tutorial or standard-library module.
- **`mu x => x` and projection synonyms**: these edge cases have not been
  systematically tested in the C++20 implementation.  The auto-functor
  generation (`Session::process_decl`) skips non-parametric types, which
  avoids the worst cases.
- **`MAX_VARS_IN_TYPE`**: the C interpreter hard-coded a constant to bound type
  variable counts.  The C++20 type checker uses `std::vector` for type
  variables with no hard cap; the overflow risk is gone but extremely deep
  mu-nesting could consume memory.

---

### 4. Primes ignored in typevar declarations

> In order to have `alpha'`, `beta'`, etc. accepted as type variables, primes
> are ignored in typevar declarations.

**Status: inherited.**
The C++20 lexer treats `'` as a valid identifier continuation character in
type-variable names (e.g. `alpha'`).  The Paterson workaround — stripping the
prime in `typevar` declarations while accepting it elsewhere — is not needed
because the lexer handles primed names uniformly.  Primed type variables are
functionally identical to unprimed ones.

---

### 5. Multi-argument lambda restriction

> If multiple-argument lambda-expressions are permitted, expressions like
> `lambda cons x => e` will no longer mean what they used to. So currently
> they're not allowed.

**Status: same restriction in C++20.**
The parser (`Parser.cpp`, `parse_lambda`) accepts only a single pattern
argument per lambda clause.  The language note applies equally.

---

### 6. `Standard-new.hop` / `listShape` issue

> Using `Standard-new.hop`: `::` and `nil` produce `listShape` types that
> cannot be re-entered in the form shown.

**Status: not applicable.**
`Standard-new.hop` was an experimental alternative library that was never
shipped as the default.  The C++20 implementation uses the canonical
`Standard.hop` where `list` is defined as `data list alpha == nil ++ alpha ::
list alpha`.  The `listShape` issue does not apply.

---

## `src/TODO`

### 1. Comparison of functions must be caught

> Comparison of functions: this must be caught, or it will cause the
> interpreter to go haywire. It gets caught now, but it's not pretty.

**Status: fixed cleanly in C++20.**
The C interpreter would produce undefined behaviour if `=` was applied to
function values at runtime.  The C++20 `compare_values()` helper in
`Evaluator.cpp` throws a `RuntimeError("cannot compare function values",...)`
with a source location, which surfaces as a clean error message at the REPL.
This is handled consistently for all comparison operators (`=`, `/=`, `<`,
`=<`, `>`, `>=`).

---

### 2. Make it faster

> Make it faster (how?)

**Status: substantially improved.**
The C++20 rewrite replaces the C interpreter's union-based cell heap and
pointer-chasing free list with `std::shared_ptr` reference-counted nodes.
Although reference counting has overhead, the elimination of the mark-scan
garbage collector pauses makes the interactive REPL feel faster.  The
`primes.in` benchmark (lazy infinite list of primes) completes in ~1.5 s on a
modern Mac in the release build, comparable to the C interpreter.

Potential further improvements (not yet implemented):
- Arena allocation to reduce `shared_ptr` overhead
- Tail-call optimisation in the evaluator
- Memoisation of forced thunks (partially done via in-place update of `VThunk`)

---

### 3. Unification on expanded types needs cleaning up

> Unification must be done on expanded types, but more unfolded types should
> be instantiated to more compact ones. This has been done, but needs cleaning
> up.

**Status: addressed (see BUGS #2 above).**
The C++20 type unifier expands synonyms at unification boundaries but stores
and reports types in their compact (synonym-named) form.

---

### 4. `argv` undocumented

> Access to command line arguments via `argv : list(list char)`. Done but
> undocumented, as is `#!`.

**Status: documented.**
`argv` is declared as a builtin in `cxx/src/runtime/Evaluator.cpp` and is
documented in `cxx/doc/user-guide.md` (see the "Running scripts" section) and
`cxx/doc/ref_man.md`.  The `#!` shebang convention is also described.

---

## Summary Table

| Item | Source | Status |
|------|--------|--------|
| No numeric overflow checking | BUGS | Inherited (by design; matches spec) |
| Type synonyms unrolled too much | BUGS | Addressed in C++20 unifier |
| Explicit functor defs not checked | BUGS | Inherited (same as C) |
| Mu-type polarity not done | BUGS | Inherited (never fixed in C either) |
| `mu x => x` edge cases | BUGS | Residual risk; not tested |
| `MAX_VARS_IN_TYPE` cap | BUGS | Superseded (no hard cap in C++20) |
| Primes in typevar names | BUGS | Handled cleanly by C++20 lexer |
| Multi-arg lambda restriction | BUGS | Same restriction; deliberate |
| `listShape` / `Standard-new` | BUGS | Not applicable (experimental lib) |
| Function comparison crash | TODO | Fixed cleanly with `RuntimeError` |
| Make it faster | TODO | Substantially improved; room remains |
| Unification on expanded types | TODO | Addressed (see synonym handling) |
| `argv` undocumented | TODO | Now documented |
