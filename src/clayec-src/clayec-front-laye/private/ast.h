#ifndef AST_H
#define AST_H

#include <stdio.h>

#include "kos/kos.h"

#include "token.h"

#define LAYE_SYNTAX_ALLOCATOR default_allocator

#define LAYE_AST_TYPE_KINDS \
    T(ERROR) \
    T(IMPLICIT) \
    T(STRUCT) \
    T(ENUM) \
    T(UNION) \
    T(FUNCTION) \
    T(ARRAY) \
    T(SLICE) \
    T(POINTER) \
    T(BUFFER) \
    T(LITERAL_BOOL) \
    T(LITERAL_STRING) \
    T(LITERAL_INT) \
    T(LITERAL_REAL) \
    T(RAWPTR) \
    T(VOID) \
    T(STRING) \
    T(CSTRING) \
    T(RUNE) \
    T(BOOL) \
    T(BOOL_SIZED) \
    T(INT) \
    T(INT_SIZED) \
    T(UINT) \
    T(UINT_SIZED) \
    T(FLOAT) \
    T(FLOAT_SIZED)

#define LAYE_AST_NODE_KINDS \
    A(BINDING_DECLARATION) \
    A(FUNCTION_DECLARATION) \
    A(STRUCT_DECLARATION) \
    A(ENUM_DECLARATION) \
    A(MODULE_DECLARATION) \
    A(NAMESPACE_DECLARATION) \
    A(MODULE_IMPORT_DECLARATION) \
    A(FOREIGN_IMPORT_DECLARATION) \
    A(FOREIGN_BLOCK) \
    A(NAMESPACE_BLOCK) \
    A(STATEMENT_BLOCK) \
    A(STATEMENT_ASSIGNMENT) \
    A(STATEMENT_IF) \
    A(STATEMENT_FOR) \
    A(STATEMENT_WHILE) \
    A(STATEMENT_SWITCH) \
    A(STATEMENT_RETURN) \
    A(STATEMENT_CONTINUE) \
    A(STATEMENT_BREAK) \
    A(STATEMENT_YIELD) \
    A(EXPRESSION_UNARY) \
    A(EXPRESSION_BINARY) \
    A(EXPRESSION_INVOKE) \
    A(EXPRESSION_TYPECAST) \
    A(EXPRESSION_LOOKUP) \
    A(EXPRESSION_NAMESPACE_RESOLVE) \
    A(EXPRESSION_NULLPTR) \
    A(EXPRESSION_TRUE) \
    A(EXPRESSION_FALSE) \
    A(EXPRESSION_STRING) \
    A(EXPRESSION_INTEGER) \
    T(NAMED) \
    LAYE_AST_TYPE_KINDS

typedef enum laye_ast_node_kind
{
    LAYE_AST_NODE_INVALID,
    
#define A(E) LAYE_AST_NODE_ ## E,
#define T(E) LAYE_AST_NODE_TYPE_ ## E,
LAYE_AST_NODE_KINDS
#undef T
#undef A
} laye_ast_node_kind;

typedef enum laye_ast_type_access
{
    LAYE_AST_ACCESS_NONE,
    LAYE_AST_ACCESS_READONLY,
    LAYE_AST_ACCESS_WRITEONLY,
} laye_ast_type_access;

typedef enum laye_ast_modifier_kind
{
    LAYE_AST_MODIFIER_INVALID,
    LAYE_AST_MODIFIER_EXPORT,
    LAYE_AST_MODIFIER_CALLCONV,
    LAYE_AST_MODIFIER_NORETURN,
    LAYE_AST_MODIFIER_INLINE,
    LAYE_AST_MODIFIER_COUNT,
} laye_ast_modifier_kind;

typedef struct laye_ast_node laye_ast_node;

typedef struct laye_ast_modifier
{
    laye_ast_modifier_kind kind;
    layec_location location;
    union
    {
        laye_ast_node* callingConventionKind;
    };
} laye_ast_modifier;

typedef struct laye_ast_enum_variant
{
    string name;
    list(laye_ast_node*) fieldBindings;
} laye_ast_enum_variant;

typedef struct laye_ast_import
{
    layec_location location;

    // could be sourced from an identifier or a string.
    string name;
    // used to rename the inferred namespace, or to define one where one cannot be inferred otherwise.
    // e.g. `import "std";` creates the namespace `std`, as does `import "my fun library" as std;`.
    string alias;

    // `import * from "std";`
    bool allMembers;
    // `import foo, bar from "std";`
    list(string) explicitMembers;

    // True if this import should also be publicly exported, false otherwise.
    // e.g. `export import "sublib.laye";`
    bool export;
} laye_ast_import;

struct laye_ast_node
{
    laye_ast_node_kind kind;
    layec_location location;

    union
    {
        union
        {
            // for primitive container types like strings
            laye_ast_type_access access;
            // for arbitrary-sized primitive types like i32
            int size;
        } primitiveType;

        // for container types like pointers or arrays
        struct
        {
            laye_ast_type_access access;
            laye_ast_node* elementType;
            // array dimension expressions
            list(laye_ast_node*) ranks;
        } containerType;

        struct
        {
            list(laye_ast_modifier) modifiers;
            laye_ast_node* declaredType;
            string name;
            laye_ast_node* initialValue;
        } bindingDeclaration;

        struct
        {
            list(laye_ast_modifier) modifiers;
            laye_ast_node* returnType;
            string name;
            list(laye_ast_node*) parameterBindings;
            laye_ast_node* body;
        } functionDeclaration;

        struct
        {
            list(laye_ast_modifier) modifiers;
            string name;
            list(laye_ast_node*) fieldBindings;
        } structDeclaration;

        struct
        {
            list(laye_ast_modifier) modifiers;
            string name;
            list(laye_ast_enum_variant) variants;
        } enumDeclaration;

        struct
        {
            laye_ast_node* target;
            laye_ast_node* value;
        } assignment;

        list(laye_ast_node*) statements;
        laye_ast_node* returnValue;

        string lookupName;

        union
        {
            u64 integerValue;
            string stringValue;
        } literal;
        
        union
        {
            laye_ast_node* target;
            list(laye_ast_node*) arguments;
        } invoke;
    };
};

typedef struct laye_ast
{
    layec_fileid fileId;
    list(laye_ast_import) imports;
    list(laye_ast_node*) topLevelNodes;
} laye_ast;

string laye_ast_node_type_to_string(laye_ast_node* typeNode);

string_view laye_ast_node_kind_name(laye_ast_node_kind kind);
void laye_ast_fprint(FILE* stream, layec_context* context, laye_ast* ast, bool colors);

#endif // AST_H