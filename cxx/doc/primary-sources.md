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
Addison-Wesley, Wokingham, England, 1988.

A comprehensive textbook (595 pages) from Imperial College, University of
London. **Chapter 2, "An Introduction to Functional Programming through Hope",**
uses Hope as the primary vehicle for introducing functional programming concepts
to students new to the paradigm — covering functions, tuples, recursion, infix
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

---

## The full book

**Bailey, Roger.**
*Functional Programming in Hope.*
Ellis Horwood, Chichester, England, 1990.

The only book-length treatment of Hope. Reviews of this book include L.C.
Paulson's assessment in *The Computer Journal*, Vol. 35, No. 5, 1992, p. 491
(ISBN 0 13 338237 0; see `cxx/doc/bailey-book-review.md` for a summary).

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
```
