# Chapter 1 Exercises — Bailey's *Functional Programming with Hope*

Source: Roger Bailey, *Functional Programming with Hope*, Ellis Horwood, 1990.
Exercises from pp. 18–19; solutions from Appendix 1 (pp. 236–241).

The test file `test/ch1_exercises.in` covers the evaluatable subset (Exercises 1–7).
Exercises 8 and 9 require written analysis and appear in this document only.

---

## Exercises

### 1.15 Exercises

**1.** Evaluate the following Hope expressions (see Appendix 4 for the ordinal values of characters):

```
(a)  2 - 3 - 4
(b)  2 - (3 - 4)
(c)  100 div 4 div 5
(d)  100 div (4 div 5)
(e)  (3 + 4 - 5) = (3 - 5 + 4)
(f)  chr (ord 'a' + 1)
(g)  2147483647 + 1
(h)  2 * (2147483647 + 1)
```

**2.** Give Hope expressions which correspond as closely as possible to the following
mathematical expressions:

```
(a) -100/3        (b) 2/3
(c) 3/(4+1)       (d) (3/4) + 1
```

**3.** Write Hope expressions to find the values of the following:

```
(a) The last digit of the number 123
(b) The penultimate digit of the number 456
(c) The eighth lower-case letter of the alphabet
(d) The "middle" upper-case letter of the alphabet
(e) The minimum number of 13-gallon barrels needed to hold the entire
    contents of an 823-gallon vat
```

**4.** Write a Hope expression to calculate the distance covered by a rocket in
24 hours if its initial speed is 1000 km/h and it accelerates at 50 km/h².
The distance covered in time *t* by a body moving in a straight line with
initial velocity *v* and acceleration *a* is given by *tv* + *t²a*/2.

**5.** Write a Hope expression to calculate the temperature in °C corresponding to
98°F. The conversion rule is to subtract 32 from the temperature in °F and
then multiply the result by 5/9.

**6.** Write down expressions which evaluate to `true` if:

```
(a) 100 is exactly divisible by 10
(b) 22 is even
(c) The penultimate digit of 139 is different from the last digit of 55
(d) The letter 'z' is the 26th lower-case letter of the alphabet
(d) The letter 'c' is either the 1st or the 3rd lower-case letter of the alphabet
(e) The letter 'b' is neither the 1st nor the 3rd lower-case letter of the alphabet
(f) As for (e) but this time do not use the not operation
(g) 100 lies between 50 and 200
(h) 10 does not lie between 50 and 200
(i) As for (h) but do not use the not operation
```

Note: the book has a duplicate `(d)` label; items are lettered (a)–(j) in the
solutions.

**7.** State the types of the following expressions where they are correct, and
identify those whose types are incorrect:

```
(a) 2 * 3 = 6
(b) 6 = 2 * 3
(c) 2 + 3 = 5
(d) not 2 * 3 = 10
(e) 3 = 4 = true
(f) true = 3 = 4
(g) if 3 = 4 then ord 'a' else ord 'b'
(h) ord if 3 = 4 then 'a' else 'b' + 1
```

**8.** Assuming that the expressions with incorrect types in Exercise 7 were a
result of the programmer accidentally omitting parentheses, suggest where they
might be added to make the types of the expressions correct.

**9.** Write down the possible sequences of reductions which may occur when the
following expressions are evaluated:

```
(a) if 0 /= 0 then 2 + 1 else 2 - 1
(b) 3 + 4 + 5 + 6
(c) ord 'x' + ord 'y'
(d) (3 - 4) = (4 - 5)
(e) ord 'a' = ord 'b' - 1
```

---

## Solutions

### Preliminary note: Bailey's Hope vs Paterson's Hope

| Property | Bailey's Hope Machine | Paterson's Hope (this interpreter) |
|---|---|---|
| `num` type | Machine integer (32-bit) | Floating-point double |
| Boolean type name | `truval` | `bool` |
| Operator precedence | `=` and `*` share level 6 (left-assoc) | `=` is looser than `*` and `+` |

The precedence difference affects Exercise 7 and is documented there.

---

### Exercise 1

**(a)** `2 - 3 - 4`

Left-associative subtraction: `(2 - 3) - 4` = `(-1) - 4` = **`-5 : num`**.

**(b)** `2 - (3 - 4)`

Explicit right grouping: `2 - (-1)` = **`3 : num`**.

**(c)** `100 div 4 div 5`

Left-associative integer division: `(100 div 4) div 5` = `25 div 5` = **`5 : num`**.

**(d)** `100 div (4 div 5)`

`4 div 5` = `0` (integer division truncates), then `100 div 0` → **error** (division by zero).
This result occurs because the parenthesised term evaluates to 0.
*(Omitted from `ch1_exercises.in` as it terminates evaluation with an error.)*

**(e)** `(3 + 4 - 5) = (3 - 5 + 4)`

LHS: `(7 - 5)` = `2`. RHS: `(-2 + 4)` = `2`. `2 = 2` → **`true : bool`**.
(Bailey's type name is `truval`; Paterson uses `bool`.)

**(f)** `chr (ord 'a' + 1)`

`ord 'a'` = 97. `97 + 1` = 98. `chr 98` = **`'b' : char`**.

**(g)** `2147483647 + 1`

**Implementation-dependent.** 2147483647 is the maximum value of a signed 32-bit integer.

- Bailey's 32-bit integer Hope: overflow wraps to **`-2147483647 : num`** (two's complement).
- Paterson's floating-point Hope: **`2147483648 : num`** (no overflow).

The test file records Paterson's result. To adapt the test for a 32-bit integer implementation,
the expression `2 * (0 - 1073741824)` gives -2147483648, and subtracting 1 gives -2147483649,
which wraps to the same machine-dependent value.

**(h)** `2 * (2147483647 + 1)`

**Implementation-dependent.**

- Bailey's 32-bit integer Hope: `2 * (-2147483648)` overflows to **`0 : num`**.
- Paterson's floating-point Hope: **`4294967296 : num`**.

---

### Exercise 2

Bailey's Hope has no unary minus operator; negative number literals are not supported.
Negative values must be expressed as `(0 - n)`.

| Mathematical | Hope expression | Value |
|---|---|---|
| (a) −100/3 | `(0 - 100) div 3` | `-34` (floor division) |
| (b) 2/3 | `2 div 3` | `0` |
| (c) 3/(4+1) | `3 div (4 + 1)` | `0` |
| (d) (3/4) + 1 | `3 div 4 + 1` | `1` |

Note: `div` in Paterson's interpreter performs floor division for negative operands
(consistent with Hope's integer semantics). `-100 div 3` = -34 rather than -33.

---

### Exercise 3

**(a)** Last digit of 123: `123 mod 10` → **`3`**.

**(b)** Penultimate digit of 456: `456 div 10 mod 10` → `45 mod 10` → **`5`**.

**(c)** Eighth lower-case letter: `chr (ord 'a' + 7)` → `chr (97 + 7)` = `chr 104` → **`'h'`**.
(The eighth letter is h: a, b, c, d, e, f, g, **h**.)

**(d)** Middle upper-case letter: Bailey's Appendix 1 (p. 237) prints:

```
chr ((ord 'Z' - ord 'A' + 1) div 2)
```

This contains a typographical error. Evaluated: `(90 - 65 + 1) div 2` = `26 div 2` = `13`.
`chr 13` is a carriage-return control character, not a letter.

The correct expression is:

```
chr (ord 'A' + (ord 'Z' - ord 'A') div 2)
```

Evaluated: `65 + (90 - 65) div 2` = `65 + 25 div 2` = `65 + 12` = `77` → **`'M'`**.
('M' is the 13th letter, i.e., the middle of the 26-letter alphabet.)

**(e)** Minimum 13-gallon barrels for 823 gallons: `823 div 13 + 1`.

`823 div 13` = 63 (remainder 4). Since 63 × 13 = 819 < 823, one more barrel is needed.
Result: **`64`**. (If 823 were exactly divisible by 13 this would over-count by one;
the exercise relies on the known remainder being non-zero.)

---

### Exercise 4

Distance = *tv* + *t²a*/2 with *t* = 24 h, *v* = 1000 km/h, *a* = 50 km/h²:

```hope
24 * 1000 + 24 * 24 * 50 div 2
```

`*` and `div` are left-associative and bind tighter than `+`:
`(24 * 1000) + ((24 * 24 * 50) div 2)` = `24000 + (28800 div 2)` = `24000 + 14400` = **`38400`**.

---

### Exercise 5

Celsius = (°F − 32) × 5/9. For 98°F:

```hope
(98 - 32) * 5 div 9
```

`66 * 5 div 9` = `330 div 9` = **`36`** (integer division; exact is 36.67°C).

---

### Exercise 6

All ten expressions evaluate to `true`.

| Item | Expression | Trace |
|---|---|---|
| (a) | `100 mod 10 = 0` | `0 = 0` → true |
| (b) | `22 mod 2 = 0` | `0 = 0` → true |
| (c) | `139 div 10 mod 10 /= 55 mod 10` | `3 /= 5` → true |
| (d) | `ord 'z' = ord 'a' + 25` | `122 = 97 + 25` = `122 = 122` → true |
| (e) | `ord 'c' = ord 'a' or ord 'c' = ord 'a' + 2` | `false or true` → true |
| (f) | `not (ord 'b' = ord 'a' or ord 'b' = ord 'a' + 2)` | `not (false or false)` → true |
| (g) | `ord 'b' /= ord 'a' and ord 'b' /= ord 'a' + 2` | `true and true` → true |
| (h) | `50 =< 100 and 100 =< 200` | `true and true` → true |
| (i) | `not (50 =< 10 and 10 =< 200)` | `not (false and true)` = `not false` → true |
| (j) | `10 < 50 or 200 < 10` | `true or false` → true |

Note: the book uses `=<` for less-than-or-equal (≤), since `<=` is reserved for
function-defining equations.

---

### Exercise 7

Bailey's analysis assumes `=` and `*` share precedence level 6 with left-associativity,
so `A = B * C` parses as `(A = B) * C`.

Paterson's interpreter gives `=` lower precedence than `*` and `+` (standard mathematical
convention), so `A = B * C` parses as `A = (B * C)`.

| Item | Expression | Bailey | Paterson |
|---|---|---|---|
| (a) | `2 * 3 = 6` | `truval`, value `true` — `(2*3)=6` | `bool`, value `true` |
| (b) | `6 = 2 * 3` | **type error** — `(6=2)*3` mixes `truval` and `num` | `bool`, value `true` — `6=(2*3)` |
| (c) | `2 + 3 = 5` | **type error** — `2+(3=5)` adds `num` and `truval` | `bool`, value `true` — `(2+3)=5` |
| (d) | `not 2 * 3 = 10` | **type error** — `(not 2)` applies `not` to `num` | **type error** — same reason |
| (e) | `3 = 4 = true` | `truval`, value `false` — `(3=4)=true` = `false=true` | `bool`, value `false` |
| (f) | `true = 3 = 4` | **type error** — `(true=3)` compares `truval` with `num` | **type error** — same reason |
| (g) | `if 3 = 4 then ord 'a' else ord 'b'` | `num`, value `98` | `num`, value `98` |
| (h) | `ord if 3 = 4 then 'a' else 'b' + 1` | **type error** — `else 'b' + 1` adds `char` and `num` | **parse error** — `ord` requires a parenthesised argument |

The test file includes (a), (b), (c), (e), (f) (parenthesised), and (g).
Items (d), (f) (unparenthesised), and (h) are type or parse errors in both interpreters
and are omitted from the test file.

---

### Exercise 8

Parenthesised corrections for the incorrectly typed expressions from Exercise 7.
(In Bailey's precedence model; all are also well-typed in Paterson's interpreter.)

| Original (type error in Bailey) | Corrected | Value |
|---|---|---|
| `6 = 2 * 3` | `6 = (2 * 3)` | `true` |
| `2 + 3 = 5` | `(2 + 3) = 5` | `true` |
| `not 2 * 3 = 10` | `not (2 * 3 = 10)` | `true` |
| `true = 3 = 4` | `true = (3 = 4)` | `false` |
| `ord if 3 = 4 then 'a' else 'b' + 1` | `ord (if 3 = 4 then 'a' else 'b') + 1` | `99` |

---

### Exercise 9

Possible reduction sequences for each expression. Hope's lazy evaluator may reduce
subexpressions in different orders; all orders that terminate give the same final value.

**(a)** `if 0 /= 0 then 2 + 1 else 2 - 1`

```
if 0 /= 0 then 2 + 1 else 2 - 1
  → if false then 2 + 1 else 2 - 1
  → 2 - 1
  → 1
```

**(b)** `3 + 4 + 5 + 6`

Left-associative: `((3 + 4) + 5) + 6`

```
((3 + 4) + 5) + 6
  → (7 + 5) + 6
  → 12 + 6
  → 18
```

**(c)** `ord 'x' + ord 'y'`

```
ord 'x' + ord 'y'
  → 120 + ord 'y'
  → 120 + 121
  → 241
```

**(d)** `(3 - 4) = (4 - 5)`

Both sides evaluate to −1:

```
(3 - 4) = (4 - 5)
  → (-1) = (4 - 5)       (or the right branch first)
  → (-1) = (-1)
  → true
```

**(e)** `ord 'a' = ord 'b' - 1`

In Paterson's interpreter, `=` is looser than `-`, so this parses as
`ord 'a' = (ord 'b' - 1)`:

```
ord 'a' = (ord 'b' - 1)
  → 97 = (ord 'b' - 1)
  → 97 = (98 - 1)
  → 97 = 97
  → true
```
