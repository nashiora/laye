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

    arena_allocator* tokenArena;
    arena_allocator* astArena;

    laye_token** tokens;
    usize currentTokenIndex;
} laye_parser;

typedef struct laye_operator_info
{
    laye_token_kind tokenKind;
    int precendence;
} laye_operator_info;

static laye_operator_info layeOperatorInfos[] = {
    { LAYE_TOKEN_OR,  5 },
    { LAYE_TOKEN_XOR,  5 },
    { LAYE_TOKEN_AND,  6 },

    { LAYE_TOKEN_EQUAL_EQUAL,  10 },
    { LAYE_TOKEN_BANG_EQUAL,  10 },

    { '<',  20 },
    { '>',  20 },
    { LAYE_TOKEN_LESS_EQUAL,  20 },
    { LAYE_TOKEN_GREATER_EQUAL,  20 },

    { '&',  30 },
    { '|',  30 },
    { '~',  30 },
    { LAYE_TOKEN_LESS_LESS,  30 },
    { LAYE_TOKEN_GREATER_GREATER_EQUAL,  30 },

    { '+', 40 },
    { '-', 40 },

    { '*', 50 },
    { '/', 50 },
    { '%', 50 },
};

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

static laye_ast_node* laye_ast_node_alloc(laye_parser* p, laye_ast_node_kind kind, layec_location location)
{
    laye_ast_node* result = arena_push(p->astArena, sizeof(laye_ast_node));
    assert(result != nullptr);
    result->kind = kind;
    result->location = location;
    return result;
}

static void laye_parser_read_file_headers(laye_parser *p, laye_ast* ast);

laye_parse_result laye_parse(layec_context* context, layec_fileid fileId)
{
    assert(context != nullptr);

    laye_parse_result result = { 0 };
    result.ast.fileId = fileId;

    laye_parser parser = {
        .context = context,
        .fileId = fileId,
        .tokenArena = arena_create(default_allocator, 10 * 1024),
        .astArena = arena_create(default_allocator, 10 * 1024),
    };

    parser.tokens = laye_lex(context, fileId, parser.tokenArena);
    if (parser.tokens == nullptr)
    {
        result.status = LAYE_PARSE_FAILURE;
        return result;
    }
    
    laye_parser_read_file_headers(&parser, &result.ast);

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

    arena_destroy(parser.tokenArena);
    parser.tokenArena = nullptr;

    result.astArena = parser.astArena;
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

static layec_location laye_parser_most_recent_location(laye_parser* p)
{
    if (laye_parser_is_eof(p))
        return laye_parser_eof_location(p);
    
    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    return current->location;
}

static bool laye_parser_check(laye_parser* p, laye_token_kind kind)
{
    assert(p != nullptr);
    if (laye_parser_is_eof(p)) return false;
    
    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    return current->kind == kind;
}

static bool laye_parser_check_conditional_keyword(laye_parser* p, const char* keyword)
{
    assert(p != nullptr);

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    if (current->kind != LAYE_TOKEN_IDENTIFIER || strlen(keyword) != current->location.length)
    {
        return false;
    }

    string sourceText = layec_context_get_file_source(p->context, current->location.fileId);
    return 0 == strncmp(keyword, cast(const char*) (sourceText.memory + current->location.offset), current->location.length);
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
            laye_token* newToken = arena_push(p->tokenArena, sizeof(laye_token));
            newToken->kind = kind;
            newToken->location = endLocation;

            *outToken = newToken;
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
            laye_token* newToken = arena_push(p->tokenArena, sizeof(laye_token));
            newToken->kind = kind;
            newToken->location = current->location;
            newToken->location.length = 0;

            *outToken = newToken;
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

    laye_ast_import result = { 0 };
    result.location = current->location;
    result.export = export;

    laye_parser_advance(p);

    if (laye_parser_check(p, LAYE_TOKEN_IDENTIFIER))
    {
        current = laye_parser_current(p);
        assert(current != nullptr);
        result.name = layec_intern_location_text(p->context, current->location);
        laye_parser_advance(p);
    }
    else
    {
        laye_token* stringToken = nullptr;
        laye_parser_expect_out(p, LAYE_TOKEN_LITERAL_STRING, "String literal expected as import library or file name.", &stringToken);
        assert(stringToken != nullptr);
        assert(stringToken->stringValue.count > 0);
        result.name = layec_intern_string_view(p->context, string_slice(stringToken->stringValue, 0, stringToken->stringValue.count));
    }

    if (laye_parser_check_conditional_keyword(p, "as"))
    {
        laye_parser_advance(p);
        laye_token* aliasToken = laye_parser_expect_identifier(p, nullptr);
        assert(aliasToken != nullptr);
        result.alias = layec_intern_location_text(p->context, aliasToken->location);
    }

    laye_parser_expect(p, ';', nullptr);
    return result;
}

static void laye_parser_read_file_headers(laye_parser *p, laye_ast* ast)
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

static laye_ast_node* laye_parse_expression(laye_parser* p);
static bool laye_parser_try_parse_type(laye_parser* p, laye_ast_node** outTypeSyntax, bool issueDiagnostics);

static list(laye_ast_template_argument) laye_parse_template_arguments(laye_parser* p)
{
    assert(p != nullptr);

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);
    assert(current->kind == '<', "when calling laye_parse_template_arguments, the current token must be '<'");

    list(laye_ast_template_argument) result = nullptr;

    laye_parser_advance(p);

    while (!laye_parser_is_eof(p) && !laye_parser_check(p, '>'))
    {
        laye_ast_node* maybeTypeNode = nullptr;
        bool typeSuccess = laye_parser_try_parse_type(p, &maybeTypeNode, false);
        if (typeSuccess)
        {
            assert(maybeTypeNode != nullptr);
            laye_ast_template_argument arg = {
                .kind = LAYE_TEMPLATE_ARG_TYPE,
                .value = maybeTypeNode,
            };
            arrput(result, arg);
        }
        else
        {
            laye_ast_node* expression = laye_parse_expression(p);
            assert(expression != nullptr);
            laye_ast_template_argument arg = {
                .kind = LAYE_TEMPLATE_ARG_VALUE,
                .value = expression,
            };
            arrput(result, arg);
        }

        if (!laye_parser_check(p, ','))
            break;

        laye_parser_advance(p);
        if (!laye_parser_check(p, '>'))
            continue;

        layec_issue_diagnostic(p->context, SEV_ERROR, laye_parser_current(p)->location, "Type parameter expected");
        break;
    }

    laye_token* closingToken = nullptr;
    laye_parser_expect_out(p, '>', nullptr, &closingToken);

    if (arrlenu(result) == 0)
    {
        layec_issue_diagnostic(p->context, SEV_ERROR, closingToken->location, "Template argument lists cannot be empty");
    }

    return result;
}

/// @brief Attempts to parse a type suffix.
/// @param p The parser to use to parse a type suffix.
/// @param typeSyntax The input base type syntax, and where to put the result type syntax node.
/// @param issueDiagnostics True if diagnostics should be issued without rewinding the token stream, false if no diagnostics should be issued and the token stream rewound on error.
/// @return True if outTypeSyntax has been populated with a type syntax node, false otherwise.
static bool laye_parser_try_parse_type_suffix(laye_parser* p, usize startIndex, laye_ast_node** typeSyntax, bool issueDiagnostics)
{
    assert(p != nullptr);
    assert( typeSyntax != nullptr);
    assert(*typeSyntax != nullptr);
    
    if (laye_parser_is_eof(p))
        return true;

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);
    
    layec_location startLocation = current->location;

    laye_ast_type_access access = LAYE_AST_ACCESS_NONE;
    if (laye_parser_check(p, LAYE_TOKEN_READONLY))
    {
        laye_parser_advance(p);
        access = LAYE_AST_ACCESS_READONLY;
    }
    else if (laye_parser_check(p, LAYE_TOKEN_WRITEONLY))
    {
        laye_parser_advance(p);
        access = LAYE_AST_ACCESS_WRITEONLY;
    }

    bool isNilable = false;
    switch (current->kind)
    {
        case '*':
        {
            layec_location typeLocation = layec_location_combine(startLocation, current->location);
            laye_parser_advance(p);

            if (laye_parser_check(p, '?'))
            {
                laye_parser_advance(p);
                isNilable = true;
            }

            laye_ast_node* newType = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_POINTER, typeLocation);
            newType->containerType.elementType = *typeSyntax;
            newType->containerType.access = access;
            newType->containerType.isNilable = isNilable;
            *typeSyntax = newType;
            return laye_parser_try_parse_type_suffix(p, startIndex, typeSyntax, issueDiagnostics);
        } break;

        case '[':
        {
            laye_parser_advance(p);
            layec_location afterOpenLocation = laye_parser_most_recent_location(p);

            // TODO(local): parse array types
            if (!laye_parser_check(p, ']'))
            {
                if (laye_parser_check(p, '*') && laye_parser_peek_check(p, ']'))
                {
                    laye_parser_advance(p);
                    current = laye_parser_current(p);
                    laye_parser_advance(p); // ']'

                    if (laye_parser_check(p, '?'))
                    {
                        laye_parser_advance(p);
                        isNilable = true;
                    }
                    
                    layec_location typeLocation = layec_location_combine(startLocation, current->location);
                    laye_ast_node* newType = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_BUFFER, typeLocation);
                    newType->containerType.elementType = *typeSyntax;
                    newType->containerType.access = access;
                    newType->containerType.isNilable = isNilable;
                    *typeSyntax = newType;
                    return laye_parser_try_parse_type_suffix(p, startIndex, typeSyntax, issueDiagnostics);
                }
                else
                {
                    list(laye_ast_node*) ranks = nullptr;
                    while (!laye_parser_is_eof(p) && !laye_parser_check(p, ']'))
                    {
                        laye_ast_node* rank = laye_parse_expression(p);
                        assert(rank != nullptr);
                        arrput(ranks, rank);

                        if (!laye_parser_check(p, ','))
                            break;
                        
                        laye_parser_advance(p);
                        if (!laye_parser_check(p, ']'))
                            continue;
                            
                        if (issueDiagnostics)
                        {
                            layec_issue_diagnostic(p->context, SEV_ERROR, afterOpenLocation, "Expected ']' or constant expression for array type rank.");
                            break;
                        }
                        else
                        {
                            p->currentTokenIndex = startIndex;
                            return false;
                        }
                    }

                    laye_parser_expect(p, ']', nullptr);

                    if (laye_parser_check(p, '?'))
                    {
                        laye_parser_advance(p);
                        isNilable = true;
                    }

                    layec_location typeLocation = layec_location_combine(startLocation, current->location);
                    laye_ast_node* newType = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_ARRAY, typeLocation);
                    newType->containerType.elementType = *typeSyntax;
                    newType->containerType.ranks = ranks;
                    newType->containerType.access = access;
                    newType->containerType.isNilable = isNilable;
                    *typeSyntax = newType;
                    return laye_parser_try_parse_type_suffix(p, startIndex, typeSyntax, issueDiagnostics);
                }

                if (issueDiagnostics)
                {
                    layec_issue_diagnostic(p->context, SEV_ERROR, afterOpenLocation, "Expected ']'.");
                    *typeSyntax = laye_ast_node_alloc(p, LAYE_AST_NODE_INVALID, afterOpenLocation);
                    return true;
                }
                
                p->currentTokenIndex = startIndex;
                return false;
            }

            current = laye_parser_current(p);
            laye_parser_advance(p); // ']'

            if (laye_parser_check(p, '?'))
            {
                laye_parser_advance(p);
                isNilable = true;
            }

            layec_location typeLocation = layec_location_combine(startLocation, current->location);
            laye_ast_node* newType = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_SLICE, typeLocation);
            newType->containerType.elementType = *typeSyntax;
            newType->containerType.access = access;
            newType->containerType.isNilable = isNilable;
            *typeSyntax = newType;
            return laye_parser_try_parse_type_suffix(p, startIndex, typeSyntax, issueDiagnostics);
        } break;

        case '(':
        {
            if (access != LAYE_AST_ACCESS_NONE && issueDiagnostics)
            {
                layec_issue_diagnostic(p->context, SEV_ERROR, startLocation, "Access specifier must be followed by '*' or '[' to construct a container type.");
            }

            laye_parser_advance(p);

            list(laye_ast_node*) parameterTypes = nullptr;
            bool endsWithComma = false;
            while (!laye_parser_is_eof(p) && !laye_parser_check(p, ')'))
            {
                endsWithComma = false;
                if (laye_parser_check(p, ','))
                {
                    layec_issue_diagnostic(p->context, SEV_ERROR, laye_parser_current(p)->location, "Type expected as function type parameter.");
                    laye_parser_advance(p);
                    continue;
                }

                laye_ast_node* parameterType = nullptr;
                bool success = laye_parser_try_parse_type(p, &parameterType, issueDiagnostics);

                if (!issueDiagnostics && !success)
                {
                    p->currentTokenIndex = startIndex;
                    return false;
                }
                // else assume we already issued sufficient diagnostics

                arrput(parameterTypes, parameterType);

                if (!laye_parser_check(p, ','))
                    break;
                laye_parser_advance(p);
                endsWithComma = true;
            }

            current = laye_parser_current(p);
            laye_parser_expect(p, ')', nullptr);

            if (endsWithComma)
            {
                if (issueDiagnostics)
                {
                    layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Type expected as function type parameter.");
                }
                else
                {
                    p->currentTokenIndex = startIndex;
                    return false;
                }
            }

            if (laye_parser_check(p, '?'))
            {
                laye_parser_advance(p);
                isNilable = true;
            }

            layec_location typeLocation = layec_location_combine(startLocation, current->location);
            laye_ast_node* newType = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_FUNCTION, typeLocation);
            newType->functionType.returnType = *typeSyntax;
            newType->functionType.parameterTypes = parameterTypes;
            newType->functionType.isNilable = isNilable;
            *typeSyntax = newType;
            return laye_parser_try_parse_type_suffix(p, startIndex, typeSyntax, issueDiagnostics);
        } break;

        default:
            if (access != LAYE_AST_ACCESS_NONE)
            {
                if (issueDiagnostics)
                {
                    layec_issue_diagnostic(p->context, SEV_ERROR, startLocation, "Access specifier must be followed by '*' or '[' to construct a container type.");
                    *typeSyntax = laye_ast_node_alloc(p, LAYE_AST_NODE_INVALID, startLocation);
                    return true;
                }

                p->currentTokenIndex = startIndex;
                return false;
            }
            break;
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
            *outTypeSyntax = laye_ast_node_alloc(p, LAYE_AST_NODE_INVALID, endLocation);
            return true;
        }

        p->currentTokenIndex = startIndex;
        return false;
    }

    laye_ast_type_access access = LAYE_AST_ACCESS_NONE;
    if (laye_parser_check(p, LAYE_TOKEN_READONLY))
    {
        laye_parser_advance(p);
        access = LAYE_AST_ACCESS_READONLY;
    }
    else if (laye_parser_check(p, LAYE_TOKEN_WRITEONLY))
    {
        laye_parser_advance(p);
        access = LAYE_AST_ACCESS_WRITEONLY;
    }

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    layec_location startLocation = current->location;

    #define WORD_TYPE(TK, TY, SX) \
        case LAYE_TOKEN_ ## TK: \
        { \
            laye_ast_node* type = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_ ## TY, current->location); \
            if (access != LAYE_AST_ACCESS_NONE && LAYE_TOKEN_ ## TK != LAYE_TOKEN_STRING && LAYE_TOKEN_ ## TK != LAYE_TOKEN_C_STRING) \
            { \
                layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Type cannot be readonly/writeonly."); \
            } else type->primitiveType.access = access; \
            if (SX) type->primitiveType.size = current->sizeParameter; \
            if (laye_parser_check(p, '?')) { \
                laye_parser_advance(p); \
                type->primitiveType.isNilable = isNilable; \
            } \
            *outTypeSyntax = type; \
            laye_parser_advance(p); \
            return laye_parser_try_parse_type_suffix(p, startIndex, outTypeSyntax, issueDiagnostics); \
        }

    list(string) path = nullptr;
    string identifierName = { 0 };
    bool isPathHeadless = false;
    bool isNilable = false;
    switch (current->kind)
    {
        case LAYE_TOKEN_COLON_COLON:
            isPathHeadless = true;
            goto start_path_resolution_parse;
        case LAYE_TOKEN_IDENTIFIER:
        {
            identifierName = layec_intern_location_text(p->context, current->location);
            laye_parser_advance(p);
            arrput(path, identifierName);
            
            if (laye_parser_check(p, LAYE_TOKEN_COLON_COLON))
            {
            start_path_resolution_parse:;
                layec_location lastIdentifierLocation = { 0 };
                while (laye_parser_check(p, LAYE_TOKEN_COLON_COLON))
                {
                    laye_parser_advance(p);
                    laye_token* nextIdent = laye_parser_expect_identifier(p, nullptr);
                    assert(nextIdent != nullptr);
                    lastIdentifierLocation = nextIdent->location;
                    string nextName = layec_intern_location_text(p->context, nextIdent->location);
                    startLocation = layec_location_combine(startLocation, nextIdent->location);
                    arrput(path, nextName);
                }
            }

            list(laye_ast_template_argument) templateArguments = nullptr;
            if (laye_parser_check(p, '<') && layec_location_immediately_follows(current->location, laye_parser_current(p)->location))
            {
                templateArguments = laye_parse_template_arguments(p);
            }

            if (laye_parser_check(p, '?'))
            {
                laye_parser_advance(p);
                isNilable = true;
            }

            laye_ast_node* type = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_NAMED, startLocation);
            assert(type != nullptr);
            type->lookupType.path = path;
            type->lookupType.templateArguments = templateArguments;
            type->lookupType.isHeadless = isPathHeadless;
            type->lookupType.isNilable = isNilable;
            *outTypeSyntax = type;
            return laye_parser_try_parse_type_suffix(p, startIndex, outTypeSyntax, issueDiagnostics);
        }

        WORD_TYPE(VAR, INFER, false)
        WORD_TYPE(NORETURN, NORETURN, false)
        WORD_TYPE(RAWPTR, RAWPTR, false)
        WORD_TYPE(VOID, VOID, false)
        WORD_TYPE(STRING, STRING, false)
        WORD_TYPE(RUNE, RUNE, false)
        WORD_TYPE(INT, INT, false)
        WORD_TYPE(UINT, UINT, false)
        WORD_TYPE(BOOL, BOOL, false)
        WORD_TYPE(FLOAT, FLOAT, false)
        WORD_TYPE(IX, INT_SIZED, true)
        WORD_TYPE(UX, UINT_SIZED, true)
        WORD_TYPE(BX, BOOL_SIZED, true)
        WORD_TYPE(FX, FLOAT_SIZED, true)
        WORD_TYPE(C_CHAR, C_CHAR, false)
        WORD_TYPE(C_SCHAR, C_SCHAR, false)
        WORD_TYPE(C_UCHAR, C_UCHAR, false)
        WORD_TYPE(C_STRING, C_STRING, false)
        WORD_TYPE(C_SHORT, C_SHORT, false)
        WORD_TYPE(C_USHORT, C_USHORT, false)
        WORD_TYPE(C_INT, C_INT, false)
        WORD_TYPE(C_UINT, C_UINT, false)
        WORD_TYPE(C_LONG, C_LONG, false)
        WORD_TYPE(C_ULONG, C_ULONG, false)
        WORD_TYPE(C_LONGLONG, C_LONGLONG, false)
        WORD_TYPE(C_ULONGLONG, C_ULONGLONG, false)
        WORD_TYPE(C_SIZE_T, C_SIZE_T, false)
        WORD_TYPE(C_PTRDIFF_T, C_PTRDIFF_T, false)
        WORD_TYPE(C_FLOAT, C_FLOAT, false)
        WORD_TYPE(C_DOUBLE, C_DOUBLE, false)
        WORD_TYPE(C_LONGDOUBLE, C_LONGDOUBLE, false)
        WORD_TYPE(C_BOOL, C_BOOL, false)

        default:
        {
            if (issueDiagnostics)
            {
                layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Identifier expected when parsing a type.");
                *outTypeSyntax = laye_ast_node_alloc(p, LAYE_AST_NODE_INVALID, current->location);
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

    if (laye_parser_is_eof(p))
        return expression;

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    switch (current->kind)
    {
        default: break;

        case '(':
        {
            laye_parser_advance(p);

            list(laye_ast_node*) argumentNodes = nullptr;
            bool endsWithComma = false;
            while (!laye_parser_is_eof(p) && !laye_parser_check(p, ')'))
            {
                endsWithComma = false;
                if (laye_parser_check(p, ','))
                {
                    layec_issue_diagnostic(p->context, SEV_ERROR, laye_parser_current(p)->location, "Type expected as function type parameter.");
                    laye_parser_advance(p);
                    continue;
                }

                laye_ast_node* argument = laye_parse_expression(p);
                assert(argument != nullptr);

                arrput(argumentNodes, argument);

                if (!laye_parser_check(p, ','))
                    break;
                laye_parser_advance(p);
                endsWithComma = true;
            }

            current = laye_parser_current(p);
            laye_parser_expect(p, ')', nullptr);

            if (endsWithComma)
            {
                layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Expression expected as invocation argument.");
            }

            layec_location location = layec_location_combine(expression->location, laye_parser_most_recent_location(p));

            laye_ast_node* invokeExpression = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_INVOKE, expression->location);
            invokeExpression->invoke.target = expression;
            invokeExpression->invoke.arguments = argumentNodes;

            return laye_parse_primary_suffix(p, invokeExpression);
        } break;
    }

    return expression;
}

static list(laye_ast_constructor_value) laye_parse_constructor(laye_parser* p, layec_location* lastLocation)
{
    assert(p != nullptr);
    assert(laye_parser_check(p, '{'));

    laye_parser_advance(p);

    list(laye_ast_constructor_value) values = nullptr;
    while (!laye_parser_is_eof(p) && !laye_parser_check(p, '}'))
    {
        laye_token* valueNameToken = laye_parser_expect_identifier(p, nullptr);
        string valueName = layec_intern_location_text(p->context, valueNameToken->location);

        laye_parser_expect(p, '=', nullptr);

        laye_ast_node* value = laye_parse_expression(p);

        laye_ast_constructor_value ctorValue = {
            .name = valueName,
            .value = value,
        };
        arrput(values, ctorValue);

        if (!laye_parser_check(p, ','))
            break;
        
        laye_parser_advance(p);
    }

    laye_token* lastToken = nullptr;
    laye_parser_expect_out(p, '}', nullptr, &lastToken);
    assert(lastToken != nullptr);

    if (lastLocation) *lastLocation = lastToken->location;
    return values;
}

static laye_ast_node* laye_parse_identifier_suffix(laye_parser* p, laye_ast_node* expression)
{
    assert(p != nullptr);
    assert(expression != nullptr);

    if (laye_parser_is_eof(p))
        return expression;

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    switch (current->kind)
    {
        default: break;

        case '{':
        {
            layec_location lastLocation = { 0 };
            list(laye_ast_constructor_value) values = laye_parse_constructor(p, &lastLocation);

            expression->kind = LAYE_AST_NODE_TYPE_NAMED;

            layec_location location = layec_location_combine(expression->location, lastLocation);
            laye_ast_node* constructorNode = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_CONSTRUCTOR, location);
            constructorNode->constructor.typeName = expression;
            constructorNode->constructor.values = values;

            expression = constructorNode;
        } break;
    }

    return laye_parse_primary_suffix(p, expression);
}

static laye_ast_node* laye_parse_primary(laye_parser* p)
{
    assert(p != nullptr);

    if (laye_parser_is_eof(p))
    {
        layec_location endLocation = laye_parser_eof_location(p);
        layec_issue_diagnostic(p->context, SEV_ERROR, endLocation, "End of file reached when parsing primary expression.");
        return laye_ast_node_alloc(p, LAYE_AST_NODE_INVALID, endLocation);
    }

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);
    layec_location startLocation = current->location;

    list(string) path = nullptr;
    string identifierName = { 0 };
    bool isPathHeadless = false;
    switch (current->kind)
    {
        case LAYE_TOKEN_COLON_COLON:
            isPathHeadless = true;
            goto start_path_resolution_parse;
        case LAYE_TOKEN_IDENTIFIER:
        {
            identifierName = layec_intern_location_text(p->context, current->location);
            laye_parser_advance(p);
            arrput(path, identifierName);

            if (laye_parser_check(p, LAYE_TOKEN_COLON_COLON))
            {
            start_path_resolution_parse:;
                layec_location lastIdentifierLocation = { 0 };
                while (laye_parser_check(p, LAYE_TOKEN_COLON_COLON))
                {
                    laye_parser_advance(p);
                    laye_token* nextIdent = laye_parser_expect_identifier(p, nullptr);
                    assert(nextIdent != nullptr);
                    lastIdentifierLocation = nextIdent->location;
                    string nextName = layec_intern_location_text(p->context, nextIdent->location);
                    startLocation = layec_location_combine(startLocation, nextIdent->location);
                    arrput(path, nextName);
                }
            }

            list(laye_ast_template_argument) templateArguments = nullptr;
            if (laye_parser_check(p, '<') && layec_location_immediately_follows(current->location, laye_parser_current(p)->location))
            {
                templateArguments = laye_parse_template_arguments(p);
            }

            laye_ast_node* resultNode = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_LOOKUP, startLocation);
            assert(resultNode != nullptr);
            resultNode->lookup.path = path;
            resultNode->lookup.templateArguments = templateArguments;
            resultNode->lookup.isHeadless = isPathHeadless;
            return laye_parse_identifier_suffix(p, resultNode);
        }

        case LAYE_TOKEN_NEW:
        {
            layec_location startLocation = current->location;
            laye_parser_advance(p);

            laye_ast_node* allocator = nullptr;
            if (laye_parser_check(p, '('))
            {
                laye_parser_advance(p);
                allocator = laye_parse_expression(p);
                laye_parser_expect(p, ')', nullptr);
            }

            laye_ast_node* typeNode = nullptr;
            bool typeSuccess = laye_parser_try_parse_type(p, &typeNode, true);
            assert(typeSuccess);
            assert(typeNode != nullptr);

            list(laye_ast_constructor_value) values = nullptr;
            layec_location lastLocation = typeNode->location;

            if (typeNode->kind == LAYE_AST_NODE_TYPE_NAMED && laye_parser_check(p, '{'))
            {
                values = laye_parse_constructor(p, &lastLocation);
            }

            layec_location location = layec_location_combine(startLocation, lastLocation);
            laye_ast_node* newNode = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_NEW, location);
            newNode->new.allocator = allocator;
            newNode->new.type = typeNode;
            newNode->new.values = values;

            return newNode;
        }

        case LAYE_TOKEN_LITERAL_STRING:
        {
            laye_ast_node* resultNode = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_STRING, current->location);
            assert(resultNode != nullptr);
            resultNode->literal.stringValue = current->stringValue;
            laye_parser_advance(p);
            return laye_parse_primary_suffix(p, resultNode);
        }

        case LAYE_TOKEN_LITERAL_INTEGER:
        {
            laye_ast_node* resultNode = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_INTEGER, current->location);
            assert(resultNode != nullptr);
            resultNode->literal.integerValue = current->integerValue;
            laye_parser_advance(p);
            return laye_parse_primary_suffix(p, resultNode);
        }

        default:
        {
            layec_issue_diagnostic(p->context, SEV_ERROR, current->location, "Unexpected token when parsing expression.");
            laye_parser_advance(p);
            return laye_ast_node_alloc(p, LAYE_AST_NODE_INVALID, current->location);
        }
    }
}

static bool is_binary_operator_with_precedence(laye_parser* p, int precedence, int* outNewPrecedence)
{
    laye_token* c = laye_parser_current(p);
    if (c == NULL)
        return false;

    usize numOps = sizeof(layeOperatorInfos) / sizeof(laye_operator_info);
    for (usize i = 0; i < numOps; i++)
    {
        if (c->kind == layeOperatorInfos[i].tokenKind && layeOperatorInfos[i].precendence >= precedence)
        {
            if (outNewPrecedence)
                *outNewPrecedence = layeOperatorInfos[i].precendence;
            return true;
        }
    }

    return false;
}

static laye_ast_node* laye_parse_binary(laye_parser* p, laye_ast_node* lhs, int precedence)
{
    assert(p != nullptr);
    assert(lhs != nullptr);
    assert(precedence >= 0);

    int nextPrecedence = 0;
    while (is_binary_operator_with_precedence(p, precedence, &nextPrecedence))
    {
        laye_token* operatorToken = laye_parser_current(p);
        assert(operatorToken != nullptr);
        laye_parser_advance(p);

        laye_ast_node* rhs = laye_parse_primary(p);
        assert(rhs != nullptr);

        int rhsPrecedence = nextPrecedence;
        while (is_binary_operator_with_precedence(p, rhsPrecedence, &nextPrecedence))
            rhs = laye_parse_binary(p, rhs, rhsPrecedence);
        
        laye_ast_node* binaryExpression = laye_ast_node_alloc(p, LAYE_AST_NODE_EXPRESSION_BINARY, layec_location_combine(lhs->location, rhs->location));
        binaryExpression->binary.lhs = lhs;
        binaryExpression->binary.rhs = rhs;
        binaryExpression->binary.operatorKind = operatorToken->kind;
        binaryExpression->binary.operatorString = layec_intern_location_text(p->context, operatorToken->location);

        lhs = binaryExpression;
    }

    assert(lhs != nullptr);
    return lhs;
}

static laye_ast_node* laye_parse_secondary(laye_parser* p)
{
    assert(p != nullptr);

    laye_ast_node* primary = laye_parse_primary(p);
    assert(primary != nullptr);

    return laye_parse_binary(p, primary, 0);
}

static laye_ast_node* laye_parse_expression(laye_parser* p)
{
    assert(p != nullptr);

    laye_ast_node* secondary = laye_parse_secondary(p);
    assert(secondary != nullptr);

    return secondary;
}

static laye_ast_node* laye_parse_grouped_statement(laye_parser* p)
{
    assert(p != nullptr);
    assert(laye_parser_check(p, '{'));

    laye_token* firstToken = laye_parser_current(p);
    laye_parser_advance(p);

    list(laye_ast_node*) body = nullptr;

    while (!laye_parser_is_eof(p) && !laye_parser_check(p, '}'))
    {
        laye_ast_node* declarationOrStatement = laye_parse_declaration_or_statement(p);
        arrput(body, declarationOrStatement);
    }

    laye_token* lastToken = nullptr;
    laye_parser_expect_out(p, '}', nullptr, &lastToken);

    layec_location groupLocation = layec_location_combine(firstToken->location, lastToken->location);

    laye_ast_node* groupedStatement = laye_ast_node_alloc(p, LAYE_AST_NODE_STATEMENT_BLOCK, groupLocation);
    groupedStatement->statements = body;

    return groupedStatement;
}

static laye_ast_node* laye_parse_statement(laye_parser* p)
{
    assert(p != nullptr);

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);

    switch (current->kind)
    {
        case LAYE_TOKEN_RETURN:
        {
            layec_location returnLocation = current->location;
            laye_parser_advance(p);

            laye_ast_node* returnValue = nullptr;
            if (!laye_parser_check(p, ';'))
            {
                returnValue = laye_parse_expression(p);
                returnLocation = layec_location_combine(returnLocation, returnValue->location);
            }

            laye_parser_expect(p, ';', nullptr);

            laye_ast_node* returnNode = laye_ast_node_alloc(p, LAYE_AST_NODE_STATEMENT_RETURN, returnLocation);
            returnNode->returnValue = returnValue;
            return returnNode;
        }

        default:
        {
            laye_ast_node* statement = laye_parse_expression(p);
            assert(statement != nullptr);

            if (laye_parser_check(p, '='))
            {
                laye_parser_advance(p);

                laye_ast_node* assignValue = laye_parse_expression(p);
                assert(assignValue != nullptr);

                laye_ast_node* assignNode = laye_ast_node_alloc(p, LAYE_AST_NODE_STATEMENT_ASSIGNMENT, layec_location_combine(statement->location, assignValue->location));
                assignNode->assignment.target = statement;
                assignNode->assignment.value = assignValue;

                statement = assignNode;
            }

            laye_parser_expect(p, LAYE_TOKEN_SEMI_COLON, nullptr);
            
            assert(statement != nullptr);
            return statement;
        }
    }
}

static list(laye_ast_template_parameter) laye_parse_template_parameters(laye_parser* p)
{
    assert(p != nullptr);

    laye_token* current = laye_parser_current(p);
    assert(current != nullptr);
    assert(current->kind == '<', "when calling laye_parse_template_parameters, the current token must be '<'");

    list(laye_ast_template_parameter) result = nullptr;

    laye_parser_advance(p);

    while (!laye_parser_is_eof(p) && !laye_parser_check(p, '>'))
    {
        if (laye_parser_check(p, LAYE_TOKEN_IDENTIFIER) && (laye_parser_peek_check(p, ',') || laye_parser_peek_check(p, '>')))
        {
            laye_token* typeParamToken = laye_parser_current(p);
            assert(typeParamToken != nullptr);
            string typeParamName = layec_intern_location_text(p->context, typeParamToken->location);
            laye_parser_advance(p);

            laye_ast_template_parameter param = {
                .kind = LAYE_TEMPLATE_PARAM_TYPE,
                .name = typeParamName,
            };
            arrput(result, param);
        }
        else
        {
            laye_ast_node* valueType = nullptr;
            bool typeSuccess = laye_parser_try_parse_type(p, &valueType, true);
            assert(typeSuccess);
            assert(valueType != nullptr);

            laye_token* valueNameToken = laye_parser_expect_identifier(p, nullptr);
            assert(valueNameToken != nullptr);
            string valueName = layec_intern_location_text(p->context, valueNameToken->location);

            laye_ast_template_parameter param = {
                .kind = LAYE_TEMPLATE_PARAM_VALUE,
                .valueType = valueType,
                .name = valueName,
            };
            arrput(result, param);
        }

        if (!laye_parser_check(p, ','))
            break;

        laye_parser_advance(p);
        if (!laye_parser_check(p, '>'))
            continue;

        layec_issue_diagnostic(p->context, SEV_ERROR, laye_parser_current(p)->location, "Type parameter expected");
        break;
    }

    laye_token* closingToken = nullptr;
    laye_parser_expect_out(p, '>', nullptr, &closingToken);

    if (arrlenu(result) == 0)
    {
        layec_issue_diagnostic(p->context, SEV_ERROR, closingToken->location, "Template parameter lists cannot be empty");
    }

    return result;
}

static laye_ast_node* laye_parse_declaration_continue(laye_parser* p, list(laye_ast_modifier) modifiers, laye_ast_node* declType, laye_token* name)
{
    assert(p != nullptr);
    assert(declType != nullptr);
    assert(name != nullptr);

    list(laye_ast_template_parameter) templateParameters = nullptr;
    if (laye_parser_check(p, '<'))
    {
        templateParameters = laye_parse_template_parameters(p);
    }

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

            if (laye_parser_check(p, LAYE_TOKEN_IDENTIFIER) && (laye_parser_peek_check(p, ',') || laye_parser_peek_check(p, ')')))
            {
                paramNameToken = laye_parser_current(p);
                laye_parser_advance(p);

                paramTypeSyntax = laye_ast_node_alloc(p, LAYE_AST_NODE_TYPE_ERROR, paramNameToken->location);

                layec_issue_diagnostic(p->context, SEV_ERROR, paramNameToken->location, "Parameter type missing.");
            }
            else
            {
                bool typeParseResult = laye_parser_try_parse_type(p, &paramTypeSyntax, true);

                // if we pass `true` for diagnostic issuing, we should *always* get back a value, even if it's invalid.
                assert(typeParseResult == true);

                paramNameToken = laye_parser_expect_identifier(p, nullptr);
            }
            
            assert(paramTypeSyntax != nullptr);
            assert(paramNameToken != nullptr);
            
            paramBinding = laye_ast_node_alloc(p, LAYE_AST_NODE_BINDING_DECLARATION, layec_location_combine(paramTypeSyntax->location, paramNameToken->location));
            paramBinding->bindingDeclaration.declaredType = paramTypeSyntax;
            paramBinding->bindingDeclaration.name = layec_intern_location_text(p->context, paramNameToken->location);

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

        laye_ast_node* functionDeclaration = laye_ast_node_alloc(p, LAYE_AST_NODE_FUNCTION_DECLARATION, name->location);
        functionDeclaration->functionDeclaration.modifiers = modifiers;
        functionDeclaration->functionDeclaration.returnType = declType;
        functionDeclaration->functionDeclaration.name = layec_intern_location_text(p->context, name->location);
        functionDeclaration->functionDeclaration.templateParameters = templateParameters;
        functionDeclaration->functionDeclaration.parameterBindings = parameterBindingNodes;
        functionDeclaration->functionDeclaration.body = functionBody;

        return functionDeclaration;
    }

    if (arrlenu(templateParameters) > 0)
    {
        layec_issue_diagnostic(p->context, SEV_ERROR, name->location, "Binding declarations cannot have template parameters.");
    }

    laye_ast_node* bindingDeclaration = laye_ast_node_alloc(p, LAYE_AST_NODE_BINDING_DECLARATION, name->location);
    bindingDeclaration->bindingDeclaration.modifiers = modifiers;
    bindingDeclaration->bindingDeclaration.declaredType = declType;
    bindingDeclaration->bindingDeclaration.name = layec_intern_location_text(p->context, name->location);

    if (laye_parser_check(p, '='))
    {
        laye_parser_advance(p);

        laye_ast_node* assignValue = laye_parse_expression(p);
        assert(assignValue != nullptr);

        bindingDeclaration->bindingDeclaration.initialValue = assignValue;
    }

    laye_parser_expect(p, LAYE_TOKEN_SEMI_COLON, nullptr);
    return bindingDeclaration;
}

static laye_ast_node* laye_parse_field_declaration(laye_parser* p)
{
    assert(p != nullptr);

    laye_ast_node* fieldType = nullptr;
    bool typeSuccess = laye_parser_try_parse_type(p, &fieldType, true);
    assert(typeSuccess);

    laye_token* fieldNameToken = laye_parser_expect_identifier(p, nullptr);
    string fieldName = layec_intern_location_text(p->context, fieldNameToken->location);

    laye_parser_expect(p, ';', nullptr);

    laye_ast_node* fieldBinding = laye_ast_node_alloc(p, LAYE_AST_NODE_BINDING_DECLARATION, fieldNameToken->location);
    fieldBinding->bindingDeclaration.declaredType = fieldType;
    fieldBinding->bindingDeclaration.name = fieldName;

    return fieldBinding;
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
        case LAYE_TOKEN_STRUCT:
        {
            layec_location startLocation = current->location;
            laye_parser_advance(p);

            laye_token* nameToken = laye_parser_expect_identifier(p, nullptr);
            string name = layec_intern_location_text(p->context, nameToken->location);

            list(laye_ast_template_parameter) templateParameters = nullptr;
            if (laye_parser_check(p, '<'))
            {
                templateParameters = laye_parse_template_parameters(p);
            }

            laye_parser_expect(p, '{', nullptr);

            list(laye_ast_node*) fieldBindings = nullptr;
            list(laye_ast_struct_variant) variants = nullptr;
            while (!laye_parser_is_eof(p) && !laye_parser_check(p, '}'))
            {
                if (laye_parser_check(p, LAYE_TOKEN_VARIANT))
                {
                    laye_parser_advance(p);

                    if (laye_parser_check(p, LAYE_TOKEN_VOID))
                    {
                        laye_parser_advance(p);
                        laye_parser_expect(p, ';', nullptr);

                        laye_ast_struct_variant variant = {
                            .isVoid = true,
                        };
                        arrput(variants, variant);
                    }
                    else
                    {
                        laye_token* variantNameToken = laye_parser_expect_identifier(p, nullptr);
                        string variantName = layec_intern_location_text(p->context, variantNameToken->location);
                        
                        list(laye_ast_node*) variantFieldBindings = nullptr;
                        if (laye_parser_check(p, ';'))
                        {
                            laye_parser_advance(p);
                        }
                        else
                        {
                            laye_parser_expect(p, '{', nullptr);

                            while (!laye_parser_is_eof(p) && !laye_parser_check(p, '}'))
                            {
                                laye_ast_node* fieldBinding = laye_parse_field_declaration(p);
                                arrput(variantFieldBindings, fieldBinding);
                            }

                            laye_parser_expect(p, '}', nullptr);
                        }

                        laye_ast_struct_variant variant = {
                            .name = variantName,
                            .fieldBindings = variantFieldBindings
                        };
                        arrput(variants, variant);
                    }
                }
                else
                {
                    laye_ast_node* fieldBinding = laye_parse_field_declaration(p);
                    arrput(fieldBindings, fieldBinding);
                }
            }
            
            laye_parser_expect(p, '}', nullptr);

            layec_location location = layec_location_combine(startLocation, laye_parser_most_recent_location(p));
            laye_ast_node* decl = laye_ast_node_alloc(p, LAYE_AST_NODE_STRUCT_DECLARATION, location);
            decl->structDeclaration.modifiers = modifiers;
            decl->structDeclaration.name = name;
            decl->structDeclaration.templateParameters = templateParameters;
            decl->structDeclaration.fieldBindings = fieldBindings;
            decl->structDeclaration.variants = variants;

            return decl;
        }

        case LAYE_TOKEN_ENUM:
        {
            layec_location startLocation = current->location;
            laye_parser_advance(p);

            laye_token* nameToken = laye_parser_expect_identifier(p, nullptr);
            string name = layec_intern_location_text(p->context, nameToken->location);

            list(laye_ast_template_parameter) templateParameters = nullptr;
            if (laye_parser_check(p, '<'))
            {
                templateParameters = laye_parse_template_parameters(p);
            }

            laye_parser_expect(p, '{', nullptr);

            list(laye_ast_enum_variant) variants = nullptr;
            while (!laye_parser_is_eof(p) && !laye_parser_check(p, '}'))
            {
                laye_token* variantNameToken = laye_parser_expect_identifier(p, nullptr);
                string variantName = layec_intern_location_text(p->context, variantNameToken->location);

                laye_ast_node* variantValue = nullptr;
                if (laye_parser_check(p, '='))
                {
                    laye_parser_advance(p);
                    variantValue = laye_parse_expression(p);
                }

                laye_ast_enum_variant variant = {
                    .name = variantName,
                    .value = variantValue,
                };
                arrput(variants, variant);

                if (!laye_parser_check(p, ','))
                    break;
                
                laye_parser_advance(p);
            }
            
            laye_parser_expect(p, '}', nullptr);

            layec_location location = layec_location_combine(startLocation, laye_parser_most_recent_location(p));
            laye_ast_node* decl = laye_ast_node_alloc(p, LAYE_AST_NODE_ENUM_DECLARATION, location);
            decl->enumDeclaration.modifiers = modifiers;
            decl->enumDeclaration.name = name;
            decl->enumDeclaration.templateParameters = templateParameters;
            decl->enumDeclaration.variants = variants;

            return decl;
        }

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
