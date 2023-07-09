#include <stdio.h>

#include "kos/kos.h"
#include "kos/ansi.h"

#include "ast.h"
#include "token.h"

static void type_to_string_builder(laye_ast_node* typeNode, string_builder* sb)
{
    switch (typeNode->kind)
    {
        default: string_builder_append_cstring(sb, "<unknown ast type>"); break;
        
        case LAYE_AST_NODE_TYPE_RAWPTR: string_builder_append_cstring(sb, "rawptr"); break;
        case LAYE_AST_NODE_TYPE_VOID: string_builder_append_cstring(sb, "void"); break;
        case LAYE_AST_NODE_TYPE_STRING: string_builder_append_cstring(sb, "string"); break;
        case LAYE_AST_NODE_TYPE_CSTRING: string_builder_append_cstring(sb, "cstring"); break;
        case LAYE_AST_NODE_TYPE_RUNE: string_builder_append_cstring(sb, "rune"); break;
        case LAYE_AST_NODE_TYPE_BOOL: string_builder_append_cstring(sb, "bool"); break;
        case LAYE_AST_NODE_TYPE_INT: string_builder_append_cstring(sb, "int"); break;
        case LAYE_AST_NODE_TYPE_UINT: string_builder_append_cstring(sb, "uint"); break;
        case LAYE_AST_NODE_TYPE_FLOAT: string_builder_append_cstring(sb, "float"); break;

        case LAYE_AST_NODE_TYPE_BOOL_SIZED:
            string_builder_append_cstring(sb, "b");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            break;

        case LAYE_AST_NODE_TYPE_INT_SIZED:
            string_builder_append_cstring(sb, "i");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            break;

        case LAYE_AST_NODE_TYPE_UINT_SIZED:
            string_builder_append_cstring(sb, "u");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            break;

        case LAYE_AST_NODE_TYPE_FLOAT_SIZED:
            string_builder_append_cstring(sb, "f");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            break;
    }
}

string laye_ast_node_type_to_string(laye_ast_node* typeNode)
{
    assert(typeNode != nullptr);

    string_builder sb = { 0 };
    string_builder_init(&sb, default_allocator);

    type_to_string_builder(typeNode, &sb);

    return (string){
        .allocator = sb.allocator,
        .memory = sb.memory,
        .count = sb.count,
    };
}

string_view laye_ast_node_kind_name(laye_ast_node_kind kind)
{
    switch (kind)
    {
#define A(E) case LAYE_AST_NODE_ ## E: return STRING_VIEW_LITERAL(#E);
#define T(E) case LAYE_AST_NODE_TYPE_ ## E: return STRING_VIEW_LITERAL(PPSTR(TYPE_ ## E));
LAYE_AST_NODE_KINDS
#undef T
#undef A
        default: return STRING_VIEW_LITERAL("<unknown ast node kind>");
    }
}

#define PUTCOLOR(C) do { if (state.colors) fprintf(state.stream, C); } while (0)
#define RESETCOLOR do { if (state.colors) fprintf(state.stream, ANSI_COLOR_RESET); } while (0)

typedef struct ast_fprint_state
{
    FILE* stream;
    layec_context* context;
    laye_ast* ast;
    bool colors;
    string_builder* indents;
} ast_fprint_state;

static void laye_ast_fprint_node(ast_fprint_state state, laye_ast_node* node, bool isLast)
{
    if (state.indents->count > 0)
    {
        fprintf(state.stream, "%.*s", (int)state.indents->count, (const char*)state.indents->memory);
    }

    const char* currentIndent = isLast ? "└─ " : "├─ ";
    const char* nestedIndent = isLast ? "   " : "│  ";

    fprintf(state.stream, "%s", currentIndent);
    PUTCOLOR(ANSI_COLOR_RED);
    fprintf(state.stream, STRING_VIEW_FORMAT, STRING_VIEW_EXPAND(laye_ast_node_kind_name(node->kind)));

    switch (node->kind)
    {
        default: break;

        case LAYE_AST_NODE_BINDING_DECLARATION:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->functionDeclaration.name));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, "> <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Type: ");
            RESETCOLOR;
            
            string typeString = laye_ast_node_type_to_string(node->functionDeclaration.returnType);
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(typeString));
            string_deallocate(typeString);

            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_FUNCTION_DECLARATION:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->functionDeclaration.name));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, "> <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Type: ");
            RESETCOLOR;

            string returnTypeString = laye_ast_node_type_to_string(node->functionDeclaration.returnType);
            fprintf(state.stream, STRING_FORMAT"(", STRING_EXPAND(returnTypeString));
            string_deallocate(returnTypeString);

            for (usize i = 0, len = arrlenu(node->functionDeclaration.parameterBindings); i < len; i++)
            {
                if (i > 0)
                    fprintf(state.stream, ", ");

                laye_ast_node* parameterBinding = node->functionDeclaration.parameterBindings[i];

                string paramTypeString = laye_ast_node_type_to_string(node->functionDeclaration.returnType);
                fprintf(state.stream, STRING_FORMAT" "STRING_FORMAT, STRING_EXPAND(paramTypeString), STRING_EXPAND(parameterBinding->bindingDeclaration.name));
                string_deallocate(paramTypeString);
            }

            fprintf(state.stream, ")");
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_LOOKUP:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->lookupName));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_INTEGER:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Value: ");
            RESETCOLOR;
            fprintf(state.stream, "%llu", cast(unsigned long long int) node->literal.integerValue);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;
    }

    RESETCOLOR;
    fprintf(state.stream, "\n");

    usize lastStringBuilderCount = state.indents->count;
    string_builder_append_cstring(state.indents, nestedIndent);

    switch (node->kind)
    {
        default: break;

        case LAYE_AST_NODE_FUNCTION_DECLARATION:
        {
            if (node->functionDeclaration.body != nullptr)
            {
                laye_ast_fprint_node(state, node->functionDeclaration.body, true);
            }
        } break;

        case LAYE_AST_NODE_STATEMENT_BLOCK:
        {
            for (usize i = 0, len = arrlenu(node->statements); i < len; i++)
                laye_ast_fprint_node(state, node->statements[i], i == len - 1);
        } break;

        case LAYE_AST_NODE_STATEMENT_ASSIGNMENT:
        {
            laye_ast_fprint_node(state, node->assignment.target, false);
            laye_ast_fprint_node(state, node->assignment.value, true);
        } break;

        case LAYE_AST_NODE_STATEMENT_RETURN:
        {
            laye_ast_fprint_node(state, node->returnValue, true);
        } break;
    }

    string_builder_set_count(state.indents, lastStringBuilderCount);
}

void laye_ast_fprint(FILE* stream, layec_context* context, laye_ast* ast, bool colors)
{
    string_builder sb = { 0 };
    string_builder_init(&sb, default_allocator);

    ast_fprint_state state = {
        .stream = stream,
        .ast = ast,
        .colors = colors,
        .indents = &sb,
    };

    string_view fileName = layec_context_get_file_name(context, ast->fileId);

    PUTCOLOR(ANSI_COLOR_RED);
    fprintf(stream, "ROOT");
    PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
    fprintf(stream, " "STRING_VIEW_FORMAT, STRING_VIEW_EXPAND(fileName));
    RESETCOLOR;
    fprintf(stream, "\n");

    usize importCount = arrlenu(ast->imports);
    usize topLevelNodeCount = arrlenu(ast->topLevelNodes);

    for (usize i = 0; i < importCount; i++)
    {
        laye_ast_import import = ast->imports[i];
        bool isLast = i == importCount - 1 && topLevelNodeCount == 0;
        fprintf(state.stream, "%s", isLast ? "└─ " : "├─ ");
        PUTCOLOR(ANSI_COLOR_RED);
        fprintf(stream, "IMPORT");
        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
        fprintf(state.stream, " <");
        PUTCOLOR(ANSI_COLOR_BLUE);
        fprintf(state.stream, "Name: ");
        RESETCOLOR;
        fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(import.name));
        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
        fprintf(state.stream, ">");
        
        if (import.alias.count > 0)
        {
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Alias: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(import.alias));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        }
        
        if (import.export)
        {
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Export");
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        }

        if (import.allMembers)
        {
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Captures All");
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        }
        else if (arrlenu(import.explicitMembers) > 0)
        {
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Captures: ");
            RESETCOLOR;
            for (usize j = 0, jLen = arrlenu(import.explicitMembers); j < jLen; j++)
            {
                if (j > 0)
                    fprintf(state.stream, ", ");
                string memberName = import.explicitMembers[i];
                fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(memberName));
            }
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        }

        RESETCOLOR;
        fprintf(state.stream, "\n");
    }

    for (usize i = 0; i < topLevelNodeCount; i++)
    {
        laye_ast_fprint_node(state, ast->topLevelNodes[i], i == topLevelNodeCount - 1);
    }

    string_builder_deallocate(state.indents);
}
