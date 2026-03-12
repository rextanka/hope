# Hope Examples

Adapted from Ross Paterson's *Some Hope Examples* (`doc/examples.src`).
Each section is a self-contained Hope program; the corresponding test
inputs live in `test/examples_*.in`.

---

## The Factorial Function

### Recursive version

```hope
dec fact : num -> num;
--- fact 0 <= 1;
--- fact n <= n * fact(n - 1);

fact 7;
```

```
>> 5040 : num
```

Hope uses best-fit pattern matching: the second clause is chosen only when
`n` is non-zero.  Swapping the clauses does not change the behaviour.

### List-based version

Using `product` from the `lists` module and the `..` range operator from
the `range` module:

```hope
uses lists, range;

dec factp : num -> num;
--- factp n <= product (1..n);

factp 7;
1..7;
product [1, 2, 3, 4, 5, 6, 7];
```

```
>> 5040 : num
>> [1, 2, 3, 4, 5, 6, 7] : list num
>> 5040 : num
```

---

## Fibonacci Numbers

### Infinite lazy list

A fast definition using circular self-reference via `whererec`.  `||`
is the zip function; the infinite list `fs` is defined in terms of
itself so that previously computed values are reused:

```hope
uses lists;

dec fibs : list num;
--- fibs <= fs whererec fs == 1::1::map (+) (tail fs || fs);

front(11, fibs);
```

```
>> [1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89] : list num
```

> The recursive version (`fib(n+2) <= fib n + fib(n+1)`) requires
> `n+k` patterns, which are not yet implemented.  See Known Limitations
> in [user-guide.md](user-guide.md).

---

## Breadth-First Tree Traversal

Rose trees (a tree is a label paired with a list of sub-trees) are
defined as a recursive type synonym:

```hope
type rose_tree alpha == alpha # list(rose_tree alpha);
```

The `.` operator from the `functions` module is reversed function
application: `x.f` means `f x`, so `x.f.g` means `g(f(x))`.  The
breadth-first algorithm:

1. Build an infinite list of levels (each level = children of the
   previous level).
2. Truncate before the first empty level (`front_with`).
3. Concatenate all levels (`concat`).
4. Extract the root label of each tree (`map fst`).

```hope
uses lists, functions, products;

type rose_tree alpha == alpha # list(rose_tree alpha);

dec bf_list : rose_tree alpha -> list alpha;
--- bf_list t <= [t].
        iterate (concat o map snd).
        front_with (/= []).
        concat.
        map fst;

bf_list (1, [(2, [(5, []),
                  (6, [(10, [])])
                 ]),
             (3, [(7, [])]),
             (4, [(8, [(11, [])]),
                  (9, [])
                 ])
            ]);
```

```
>> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11] : list num
```

---

## Symbolic Boolean Expressions

A complete example demonstrating:
- Algebraic data types with infix constructors
- Evaluating expressions against variable assignments
- Generating all possible assignments (truth tables)
- Testing for tautologies

### Type definitions

```hope
uses list;

type var == char;

infixrl IMPLIES : 1;
infix   OR : 2;
infix   AND : 3;

data bexp == VAR var ++ bexp AND bexp ++ bexp OR bexp ++
             NOT bexp ++ bexp IMPLIES bexp;

type assignment == list(var # truval);
```

> `infixrl` is a backward-compatible alias for `infixr`.

### Evaluation

```hope
dec lookup : alpha # list(alpha # beta) -> beta;
--- lookup(a, (key, datum)::rest) <=
        if a = key then datum else lookup(a, rest);

dec evaluate : bexp # assignment -> truval;
--- evaluate(VAR v, a) <= lookup(v, a);
--- evaluate(e1 AND e2, a) <= evaluate(e1, a) and evaluate(e2, a);
--- evaluate(e1 OR e2, a) <= evaluate(e1, a) or evaluate(e2, a);
--- evaluate(NOT e, a) <= not(evaluate(e, a));
--- evaluate(e1 IMPLIES e2, a) <=
        not(evaluate(e1, a)) or evaluate(e2, a);

evaluate (VAR 'a' AND VAR 'b' OR VAR 'c',
          [('a', false), ('b', true), ('c', false)]);
evaluate (VAR 'a' AND VAR 'b' OR VAR 'c',
          [('a', true), ('b', true), ('c', false)]);
```

```
>> false : bool
>> true : bool
```

### Variable extraction

```hope
dec merge : list alpha # list alpha -> list alpha;
--- merge([], l) <= l;
--- merge(l, []) <= l;
--- merge(x1::l1, x2::l2) <=
        if x1 = x2 then x1::merge(l1, l2)
        else if x1 < x2 then x1::merge(l1, x2::l2)
                        else x2::merge(x1::l1, l2);

dec vars : bexp -> list var;
--- vars(VAR v) <= [v];
--- vars(e1 AND e2) <= merge(vars e1, vars e2);
--- vars(e1 OR e2) <= merge(vars e1, vars e2);
--- vars(NOT e) <= vars e;
--- vars(e1 IMPLIES e2) <= merge(vars e1, vars e2);

vars (VAR 'a' AND VAR 'b' OR VAR 'a');
```

```
>> "ab" : list var
```

The K combinator axiom — `(a => b => c) => (a => b) => a => c` — is a
tautology; its variables are `a`, `b`, `c`:

```hope
vars ((VAR 'a' IMPLIES VAR 'b' IMPLIES VAR 'c') IMPLIES
        (VAR 'a' IMPLIES VAR 'b') IMPLIES
        VAR 'a' IMPLIES VAR 'c');
```

```
>> "abc" : list var
```

### Truth tables and tautology

`interpretations` generates all 2ⁿ assignments for n variables:

```hope
dec interpretations : list var -> list assignment;
--- interpretations [] <= [[]];
--- interpretations(h::t) <=
        (map ((h, false)::) l <> map ((h, true)::) l)
        where l == interpretations t;

interpretations "ab";
```

```
>> [[('a', false), ('b', false)], [('a', false), ('b', true)],
    [('a', true), ('b', false)], [('a', true), ('b', true)]] : list assignment
```

`dist(e, assignments)` pairs the expression with each assignment
(from `list.hop`).  `tautology` checks all assignments:

```hope
dec tautology : bexp -> truval;
--- tautology e <=
        foldr(true, (and))
                (map evaluate
                        (dist(e, interpretations(vars e))));

tautology (VAR 'a' AND VAR 'b' OR VAR 'a');
tautology ((VAR 'a' IMPLIES VAR 'b' IMPLIES VAR 'c') IMPLIES
                (VAR 'a' IMPLIES VAR 'b') IMPLIES
                VAR 'a' IMPLIES VAR 'c');
```

```
>> false : bool
>> true : bool
```

`a AND b OR a` is not a tautology (false when `a` is false).
The K axiom is.
