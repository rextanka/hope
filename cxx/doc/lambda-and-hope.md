# Hope and the Lambda Calculus

## Types, Algebraic Data, and Equational Reasoning

This document is for readers who already know the λ calculus and want to
understand Hope's formal underpinnings. It is not a λ calculus primer; for
that, see Rojas [1] or Jung [2]. It is also not a Hope tutorial; for that, see
Bailey's *Byte* article [3] or `cxx/doc/hope_tut.md`. The goal here is to make
the connection between the two precise: to show that Hope's syntax is a direct
notational variant of typed λ calculus, that its type inference is
Hindley-Milner, that its `data` declarations correspond to Scott encodings, and
that its equations support equational reasoning and structural induction.
Readers interested in formal methods, program verification, or the theoretical
foundations of functional programming will find this the most useful entry
point into Hope.

**Prerequisites**: untyped λ calculus (syntax, β-reduction, α-equivalence,
Church-Rosser), basic type theory (simple types, function types), and
familiarity with Hope syntax (the user guide at `cxx/doc/user-guide.md` or
`cxx/doc/hope_tut.md`).

---

## 1. From λ Terms to Hope Expressions

### 1.1 Syntax

The untyped λ calculus has three forms [1]:

```
M, N ::=  x          variable
        | λx.M       abstraction
        | M N        application (left-associative)
```

Hope's expression syntax is a direct notational variant. The mapping is
one-to-one:

| λ calculus      | Hope                          | Notes                            |
|-----------------|-------------------------------|----------------------------------|
| `x`             | `x`                           | Variable                         |
| `λx.M`          | `lambda x => M`               | Abstraction; `\` also accepted   |
| `M N`           | `f x` or `f(x)`               | Application; left-associative    |
| `λx.λy.M`       | `lambda x => lambda y => M`   | Curried two-argument function    |
| `(λx.M) N`      | `(lambda x => M)(N)`          | Explicit application             |

Hope also supports a multi-argument constructor-style syntax `f(x, y)`, but
this is syntactic sugar for applying `f` to the *pair* `(x, y)` — the type
of the argument is `tau # sigma`, not two separate arguments. Curried style
(`f x y`) and tupled style (`f(x, y)`) coexist in Hope but have different
types.

Application is left-associative in both formalisms: `f x y` means `(f x) y`,
exactly as `f x y` means `(f x) y` in the λ calculus.

### 1.2 Beta-reduction and Hope evaluation

In the λ calculus, computation is β-reduction:

```
(λx.M) N  →β  M[N/x]
```

Hope evaluation performs the same substitution when a function is applied to
an argument. The difference is that Hope functions are *named* and defined by
*equations* rather than anonymous lambda terms, so evaluation proceeds by
matching the argument against the equation patterns and substituting into the
right-hand side. For a single-equation definition with a variable pattern:

```hope
dec double : num -> num;
--- double n <= n * 2;
```

The equation `--- double n <= n * 2` acts as the rewrite rule
`double N →  N * 2`, which is precisely β-reduction of `(λn. n * 2) N`.

### 1.3 Lambda expressions in Hope

Hope retains first-class lambda expressions. The standard composition function
from Burstall, MacQueen & Sannella (1980) [5] demonstrates:

```hope
dec compose : (alpha -> beta) # (tau -> alpha) -> tau -> beta;
--- compose(f, g) <= lambda x => f(g x);
```

The right-hand side `lambda x => f(g x)` is a λ abstraction. Evaluating
`compose(inc, double)(5)`:

```
compose(inc, double)(5)
→  (lambda x => inc(double x))(5)
→β  inc(double 5)
→   inc(10)
→   11
```

This is plain β-reduction, step by step.

---

## 2. Types

### 2.1 Simply typed λ calculus

The simply typed λ calculus adds type annotations to the untyped calculus.
Types are:

```
τ ::=  o          base type
     | τ → τ      function type (right-associative)
```

Typing rules:

```
  x : τ ∈ Γ
  ─────────────  Var
  Γ ⊢ x : τ

  Γ, x : σ ⊢ M : τ
  ─────────────────────  →I (abstraction)
  Γ ⊢ λx.M : σ → τ

  Γ ⊢ M : σ → τ    Γ ⊢ N : σ
  ─────────────────────────────  →E (application)
  Γ ⊢ M N : τ
```

These rules are directly reflected in Hope. The `dec` declaration is an
explicit typing assertion: `dec f : sigma -> tau` places `f : sigma -> tau`
into the type environment Γ. Hope then checks that the equations for `f` are
consistent with this declared type.

### 2.2 Parametric polymorphism and Hindley-Milner

Simply typed λ requires every function to have a *monomorphic* type.
`identity : num -> num` and `identity : bool -> bool` would be two different
functions with two different types. This is unsatisfactory.

Hindley (1969) and Milner (1978), following Damas and Milner (1982), extended
the simply typed calculus with *universally quantified type variables*. The
identity function has the *polymorphic* type `∀α. α → α`: for any type `α`,
if you supply a value of type `α` you get back a value of type `α`. This is
parametric polymorphism [6].

In the Hindley-Milner (HM) system, every well-typed term has a *principal
type* — a most general type from which all other valid types can be obtained
by substituting concrete types for the type variables. This is the property
that makes type inference tractable: the type checker does not guess; it
computes the unique most general type.

The Damas-Milner algorithm W [6] infers principal types without any
annotations, using unification to solve the constraints generated by the
typing rules. This is why Hope programs need no type annotations on
expressions — the types are computed, not declared.

### 2.3 Type variables in Hope

Hope's `typevar` declaration introduces a type variable into scope:

```hope
typevar alpha;
typevar beta;
```

These play the role of the universally quantified variables in the HM type
scheme. A `dec` using these variables declares the principal type:

```hope
dec identity : alpha -> alpha;
--- identity x <= x;
```

The inferred type of `identity` is the HM type scheme `∀α. α → α`. When
`identity` is applied to a `num`, `α` is instantiated to `num`; when applied
to a `bool`, `α` is instantiated to `bool`.

The same applies to multi-parameter polymorphism:

```hope
dec compose : (beta -> gamma) # (alpha -> beta) -> alpha -> gamma;
--- compose(f, g) <= lambda x => f(g x);
```

The three type variables `alpha`, `beta`, `gamma` are all universally
quantified: `∀α β γ. (β→γ) × (α→β) → α → γ`. This is exactly the type of
function composition in category theory.

### 2.4 Product types

Hope's `#` is the product type constructor: `alpha # beta` is the type of
pairs `(a, b)` where `a : alpha` and `b : beta`. In the λ calculus, products
are encoded (Church-style: `pair = λa.λb.λf.f a b`; projection functions
`fst = λp.p(λa.λb.a)`, `snd = λp.p(λa.λb.b)`). Hope's `#` names this
encoding directly, and the pair syntax `(e1, e2)` is the corresponding
constructor.

The function type `(alpha # beta) -> gamma` and the curried type
`alpha -> beta -> gamma` are not the same in Hope. The first takes a single
pair argument; the second takes arguments one at a time. Conversion between
them corresponds to the curry/uncurry adjunction: `curry f = λa.λb.f(a,b)`.

### 2.5 Type synonyms and abbreviations

Hope's `type` declaration introduces a type abbreviation:

```hope
type string == list char;
```

This is purely a notational convenience; `string` and `list char` are
interchangeable everywhere. In the λ calculus terms, this is a definitional
equality at the type level: `string =def list char`.

Recursive type synonyms (Paterson's *regular types*) use a `mu` notation:

```hope
type stream alpha == alpha # stream alpha;
```

This denotes the *least fixed point* `μS. α × S` — the type of infinite
streams. Regular types are discussed further in section 5.

---

## 3. Algebraic Data Types

### 3.1 Church vs. Scott encodings

In the pure untyped λ calculus, data must be encoded as functions. There are
two classical approaches for encoding sums and products.

**Church encodings** represent a value by the *function that consumes it*:

```
zero  =  λf. λx. x
succ  =  λn. λf. λx. f (n f x)
```

A Church numeral `n` is the function that applies `f` exactly `n` times to
`x`. Addition, multiplication, and comparison can be written directly. The
disadvantage is that pattern matching requires the consumer to be passed in as
an argument — there is no way to *inspect* the structure of the value; you
can only apply it.

**Scott encodings** represent a value by the *case analysis it supports*:

```
zero  =  λz. λs. z
succ  =  λn. λz. λs. s n
```

A Scott numeral `n` is passed two alternatives: what to return for zero, and
what to do with the predecessor for a successor. This supports pattern
matching directly: `n e1 (λpred. e2)` branches on whether `n` is zero or
successor, binding `pred` to the predecessor in the successor branch.

Scott encodings are the theoretical underpinning of Hope's `data`
declarations [4, Ch. 5].

### 3.2 Hope's `data` declarations

A `data` declaration names a set of Scott-encoded constructors:

```hope
data bool == false ++ true;
```

This declares two nullary constructors, which correspond to the Scott booleans
`λt. λf. t` (true) and `λt. λf. f` (false). The `++` separates alternatives,
corresponding to the disjoint union (sum) at the type level.

```hope
data list alpha == nil ++ a :: (list alpha);
```

This declares the list type with two constructors: `nil` (the empty list) and
`(::)` which takes an `alpha # list alpha` pair. In Scott encoding:

```
nil   =  λn. λc. n
(::)  =  λa. λl. λn. λc. c a l
```

A list value `l` applied to two arguments selects: `l e_nil (λhd.λtl.e_cons)`
gives `e_nil` if `l` is empty, and `e_cons hd tl` if `l` is `hd :: tl`. This
is exactly what Hope's pattern matching does.

The general form:

```hope
data T alpha == C1(tau_1) ++ C2(tau_2) ++ ... ++ Cn(tau_n);
```

declares an n-alternative sum type where each constructor `Ci` has argument
type `tau_i` (omitted for nullary constructors). The Scott encoding of a value
`Ci(v)` is a function of `n` arguments that selects its `i`th argument and
applies it to `v`.

### 3.3 Constructors as functions

In Hope, every constructor is a first-class function:

- `succ : num -> num`
- `(::) : alpha # list alpha -> list alpha`
- `nil : list alpha`

This is exactly correct for Scott encodings, where constructors are λ
abstractions. They can be passed as arguments, returned as results, and
composed:

```hope
dec map : (alpha -> beta) # list alpha -> list beta;
--- map(f, nil)     <= nil;
--- map(f, x :: xs) <= f(x) :: map(f, xs);
```

`map(succ, [1, 2, 3])` applies `succ` as a first-class function to each
element.

---

## 4. Pattern Matching and Structural Recursion

### 4.1 Pattern matching as case analysis

In the pure λ calculus, case analysis on a Scott-encoded value `v` is written:

```
v  (result for case 1)  (λx. result for case 2, binding x)  ...
```

Hope's equation syntax `--- f(pattern) <= expr` is syntactic sugar for
exactly this. The evaluator matches the argument against the LHS pattern and
reduces to the RHS when the pattern matches — this is Scott elimination.

A two-clause definition:

```hope
--- not(false) <= true;
--- not(true)  <= false;
```

corresponds to the λ term `λb. b true false` (applying the Scott bool `b` to
its two alternatives, `true` for the false-case and `false` for the
true-case).

### 4.2 Nested patterns

Patterns in Hope can be nested to arbitrary depth:

```hope
dec and : bool # bool -> bool;
--- and(true, true) <= true;
--- and(_,    _   ) <= false;
```

The underscore `_` is the wildcard pattern, matching anything. Nested
patterns desugar into a sequence of case analyses: the outer pattern matches
the pair; within each alternative, each component is matched further.

### 4.3 Structural recursion and termination

A recursive function is *structurally recursive* if every recursive call is
on a *strict subterm* of the input. Hope's numeric patterns make structural
recursion on `num` explicit:

```hope
dec fib : num -> num;
--- fib(0)          <= 1;
--- fib(succ(0))    <= 1;
--- fib(succ(succ(n))) <= fib(succ(n)) + fib(n);
```

Both recursive calls (`fib(succ(n))` and `fib(n)`) are on values strictly
smaller than the input `succ(succ(n))`. Termination follows by structural
induction on `num`: the argument decreases at every recursive call and reaches
the base cases `0` and `succ(0)`.

For lists:

```hope
dec length : list alpha -> num;
--- length(nil)     <= 0;
--- length(_ :: xs) <= succ(length(xs));
```

The recursive call is on `xs`, the tail of the input list, which is a strict
subterm. Termination follows by structural induction on the list.

**Structural recursion implies termination** for any finite input. This is the
formal basis for proving Hope functions correct: a structurally recursive Hope
function over a well-founded data type always terminates. Functions defined
over `num` can be proved correct by mathematical induction; functions over
`list alpha` can be proved correct by list induction (induction on the length,
equivalently on the structure).

### 4.4 The `n+k` pattern

Hope allows numeric patterns of the form `n+k` (syntactic sugar introduced by
Paterson), which match numbers greater than or equal to `k` and bind `n` to
the difference:

```hope
dec fac : num -> num;
--- fac(0) <= 1;
--- fac(n) <= n * fac(n - 1);
```

The second equation matches any `num` and is therefore a catch-all. With `n+k`
patterns this can be made structural:

```hope
--- fac(0)   <= 1;
--- fac(n+1) <= (n+1) * fac(n);
```

Now the recursive call is on `n`, which is strictly less than `n+1`, and the
pattern is structurally recursive on `num` in the `succ`-based sense.

---

## 5. Laziness and Infinite Structures

### 5.1 Reduction strategies

The λ calculus is confluent (Church-Rosser): if two reduction sequences
terminate, they arrive at the same normal form. But not all strategies
terminate when a normal form exists. There are three important strategies:

**Call-by-value** (strict, innermost-leftmost): reduce arguments to normal
form before substituting. This is the strategy of ML, Haskell's `seq`, and
C. It may fail to terminate when an argument does not have a normal form,
even if the result does not depend on that argument.

**Call-by-name** (lazy, outermost-leftmost): substitute the unreduced argument
and only reduce if needed. This *is* normal-order reduction. By the
standardisation theorem, if any reduction sequence terminates, normal-order
reduction terminates. However, the argument may be duplicated: if it appears
more than once in the function body, it is re-evaluated each time.

**Call-by-need** (lazy with sharing): call-by-name, but the argument is
evaluated at most once — its value is memoised and shared across all uses.
This is Haskell's default strategy and Paterson's extension of Hope.

### 5.2 Lazy constructors in Paterson's Hope

The original Hope (Burstall, MacQueen & Sannella 1980) [5] used strict
evaluation. Paterson's extension introduces call-by-need evaluation for the
arguments of list constructors. This means the second argument to `::` is not
evaluated until needed.

The consequence is that *infinite lists* can be defined and computed with:

```hope
dec allsucs : num -> list num;
--- allsucs n <= n :: allsucs(n + 1);
```

Under strict evaluation, `allsucs(0)` diverges immediately. Under call-by-need,
the `::` constructor returns immediately with `0` as head and a *thunk* (a
suspended computation) as tail. Consuming `front(5, allsucs(0))` forces only
the first five elements:

```
front(5, allsucs(0))
= front(5, 0 :: allsucs(1))       -- force head only
= 0 :: front(4, allsucs(1))
= 0 :: front(4, 1 :: allsucs(2))
= 0 :: 1 :: front(3, allsucs(2))
...
= [0, 1, 2, 3, 4]
```

### 5.3 Regular types and coinduction

Paterson's *regular types* (`type` definitions using `mu`) denote recursive
type equations whose solutions are *infinite* types. The stream type:

```hope
type stream alpha == alpha # stream alpha;
```

denotes `μS. α × S`, the greatest fixed point — the type of infinite sequences.
Values of type `stream alpha` are genuinely infinite under call-by-need
evaluation.

Reasoning about infinite structures requires *coinduction* rather than
induction. A coinductive proof establishes a property of an infinite object
by showing the property is preserved at each unfolding, rather than reaching
a base case. The standard references are Jacobs & Rutten (1997) on coalgebras;
for Hope-level reasoning, it suffices to note that any property provable by
equational unfolding for an arbitrary finite prefix holds for the full infinite
value.

### 5.4 Termination and non-termination

Call-by-need does not eliminate divergence: `let loop = loop in loop` diverges
regardless of strategy. What call-by-need buys is that a function that does
not need the value of an argument will not evaluate it:

```hope
dec const : alpha # beta -> alpha;
--- const(x, _) <= x;
```

`const(42, loop)` returns `42` under call-by-need even though `loop` diverges,
because the second component is matched by `_` (wildcard) and never forced.
Under call-by-value, it diverges.

This has consequences for formal reasoning: call-by-need programs can be
total on a strictly larger domain than their call-by-value equivalents.
A Hope function that pattern-matches only on its first argument and ignores
its second is total even when the second argument is undefined.

---

## 6. Equational Reasoning

### 6.1 Equations as axioms

Henderson (1986) [7] argues that a functional program *is* its own formal
specification: the equations that define a function are equational axioms
from which the function's behaviour follows by substitution and reduction.
There are no side effects to model, no memory addresses to track, no
sequencing to account for. The semantics is purely algebraic.

Hope equations in particular are well-suited to this view. The declaration:

```hope
--- reverse(nil)   <= nil;
--- reverse(a :: l) <= reverse(l) <> [a];
```

asserts two equations:

```
reverse nil = nil
reverse (a :: l) = reverse l <> [a]
```

These can be used directly as rewrite rules in a formal proof. The `<=` of
Hope corresponds to the `=` of equational logic: the left-hand side and
right-hand side denote the same value.

### 6.2 Proof by structural induction

The most common proof technique for Hope functions is *structural induction*,
which mirrors the structure of the `data` type being processed.

**Theorem**: `length (xs <> ys) = length xs + length ys`

*Proof by induction on `xs`.*

**Base case** (`xs = nil`):
```
length (nil <> ys)
= length ys                    (by def of <>: nil <> ys = ys)
= 0 + length ys                (arithmetic)
= length nil + length ys       (by def of length: length nil = 0)
```

**Inductive case** (`xs = a :: l`, inductive hypothesis: `length(l <> ys) = length l + length ys`):
```
length ((a :: l) <> ys)
= length (a :: (l <> ys))      (by def of <>)
= succ(length(l <> ys))        (by def of length)
= succ(length l + length ys)   (by inductive hypothesis)
= succ(length l) + length ys   (arithmetic)
= length(a :: l) + length ys   (by def of length)
```

Both cases hold; the theorem is proved. Notice that the proof uses *only*
the equations for `<>` and `length` — no operational semantics, no memory
model, no reduction strategy.

### 6.3 Proof by induction: reverse-reverse

A more substantial example. We prove `reverse(reverse xs) = xs`.

First, a lemma.

**Lemma**: `reverse(xs <> [a]) = a :: reverse xs`

*Proof by induction on `xs`:*

Base:
```
reverse(nil <> [a])
= reverse [a]                  (nil <> xs = xs)
= reverse(a :: nil)
= reverse(nil) <> [a]          (by def of reverse)
= nil <> [a]
= [a]                          (nil <> xs = xs)
= a :: nil
= a :: reverse nil             (length nil = 0, reverse nil = nil)
```

Inductive step (`xs = b :: l`, IH: `reverse(l <> [a]) = a :: reverse l`):
```
reverse((b :: l) <> [a])
= reverse(b :: (l <> [a]))     (def of <>)
= reverse(l <> [a]) <> [b]     (def of reverse)
= (a :: reverse l) <> [b]      (by IH)
= a :: (reverse l <> [b])      (def of <>)
= a :: reverse(b :: l)         (def of reverse, reversed)
```

**Theorem**: `reverse(reverse xs) = xs`

*Proof by induction on `xs`:*

Base: `reverse(reverse nil) = reverse nil = nil`. ✓

Inductive step (`xs = a :: l`, IH: `reverse(reverse l) = l`):
```
reverse(reverse(a :: l))
= reverse(reverse(l) <> [a])   (def of reverse)
= a :: reverse(reverse l)      (by Lemma)
= a :: l                       (by IH)
= a :: l                       ✓
```

### 6.4 Higher-order functions and free theorems

Parametrically polymorphic functions satisfy *free theorems* (Wadler 1989):
constraints on their behaviour that follow solely from their type, without
consulting the equations. A function of type `alpha -> alpha` *must* be
the identity: there is no other function that works uniformly for all types.
A function of type `list alpha -> list alpha` can only rearrange, duplicate,
or drop elements — it cannot conjure new ones.

In Hope, these constraints are enforced by the type system. The declaration:

```hope
dec id : alpha -> alpha;
```

together with the principal type theorem guarantees that any well-typed
definition of `id` must behave as the identity on every type. Free theorems
are *not* proved from the equations; they follow from parametricity of the
type system, which is a property of Hindley-Milner as a whole [6].

---

## 7. Abstract Types and Existential Quantification

Hope's `abstype` declaration introduces an *abstract type* — a type whose
representation is hidden from clients:

```hope
abstype counter;
dec new_counter : num -> counter;
dec increment   : counter -> counter;
dec value       : counter -> num;
```

The implementation is given in a `private` section of a module and is
invisible outside it. This corresponds to *existential quantification* in the
type system: the abstract type `counter` is `∃τ. (num → τ) × (τ → τ) × (τ → num)`.
A client knows the interface (the three functions) but not the witness type `τ`.

Perry (ch. 7) [8] develops the theory of existential quantification for Hope+C
in detail. The key result is that the existential package is *coherent*: no
matter which representation the implementation chooses, the observable
behaviour of the interface functions is the same. This is the formal basis for
*data abstraction* and *representation independence* in Hope.

---

## 8. Summary: Formal Properties of Hope

| Property                  | Formal basis                          | Consequence for Hope                                      |
|---------------------------|---------------------------------------|-----------------------------------------------------------|
| Type soundness            | Hindley-Milner                        | A well-typed program never applies a function to a value of the wrong type |
| Principal types           | Damas-Milner algorithm W              | Every expression has a most general type; no annotations needed on expressions |
| Parametricity             | Polymorphic λ calculus (Reynolds)     | Polymorphic functions satisfy free theorems               |
| Equational reasoning      | Algebraic semantics                   | Equations define values; equals can be substituted for equals |
| Structural induction      | Well-foundedness of data types        | Structurally recursive functions over finite data terminate |
| Totality (structural rec.)| Termination argument from data shape  | Proofs by induction are sound                             |
| Data abstraction          | Existential quantification            | Representation independence for `abstype`                  |
| Laziness (Paterson)       | Call-by-need / normal order reduction | More programs terminate; infinite structures are expressible |
| Coinduction (Paterson)    | Greatest fixed points                 | Properties of infinite structures can be proved           |

---

## References

[1] Rojas, Raúl. "A Tutorial Introduction to the Lambda Calculus." FU Berlin,
WS-97/98.

[2] Jung, Achim. "A Short Introduction to the Lambda Calculus." School of
Computer Science, University of Birmingham, March 2004.

[3] Bailey, Roger. "A Hope Tutorial." *Byte*, Vol. 10, No. 8, August 1985,
pp. 235–258. (Reproduced as `cxx/doc/hope_tut.md`.)

[4] Michaelson, Greg. *An Introduction to Functional Programming through
Lambda Calculus.* Addison-Wesley, 1989. (Chapter 5 covers Scott encodings and
algebraic data; Chapter 7 covers lazy evaluation and infinite structures.)

[5] Burstall, R.M., MacQueen, D.B., and Sannella, D.T. "HOPE: An Experimental
Applicative Language." *Proceedings of the 1980 LISP Conference*, Stanford,
pp. 136–143. (See `cxx/doc/primary-sources.md`.)

[6] Shan, Chung-chieh. "Functional Programming: Sexy Types in Action."
Harvard University, 2005. (Annotated bibliography on Hindley-Milner, System F,
and higher-rank polymorphism.)

[7] Henderson, Peter. "Functional Programming, Formal Specification, and Rapid
Prototyping." *IEEE Transactions on Software Engineering*, Vol. 12, No. 2,
February 1986, pp. 241–250.

[8] Perry, Nigel. *The Implementation of Practical Functional Programming
Languages.* PhD thesis, Imperial College of Science, Technology and Medicine,
University of London. (Chapter 7 covers existential quantification and
overloading; see `cxx/doc/primary-sources.md`.)

[9] Evans, Arthur Jr. "The Lambda Calculus and its Relation to Programming
Languages." MIT Lincoln Laboratory, *Communications of the ACM*, 1982,
p. 714.

[10] Paterson, Ross. *A Hope Interpreter — Reference.* Distributed with
source code, 1999. (Reproduced as `cxx/doc/ref_man.md`.)
