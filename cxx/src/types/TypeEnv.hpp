#pragma once

// TypeEnv — the static environment maintained during type checking.
//
// It holds:
//   - type definitions (data, synonym, abstract)
//   - constructor declarations
//   - function type declarations (from `dec f : type;`)
//
// Built-in types are seeded in the constructor.

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ast/Ast.hpp"
#include "lexer/SourceLocation.hpp"

namespace hope {

// ---------------------------------------------------------------------------
// ConInfo — information about a declared constructor
// ---------------------------------------------------------------------------

struct ConInfo {
    std::string              name;
    std::string              type_name;   // name of the data type it belongs to
    std::vector<std::string> params;      // type parameters of the data type
    std::optional<TypePtr>   arg;         // argument type (nullopt if nullary)
    SourceLocation           loc;

    // ConInfo is not copyable because TypePtr is unique_ptr.
    ConInfo() = default;
    ConInfo(const ConInfo&)            = delete;
    ConInfo& operator=(const ConInfo&) = delete;
    ConInfo(ConInfo&&)                 = default;
    ConInfo& operator=(ConInfo&&)      = default;
};

// ---------------------------------------------------------------------------
// TypeDef — information about a declared type
// ---------------------------------------------------------------------------

struct TypeDef {
    std::string              name;
    std::vector<std::string> params;
    bool                     is_private = false;

    // One of: data (constructors), synonym (body), or abstract (monostate)
    std::variant<
        std::vector<ConInfo>, // data type
        TypePtr,              // type synonym
        std::monostate        // abstract type
    > def;

    bool is_data()     const { return std::holds_alternative<std::vector<ConInfo>>(def); }
    bool is_synonym()  const { return std::holds_alternative<TypePtr>(def); }
    bool is_abstract() const { return std::holds_alternative<std::monostate>(def); }

    const std::vector<ConInfo>& constructors() const {
        return std::get<std::vector<ConInfo>>(def);
    }
    const TypePtr& synonym_body() const {
        return std::get<TypePtr>(def);
    }

    // TypeDef is move-only because ConInfo (and TypePtr) are move-only.
    TypeDef() = default;
    TypeDef(const TypeDef&)            = delete;
    TypeDef& operator=(const TypeDef&) = delete;
    TypeDef(TypeDef&&)                 = default;
    TypeDef& operator=(TypeDef&&)      = default;
};

// ---------------------------------------------------------------------------
// FuncDecl — a declared function type from `dec f : type;`
// ---------------------------------------------------------------------------

struct FuncDecl {
    std::vector<std::string> names;   // names covered by this declaration
    std::vector<std::string> params;  // universally-quantified type variables
    TypePtr                  type;    // the declared type (from AST)
    SourceLocation           loc;

    FuncDecl() = default;
    FuncDecl(const FuncDecl&)            = delete;
    FuncDecl& operator=(const FuncDecl&) = delete;
    FuncDecl(FuncDecl&&)                 = default;
    FuncDecl& operator=(FuncDecl&&)      = default;
};

// ---------------------------------------------------------------------------
// TypeEnv
// ---------------------------------------------------------------------------

class TypeEnv {
public:
    // Seed built-in types in the constructor.
    TypeEnv();

    // Register a new type definition.  Constructors within it are also indexed.
    void add_typedef(TypeDef def);

    // Register a function type declaration.
    void add_funcdecl(FuncDecl decl);

    // Look up a type by name.  Returns nullptr if not found.
    const TypeDef* lookup_type(const std::string& name) const;

    // Look up a constructor by name.  Returns nullptr if not found.
    const ConInfo* lookup_con(const std::string& name) const;

    // Look up a function declaration.  Returns nullptr if not found.
    const FuncDecl* lookup_func(const std::string& name) const;

    // Remove a function declaration (and all its aliases) from the env.
    // Used to hide private module functions after a module load completes.
    void remove_funcdecl(const std::string& name);

private:
    // TypeDef is heap-allocated so that ConInfo* pointers into its constructor
    // vector remain stable across unordered_map rehashing.
    std::unordered_map<std::string, std::unique_ptr<TypeDef>> typedefs_;
    std::unordered_map<std::string, const ConInfo*>           condecls_;
    std::unordered_map<std::string, std::unique_ptr<FuncDecl>> funcdecls_;
};

} // namespace hope
