# Hope Language Reference

A concise reference for the Hope functional programming language as implemented
in this interpreter (Paterson's lazy Hope, C++20 rewrite). Intended as LLM
context for generating correct Hope programs.

---

## Quick example

```hope
! Sieve of Eratosthenes
uses list;

dec from : num -> list num;
--- from n <= n :: from(n + 1);

dec sieve : list num -> list num;
--- sieve(p :: xs) <= p :: sieve(filter(lambda n => not(n mod p = 0), xs));

dec primes : list num;
--- primes <= sieve(from 2);

front(20, primes);
```

---

## Lexical basics

- Comments: `!` to end of line. No block comments.
- Identifiers: start with a letter or `_`, continue with letters, digits, `_`, `'`.
  - Lowercase: variables and (optionally) constructors.
  - Uppercase: type constructors and constructors by convention (not enforced).
- Operators: sequences of graphic characters (`+ - * / < > = | & @ ^ ~ . : ?` etc.)
  Operators may be used as identifiers when parenthesised: `(+)`, `(::)`.
- String literals: `"hello"` — type `list char`.
- Character literals: `'a'` — type `char`.
- Numeric literals: integer or real (`3.14`), negative via unary `-`.
- Semicolons `;` terminate top-level declarations and expressions.

---

## Reserved words

```
abstype  data     dec      display  edit     else     exit
if       in       infix    infixr   lambda   let      letrec
private  save     then     to       type     typevar  uses
where    whererec write    \
```

Also reserved: `---` (equation marker), `<=` (equation separator), `++`
(constructor separator in `data`), `==` (definition marker in `data`/`type`).

---

## Types

### Built-in types

| Type          | Meaning                                  |
|---------------|------------------------------------------|
| `num`         | Arbitrary-precision integer (also reals) |
| `bool`        | `false` or `true`                        |
| `char`        | A character (abstract, ordered as ASCII) |
| `list alpha`  | Polymorphic list                         |
| `alpha # beta`| Pair (product type), right-associative   |
| `alpha -> beta`| Function type, right-associative        |

### Type declarations

```hope
typevar alpha;               ! introduce a type variable
typevar alpha, beta;         ! multiple at once

type string == list char;    ! type synonym (abbreviation)

data colour == red ++ green ++ blue;       ! sum type, nullary constructors
data tree alpha == leaf ++ node(tree alpha # num # tree alpha);  ! recursive

abstype queue alpha;         ! abstract type (implementation hidden in module)
```

`data` constructor syntax: `Name ++ Name(arg_type) ++ ...`
`#` is right-associative: `a # b # c` = `a # (b # c)`.

### Type variables in `dec`

`typevar` declarations bring type variables into scope for subsequent `dec`
declarations:

```hope
typevar alpha, beta;
dec map : (alpha -> beta) -> list alpha -> list beta;
```

---

## Declarations

### Function declarations

```hope
dec f : tau;          ! declare type of f (required before equations)
--- f(pattern) <= expr;    ! first equation
--- f(pattern) <= expr;    ! further equations, tried top-to-bottom
```

`dec` and equations may appear in any order, but `dec` must precede use.

### Multiple arguments: tupled vs. curried

```hope
dec add : num # num -> num;        ! tupled (single pair argument)
--- add(x, y) <= x + y;
add(3, 4);                         ! call: 7

dec add2 : num -> num -> num;      ! curried (two arguments)
--- add2 x y <= x + y;
add2 3 4;                          ! call: 7
```

Curried application is left-associative: `f x y` = `(f x) y`.

### Operator declarations

```hope
infix + : 5;           ! left-associative, precedence 5
infixr :: : 5;         ! right-associative, precedence 5
dec + : num # num -> num;
```

Higher precedence binds tighter. Precedence 1 is loosest, 9 is tightest.

---

## Patterns

Patterns appear on the LHS of `---` equations and in `lambda`.

| Pattern         | Matches                                | Binds       |
|-----------------|----------------------------------------|-------------|
| `x`             | Anything (variable)                    | `x`         |
| `_`             | Anything (wildcard)                    | nothing     |
| `C`             | Nullary constructor `C`                | —           |
| `C(p)`          | Constructor `C` applied to `p`         | from `p`    |
| `p1, p2`        | Pair `(p1, p2)`                        | from both   |
| `[]`            | Empty list                             | —           |
| `p :: ps`       | Non-empty list                         | head, tail  |
| `[p1, p2, ...]` | List of exact length                   | elements    |
| `0`             | Zero                                   | —           |
| `n+k`           | Number ≥ k, binds `n = value - k`      | `n`         |
| `'a'`           | Character literal                      | —           |
| `"str"`         | String (= `list char`) literal         | —           |
| `~p`            | Irrefutable pattern (always matches, lazy) | from `p` |

**Lowercase constructor names** must be declared via `data` before they can be
used as patterns. Without a `data` declaration, a lowercase name in pattern
position is treated as a variable.

---

## Expressions

### Application

```hope
f x          ! f applied to x (curried)
f(x)         ! same
f(x, y)      ! f applied to pair (x, y)
f x y        ! (f x) y — left-associative curried application
```

### Lambda

```hope
lambda x => expr
\ x => expr              ! backslash alias

lambda x => lambda y => expr   ! curried lambda
\ x => \ y => expr

! Multi-clause lambda (pattern matching):
\ 0 => "zero" | succ n => "positive"
```

### Conditional

```hope
if condition then expr1 else expr2
```

### Let bindings

```hope
let x = expr1 in expr2             ! non-recursive binding
letrec x = expr1 in expr2          ! recursive binding

expr where x = expr1               ! where clause (non-recursive)
expr whererec f x = expr1          ! recursive where
```

### Operator sections

```hope
(+ 1)       ! lambda x => x + 1
(1 +)       ! lambda x => 1 + x
(mod 2)     ! lambda x => x mod 2
```

### List notation

```hope
[]                    ! empty list (= nil)
[1, 2, 3]             ! list literal
x :: xs               ! cons
xs <> ys              ! append (from Standard)
```

### Tuple notation

```hope
(a, b)        ! pair of type alpha # beta
(a, b, c)     ! triple = (a, (b, c)) — right-nested
```

### Case (via lambda)

Multi-clause `lambda` serves as case expression:

```hope
(\ true => 1 | false => 0) b     ! case on bool
```

---

## Evaluation model

This interpreter uses **call-by-need** (lazy) evaluation:

- Arguments are not evaluated before function application.
- Values are computed at most once (shared thunks).
- Constructor arguments are lazy: `x :: xs` does not force `xs`.
- **Infinite lists are allowed** and commonly used with `from`, `iterate`, etc.
- `print` and `write_element` are strict built-ins that force evaluation.

**What this means in practice:**

```hope
dec ones : list num;
--- ones <= 1 :: ones;            ! infinite list — OK under call-by-need

front(5, ones);                   ! [1, 1, 1, 1, 1]
```

Pattern matching forces evaluation of the matched expression to head-normal
form (enough to determine which constructor applies).

---

## Standard library (always available)

No `uses` directive needed for these:

### Bool

```hope
not  : bool -> bool
and  : bool # bool -> bool    (infix)
or   : bool # bool -> bool    (infix)
```

### Comparison (polymorphic)

```hope
=, /=  : alpha # alpha -> bool    (infix, prec 3)
<, =<, >, >=  : alpha # alpha -> bool    (infix, prec 4)
compare : alpha # alpha -> relation

data relation == LESS ++ EQUAL ++ GREATER;
```

### Arithmetic

```hope
+, -, *, /  : num # num -> num    (infix)
div, mod    : num # num -> num    (infix; integer division/remainder)
abs, ceil, floor, sqrt, exp, log  : num -> num
sin, cos, tan, asin, acos, atan   : num -> num
atan2, hypot : num # num -> num
pow  : num # num -> num
succ : num -> num    (= n+1)
```

### Characters and strings

```hope
ord  : char -> num
chr  : num -> char
num2str : num -> list char
str2num : list char -> num
```

### Lists (built-in, no `uses` needed)

```hope
<>   : list alpha # list alpha -> list alpha    (append, infix prec 5)
::   : alpha # list alpha -> list alpha         (cons, infix prec 5, right-assoc)
```

### I/O

```hope
write expr;              ! print value lazily (streaming)
write expr to "file";    ! write to file
input                    ! lazy list char from stdin
argv                     ! list(list char) — command-line arguments
error : list char -> alpha    ! abort with message
```

---

## Standard library modules (`uses`)

Load with `uses ModuleName;` before using the functions.

### `uses list;`

```hope
reverse  : list alpha -> list alpha
length   : list alpha -> num
front    : num # list alpha -> list alpha    ! take first n elements
after    : num # list alpha -> list alpha    ! drop first n elements
map      : (alpha -> beta) -> list alpha -> list beta
filter   : (alpha -> bool) -> list alpha -> list alpha
foldr    : beta # (alpha # beta -> beta) -> list alpha -> beta
foldl    : beta # (beta # alpha -> beta) -> list alpha -> beta
iterate  : (alpha -> alpha) -> alpha -> list alpha
partition: (alpha -> bool) -> list alpha -> list alpha # list alpha
span     : (alpha -> bool) -> list alpha -> list alpha # list alpha
front_with, after_with : (alpha -> bool) -> list alpha -> list alpha
||       : list alpha # list beta -> list(alpha # beta)    (zip, infix)
@        : list alpha # num -> alpha                       (index, infix)
dist     : alpha # list beta -> list(alpha # beta)
shunt    : list alpha # list alpha -> list alpha           (reverse-append)
```

Note: `map` and `filter` are **curried**: `map f xs`, `filter pred xs`.

### `uses arith;`

```hope
! Arithmetic sequences and folds over numbers
sum, product : list num -> num
maximum, minimum : list num -> num
```

### `uses sort;`

```hope
sort     : list num -> list num
mergesort, insertsort, treesort : list num -> list num
```

### `uses maybe;`

```hope
data maybe alpha == nothing ++ just(alpha);
```

### `uses set;`

```hope
! Sets as sorted lists
```

### `uses lines;` / `uses words;`

```hope
lines : list char -> list(list char)    ! split on newlines
words : list char -> list(list char)    ! split on whitespace
```

### `uses ctype;`

```hope
isalpha, isdigit, isspace, isupper, islower : char -> bool
toupper, tolower : char -> char
```

---

## Common patterns

### List comprehension (manual)

Hope has no `[x | x <- xs, pred x]` syntax. Use `map` and `filter`:

```hope
map f (filter pred xs)
```

### Accumulator pattern

```hope
dec sum_acc : num # list num -> num;
--- sum_acc(acc, [])     <= acc;
--- sum_acc(acc, x::xs)  <= sum_acc(acc + x, xs);

dec sum : list num -> num;
--- sum xs <= sum_acc(0, xs);
```

### Infinite list + `front`

```hope
uses list;
dec from : num -> list num;
--- from n <= n :: from(n + 1);

front(10, from 1);    ! [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
```

### Higher-order with curried `map`/`filter`

```hope
uses list;
map (+ 1) [1, 2, 3];          ! [2, 3, 4]
filter (> 2) [1, 2, 3, 4];    ! [3, 4]
```

---

## Key differences from Haskell / ML

| Haskell / ML                    | Hope equivalent                          |
|---------------------------------|------------------------------------------|
| `data T = A \| B Int`           | `data T == A ++ B(num)`                  |
| `f x = expr`                    | `--- f x <= expr;`                       |
| Type annotation `f :: a -> b`   | `dec f : alpha -> beta;`                 |
| Type variable (implicit)        | `typevar alpha;` (must be declared)      |
| `take n xs`                     | `front(n, xs)` (requires `uses list`)    |
| `xs ++ ys`                      | `xs <> ys`                               |
| `map f xs`                      | `map f xs` (curried, requires `uses list`) |
| `filter p xs`                   | `filter p xs` (curried, requires `uses list`) |
| `show x` / `print x`           | `write x;`                               |
| `case x of { A -> e1; B n -> e2 }` | Multi-clause `---` equations or `\ A => e1 \| B n => e2` lambda |
| `let x = e1 in e2`             | `let x = e1 in e2` (same)               |
| `where`                         | `expr where x = e` (same keyword)       |
| `mod`                           | `mod` (same; also `div`)                |
| `True` / `False`               | `true` / `false` (lowercase)            |
| `[]` / `:`                      | `[]` / `::` (double colon)              |
| `(,)` pair constructor          | `(,)` — type written `alpha # beta`     |
| `Integer` / `Int`               | `num`                                    |
| `String`                        | `list char`                              |
| `\x -> e`                       | `lambda x => e` or `\ x => e`           |
| Guards `f x \| p = e`           | Separate equations with `if/then/else`  |

---

## Gotchas

1. **`typevar` is required.** Type variables used in `dec` must be declared with
   `typevar` first, or the name is treated as a type constructor (causing an
   error). In many programs you need `typevar alpha;` at the top.

2. **`dec` precedes equations.** You must write `dec f : tau;` before writing
   `--- f ...` equations. Both must appear before any call to `f`.

3. **Lowercase constructors.** Hope allows lowercase constructor names in `data`
   declarations (e.g. `data colour == red ++ green ++ blue`). The parser
   recognises these as constructors only after the `data` declaration has been
   processed. In a multi-declaration program, all `data` declarations should
   come before equations that pattern-match on their constructors.

4. **No multi-parameter type constructors in `dec`.** Writing
   `dec f : either alpha beta -> bool` will parse `alpha beta` as a single type
   application rather than two arguments to `either`. Workaround: use a `type`
   synonym or parenthesise: `dec f : (either alpha beta) -> bool` is also
   ambiguous. Best avoided; use type synonyms instead.

5. **`uses` is positional.** Functions from a module are only available after
   the `uses ModuleName;` directive. Put `uses` declarations at the top of your
   program.

6. **`=` is infix.** `x = y` is a boolean comparison (not assignment). There is
   no assignment in Hope.

7. **Operator precedence.** The precedence table (lowest to highest):
   `or`(1) `and`(2) `=,/=`(3) `<,=<,>,>=`(4) `+,-,<>,::` (5) `*,/,div,mod`(6).
   Function application binds tighter than all operators.

8. **`/` is real division; use `div` for integer division.**
   `7 / 2 = 3.5`; `7 div 2 = 3`.
