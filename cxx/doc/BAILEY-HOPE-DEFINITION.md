# Bailey Hope Definition

Transcription of Appendices 2, 3, and 4 from:

> Roger Bailey, *Functional Programming with Hope*,
> Ellis Horwood, 1990. ISBN 0-13-338237-0.

Source: 15 HEIC scans of the physical book, photographed March 2026.
Pages are numbered in the book as pp. 269–280+. Transcription is faithful
to the printed text; uncertain readings are marked *(uncertain)*.

---

## Appendix 1 — Answers to Exercises

Bailey's Appendix 1 contains worked solutions to the exercises from all nine
chapters. These are complete, author-verified Hope programs covering:

- Chapter 1–2: Basic types, arithmetic, function definitions
- Chapter 3: Lists, pattern matching, standard list operations
- Chapter 4: Data type definitions, trees
- Chapter 5: Polymorphic type system
- Chapter 6–7: Higher-order functions (`map`, `filter`, `fold`, sections)
- Chapter 8: Lazy lists and infinite structures
- Chapter 9: Lazy I/O (Hope Machine specific)

This appendix represents a substantial pre-validated test suite written by
Bailey himself. Working through it systematically would give high confidence
that our interpreter correctly implements Bailey's Hope semantics. This is a
longer-term project, dependent on having scans of Appendix 1 available.

---

## Appendix 2 — Language Summary (pp. 269–277)

### A2.1 Character Classes

Hope programs are written using a set of characters divided into classes:

| Class | Characters |
|-------|-----------|
| `letter` | `A B C D E F G H I J K L M N O P Q R S T U V W X Y Z`<br>`a b c d e f g h i j k l m n o p q r s t u v w x y z` |
| `digit` | `0 1 2 3 4 5 6 7 8 9` |
| `sign` | `# $ * \ ^ _` *(and possibly others — partially obscured)* |
| `punctuation` | `! " ' ( ) , . ; [ ] { }` *(and the multi-char tokens `<=` `==` `=>` `--` `---` `->`)* |
| `layout` | space, tab, newline |

### A2.2 Words

The sequence of characters forming a Hope program is considered to be grouped
into a sequence of *words*. Adjacent words which are formed from the same class
of non-punctuation characters must be separated by at least one layout character.

### A2.3 Program Structure

The following grammar is given as railroad (syntax) diagrams in the book.
Transcribed here as EBNF. Round-ended boxes in the diagrams represent words
formed according to A2.2. Square boxes are defined terms (non-terminals) or
named tokens. The label on a square box is either "constructor", "operation",
or "function", indicating the particular kind of name expected.

#### Lexical productions

```ebnf
number  ::= digit+

name    ::= letter (letter | digit)*

string-literal  ::= '"' character* '"'

character-literal ::= "'" character "'"

comment  ::= '!' [^newline]* newline
```

#### Constants and operands

```ebnf
constant ::= nonop
           | prefix-operation-name
           | number
           | character-literal
           | string-literal
           | '(' constant ')'
           | '[' [constant {',' constant}] ']'
           | '(' constant ',' constant ')'

operand  ::= expression
           | constant
           | formal-parameter-name
```

#### Expressions

```ebnf
prefix-operation ::= nonop
                   | prefix-operation-name
                   | infix-operation-name
                   | '~'                     -- negation section
                   | formal-parameter-name
                   | expression              -- parenthesised

expression-primary ::= prefix-operation operand
                     | '(' operand ')'
                     | infix-operation-name operand   -- left section

expression-factor  ::= expression-primary
                      | expression-primary (infix-operation-name operand)+

expression-term ::= lambda-expression
                  | expression-term

expression ::= expression-term
             | 'if' expression 'then' expression-term 'else' expression-term
             | expression let-qualifier* expression-term 'where' local-definition*

local-definition ::= pattern '==' expression-term

let-qualifier ::= 'let' local-definition 'in'

lambda-expression ::= 'lambda' ['-'] pattern expression
                    | lambda-expression
```

> **Note**: The railroad diagram on p.274 shows `lambda` with an optional `-`
> before the pattern, then the body expression. The exact separator token
> between pattern and body is partially obscured in the scan — likely `>` or
> `->` based on the original Hope tutorial syntax (`lambda x > x + 1`).

#### Equations

```ebnf
equation ::= '---' prefix-function-name pattern-term* '<=' expression
           | '---' [infix-function-name] pattern-term '<=' expression
```

#### Patterns

```ebnf
pattern-term ::= prefix-constructor-name pattern-term*
               | '(' pattern-term ',' pattern-term ')'
               | constant
               | formal-parameter-name

pattern      ::= pattern-synonym
               | '(' pattern ')'
               | pattern-term
               | '(' pattern-term ',' pattern-term ')'
               | constant
               | formal-parameter-name
```

#### Type expressions

```ebnf
type-factor ::= type-constructor-name
              | type-expression
              | type-synonym
              | type-variable-name

type-term   ::= type-factor ('#' type-factor)*

type-expression ::= type-term ('->' type-term)*

type-variable-declaration ::= 'typevar' type-variable-name+ ';'

type-synonym-declaration  ::= 'type' type-synonym '==' type-expression ';'
```

#### Data declarations

```ebnf
data-declaration ::= 'data' type-constructor-name type-variable-name* '=='
                     constructor-alternative ('+' constructor-alternative)*
                     ['with' ...]
                     ';'

constructor-alternative ::= infix-constructor-name
                           | prefix-constructor-name (type-expression ('++' type-expression)*)?
```

> **Note**: The diagram on p.276 shows `++` separating the argument type
> expressions within a single constructor alternative, and `+` (a circle node)
> separating alternatives. This is the standard Hope `data` syntax.

#### Declarations and programs

```ebnf
fixity-declaration ::= 'infix' infix-operation-name+ ':' number ';'

function-declaration ::= 'dec' function-name ':' type-expression ';'

program ::= (declaration | equation)*

declaration ::= type-variable-declaration
              | type-synonym-declaration
              | data-declaration
              | fixity-declaration
              | function-declaration
```

### A2.4 Program Order

Bailey states (p.277):

> Fixity declarations must appear before the operations are used in data
> declarations. Type variables must be declared before they are used in data
> or function declarations. Function declarations must appear before any of
> their recursion equations or expressions. The recursion equations must appear
> before evaluating an application of the function. Provided these constraints
> are observed, the elements of the program can appear in any order.

---

## Appendix 3 — Standard Facilities

### A3.1 Reserved Words

The following words have predefined meanings and cannot be chosen as
user-defined names (cf. Appendix 2):

**Tokens:**
```
!  "  #  '  (  )  ,  ;  <=  ==  =>  {  }  --  ---  ->
```

**Keywords:**
```
data  dec  else  if  infix  lambda  let  nonop  then  type  typevar  where  with
```

### A3.2 Predeclared Names

The following declarations and functions are introduced by the Hope system and
cannot be chosen for a user-defined name (cf. Appendix 2).

#### Type variables

```hope
typevar alpha, beta, gamma ;
```

#### Predeclared infix operators

```hope
infix <>, ::, :::, or                    : 4 ;
infix +, -, and                          : 5 ;
infix =, /=, <, >, =<, >=, *, div, mod  : 6 ;
```

Precedence levels from the unnumbered first page of Appendix 3 (p.279 in
the print edition, confirmed directly from the book).

- `<>` — **list append** (Section 3.9, p.61):
  `dec <> : list alpha # list alpha -> list alpha`
  e.g. `[1,2,3] <> [4,5,6]` gives `[1,2,3,4,5,6]`
- `:::` — **lazy cons** (Bailey ch.8): in Bailey's strict interpreter, `:::` defers
  evaluation of the tail, making it a lazy list. In Paterson's call-by-need
  interpreter (and our C++20 implementation), all constructors are already
  lazy — `::` and `:::` are therefore equivalent and `:::` is not implemented.
- `=<` — less-than-or-equal. Note: `<=` is reserved for function equations
  so the relational operator uses the reversed spelling `=<`.

> **Transcription correction**: an earlier reading of the scan introduced a
> spurious `^` operator and collapsed the three precedence levels into two.
> The table above is the corrected version read directly from the book.

#### Predeclared data types

```hope
data num    == 0 ++ succ num ;
data trueval == true ++ false ;
data char   == 'A' ++ 'B' ++ 'C' ++ ... ;
```

> The full set of constructors for `char` is the set of printable characters
> (cf. Appendix 4). Their ordinal order is given in Appendix 4.

```hope
data list alpha == nil ++ alpha :: list alpha ;
alpha ::: list alpha ;
```

#### Predeclared relational operators

The operators `<`, `>`, `>=`, `<=`, `=`, and `/=` are overloaded over types
`num` and `char`, with `char` ordered by ordinal value (cf. Appendix 4).
Additionally `=` and `/=` are overloaded over all non-`truval` types.

```hope
dec =, /=, >, >=, <, <= : alpha # alpha -> truval ;
```

#### Predeclared logical operators

```hope
dec and, or : truval # truval -> truval ;
dec not     : truval -> truval ;
```

#### Predeclared arithmetic

```hope
dec +, -, *, div, mod : num # num -> num ;
```

#### Predeclared list operations

```hope
dec nil    : list alpha ;
dec hd     : list alpha -> alpha ;
dec tl     : list alpha -> list alpha ;
```

#### Predeclared character and I/O utilities

```hope
dec digits : num -> list char ;
dec nl, tab : list char ;
```

#### Hope Machine I/O library (Chapter 9 — not implemented)

Bailey's Chapter 9 describes a terminal-oriented I/O library built around the
concept of the "Hope Machine" — a specific PC/terminal environment. The library
includes screen-dimension queries, cursor positioning, and character-list I/O
helpers. The predeclared names include (but are not limited to):

```hope
dec nl      : list char ;       -- newline as a character list
dec tab     : list char ;       -- tab as a character list
dec screen  : list alpha -> {} ;
dec goto    : num # num -> {} ;
dec clear   : {} ;
dec skipnl  : list char -> list char ;
```

These functions are conceptually adaptable to a modern terminal window (the
lazy character-list model maps naturally onto POSIX I/O), but represent a
self-contained subsystem. They are noted here for completeness and may be
revisited in a future release.

### A3.3 System Commands Behaving as Functions

The commands described here are those introduced in Chapters 8 and 9 of the
book. They may vary slightly between different versions of the Hope Machine.
The following can appear as an operand (cf. Appendix 2) in a top-level
expression:

---

**`get : filename -> object`**

`filename` is a string literal (cf. Appendix 2) identifying a file in the
filestore of the Hope Machine. The command returns a copy of the `object`
(of type `alpha`) held in the file with all lists lazily constructed.

---

**`prompt : prompt -> data object`** *(reading uncertain)*

`prompt` is a string literal. The command causes a message to be displayed on
the screen of the Hope Machine and returns a copy of the `object` entered by
the user.

---

**`input : prompt -> list char`**

Returns a lazy list of characters entered from the keyboard, after displaying
the prompt string on the screen.

---

**`keyboard : list char`**

Returns a lazy list of all characters entered from the keyboard.

---

### A3.4 Top-Level System Commands

The following commands can only be entered at the top level and cause the Hope
Machine to perform some action:

---

**`put : object # filename`**

`filename` is a string literal (cf. Appendix 2) identifying a file in the
filestore of the Hope Machine. The command causes a copy of the `object`
(of type `alpha`) to replace the original contents of the file, with lazy
list constructors forced.

---

**`show : object`**

The command causes a copy of the `object` (of type `alpha`) to be displayed
on the screen of the Hope Machine in the standard format without type
information, and with lazy list constructors forced.

---

**`display : list char`**

The command causes the characters to be displayed eagerly on the screen of
the Hope Machine without enclosing quotes or type information.

---

## Appendix 4 — Ordinal Values of Characters (p.280)

This table gives the ordinal values used by the predeclared relational operators
when comparing values of type `char`. Corresponds to ASCII.

| Ordinal | Char | Ordinal | Char | Ordinal | Char | Ordinal | Char |
|---------|------|---------|------|---------|------|---------|------|
| 9 | tab | 78 | `N` | 97 | `a` | 110 | `n` |
| 13 | newline | 79 | `O` | 98 | `b` | 111 | `o` |
| 32 | ` ` | 80 | `P` | 99 | `c` | 112 | `p` |
| 33 | `!` | 81 | `Q` | 100 | `d` | 113 | `q` |
| 34 | `"` | 82 | `R` | 101 | `e` | 114 | `r` |
| 35 | `#` | 83 | `S` | 102 | `f` | 115 | `s` |
| 36 | `$` | 84 | `T` | 103 | `g` | 116 | `t` |
| 37 | `%` | 85 | `U` | 104 | `h` | 117 | `u` |
| 38 | `&` | 86 | `V` | 105 | `i` | 118 | `v` |
| 39 | `'` | 87 | `W` | 106 | `j` | 119 | `w` |
| 40 | `(` | 88 | `X` | 107 | `k` | 120 | `x` |
| 41 | `)` | 89 | `Y` | 108 | `l` | 121 | `y` |
| 42 | `*` | 90 | `Z` | 109 | `m` | 122 | `z` |
| 43 | `+` | 91 | `[` | | | 123 | `{` |
| 44 | `,` | 92 | `\` | | | 124 | `\|` |
| 45 | `-` | 93 | `]` | | | 125 | `}` |
| 46 | `.` | 94 | `^` | | | 126 | `~` |
| 47 | `/` | 95 | `_` | | | | |
| 48 | `0` | 96 | `` ` `` | | | | |
| 49 | `1` | 65 | `A` | | | | |
| 50 | `2` | 66 | `B` | | | | |
| 51 | `3` | 67 | `C` | | | | |
| 52 | `4` | 68 | `D` | | | | |
| 53 | `5` | 69 | `E` | | | | |
| 54 | `6` | 70 | `F` | | | | |
| 55 | `7` | 71 | `G` | | | | |
| 56 | `8` | 72 | `H` | | | | |
| 57 | `9` | 73 | `I` | | | | |
| 58 | `:` | 74 | `J` | | | | |
| 59 | `;` | 75 | `K` | | | | |
| 60 | `<` | 76 | `L` | | | | |
| 61 | `=` | 77 | `M` | | | | |
| 62 | `>` | | | | | | |
| 63 | `?` | | | | | | |
| 64 | `@` | | | | | | |

> The table in the book runs from ordinal 9 to 126 and matches standard ASCII.
> The character ordering for `char` comparisons follows these ordinal values.

---

## Comparison with the C++20 Implementation

### Consistent with Bailey

| Feature | Bailey Appendix 2 | Our implementation |
|---------|------------------|--------------------|
| `data` / `type` / `typevar` / `dec` | As specified | Implemented |
| `infix` with numeric precedence | As specified | Implemented |
| `nonop` keyword | Present | Implemented |
| `if then else` | As specified | Implemented |
| `where` / `let ... in` | As specified | Implemented |
| `lambda` expressions | As specified | Implemented (`lambda` and `\`) |
| Pattern matching with `---` and `<=` | As specified | Implemented |
| `data num == 0 ++ succ num` | As specified | Implemented |
| `data truval == true ++ false` | As specified | Implemented |
| `data char` (printable ASCII set) | As specified | Implemented |
| `data list alpha == nil ++ alpha :: list alpha` | As specified | Implemented |
| Relational operators overloaded on `num` and `char` | As specified | Implemented |
| `and`, `or`, `not` | As specified | Implemented |
| `+`, `-`, `*`, `div`, `mod` | As specified | Implemented |
| `hd`, `tl`, `nil` | As specified | Implemented |
| `display : list char` | As specified | Implemented (`write` / `display`) |
| `keyboard : list char` | As specified | Implemented (`keyboard` builtin) |
| `input` | As specified | Implemented |

### Differences / gaps to investigate

| Feature | Bailey | Our implementation | Notes |
|---------|--------|--------------------|-------|
| `:::` (forcing cons) | `infix ::: : 4` | Not implemented | Forces head before consing; strict list building |
| `get : filename -> object` | Reads a Hope object from a file | Not implemented | Our `read` reads chars, not typed objects |
| `put : object # filename` | Writes a Hope object to a file | Not implemented | Our `save` writes source, not a serialised object |
| `show : object` | Displays without type info, forces thunks | Partial — `write` covers list char | `show` applies to any type |
| `prompt : prompt -> data object` | Interactive prompt returning typed object | Not implemented as a function | Our `prompt` support is via `input` |
| Chapter 9 I/O library (`nl`, `tab`, `screen`, `goto`, `clear`, `skipnl`, …) | Hope Machine terminal I/O layer (Bailey ch.9) | Not implemented | A coherent terminal subsystem; may be revisited in a future release |
| `<>` list append | `list alpha # list alpha -> list alpha`, prec 5 (s.3.9 p.61) | Implemented — Standard.hop + Evaluator.cpp builtin | |
| `=<` less-or-equal | Relational, prec 4 — `<=` reserved for equations | Implemented — Standard.hop line 111 | |
| `:::` lazy cons | Defers tail evaluation (Bailey ch.8) | Not needed | Bailey's interpreter is strict by default; `:::` opts into laziness. Paterson's interpreter is call-by-need throughout — `::` is already lazy, so `:::` and `::` are equivalent. Not implemented and not required. |
| `digits : num -> list char` | Decimal string of a number (s.3.9) | Implemented — alias for `num2str` in Standard.hop | e.g. `digits 426` gives `"426"` |
| `nl`, `tab` | Predeclared `list char` | *(verify)* | Newline and tab as char lists |

### Lambda syntax note

Bailey's railroad diagram (p.274) shows `lambda` followed by an optional
`-` token before the pattern. The tutorial syntax `lambda x > x + 1` uses
`>` as the separator. Our implementation accepts both `lambda x > expr` and
`\ x -> expr`. This matches Paterson's extensions over Bailey's original.

### Reserved word differences

Bailey's Appendix 3 reserved list does not include several tokens that
Paterson added in his extended interpreter:
- `letrec`, `whererec` — Paterson extensions for explicit recursion
- `abstype` — Paterson extension for abstract types
- `uses`, `private`, `save` — module system (Paterson)
- `write`, `edit`, `exit`, `display` — REPL commands (Paterson/our extension)

These are all upward-compatible with Bailey's language definition.
