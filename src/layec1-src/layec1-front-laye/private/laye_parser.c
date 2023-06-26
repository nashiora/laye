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

static void* ast_node_allocator(allocator_action action, void* memory, usize count)
{
    switch (action)
    {
        case KOS_ALLOC_ALLOCATE:
        {
            assert(count == sizeof(laye_ast_node));
            return default_allocator(action, memory, count);
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


static void laye_parser_check_file_headers(laye_parser *p, laye_ast* ast);

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
    
    laye_parser_check_file_headers(&parser, &result.ast);

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

    string fileSource = layec_context_get_file_source(p->context, p->fileId);
    return (layec_location){ .fileId = p->fileId, .offset = fileSource.count, .length = 0 };
}

static bool laye_parser_check(laye_parser* p, laye_token_kind kind)
{
    assert(p != nullptr);
    if (laye_parser_is_eof(p)) return false;
    
    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    return current->kind == kind;
}

static bool laye_parser_peek_check(laye_parser* p, laye_token_kind kind)
{
    assert(p != nullptr);
    if (p->currentTokenIndex + 1 >= arrlenu(p->tokens))
        return false;
    
    laye_token* current = p->tokens[p->currentTokenIndex + 1];
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

static laye_token* laye_parser_expect_identifier(laye_parser* p, const char* fmt)
{
    laye_token* identifierToken = nullptr;
    if (fmt == nullptr)
        fmt = "Identifier expected.";
    laye_parser_expect_out(p, LAYE_TOKEN_IDENTIFIER, fmt, &identifierToken);
    return identifierToken;
}

static laye_ast_import laye_parse_import_declaration(laye_parser* p, bool export)
{
    assert(p != nullptr);

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);
    assert(current->kind == LAYE_TOKEN_IMPORT);

    laye_parser_advance(p);

    laye_ast_import result = { 0 };
    result.export = export;

    if (laye_parser_check(p, LAYE_TOKEN_IDENTIFIER))
    {
        result.name = layec_intern_string_view(p->context, laye_parser_current(p)->atom);
        laye_parser_advance(p);
    }
    else
    {
        laye_token* stringToken = nullptr;
        laye_parser_expect_out(p, LAYE_TOKEN_LITERAL_STRING, "String literal expected as import library or file name.", &stringToken);
        result.name = layec_intern_string_view(p->context, stringToken->atom);
    }

    laye_parser_expect(p, ';', nullptr);
    return result;
}

static void laye_parser_check_file_headers(laye_parser *p, laye_ast* ast)
{
    while (!laye_parser_is_eof(p))
    {
        usize startIndex = p->currentTokenIndex;
        bool isExport = false;

        if (laye_parser_check(p, LAYE_TOKEN_EXPORT))
        {
            isExport = true;
            laye_parser_advance(p);
        }

        laye_token* current = laye_parser_current(p);
        assert(current != nullptr);

        switch (current->kind)
        {
            case LAYE_TOKEN_IMPORT:
            {
                laye_ast_import importData = laye_parse_import_declaration(p, isExport);
                arrput(ast->imports, importData);
            } continue;

            default:
            {
                p->currentTokenIndex = startIndex;
                return;
            }
        }
    }
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
            sizedIntType->primitiveType.size = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        case LAYE_TOKEN_UX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_UINT_SIZED, current->location);
            sizedIntType->primitiveType.size = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        case LAYE_TOKEN_BX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_BOOL_SIZED, current->location);
            sizedIntType->primitiveType.size = current->sizeParameter;
            *outTypeSyntax = sizedIntType;
            laye_parser_advance(p);
            return laye_parser_try_parse_type_suffix(p, outTypeSyntax, issueDiagnostics);
        }

        case LAYE_TOKEN_FX:
        {
            laye_ast_node* sizedIntType = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_FLOAT_SIZED, current->location);
            sizedIntType->primitiveType.size = current->sizeParameter;
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
            resultNode->literal.integerValue = current->integerValue;
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

static laye_ast_node* laye_parse_grouped_statement(laye_parser* p)
{
    assert(p != nullptr);
    assert(laye_parser_check(p, '{'));

    // TODO(local); parse grouped statements;
    TODO("parse grouped statements");
}

static laye_ast_node* laye_parse_statement(laye_parser* p)
{
    assert(p != nullptr);
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
    if (laye_parser_check(p, '('))
    {
        laye_parser_advance(p);

        list(laye_ast_node*) parameterBindingNodes = nullptr;
        while (!laye_parser_is_eof(p))
        {
            if (laye_parser_check(p, ')'))
                break;

            laye_ast_node* paramTypeSyntax = nullptr;
            laye_token* paramNameToken = nullptr;

            laye_ast_node* paramBinding = nullptr;

            if (laye_parser_check(p, LAYE_TOKEN_IDENTIFIER) && laye_parser_peek_check(p, ','))
            {
                paramNameToken = laye_parser_current(p);
                laye_parser_advance(p);

                paramTypeSyntax = laye_ast_node_alloc(LAYE_AST_NODE_TYPE_ERROR, paramNameToken->location);

                layec_issue_diagnostic(p->context, SEV_ERROR, paramNameToken->location, "Parameter type missing.");
            }
            else
            {
                bool typeParseResult = laye_parser_try_parse_type(p, &paramTypeSyntax, true);

                // if we pass `true` for diagnostic issuing, we should *always* get back a value, even if it's invalid.
                assert(typeParseResult == true);

                paramNameToken = laye_parser_expect_identifier(p, nullptr);
                laye_parser_advance(p);
            }
            
            assert(paramTypeSyntax != nullptr);
            assert(paramNameToken != nullptr);
            
            paramBinding = laye_ast_node_alloc(LAYE_AST_NODE_BINDING_DECLARATION, layec_location_combine(paramTypeSyntax->location, paramNameToken->location));
            paramBinding->bindingDeclaration.declaredType = paramTypeSyntax;
            paramBinding->bindingDeclaration.name = layec_intern_string_view(p->context, paramNameToken->atom);

            arrput(parameterBindingNodes, paramBinding);
            
            if (laye_parser_check(p, ','))
            {
                laye_parser_advance(p);
                continue;
            }
            else break;
        }

        laye_parser_expect(p, ')', nullptr);

        laye_ast_node* functionBody = nullptr;
        if (laye_parser_check(p, '{'))
            functionBody = laye_parse_grouped_statement(p);
        else if (laye_parser_check(p, LAYE_TOKEN_EQUAL_GREATER))
        {
            laye_parser_advance(p);
            functionBody = laye_parse_expression(p);
            laye_parser_expect(p, ';', nullptr);
        }
        else laye_parser_expect(p, ';', nullptr);

        laye_ast_node* functionDeclaration = laye_ast_node_alloc(LAYE_AST_NODE_FUNCTION_DECLARATION, name->location);
        functionDeclaration->functionDeclaration.modifiers = modifiers;
        functionDeclaration->functionDeclaration.returnType = declType;
        functionDeclaration->functionDeclaration.name = layec_intern_string_view(p->context, name->atom);

        return functionDeclaration;
    }

    laye_ast_node* bindingDeclaration = laye_ast_node_alloc(LAYE_AST_NODE_BINDING_DECLARATION, name->location);
    bindingDeclaration->bindingDeclaration.modifiers = modifiers;
    bindingDeclaration->bindingDeclaration.declaredType = declType;
    bindingDeclaration->bindingDeclaration.name = layec_intern_string_view(p->context, name->atom);

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
