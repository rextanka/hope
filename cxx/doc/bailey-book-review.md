# Roger Bailey: *Functional Programming with Hope* — Book Review Summary

**Book:** Roger Bailey, *Functional Programming with Hope*
Prentice-Hall, Hemel Hempstead, 1992.
£33.50. ISBN 0 13 338237 0.

**Review by:** L. C. Paulson, Cambridge.
Published in *The Computer Journal*, Vol. 35, No. 5, 1992, p. 491.

A scan of the original review is in the repository archive at
`hope-orig/Functional_Programming_with_Hope.pdf`.

---

## Context

The review opens by noting that functional programming had by 1992 found its
greatest success in education, but that suitable beginner's textbooks were
scarce.  Paulson surveys the alternatives:

- **Bird and Wadler**, *Introduction to Functional Programming* — valuable but
  closer to a research monograph than a beginner's text
- **Abelson and Sussman**, *Structure and Interpretation of Computer Programs*
  — remarkable examples but too difficult for beginners
- **Wikström**, *Functional Programming Using ML* — the opposite problem: too
  slow for students

Bailey's book, in Paulson's assessment, **"has the pace just about right"**.

---

## Chapter structure

| Chapter | Contents |
|---------|----------|
| 1 | Basic types (integers, characters, etc.) and expression evaluation |
| 2 | Functions and variable binding |
| 3 | Lists and pattern matching |
| 4 | Data type definitions and trees (extensive examples) |
| 5 | Hope's polymorphic type system |
| 6–7 | Higher-order functions: functions as arguments and results |
| 8 | Lazy lists |
| 9 | Lazy lists applied to input and output |
| Appendices | Answers to exercises; syntax charts for Hope |

---

## Assessment

**Strengths:**

- Orderly, deliberate presentation — every concept introduced after thorough
  preparation, connections with previous concepts explored in detail
- Clear, simple prose with touches of humour
- Each chapter ends with a summary and exercises
- "One of the best textbooks available for an introductory programming course
  using a functional language"

**Criticisms:**

- In places too many examples, making it hard to see where they are leading
- `map` appears in four variations and is illustrated against seemingly every
  other feature — Paulson would have preferred fewer but larger worked examples
- List reversal appears only as an exercise; the solution given requires
  quadratic rather than linear time
- The index is sparse

**Hope versus ML:**

Paulson notes he prefers ML for teaching because ML deduces function types
automatically while Hope requires explicit `dec` declarations for all
functions.  Although type declarations make programs easier to read, they are
"a great deterrent when students work interactively."

He also observes that Hope's purely functional lazy-list I/O (characters join
the back of the list as typed, leave the front as demanded) is semantically
clean but "involves many complications," while ML sidesteps the issue with
imperative I/O facilities.

**Price:**

At £33.50 (approximately £80 in 2025 terms), Paulson considers the book out
of reach of many students and calls on Prentice-Hall to bring out an
inexpensive edition.  That edition does not appear to have materialised; the
book has a small print run and is now out of print.  Second-hand copies
occasionally appear.

---

## Bibliographic note

The book is the primary textbook companion to Paterson's Hope interpreter.
The tutorial distributed with this repository (`doc/hope_tut.src`,
`cxx/doc/hope_tut.md`) is Roger Bailey's separate tutorial article
(*Byte*, August 1985), which predates the book and covers similar ground at a
faster pace.  The book contains substantially more material, particularly on
data type definitions, higher-order functions, and the formal treatment of
polymorphism.
