# A Hope Interpreter — Reference

*Ross Paterson*

This manual is not a tutorial on functional programming or on the language
Hope.  If you don't know about both, start with Roger Bailey's tutorial
([`hope_tut.md`](hope_tut.md) in this directory, converted from `doc/hope_tut.src`).

---

## 1  Lexical structure

The input is divided into a sequence of symbols of the following kinds:

- a **punctuation character**: one of `( ) , ; [ ]`
- an **identifier**: either
  1. a letter or underscore followed by zero or more letters, underscores or
     digits (followed by zero or more single quotes), or
  2. a sequence of graphic characters that are neither white-space, letters,
     underscores, digits, punctuation, nor one of `` ! " ' ``
- a **numeric literal**: a sequence of digits (optionally with an embedded
  decimal point and an optionally signed exponent on systems that support it)
- a **character literal**: either a single character (but not a newline) or a
  character escape sequence (as in C), enclosed in single quotes
- a **string literal**: a sequence of zero or more characters (but not
  newlines or double-quotes) or character escape sequences, enclosed in double
  quotes

Symbols may be separated by white-space and comments.  A comment is
introduced by `!` and continues to the end of the line.

The following identifiers are reserved by the language:

```
++ --- : <= == => |
abstype data dec display else edit exit if in
infix infixr lambda let letrec private save then
type typevar uses where whererec write
```

The following identifiers are also reserved for compatibility with other
implementations:

```
end module nonop pubconst pubfun pubtype
```

Available synonyms: `\` for `lambda`, `use` for `uses`, `infixrl` for
`infixr`.

---

## 2  Identifiers

Identifiers may refer to:

- modules
- types
- type constructors
- data constants
- data constructors
- values

The same identifier may refer to more than one of these kinds, but in any
scope it cannot refer to more than one of the last three classes.

Identifiers may be defined as **infix operators** by `infix` and `infixr`
declarations (see [Definitions](#3-2-definitions)).  This affects only the way
they are parsed and printed: if an identifier *op* is declared infix,
constructs of the form `op(a, b)` are written `a op b`.  To refer to *op* on
its own, use `(op)`.

A special case: in any subsequent declaration of *op* as a data constructor,
`op(type1 # type2)` is written `type1 op type2`.

A number of identifiers are predefined in the module `Standard`, which is
implicitly imported by every module and session.

---

## 3  Composite structures

In the descriptions below, `constant width` text denotes literal program
text; *italic* text denotes syntactic classes.  `[` and `]` around an item
indicate it is optional.  Alternative forms apply when identifiers are
declared as infix operators.

### 3.1  Modules

A Hope module *name* is a text file `name.hop` consisting of definitions.
Definitions may occur in any order, but identifiers must be appropriately
declared before use.

There is a special Hope module called `Standard`, implicitly imported by
every other Hope module and session.

An interactive session consists of definitions together with interactive
commands.

### 3.2  Definitions

**`uses` *module*, …, *module* `;`**
> All definitions visible in the named modules become visible in the current
> module or session.  Modules are searched first in the current directory,
> then in the library directory (`HOPEPATH`).  Used modules may use other
> modules, provided there are no cycles.  Definitions visible in the using
> module will not be visible in a used module unless they reside in a third
> module used by both.

**`private;`**
> All subsequent definitions in this module will be hidden from other modules
> that use it.  In particular, definitions visible in a module used *after*
> this directive will not be visible to any module using this one, whereas
> they would be if the module was used before the directive.

**`infix` *ident*, …, *ident* `:` *n* `;`**
> Declare left-associative infix operators with the stated precedence, from
> `1` (weakest) to `9` (strongest).

**`infixr` *ident*, …, *ident* `:` *n* `;`**
> Like `infix`, except the operators are right-associative.

**`abstype` *ident* `[(`*ident₁*`, …,` *identₙ*`)]` `;`**
> An abstract type definition, declaring *ident* as a type or type-constructor
> identifier for use in subsequent definitions.  *ident* may be defined later
> by a `data` or `type` definition.  Parentheses may be omitted when there is
> only one argument.

**`data` *ident* `[(`*ident₁*`, …,` *identₙ*`)]` `==` *ident'₁* `[`*type₁*`]` `++` … `++` *ident'ₖ* `[`*typeₖ*`]` `;`**
> Define *ident* as a type or type constructor, with *ident'₁* … *ident'ₖ* as
> its data constants and constructors.  Parentheses may be omitted when there
> is only one simple argument.  The definition may be recursive; any use of
> *ident* in *typeᵢ* must have the same parameters as the left-hand side.

**`type` *ident* `[(`*ident₁*`, …,` *identₙ*`)]` `==` *type* `;`**
> Define *ident* as an abbreviation for *type*, which may refer to the
> argument type identifiers.  Parentheses may be omitted when there is only
> one simple argument.  The definition may be recursive (see
> [Regular types](#a-2-regular-types)).

**`typevar` *ident*, …, *ident* `;`**
> Declare new type variables for use in `dec` declarations.  If *ident* is
> declared, then *ident*`'`, *ident*`''`, and so on may also be used as type
> variables.

**`dec` *ident*, …, *ident* `:` *type* `;`**
> Declare the identifiers as value identifiers of the given type.  Type
> variables stand for any type.  If only one identifier is being declared, the
> keyword `dec` is optional.

**`---` *ident* *pat₁* … *patₙ* `<=` *expr* `;`**
> Define the value of an identifier.  If *n* is zero, only one such definition
> is allowed per identifier.  Otherwise the identifier may have one or more
> definitions, each with the same number of arguments.  The keyword `---` is
> optional.

### 3.3  Types

| Syntax | Meaning |
|--------|---------|
| *type-identifier* | The type referred to |
| *type-constructor*`(`*type*, …, *type*`)` | A constructed type or type abbreviation.  Parentheses may be omitted for a single argument. |
| `(`*type*`)` | Same as *type* |

### 3.4  Patterns

| Syntax | Meaning |
|--------|---------|
| *identifier* (not a data constant) | A variable matching any value; may not occur twice in the same pattern |
| *data-constant* | Matches the named data constant |
| *data-constructor* *pat* | Matches a value formed by applying the constructor to a value matching *pat* |
| *n* | `1`, `2`, … are abbreviations for `succ(0)`, `succ(succ(0))`, … |
| *pat* `+` *k* | Abbreviation for `succ` applied *k* times to *pat* |
| `'`*c*`'` | Matches the character constant |
| `"`*string*`"` | Abbreviation for a list of characters |
| `[`*pat₁*`, …,` *patₙ*`]` | Equivalent to `pat₁ :: … :: patₙ :: nil` |
| `[]` | Equivalent to `nil` |
| *pat₁*`,` *pat₂* | Matches a pair of values matching *pat₁* and *pat₂* |
| `(`*pat*`)` | Same as *pat* |

### 3.5  Expressions

| Syntax | Meaning |
|--------|---------|
| *value-identifier* | The value bound to the identifier |
| *data-constant* | A data constant |
| *data-constructor* | A data constructor |
| *n* | `1`, `2`, … are abbreviations for `succ(0)`, `succ(succ(0))`, … |
| `'`*c*`'` | Character constant |
| `"`*string*`"` | Abbreviation for a list of characters |
| `[`*e₁*`, …,` *eₙ*`]` | Equivalent to `e₁ :: … :: eₙ :: nil` |
| `[]` | Equivalent to `nil` |
| *e₁*`,` *e₂* | A pair formed from the values of *e₁* and *e₂* |
| `(`*expr*`)` | Same as *expr* |
| *e₁* *e₂* | Apply the function or constructor value of *e₁* to *e₂* |
| `lambda` *pat₁* `=>` *e₁* `\|` … `\|` *patₖ* `=>` *eₖ* | An anonymous function |
| `if` *e₁* `then` *e₂* `else` *e₃* | Conditional: *e₂* if *e₁* is `true`, *e₃* if `false` |
| `let` *pat* `==` *e₁* `in` *e₂* | Local binding (equivalent to `(lambda pat => e₂) e₁`) |
| `letrec` *pat* `==` *e₁* `in` *e₂* | Recursive local binding; *pat* must be irrefutable; variables in *pat* may appear in *e₁* |
| *e₁* `where` *pat* `==` *e₂* | Same as `let pat == e₂ in e₁` |
| *e₁* `whererec` *pat* `==` *e₂* | Same as `letrec pat == e₂ in e₁` |

**Binding precedences** (weakest to strongest):

1. comma (right-associative)
2. `lambda`
3. `let` and `letrec`
4. `where` and `whererec`
5. `if`
6. infix operators, precedence 1 to 9
7. function application (left-associative)

**Operator sections:** for any infix operator *op* and expression *e*:

- `(e op)` is short for `lambda x => e op x`
- `(op e)` is short for `lambda x => x op e`

### 3.6  Interactive commands

In a Hope session, any definition (see [Definitions](#3-2-definitions)) may be
entered (though `private` is ignored).  The following commands are also
available:

**`expr;`**
> Evaluate *expr* and display its value and most general type.  Lazy
> evaluation is used; the value is fully evaluated before display.
>
> *expr* may use the special variable `input`, which denotes the list of
> characters typed at the terminal.

**`write` *expr* `[to "` *file* `"];`**
> Output the value of *expr* (which must be a list) more directly: if the
> value is a list of characters, the characters are printed; otherwise each
> element is printed on its own line.  Each element is printed as soon as it
> is computed (lazy streaming).  If the `to` clause is present, output is
> written to the named file; otherwise it appears on screen.
>
> *expr* may use the special variable `input`.

**`display;`**
> Display the definitions of the current session.

**`save` *ModuleName* `;`**
> Save the definitions of the current session to a new module file
> *ModuleName*`.hop`.

**`edit [` *ModuleName* `];`**
> *(Unix only)* Invoke the default editor on the named module, or on a file
> containing the current definitions if no module is given, then re-enter
> the interpreter with the revised definitions.
>
> *Not implemented in the C++20 interpreter.*

**`exit;`**
> Exit the interpreter.  Equivalent to the REPL meta-command `:quit`.

---

## 4  Semantics of pattern matching

When patterns overlap, more specific patterns are preferred.  Formally:

Matching a value against a pattern evaluates the value sufficiently to
compare its data constants and constructors with those in the pattern,
in a consistent top-to-bottom (but otherwise unspecified) order.  This
match results in one of:

- **success** — the value is an instance of the pattern
- **failure** — the value has a different data constant or constructor
- **non-termination** — the value cannot be sufficiently evaluated to decide

If the match succeeds, it supplies values for the variables of the pattern.

A pattern (or sub-pattern) consisting only of variables and pairs always
matches a value of the appropriate type, even if computing the value would
not terminate.  Such patterns are called **irrefutable**.

When a function defined by a series of equations is applied to *n* arguments:

- If some clause's patterns all succeed and all more-specific patterns (if any)
  fail, the value is the result of that clause's expression with the matched
  bindings.
- If some pattern match does not terminate (under some checking order),
  non-termination is a possible outcome.

If no clause matches, the application is a run-time error.  When multiple
outcomes are possible, one is chosen deterministically (but otherwise
unspecified).

---

## Appendix A  Deviations from other versions of Hope

The following features are specific to Paterson's interpreter:

- irrefutable patterns
- operator sections
- functions of more than one argument
- recursive `type` definitions
- `input`, `read`, `write`
- `private`, `abstype`
- `letrec` and `whererec`
- curried type and data constructors

Other implementations may not support full lazy evaluation.

### A.1  Regular types

The type system permits **regular types** — possibly infinite types that are
unrollings of finite trees.  This makes possible Curry's Y combinator:

```hope
dec Ycurry : (alpha -> alpha) -> alpha;
--- Ycurry f <= Z Z where Z == lambda z => f(z z);
```

This fails in the usual type system because `Z` must have a function type
with argument type equal to the whole type.

Recursive type synonyms describe infinite types:

```hope
type seq alpha == alpha # seq alpha;
```

Recursive uses of the type constructor being defined must have exactly the
same arguments as the left-hand side (though arguments may be permuted):

```hope
type alternate alpha beta == alpha # alternate beta alpha;
```

The `mu` notation also expresses regular types:

```hope
type seq alpha       == mu s => alpha # s;
type alternate alpha beta == mu a => alpha # (beta # a);
```

The system uses `mu` notation when printing regular types.

### A.2  Functors

Each `data` or `type` definition introduces a `map`-like function (functor)
with the same name and arity as the type constructor.  For example:

```hope
data tree alpha == Empty ++ Node (tree alpha) alpha (tree alpha);
```

also defines:

```hope
tree : (alpha -> beta) -> tree alpha -> tree beta
```

This automatic definition may be explicitly overridden.

If a type argument is used **negatively** (e.g. contravariant position):

```hope
type cond alpha == alpha -> bool;
-- functor: cond : (alpha -> beta) -> cond beta -> cond alpha
```

If used both positively and negatively:

```hope
type endo alpha == alpha -> alpha;
-- functor: endo : (alpha -> alpha) -> endo alpha -> endo alpha
```

An `abstype` declaration also gets a corresponding functor.  Each argument is
assumed used both positively and negatively unless replaced with one of the
keywords `pos`, `neg`, or `none` (indicating unused).
