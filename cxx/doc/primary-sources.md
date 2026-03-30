# Primary Sources for Hope

This document is an annotated bibliography of the primary sources for the Hope
functional programming language. It supersedes `doc/hope.bib`, which contained
four BibTeX entries; all four are preserved and extended here with additional
sources discovered during the C++20 rewrite project.

---

## The precursor: recursion equations

**Burstall, R.M. and Darlington, J.**
"A Transformation System for Developing Recursive Programs."
*Journal of the ACM*, Vol. 24, No. 1, January 1977, pp. 44–67.

Rod Burstall and John Darlington's 1977 JACM paper introduced the style of
defining functions by *recursion equations* with *pattern matching* on the
left-hand side — the direct precursor to Hope's syntax. The paper is not about
Hope (which did not yet exist) but about a program-transformation system that
rewrites lucid, recursive definitions into more efficient ones. It established
the formal notation that Burstall, MacQueen, and Sannella later adopted for
Hope.

---

## The founding paper

**Burstall, R.M., MacQueen, D.B., and Sannella, D.T.**
"HOPE: An Experimental Applicative Language."
*Proceedings of the 1980 LISP Conference*, Stanford, August 1980, pp. 136–143.
Also published as Technical Report CSR-62-80, Department of Computer Science,
University of Edinburgh.

*The* founding document for Hope. Eight pages covering the full language
design: algebraic data types with `data`, polymorphic type variables with
`typevar`, multi-clause function definitions with `---`/`<=`, lazy evaluation
for `list`, modules (`module`/`subtype`/`subconst`), and the rationale for
each design choice. The paper includes a complete worked example — an
`ordered_trees` module with `insert`, `flatten`, and `tree_sort` — that is
reproduced (adapted to Paterson's syntax) in `test/burstall1980.in`.

Note: the author's name is "Sannella" (two n's); `doc/hope.bib` has a typo
("Sanella").

---

## Popularisation: the Byte tutorial

**Bailey, Roger.**
"A Hope Tutorial."
*Byte*, Vol. 10, No. 8, August 1985, pp. 235–258.

Roger Bailey's eight-page *Byte* article introduced Hope to a broad audience
of practising programmers. It is the source of the `hope_tut.in` / `hope_tut.out`
regression test that ships with Paterson's C interpreter, and remains the most
accessible introduction to the language. The article is reproduced in the repo
as `doc/hope_tut.src` (LaTeX) and as `cxx/doc/hope_tut.md` (Markdown
conversion).

---

## Hope in the context of the Alvey programme

**Dettmer, Roger.**
"A Declaration of Hope: The Promise of Functional Programming."
*Electronics & Power*, Vol. 31, November/December 1985, pp. 819–823.

A five-page article in the IEE's practitioner magazine, aimed at hardware and
systems engineers rather than theoreticians. Dettmer situates Hope within the
UK Alvey programme's Flagship project, which sought to exploit functional
languages on parallel multiprocessor architectures. The article argues that the
absence of side effects makes functional programs inherently parallel —
subexpressions can be evaluated on separate processors without coordination —
and that Hope's strong type system makes this safe. It includes a `factorial`
example and a brief description of how Hope programs could be distributed across
the ICL Flagship machine.

The Flagship connection explains why Paterson's lazy extension of Hope
(developed at the University of Queensland from 1988) was of academic interest
beyond Australia: lazy evaluation is directly relevant to demand-driven parallel
evaluation strategies.

---

**Perry, Nigel.**
*The Implementation of Practical Functional Programming Languages.*
PhD thesis, Imperial College of Science, Technology and Medicine, University of
London. (Second edition distributed with the Hope* source.)

Perry's doctoral thesis is the primary implementation research document from the
Alvey Flagship Hope project. Its language is Hope+C (also written Hope*C), a
minor syntactic extension of Hope with an explicit I/O interface. The thesis
makes four contributions:

**I/O via result continuations (ch. 3).** Perry surveys all contemporary I/O
schemes for functional languages — lazy streams (Stoye), message streams,
Thompson/Dwelly combinator I/O, Karlsson continuation streams, and early
Haskell monadic I/O — and argues each has "accidental breakages" that
compromise referential transparency or composability. His solution, *result
continuations*, arranges for all I/O to be performed by the OS *outside* the
functional domain. This survey is the best available contemporary account of
why different I/O designs were tried and rejected, and directly illuminates why
Paterson chose the simpler `write`/`input` stream approach rather than adopting
any of the continuation or monadic schemes.

**The Fpm2 abstract machine (ch. 4).** Perry describes a compiled implementation
of Hope, targeting the Fpm/AM register machine (not a Krivine-style tree
walker). Section 4.4.5 covers value representation; section 4.5 gives
performance benchmarks for nfib, queens, Tak/Takl, Triangle, and FFT compared
to equivalent C programs. The Fpm2 path is the compiled-code route Imperial
College was pursuing for the Flagship hardware; Paterson's Krivine interpreter
was the alternative correctness-first approach developed independently at
Queensland.

**An extended type system with overloading (ch. 7).** Perry introduces
user-defined identifier overloading (the same symbol usable for integer and
matrix multiplication) and existential quantification for abstract data types.
His type-checking algorithm (ch. 8, "Upscan/Downscan") handles both
polymorphism and overloading resolution simultaneously. The existential
quantification material is directly relevant to the semantics of Hope's
`abstype` declarations; section 7.4 gives the Hope+C formulation.

**Inter-language working (ch. 5).** A functional/imperative foreign-function
interface allowing Hope+C programs to call C and Fortran, preserving
referential transparency at the language boundary. Not relevant to Paterson's
interpreter.

Appendix 1 lists the Hope+C continuation primitives. Appendix 2 gives complete
Hope+C source for several I/O programs. Appendix 3 lists all Fpm/AM opcodes and
built-in functions. Appendix 4 contains the benchmark programs in full.

---

## Hope as primary FP teaching language

**Glaser, Hugh, Hankin, Chris, and Till, David.**
*Principles of Functional Programming.*
Prentice Hall International, 1984.

Chapter 8.3, "HOPE", is a self-contained introduction to the language within a
comparative survey of functional languages (the chapter also covers KRC, SUGAR,
LISP, and Miranda). It covers HOPE's type system, data declarations, algebraic
data types, parametric polymorphism, lazy evaluation (`from`, `lcons`), and
abstract types (`stack`). Section 8.4 presents a larger worked example: a
combinator-graph reduction machine implemented in Hope.

Selected examples from chapter 8.3 are reproduced in `test/glaser1984.in`.

---

**Field, Anthony J. and Harrison, Peter G.**
*Functional Programming.*
Addison-Wesley Publishing Company, Wokingham, England, 1988.
Series: International Computer Science Series.
ISBN 0-201-19249-7.
Library of Congress: QA76.6.F477 1988 — 005.1′1 (CIP 88-1265).
Authors: Field, Anthony J., 1960–; Harrison, Peter G., 1951–.
595 pages. First printed 1988.

A comprehensive textbook from Imperial College, University of London.
**Chapter 2, "An Introduction to Functional Programming through Hope",** uses
Hope as the primary vehicle for introducing functional programming concepts to
students new to the paradigm — covering functions, tuples, recursion, infix
operators, qualified expressions, and user-defined data types. Hope is used
throughout Part I (Programming with Functions) and Part II (Implementation),
where Chapter 8 ("Intermediate Forms") shows how to translate Hope into
intermediate code and compile pattern matching. **Appendix A** is a complete
Hope language summary: BNF grammar (A.1) and predefined types and functions
(A.2).

The decision by two Imperial College researchers to adopt Hope — rather than
Miranda, ML, or a custom notation — as their primary teaching language in 1988
reflects Hope's status as a clean, well-specified reference language for
functional programming pedagogy. This is the book cited in `doc/hope.bib` as
`field&harrison`.

This book is out of print and has not yet been obtained for this project.
Its Appendix A would provide validation material for the C++20 reimplementation
independent of Bailey's Appendices.

---

## The full book

**Bailey, Roger.**
*Functional Programming with Hope.*
Ellis Horwood Limited (a division of Simon & Schuster International Group),
Market Cross House, Cooper Street, Chichester, West Sussex, England, 1990.
Series: Ellis Horwood Series in Computers and Their Applications.
ISBN 0-13-338237-0.
Library of Congress: QA76.73.H65B35 1990 — 005.13′3—dc20 (CIP 89-24920).

> Note: The ISBN prefix 0-13 is Prentice-Hall's; Ellis Horwood distributed
> through the Simon & Schuster / Prentice-Hall network, so the book appears
> in some library catalogues under "Prentice-Hall" as publisher.

The only book-length treatment of Hope, written by the person who developed
it as a teaching language at Imperial College London. 234 pages of text plus
three appendices. Contents:

| Chapter / Appendix | Topic |
|---|---|
| 1–2 | Basic types, function definitions |
| 3 | Lists and standard list operations (including `digits`, `<>`) |
| 4 | Data type definitions and trees |
| 5 | Polymorphic type system |
| 6–7 | Higher-order functions |
| 8 | Lazy lists (`:::` constructor; strict-by-default interpreter) |
| 9 | Lazy I/O (Hope Machine terminal environment) |
| Appendix 1 | Worked answers to all exercises — a pre-validated test suite |
| Appendix 2 | Language grammar (railroad diagrams, pp. 269–277) |
| Appendix 3 | Standard facilities: reserved words, predeclared names, I/O |
| Appendix 4 | Ordinal values of characters (ASCII table) |

Appendices 2, 3, and 4 have been transcribed and compared against the C++20
reimplementation in [`cxx/doc/BAILEY-HOPE-DEFINITION.md`](BAILEY-HOPE-DEFINITION.md).

Reviews: L.C. Paulson, *The Computer Journal*, Vol. 35, No. 5, 1992, p. 491
(see [`cxx/doc/bailey-book-review.md`](bailey-book-review.md) for a summary).

---

## Extensions and descendants

**Darlington, John and Guo, Yi-ke.**
"The Unification of Functional and Logic Languages — Towards Constraint
Functional Programming."
*Proceedings of the IEEE*, 1989 (CH2803-5/89/0000-0162). Also published as
Imperial College Department of Computing technical report.

Darlington — co-author of the 1977 Burstall/Darlington transformation paper
that established Hope's equation-and-pattern-matching syntax — returns to the
Hope lineage here to introduce *Constraint Hope* (CHOPE), a constraint
functional programming language using Hope as its syntactic and semantic base.
The paper is significant both for what it reveals about Hope's design qualities
and for where the language was heading by 1989.

**Hope as the right base.** The paper preserves Hope syntax intact (`data`,
`dec`, `---`/`<=`, pattern matching, algebraic types). Constraints are grafted
on as a separate syntactic layer appended after the `<=` of each defining
equation. Any valid Hope program is a valid CHOPE program. This is a deliberate
choice: the authors argue that Hope's clean algebraic type system and
first-order-equation semantics make it the natural vehicle for constraint
extension, whereas languages with side effects or ad hoc overloading would
complicate the constraint semantics.

**The constraint mechanism.** A CHOPE defining rule has the form
`f(pattern) <= expression | constraint`, where the constraint is a conjunction
of L-constraint predicates over the domain of computation. The interpreter is
non-deterministic: it performs *goal rewriting* (standard functional reduction)
interleaved with *constraint solving* (maintaining a satisfiable constraint set
by intersection). Theorems 3.5–4.5 establish soundness and completeness of this
operational model. The two mechanisms are kept strictly separate, which avoids
the coherence problems of earlier functional-logic hybrids.

**Hope examples.** Section 5 gives substantial Hope programs in readable syntax
including: an address book database (`dec member`, `dec trans`), a sales/
purchases database, a circuit resistance calculator, a queue implemented as two
lists (`data queue alpha == q(list alpha # list alpha)`), and a permutations
example (`dec Perms : list alpha -> list(list alpha)`). These are all well-typed
Hope programs with constraint annotations and could serve as additional test
cases.

**Historical position.** CHOPE is a direct antecedent of the constraint
functional-logic languages that followed (Curry, Mercury). The paper cites
Darlington's earlier CLP work and Reddy's treatment of logic variables in
functional languages. That Darlington chose Hope — not Miranda, not ML — as the
base for this extension in 1989 is further evidence of Hope's standing as the
reference language for clean functional semantics at Imperial College.

---

## Paterson's reference manual

**Paterson, Ross.**
*A Hope Interpreter — Reference.*
Distributed with the source code, 1999. Available in this repository as
`doc/ref_man.tex` (LaTeX source) and `cxx/doc/ref_man.md` (Markdown
conversion).

The authoritative reference for Paterson's lazy extension of Hope, which adds:
irrefutable patterns, operator sections, multi-argument function definitions,
recursive `type` synonyms (regular types), `letrec`/`whererec`, `input`/`write`,
`private`, `abstype`, and auto-generated functors (map functions) for each data
type.

---

## BibTeX entries (updated from `doc/hope.bib`)

```bibtex
@article{burstall-darlington,
    author  = {R.M. Burstall and J. Darlington},
    title   = {A Transformation System for Developing Recursive Programs},
    journal = {Journal of the ACM},
    volume  = {24},
    number  = {1},
    pages   = {44--67},
    year    = {1977}
}

@inproceedings{hope,
    author    = {R.M. Burstall and D.B. MacQueen and D.T. Sannella},
    title     = {{HOPE}: An Experimental Applicative Language},
    booktitle = {Proceedings of the 1980 {LISP} Conference},
    address   = {Stanford},
    pages     = {136--143},
    month     = aug,
    year      = {1980},
    note      = {Also CSR-62-80, Dept of Computer Science, University of Edinburgh}
}

@article{bailey-byte,
    author  = {Roger Bailey},
    title   = {A {Hope} Tutorial},
    journal = {Byte},
    volume  = {10},
    number  = {8},
    pages   = {235--258},
    month   = aug,
    year    = {1985}
}

@article{dettmer,
    author  = {Roger Dettmer},
    title   = {A Declaration of Hope: The Promise of Functional Programming},
    journal = {Electronics \& Power},
    volume  = {31},
    pages   = {819--823},
    month   = nov,
    year    = {1985}
}

@book{glaser-hankin-till,
    author    = {Hugh Glaser and Chris Hankin and David Till},
    title     = {Principles of Functional Programming},
    publisher = {Prentice Hall International},
    year      = {1984}
}

@book{field-harrison,
    author    = {A.J. Field and P.G. Harrison},
    title     = {Functional Programming},
    publisher = {Addison-Wesley},
    address   = {Wokingham, England},
    year      = {1988}
}

@book{bailey-book,
    author    = {Roger Bailey},
    title     = {Functional Programming in {Hope}},
    publisher = {Ellis Horwood},
    address   = {Chichester, England},
    year      = {1990}
}

@phdthesis{perry,
    author  = {Nigel Perry},
    title   = {The Implementation of Practical Functional Programming Languages},
    school  = {Imperial College of Science, Technology and Medicine,
               University of London},
    note    = {Second edition distributed with the {Hope*} source}
}

@inproceedings{darlington-guo,
    author    = {John Darlington and Yi-ke Guo},
    title     = {The Unification of Functional and Logic Languages ---
                 Towards Constraint Functional Programming},
    booktitle = {Proceedings of the {IEEE}},
    pages     = {162--167},
    year      = {1989},
    note      = {CH2803-5/89/0000-0162. Introduces {CHOPE} (Constraint {Hope})}
}
```
