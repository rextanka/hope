#include "types/TypeEnv.hpp"

#include <memory>

namespace hope {

// ---------------------------------------------------------------------------
// Helper — build AST TypePtr nodes for seeding built-in types
// ---------------------------------------------------------------------------

static TypePtr tcons0(const std::string& name) {
    SourceLocation builtin{"<builtin>", 0, 0, 0};
    return make_type(TCons{name, {}}, builtin);
}

static TypePtr tvar_ast(const std::string& name) {
    SourceLocation builtin{"<builtin>", 0, 0, 0};
    return make_type(TVar{name}, builtin);
}

static TypePtr tcons1(const std::string& name, TypePtr arg) {
    SourceLocation builtin{"<builtin>", 0, 0, 0};
    std::vector<TypePtr> args;
    args.push_back(std::move(arg));
    return make_type(TCons{name, std::move(args)}, builtin);
}

static TypePtr tprod_ast(TypePtr l, TypePtr r) {
    SourceLocation builtin{"<builtin>", 0, 0, 0};
    return make_type(TProd{std::move(l), std::move(r)}, builtin);
}

static TypePtr tfun_ast(TypePtr dom, TypePtr cod) {
    SourceLocation builtin{"<builtin>", 0, 0, 0};
    return make_type(TFun{std::move(dom), std::move(cod)}, builtin);
}

static TypePtr tnum() { return tcons0("num"); }
static TypePtr tchar() { return tcons0("char"); }
static TypePtr tbool() { return tcons0("bool"); }
static TypePtr tstr() { return tcons1("list", tchar()); }
static TypePtr tlist_a() { return tcons1("list", tvar_ast("alpha")); }

// Helper to create a FuncDecl for a builtin function with given params and type.
static FuncDecl make_builtin_fd(std::string name,
                                std::vector<std::string> params,
                                TypePtr type) {
    FuncDecl fd;
    fd.names  = {std::move(name)};
    fd.params = std::move(params);
    fd.type   = std::move(type);
    fd.loc    = SourceLocation{"<builtin>", 0, 0, 0};
    return fd;
}

// ---------------------------------------------------------------------------
// Helper — register a typedef into the map and index its constructors
// ---------------------------------------------------------------------------

static void register_typedef(
    std::unordered_map<std::string, std::unique_ptr<TypeDef>>& typedefs,
    std::unordered_map<std::string, const ConInfo*>&           condecls,
    TypeDef def)
{
    const std::string name = def.name;
    auto uptr = std::make_unique<TypeDef>(std::move(def));

    // Insert or overwrite.  The unique_ptr ensures the TypeDef lives at a
    // stable address so ConInfo* pointers into it remain valid.
    auto& slot = typedefs[name];
    slot = std::move(uptr);

    if (slot->is_data()) {
        for (const auto& ci : slot->constructors())
            condecls[ci.name] = &ci;
    }
}

// ---------------------------------------------------------------------------
// TypeEnv constructor — seed built-in types
// ---------------------------------------------------------------------------

TypeEnv::TypeEnv() {
    SourceLocation builtin{"<builtin>", 0, 0, 0};

    // -----------------------------------------------------------------------
    // num — zero-arity abstract type (treated specially by literals)
    // -----------------------------------------------------------------------
    {
        TypeDef td;
        td.name   = "num";
        td.params = {};
        td.def    = std::monostate{};
        register_typedef(typedefs_, condecls_, std::move(td));
    }

    // -----------------------------------------------------------------------
    // char — zero-arity abstract type
    // -----------------------------------------------------------------------
    {
        TypeDef td;
        td.name   = "char";
        td.params = {};
        td.def    = std::monostate{};
        register_typedef(typedefs_, condecls_, std::move(td));
    }

    // -----------------------------------------------------------------------
    // bool — zero-arity data type with constructors `false` and `true`
    // -----------------------------------------------------------------------
    {
        TypeDef td;
        td.name   = "bool";
        td.params = {};

        std::vector<ConInfo> cons;

        // false : bool
        {
            ConInfo ci;
            ci.name      = "false";
            ci.type_name = "bool";
            ci.params    = {};
            ci.arg       = std::nullopt;
            ci.loc       = builtin;
            cons.push_back(std::move(ci));
        }
        // true : bool
        {
            ConInfo ci;
            ci.name      = "true";
            ci.type_name = "bool";
            ci.params    = {};
            ci.arg       = std::nullopt;
            ci.loc       = builtin;
            cons.push_back(std::move(ci));
        }

        td.def = std::move(cons);
        register_typedef(typedefs_, condecls_, std::move(td));
    }

    // -----------------------------------------------------------------------
    // truval — synonym for bool  (used as the condition type in if/then/else)
    // -----------------------------------------------------------------------
    {
        TypeDef td;
        td.name   = "truval";
        td.params = {};
        td.def    = tcons0("bool");
        register_typedef(typedefs_, condecls_, std::move(td));
    }

    // -----------------------------------------------------------------------
    // list alpha — one-arity data type
    //   nil  : list alpha
    //   ::   : alpha # list alpha -> list alpha
    // -----------------------------------------------------------------------
    {
        TypeDef td;
        td.name   = "list";
        td.params = {"alpha"};

        std::vector<ConInfo> cons;

        // nil : list alpha
        {
            ConInfo ci;
            ci.name      = "nil";
            ci.type_name = "list";
            ci.params    = {"alpha"};
            ci.arg       = std::nullopt;
            ci.loc       = builtin;
            cons.push_back(std::move(ci));
        }

        // :: : alpha # list alpha -> list alpha
        // The arg type is the product: alpha # list alpha
        {
            ConInfo ci;
            ci.name      = "::";
            ci.type_name = "list";
            ci.params    = {"alpha"};
            ci.arg       = tprod_ast(tvar_ast("alpha"),
                                     tcons1("list", tvar_ast("alpha")));
            ci.loc       = builtin;
            cons.push_back(std::move(ci));
        }

        td.def = std::move(cons);
        register_typedef(typedefs_, condecls_, std::move(td));
    }

    // -----------------------------------------------------------------------
    // -> and # — binary abstract type constructors (built-in, no constructors)
    // -----------------------------------------------------------------------
    {
        TypeDef td;
        td.name   = "->";
        td.params = {"alpha", "beta"};
        td.def    = std::monostate{};
        register_typedef(typedefs_, condecls_, std::move(td));
    }
    {
        TypeDef td;
        td.name   = "#";
        td.params = {"alpha", "beta"};
        td.def    = std::monostate{};
        register_typedef(typedefs_, condecls_, std::move(td));
    }

    // -----------------------------------------------------------------------
    // Built-in function declarations
    // -----------------------------------------------------------------------

    // Arithmetic: num # num -> num
    for (const char* op : {"+", "-", "*", "/", "div", "mod"}) {
        add_funcdecl(make_builtin_fd(op, {},
            tfun_ast(tprod_ast(tnum(), tnum()), tnum())));
    }

    // Unary numeric: num -> num
    for (const char* op : {"~", "neg", "abs", "sqrt", "floor",
                            "ceiling", "round", "trunc", "float",
                            "succ", "pred"}) {
        add_funcdecl(make_builtin_fd(op, {}, tfun_ast(tnum(), tnum())));
    }

    // Comparison: alpha # alpha -> bool
    for (const char* op : {"=", "/=", "<", ">", "=<", ">="}) {
        add_funcdecl(make_builtin_fd(op, {"alpha"},
            tfun_ast(tprod_ast(tvar_ast("alpha"), tvar_ast("alpha")),
                     tbool())));
    }

    // Boolean: bool # bool -> bool
    for (const char* op : {"and", "or"}) {
        add_funcdecl(make_builtin_fd(op, {},
            tfun_ast(tprod_ast(tbool(), tbool()), tbool())));
    }

    // not: bool -> bool
    add_funcdecl(make_builtin_fd("not", {}, tfun_ast(tbool(), tbool())));

    // ord: char -> num, chr: num -> char
    add_funcdecl(make_builtin_fd("ord", {}, tfun_ast(tchar(), tnum())));
    add_funcdecl(make_builtin_fd("chr", {}, tfun_ast(tnum(), tchar())));

    // num2str: num -> list char, str2num: list char -> num
    add_funcdecl(make_builtin_fd("num2str", {}, tfun_ast(tnum(), tstr())));
    add_funcdecl(make_builtin_fd("str2num", {}, tfun_ast(tstr(), tnum())));

    // id: alpha -> alpha
    add_funcdecl(make_builtin_fd("id", {"alpha"},
        tfun_ast(tvar_ast("alpha"), tvar_ast("alpha"))));

    // const: alpha -> beta -> alpha
    add_funcdecl(make_builtin_fd("const", {"alpha", "beta"},
        tfun_ast(tvar_ast("alpha"),
                 tfun_ast(tvar_ast("beta"), tvar_ast("alpha")))));

    // o: (beta -> gamma) # (alpha -> beta) -> alpha -> gamma
    add_funcdecl(make_builtin_fd("o", {"alpha", "beta", "gamma"},
        tfun_ast(tprod_ast(
                     tfun_ast(tvar_ast("beta"), tvar_ast("gamma")),
                     tfun_ast(tvar_ast("alpha"), tvar_ast("beta"))),
                 tfun_ast(tvar_ast("alpha"), tvar_ast("gamma")))));

    // fst: alpha # beta -> alpha, snd: alpha # beta -> beta
    add_funcdecl(make_builtin_fd("fst", {"alpha", "beta"},
        tfun_ast(tprod_ast(tvar_ast("alpha"), tvar_ast("beta")),
                 tvar_ast("alpha"))));
    add_funcdecl(make_builtin_fd("snd", {"alpha", "beta"},
        tfun_ast(tprod_ast(tvar_ast("alpha"), tvar_ast("beta")),
                 tvar_ast("beta"))));

    // head: list alpha -> alpha, tail: list alpha -> list alpha
    add_funcdecl(make_builtin_fd("head", {"alpha"},
        tfun_ast(tlist_a(), tvar_ast("alpha"))));
    add_funcdecl(make_builtin_fd("tail", {"alpha"},
        tfun_ast(tlist_a(), tlist_a())));

    // null: list alpha -> bool
    add_funcdecl(make_builtin_fd("null", {"alpha"},
        tfun_ast(tlist_a(), tbool())));

    // <> (append): list alpha # list alpha -> list alpha
    add_funcdecl(make_builtin_fd("<>", {"alpha"},
        tfun_ast(tprod_ast(tlist_a(), tlist_a()), tlist_a())));

    // error: list char -> alpha
    add_funcdecl(make_builtin_fd("error", {"alpha"},
        tfun_ast(tstr(), tvar_ast("alpha"))));
}

// ---------------------------------------------------------------------------
// add_typedef
// ---------------------------------------------------------------------------

void TypeEnv::add_typedef(TypeDef def) {
    register_typedef(typedefs_, condecls_, std::move(def));
}

// ---------------------------------------------------------------------------
// add_funcdecl
// ---------------------------------------------------------------------------

void TypeEnv::add_funcdecl(FuncDecl decl) {
    if (decl.names.empty()) return;

    // The canonical name is the first in the list.  The full FuncDecl
    // (including the TypePtr) is stored under the canonical name; alias
    // entries with null TypePtr are stored under every other name.
    // lookup_func() redirects alias lookups to the canonical entry.

    const std::string              canonical   = decl.names[0];
    const SourceLocation           decl_loc    = decl.loc;
    const std::vector<std::string> names_copy  = decl.names;
    const std::vector<std::string> params_copy = decl.params;

    // Store canonical (moves the unique TypePtr in).
    funcdecls_[canonical] = std::make_unique<FuncDecl>(std::move(decl));

    // Store lightweight alias entries for additional names.
    for (size_t i = 1; i < names_copy.size(); ++i) {
        auto alias    = std::make_unique<FuncDecl>();
        alias->names  = names_copy;   // alias->names[0] == canonical
        alias->params = params_copy;
        alias->type   = nullptr;      // real type lives in canonical entry
        alias->loc    = decl_loc;
        funcdecls_[names_copy[i]] = std::move(alias);
    }
}

// ---------------------------------------------------------------------------
// Lookup helpers
// ---------------------------------------------------------------------------

const TypeDef* TypeEnv::lookup_type(const std::string& name) const {
    auto it = typedefs_.find(name);
    if (it == typedefs_.end()) return nullptr;
    return it->second.get();
}

const ConInfo* TypeEnv::lookup_con(const std::string& name) const {
    auto it = condecls_.find(name);
    if (it == condecls_.end()) return nullptr;
    return it->second;
}

const FuncDecl* TypeEnv::lookup_func(const std::string& name) const {
    auto it = funcdecls_.find(name);
    if (it == funcdecls_.end()) return nullptr;

    const FuncDecl* fd = it->second.get();

    // If this is an alias entry (null type), redirect to the canonical name.
    if (fd->type == nullptr && !fd->names.empty()) {
        const std::string& canon = fd->names[0];
        if (canon != name) {
            auto it2 = funcdecls_.find(canon);
            if (it2 != funcdecls_.end()) return it2->second.get();
        }
    }
    return fd;
}

} // namespace hope
