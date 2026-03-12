# Porting Notes

This file records the changes made to Ross Paterson's original source when
it was retrieved from the Internet Archive, and notes on the C++20
reimplementation.  Ross's original README is in [`readme`](readme).
The GitHub-facing overview is in [`README.md`](README.md).

---

## How this copy was obtained

Paterson's files were hosted at `http://www.soi.city.ac.uk/~ross/Hope/`
and disappeared around 2013.  This copy was retrieved from:

  https://web.archive.org/web/20131022235656/http://www.soi.city.ac.uk/~ross/Hope/

---

## Changes to the original C source

Two minimal changes were made to build on modern systems:

1. **`datarootdir`** added to `Makefile.in` files — required for autoconf 2.60+.

2. **`getline` renamed to `hope_getline`** — `getline` became a POSIX standard
   function (POSIX.1-2008) and clashes with the system declaration on modern
   Linux and macOS.

No changes were made to the interpreter logic, the standard library, or the
language semantics.

---

## Build notes

**Note on line endings:** Bison and Yacc produce strange build failures if
source files have incorrect line endings for the target platform.  Ensure LF
endings on Linux/macOS.

**macOS (Homebrew):**
```bash
brew install bison autoconf
export PATH="/opt/homebrew/opt/bison/bin:$PATH"
./configure
make
make check
```

**Debian / Ubuntu:**
```bash
sudo apt-get install gcc make bison autoconf byacc gawk
./configure
make
make check
```

For the LaTeX documentation: also install `texlive` and `latex2html`.

---

## C++20 reimplementation

A complete reimplementation is in `cxx/`.  See
[`README.md`](README.md) §3 for the overview and
[`cxx/doc/user-guide.md`](cxx/doc/user-guide.md) for the full user guide.

---

## doc/Standard.tex

`doc/Standard.tex` is empty in this archive and in the `dmbaturin/hope`
GitHub mirror.  It is `\input`-ted by `doc/ref_man.tex` as an appendix
("The Standard module").  Paterson apparently never completed this section;
the LaTeX build succeeds with an empty file but the appendix is blank.

The content of the Standard module is the file `lib/Standard.hop` itself,
which is fully annotated in `claude-docs/09-standard-library.md` and
summarised in `cxx/doc/user-guide.md`.
