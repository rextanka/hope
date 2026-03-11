# Module System

## Files
- `src/module.c` — module loading, scope, `uses`/`private`, symbol lookup
- `src/source.c` — source file reading and interactive input
- `src/main.c` — startup, argument parsing

---

## Module Struct

Each loaded module is a `Module`:

```c
struct _Module {
    String mod_name;               // interned name (e.g. "Standard", "list")
    unsigned short mod_num;        // index in mod_list[] (0=Session, 1=Standard, 2+)
    SET(mod_uses, MAX_MODULES);    // direct uses
    SET(mod_all_uses, MAX_MODULES);// transitive closure of uses
    SET(mod_tvars, MAX_TVARS);     // type variables declared in this module
    SET(mod_all_tvars, MAX_TVARS); // all tvars visible (incl. from used modules)
    Table mod_ops;                 // infix operators
    Table mod_types;               // type constructors (DefType)
    Table mod_fns;                 // value identifiers (Func)
    Module *mod_public;            // non-NULL if this is a private extension
};
```

The three hash tables `mod_ops`, `mod_types`, `mod_fns` use the `table.c` hash table implementation. Each entry is a `TabElt` (linked list node with a `String` key).

### Special module indices
```
SESSION  = 0   // the interactive REPL session
STANDARD = 1   // lib/Standard.hop (always loaded first)
ORDINARY = 2   // user modules start here
```

There is a hard limit of `MAX_MODULES = 32` modules.

---

## Initialisation (`mod_init`)

Called from `main()` before `yyparse()`:

```c
void mod_init(void) {
    split_path();           // parse HOPEPATH environment variable
    mod_count = 0;
    mod_current = mod_stack;
    *mod_current = mod_new(SESSION_NAME);  // create Session module
    mod_use(Standard_name);               // queue Standard for loading
    mod_fetch();                          // actually load Standard
}
```

After `mod_init`, the Standard module is fully loaded and its heap cells are marked permanent by `preserve()` in `main()`.

---

## Module Path Resolution

Module files are located by searching a list of directories stored in `dir[]`.

The search path is initialised from the `HOPEPATH` environment variable:
- If set: split on `:` (Unix) or `;` (MSDOS)
- Empty path entries are replaced by `HOPELIB` (compile-time constant, the installed `lib/` directory)
- Default if `HOPEPATH` not set: `[".", ""]` (current dir, then HOPELIB)

To load module `Foo`:
1. Search `dir[0]/Foo.hop`, `dir[1]/Foo.hop`, ... until found
2. Open the file and push it as the new source for `yyparse()`

---

## Loading a Module (`mod_use` / `mod_fetch`)

`mod_use(name)` is called when the parser sees `uses Name;`:
1. Find or create a `Module` struct for `name`
2. Check for cycles (if `name` is already on the `mod_stack`, it's a cyclic dependency — error)
3. Add `name` to the current module's `mod_uses` set
4. Compute transitive closure: `mod_all_uses |= name.mod_all_uses`

`mod_fetch()` is called after each `;` in the parser (`mod_fetch()` in `clean_slate()`):
1. Scan `mod_unread` for any module that is:
   - Not yet read, AND
   - In the current module's `mod_uses`
2. If found: open the file, push the module onto `mod_stack`, recurse into the YACC parser to read it
3. Once read, pop the stack and continue

This lazy loading scheme means modules are loaded on demand, in dependency order, depth-first.

---

## Symbol Lookup

Symbol lookup searches a sequence of modules in a well-defined order:

```c
void *look_everywhere(String name, LookFn *look_fn);
```

Searches:
1. The current session module
2. All transitively used modules (in `mod_all_uses`)
3. The Standard module

Within each module, lookup is a hash table search by interned string pointer.

Three specialised lookup functions:
```c
Op  *op_lookup(String name)   // look up infix operator
DefType *dt_lookup(String name)  // look up type constructor
Func *fn_lookup(String name)     // look up value identifier
```

And the "local only" variants (for checking redeclaration):
```c
DefType *dt_local(String name)
Func *fn_local(String name)
Cons *cons_local(String name)
```

---

## Private Definitions (`private;`)

When `private;` is encountered in a module:

1. A new shadow module is created with the same name
2. The shadow module's `mod_public` pointer is set to the original module
3. All subsequent definitions go into the shadow module — they are NOT visible to modules that `use` the original
4. `abstype` definitions in the original are reset: their constructors become inaccessible (abstract interface is preserved, implementation is hidden)

When the module finishes loading, `mod_finish()` handles cleanup: private types are reset, the shadow is discarded, and only the public interface remains visible.

---

## Saving a Module (`mod_save`)

`mod_save(name)` writes the current session's definitions to `name.hop`:

1. Opens `name.hop` for writing
2. Writes all `infix`/`infixr` declarations
3. Writes all `data`/`type`/`abstype` definitions
4. Writes all `dec` declarations
5. Writes all function equations (in their original text form)
6. Replaces the session's definitions with a single `uses Name;`

This is the mechanism by which work done interactively can be saved as a reusable module.

---

## `display;` Command

Prints the current session's definitions to stdout:
- Type variable declarations
- Infix operator declarations
- Type definitions (`data`, `type`, `abstype`)
- Value declarations (`dec`)
- Function definitions (equations)

---

## Source File Reading (`source.c`)

The source subsystem manages the input buffer for `yylex()`. It supports:
- **Interactive mode** (stdin): reads one line at a time with `hope_getline()` — the renamed `getline()` (renamed to avoid POSIX clash)
- **File mode** (`-f file`): reads from a file opened in `main()`
- **Listing mode** (`-l`): echoes each line to stderr as it is read

Key functions:
```c
void init_source(FILE *src, Bool gen_listing);
Bool hope_getline(void);      // read next line into buffer
Bool interactive(void);       // true if reading from stdin
```

The input buffer (`inptr`) is read character by character by `yylex()` using `FetchChar()` and `BackChar()` (from `text.h`).

---

## `main.c` Startup Sequence

```c
int main(int argc, const char *const argv[]) {
    // 1. Parse command-line flags: -f file, -l (listing), -r (restricted), -t nsecs
    // 2. Open source file (or use stdin)
    // 3. init_memory()    — allocate the main arena
    // 4. init_strings()   — initialise interned string table
    // 5. init_lex()       — pre-intern reserved words
    // 6. init_source()    — attach input source
    // 7. mod_init()       — load Standard module
    // 8. preserve()       — mark Standard's heap as permanent
    // 9. yyparse()        — main parse/execute loop (runs until exit; or EOF)
    // 10. heap_stats()    — print stats if STATS defined
}
```

Command-line flags:
| Flag | Effect |
|------|--------|
| `-f file` | Read from file instead of stdin |
| `-l` | Echo each input line to stderr (listing) |
| `-r` | Restricted mode: disable file I/O (no `read`, no `write to`) |
| `-t N` | Set evaluation time limit to N seconds |

The `cmd_args` global is set to the remaining non-flag arguments, accessible in Hope via `argv : list(list char)`.

On Unix, the `RE_EDIT` feature is enabled: the `edit` command invokes `$EDITOR` on the current session and re-executes by calling `execlp()` to restart the interpreter with a script file.

---

## Implications for the C++20 Rewrite

### Module system redesign
The C++20 rewrite will restructure the module system significantly:

1. **No global tables**: each `Module` is a self-contained object with `std::unordered_map<std::string, ...>` tables
2. **No hard limit**: `MAX_MODULES = 32` disappears; use `std::vector<Module>`
3. **REPL session as a module**: the interactive session is a module like any other, supporting `:save`, `:load`, `:clear`

### Path resolution
```cpp
std::optional<std::filesystem::path> findModule(std::string_view name,
    const std::vector<std::filesystem::path>& searchPath);
```
Uses `std::filesystem` instead of manual string concatenation.

### REPL commands
The new REPL adds meta-commands not in Paterson's interpreter:
- `:load <file>` — load a .hop file into the session
- `:reload` — reload the last loaded file
- `:type <expr>` — show inferred type without evaluating
- `:display` — show current definitions
- `:save <name>` — save session to a module
- `:clear` — reset the session
- `:quit` — exit

See [11-rewrite-roadmap.md](11-rewrite-roadmap.md).
