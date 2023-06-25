#include <assert.h>
#include <stdio.h>

#include "token.h"
#include "ast.h"
#include "parser.h"

#define AST_ALLOCATOR ast_node_allocator
#define AST_ALLOCATOR_BLOCK_SIZE ((sizeof(laye_ast_node) * 1024) * (4 * 4))

typedef struct laye_parser
{
    layec_context* context;
    layec_fileid fileId;

    laye_token** tokens;
    usize currentTokenIndex;
} laye_parser;

static struct
{
    void* memory;
    usize byteCapacity;
    usize bytesConsumed;
} ast_node_allocator_state = { 0 };

static void* ast_node_allocator(allocator_action action, void* memory, usize count)
{
    switch (action)
    {
        case KOS_ALLOC_ALLOCATE:
        {
            assert(count == sizeof(laye_ast_node));
            if (ast_node_allocator_state.bytesConsumed + count >= ast_node_allocator_state.byteCapacity)
            {
                usize newCapacity = ast_node_allocator_state.byteCapacity + AST_ALLOCATOR_BLOCK_SIZE;
                void* newMemory = reallocate(default_allocator, ast_node_allocator_state.memory, newCapacity);
                assert(newMemory);
                ast_node_allocator_state.memory = newMemory;
                ast_node_allocator_state.byteCapacity = newCapacity;
            }

            void* resultAddress = (cast(char*) ast_node_allocator_state.memory) + ast_node_allocator_state.bytesConsumed;
            ast_node_allocator_state.bytesConsumed += count;

            return resultAddress;
        }

        case KOS_ALLOC_REALLOCATE: ICE("should never attempt to reallocate an AST node"); return nullptr;
        case KOS_ALLOC_DEALLOCATE: ICE("should never attempt to deallocate an AST node"); return nullptr;

        default: ICE("how did we get here"); return nullptr;
    }
}

static bool laye_parser_is_eof(laye_parser* p);
static void laye_parser_advance(laye_parser* p);
static laye_ast_node* laye_parse_top_level(laye_parser* p);

laye_ast_node* laye_ast_node_alloc(laye_ast_node_kind kind, layec_location location)
{
    laye_ast_node* result = allocate(AST_ALLOCATOR, sizeof(laye_ast_node));
    assert(result != nullptr);
    result->kind = kind;
    result->location = location;
    return result;
}

void laye_ast_node_dealloc(laye_ast_node* node)
{
    assert(node != nullptr);
    deallocate(AST_ALLOCATOR, node);
}

laye_parse_result laye_parse(layec_context* context, layec_fileid fileId)
{
    assert(context != nullptr);

    laye_parse_result result = { 0 };
    result.ast.fileId = fileId;

    laye_parser parser = {
        .context = context,
        .fileId = fileId,
    };
    
    parser.tokens = laye_lex(context, fileId);
    if (parser.tokens == nullptr)
    {
        result.status = LAYE_PARSE_FAILURE;
        return result;
    }

    while (!laye_parser_is_eof(&parser))
    {
        usize startIndex = parser.currentTokenIndex;

        laye_ast_node* node = laye_parse_top_level(&parser);
        if (node == nullptr)
        {
            result.status = LAYE_PARSE_FAILURE;
            return result;
        }

        assert(node != nullptr);
        arrput(result.ast.topLevelNodes, node);

        if (startIndex == parser.currentTokenIndex)
        {
            laye_parser_advance(&parser);
            // TODO(local): should the parser functions handle this? I never know for sure.
        }
    }

    return result;
}

static bool laye_parser_is_eof(laye_parser* p)
{
    assert(p != nullptr);
    return p->currentTokenIndex >= arrlenu(p->tokens);
}

static laye_ast_node* laye_parse_declaration_or_statement(laye_parser* p);

static laye_ast_node* laye_parse_top_level(laye_parser* p)
{
    assert(p != nullptr);
    if (laye_parser_is_eof(p)) return nullptr;

    laye_ast_node* topLevelStatement = laye_parse_declaration_or_statement(p);
    return topLevelStatement;
}

static void laye_parser_advance(laye_parser* p)
{
    assert(p != nullptr);
    if (laye_parser_is_eof(p)) return;
    p->currentTokenIndex++;
}

static laye_token* laye_parser_current(laye_parser* p)
{
    assert(p != nullptr);
    assert(p->currentTokenIndex < arrlenu(p->tokens));

    laye_token* resultToken = p->tokens[p->currentTokenIndex];
    assert(resultToken);
    return resultToken;
}

static layec_location laye_parser_eof_location(laye_parser* p)
{
    assert(p != nullptr);
    
    usize offset = 0;
    if (arrlenu(p->tokens) > 0)
    {
        laye_token* lastToken = p->tokens[arrlenu(p->tokens)];
        offset = lastToken->location.offset + lastToken->location.length;
    }

    return (layec_location){ .fileId = p->fileId, .offset = offset, .length = 0 };
}

static bool laye_parser_check(laye_parser* p, laye_token_kind kind)
{
    assert(p != nullptr);
    if (laye_parser_is_eof(p)) return false;
    
    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    return current->kind == kind;
}

static void laye_parser_expect_out(laye_parser* p, laye_token_kind kind, const char* fmt, laye_token** outToken)
{
    assert(p != nullptr);

    if (laye_parser_is_eof(p))
    {
        if (fmt == nullptr)
        {
            fmt = "'%c' expected at end of file.";
        }

        layec_location endLocation = laye_parser_eof_location(p);
        layec_issue_diagnostic(p->context, SEV_ERROR, endLocation, fmt, cast(char) kind);

        if (outToken != nullptr)
        {
            *outToken = laye_token_alloc(kind, endLocation, STRING_VIEW_LITERAL("<invalid>"));
        }

        return;
    }

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    if (current->kind != kind)
    {
        if (fmt == nullptr)
        {
            fmt = "'%c' expected.";
        }

        layec_issue_diagnostic(p->context, SEV_ERROR, current->location, fmt, cast(char) kind);
        
        if (outToken != nullptr)
        {
            *outToken = laye_token_alloc(kind, current->location, STRING_VIEW_LITERAL("<invalid>"));
        }

        return;
    }

    if (outToken != nullptr)
    {
        *outToken = current;
    }

    laye_parser_advance(p);
}

static void laye_parser_expect(laye_parser* p, laye_token_kind kind, const char* fmt)
{
    laye_parser_expect_out(p, kind, fmt, nullptr);
}

/// @brief Attempts to parse a type suffix.
/// @param p The parser to use to parse a type suffix.
/// @param typeSyntax The input base type syntax, and where to put the result type syntax node.
/// @param issueDiagnostics True if diagnostics should be issued without rewinding the token stream, false if no diagnostics should be issued and the token stream rewound on error.
/// @return True if outTypeSyntax has been populated with a type syntax node, false otherwise.
static bool laye_parser_try_parse_type_suffix(laye_parser* p, laye_ast_node** typeSyntax, bool issueDiagnostics)
{
    assert(p != nullptr);
    assert( typeSyntax != nullptr);
    assert(*typeSyntax != nullptr);
    
    if (laye_parser_is_eof(p))
        return true;

    usize startIndex = p->currentTokenIndex;

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    // TODO(local): how will we free syntax nodes?
    switch (current->kind)
    {
        default: break;
    }

    return true;
}

/// @brief Attempts to parse a type.
/// @param p The parser to use to parse a type.
/// @param outTypeSyntax Where to put the result type syntax node.
/// @param issueDiagnostics True if diagnostics should be issued without rewinding the token stream, false if no diagnostics should be issued and the token stream rewound on error.
/// @return True if outTypeSyntax has been populated with a type syntax node, false otherwise.
static bool laye_parser_try_parse_type(laye_parser* p, laye_ast_node** outTypeSyntax, bool issueDiagnostics)
{
    assert(p != nullptr);
    assert(outTypeSyntax != nullptr);

    *outTypeSyntax = nullptr;

    usize startIndex = p->currentTokenIndex;

    if (laye_parser_is_eof(p))
    {
        if (issueDiagnostics)
        {
            layec_location endLocation = laye_parser_eof_location(p);
            layec_issue_diagnostic(p->context, SEV_ERROR, endLocation, "Unexpected end of file reached when parsing a type.");
            *outTypeSyntax = laye_ast_node_alloc(LAYE_AST_NODE_INVALID, endLocation);
            return true;
        }

        p->currentTokenIndex = startIndex;
        return false;
    }

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    switch (current->kind)
    {
        case LAYE_TOKEN_IX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_INT_SIZED, current->location);
            sizedIntType->typeSizeParameter = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        case LAYE_TOKEN_UX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_UINT_SIZED, current->location);
            sizedIntType->typeSizeParameter = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        case LAYE_TOKEN_BX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_BOOL_SIZED, current->location);
            sizedIntType->typeSizeParameter = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        case LAYE_TOKEN_FX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_FLOAT_SIZED, current->location);
            sizedIntType->typeSizeParameter = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        default:
        {
            if (issueDiagnostics)
            {
                layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Identifier expected when parsing a type.");
                *outTypeSyntax = laye_ast_node_alloc(LAYE_AST_NODE_INVALID, current->location);
                laye_parser_advance(p);
                return true;
            }

            p->currentTokenIndex = startIndex;
            return false;
        }
    }
}

static laye_ast_node* laye_parse_primary_suffix(laye_parser* p, laye_ast_node* expression)
{
    assert(p != nullptr);
    assert(expression != nullptr);

    return expression;
}

static laye_ast_node* laye_parse_primary(laye_parser* p)
{
    assert(p != nullptr);

    if (laye_parser_is_eof(p))
    {
        layec_location endLocation = laye_parser_eof_location(p);
        layec_issue_diagnostic(p->context, SEV_ERROR, endLocation, "End of file reached when parsing primary expression.");
        return laye_ast_node_alloc(LAYE_AST_NODE_INVALID, endLocation);
    }

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    switch (current->kind)
    {
        case LAYE_TOKEN_LITERAL_INTEGER:
        {
            laye_ast_node* resultNode = laye_ast_node_alloc(LAYE_AST_NODE_EXPRESSION_INTEGER, current->location);
            assert(resultNode != nullptr);
            resultNode->literal = current;
            laye_parser_advance(p);
            return laye_parse_primary_suffix(p, resultNode);
        }

        default:
        {
            layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Unexpected token when parsing expression.");
            return laye_ast_node_alloc(LAYE_AST_NODE_INVALID, current->location);
        }
    }
}

static laye_ast_node* laye_parse_secondary(laye_parser* p, laye_ast_node* left)
{
    return left;
}

static laye_ast_node* laye_parse_expression(laye_parser* p)
{
    assert(p != nullptr);

    laye_ast_node* primary = laye_parse_primary(p);
    assert(primary != nullptr);

    laye_ast_node* secondary = laye_parse_secondary(p, primary);
    assert(secondary != nullptr);

    return secondary;
}

static laye_ast_node* laye_parse_statement(laye_parser* p)
{
    // TODO(local): parse non-expression statements

    laye_ast_node* expressionStatement = laye_parse_expression(p);
    assert(expressionStatement != nullptr);

    laye_parser_expect(p, LAYE_TOKEN_SEMI_COLON, nullptr);
    return expressionStatement;
}

static laye_ast_node* laye_parse_declaration_continue(laye_parser* p, list(laye_ast_modifier) modifiers, laye_ast_node* declType, laye_token* name)
{
    assert(p != nullptr);
    assert(declType != nullptr);
    assert(name != nullptr);

    // TODO(local): parse functions

    laye_ast_node* bindingDeclaration = laye_ast_node_alloc(LAYE_AST_NODE_BINDING_DECLARATION, name->location);
    bindingDeclaration->bindingDeclaration.modifiers = modifiers;
    bindingDeclaration->bindingDeclaration.declaredType = declType;
    bindingDeclaration->bindingDeclaration.nameToken = name;

    laye_parser_expect(p, LAYE_TOKEN_SEMI_COLON, nullptr);
    return bindingDeclaration;
}

static laye_ast_node* laye_parse_declaration_or_statement(laye_parser* p)
{
    assert(p != nullptr);
    // TODO(local): report EOF error and return EOF node?
    assert(!laye_parser_is_eof(p));

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    list(laye_ast_modifier) modifiers = nullptr;
    bool appliedModifiers[LAYE_AST_MODIFIER_COUNT] = { 0 };

    while (!laye_parser_is_eof(p))
    {
        current = laye_parser_current(p);
        switch (current->kind)
        {
#define MODIFIER_CASE(K, M) \
            case K: \
            { \
                if (appliedModifiers[M]) { \
                    layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Duplicate modifier."); \
                } \
                appliedModifiers[M] = true; \
                laye_ast_modifier modifier = (laye_ast_modifier){ .kind = M, .location = current->location }; \
                arrput(modifiers, modifier); \
                laye_parser_advance(p); \
            } break
            MODIFIER_CASE(LAYE_TOKEN_EXPORT, LAYE_AST_MODIFIER_EXPORT);
            MODIFIER_CASE(LAYE_TOKEN_NORETURN, LAYE_AST_MODIFIER_NORETURN);
            MODIFIER_CASE(LAYE_TOKEN_INLINE, LAYE_AST_MODIFIER_INLINE);
#undef MODIFIER_CASE

            case LAYE_TOKEN_CALLCONV:
            {
                // TODO(local): parse calling convention expression
                TODO("parse calling convention expression");

                if (appliedModifiers[LAYE_AST_MODIFIER_CALLCONV]) {
                    layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Duplicate modifier.");
                }
                appliedModifiers[LAYE_AST_MODIFIER_CALLCONV] = true;

                laye_ast_modifier callconvModifier = (laye_ast_modifier){ .kind = LAYE_AST_MODIFIER_CALLCONV, .location = current->location };
                arrput(modifiers, callconvModifier);
                laye_parser_advance(p);
            } break;

            default: goto after_modifier_parse;
        }
    }

after_modifier_parse:;
    switch (current->kind)
    {
        default:
        {
            usize startIndex = p->currentTokenIndex;

            laye_ast_node* typeSyntax = nullptr;
            if (laye_parser_try_parse_type(p, &typeSyntax, false))
            {
                assert(typeSyntax != nullptr);

                current = laye_parser_current(p);
                assert(current != nullptr);

                // TODO(local): how will we free syntax nodes?
                if (!laye_parser_check(p, LAYE_TOKEN_IDENTIFIER))
                    goto parse_decl_failed;

                laye_token* declNameToken = current;
                laye_parser_advance(p);

                return laye_parse_declaration_continue(p, modifiers, typeSyntax, declNameToken);
            }

        parse_decl_failed:;
            p->currentTokenIndex = startIndex;

            if (arrlenu(modifiers) > 0)
            {
                layec_location modifiersLocation = modifiers[0].location;
                layec_issue_diagnostic(p->context, SEV_ERROR, modifiersLocation, "Can only apply modifiers to declarations.");
            }

            laye_ast_node* statement = laye_parse_statement(p);
            assert(statement != nullptr);

            return statement;
        }
    }
}
