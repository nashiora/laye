#ifndef AST_H
#define AST_H

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
    laye_token* nameToken;
    list(laye_ast_node*) fieldBindings;
} laye_ast_enum_variant;

struct laye_ast_node
{
    laye_ast_node_kind kind;
    layec_location location;

    union
    {
        laye_token* literal;
        int typeSizeParameter;

        union
        {
            list(laye_ast_modifier) modifiers;
            laye_ast_node* declaredType;
            laye_token* nameToken;
            laye_ast_node* initialValue;
        } bindingDeclaration;

        union
        {
            list(laye_ast_modifier) modifiers;
            laye_ast_node* returnType;
            laye_token* nameToken;
            list(laye_ast_node*) parameterBindings;
            laye_ast_node* body;
        } functionDeclaration;

        union
        {
            list(laye_ast_modifier) modifiers;
            laye_token* nameToken;
            list(laye_ast_node*) fieldBindings;
        } structDeclaration;

        union
        {
            list(laye_ast_modifier) modifiers;
            laye_token* nameToken;
            list(laye_ast_enum_variant) variants;
        } enumDeclaration;
    };
};

typedef struct laye_ast
{
    layec_fileid fileId;
    // TODO(local): modules/imports
    list(laye_ast_node*) topLevelNodes;
} laye_ast;

laye_ast_node* laye_ast_node_alloc(laye_ast_node_kind kind, layec_location location);
void laye_ast_node_dealloc(laye_ast_node* node);

#endif // AST_H
