#include <stdio.h>

#include "kos/kos.h"
#include "kos/ansi.h"

#include "ast.h"
#include "token.h"

static void type_to_string_builder(laye_ast_node* typeNode, string_builder* sb);

static void template_args_to_string_builder(list(laye_ast_template_argument) args, string_builder* sb)
{
    string_builder_append_cstring(sb, "<");
    for (usize i = 0, iLen = arrlenu(args); i < iLen; i++)
    {
        if (i > 0)
            string_builder_append_cstring(sb, ", ");

        laye_ast_template_argument arg = args[i];
        switch (arg.kind)
        {
            case LAYE_TEMPLATE_ARG_TYPE:
                type_to_string_builder(arg.value, sb);
                break;

            case LAYE_TEMPLATE_ARG_VALUE:
                string_builder_append_cstring(sb, "[expression]");
                break;
        }
    }
    string_builder_append_cstring(sb, ">");
}

static void type_to_string_builder(laye_ast_node* typeNode, string_builder* sb)
{
    switch (typeNode->kind)
    {
        default: string_builder_append_cstring(sb, "<unknown ast type>"); break;

        case LAYE_AST_NODE_TYPE_FUNCTION:
        {
            type_to_string_builder(typeNode->functionType.returnType, sb);
            string_builder_append_rune(sb, cast(rune) '(');
            for (usize i = 0, iLen = arrlenu(typeNode->functionType.parameterTypes); i < iLen; i++)
            {
                if (i > 0)
                    string_builder_append_cstring(sb, ", ");
                type_to_string_builder(typeNode->functionType.parameterTypes[i], sb);
            }
            string_builder_append_rune(sb, cast(rune) ')');
        } break;

        case LAYE_AST_NODE_TYPE_POINTER:
        {
            type_to_string_builder(typeNode->containerType.elementType, sb);
            if (typeNode->containerType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, " readonly");
            else if (typeNode->containerType.access == LAYE_AST_ACCESS_WRITEONLY)
            {
                string_builder_append_cstring(sb, " writeonly");
            }

            string_builder_append_rune(sb, cast(rune) '*');
        } break;

        case LAYE_AST_NODE_TYPE_SLICE:
        {
            type_to_string_builder(typeNode->containerType.elementType, sb);
            if (typeNode->containerType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, " readonly");
            else if (typeNode->containerType.access == LAYE_AST_ACCESS_WRITEONLY)
            {
                string_builder_append_cstring(sb, " writeonly");
            }
            
            string_builder_append_cstring(sb, "[]");
        } break;

        case LAYE_AST_NODE_TYPE_BUFFER:
        {
            type_to_string_builder(typeNode->containerType.elementType, sb);
            if (typeNode->containerType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, " readonly");
            else if (typeNode->containerType.access == LAYE_AST_ACCESS_WRITEONLY)
            {
                string_builder_append_cstring(sb, " writeonly");
            }
            
            string_builder_append_cstring(sb, "[*]");
        } break;

        case LAYE_AST_NODE_TYPE_ARRAY:
        {
            type_to_string_builder(typeNode->containerType.elementType, sb);
            if (typeNode->containerType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, " readonly");
            else if (typeNode->containerType.access == LAYE_AST_ACCESS_WRITEONLY)
            {
                string_builder_append_cstring(sb, " writeonly");
            }

            string_builder_append_cstring(sb, "[rank ");
            string_builder_append_uint(sb, arrlenu(typeNode->containerType.ranks));
            string_builder_append_cstring(sb, " array]");
        } break;
        
        case LAYE_AST_NODE_TYPE_NAMED:
        {
            string_builder_append_string(sb, typeNode->nameLookup.name);
            if (arrlenu(typeNode->nameLookup.templateArguments) > 0)
                template_args_to_string_builder(typeNode->nameLookup.templateArguments, sb);
        } break;

        case LAYE_AST_NODE_TYPE_PATH_RESOLVE:
        {
            if (typeNode->pathLookup.isHeadless)
                string_builder_append_cstring(sb, "::");
            for (usize i = 0, iLen = arrlenu(typeNode->pathLookup.path); i < iLen; i++)
            {
                if (i > 0)
                    string_builder_append_cstring(sb, "::");
                string_builder_append_string(sb, typeNode->pathLookup.path[i]);
            }
            if (arrlenu(typeNode->pathLookup.templateArguments) > 0)
                template_args_to_string_builder(typeNode->pathLookup.templateArguments, sb);
        } break;

        case LAYE_AST_NODE_TYPE_INFER: string_builder_append_cstring(sb, "var"); break;
        case LAYE_AST_NODE_TYPE_NORETURN: string_builder_append_cstring(sb, "noreturn"); break;
        case LAYE_AST_NODE_TYPE_RAWPTR: string_builder_append_cstring(sb, "rawptr"); break;
        case LAYE_AST_NODE_TYPE_VOID: string_builder_append_cstring(sb, "void"); break;
        case LAYE_AST_NODE_TYPE_STRING:
            if (typeNode->primitiveType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, "readonly ");
            else if (typeNode->primitiveType.access == LAYE_AST_ACCESS_WRITEONLY)
                string_builder_append_cstring(sb, "writeonly ");
            string_builder_append_cstring(sb, "string");
            break;
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

        case LAYE_AST_NODE_TYPE_C_STRING:
            if (typeNode->primitiveType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, "readonly ");
            else if (typeNode->primitiveType.access == LAYE_AST_ACCESS_WRITEONLY)
                string_builder_append_cstring(sb, "writeonly ");
            string_builder_append_cstring(sb, "c_string");
            break;
        case LAYE_AST_NODE_TYPE_C_CHAR: string_builder_append_cstring(sb, "c_char"); break;
        case LAYE_AST_NODE_TYPE_C_SCHAR: string_builder_append_cstring(sb, "c_schar"); break;
        case LAYE_AST_NODE_TYPE_C_UCHAR: string_builder_append_cstring(sb, "c_uchar"); break;
        case LAYE_AST_NODE_TYPE_C_SHORT: string_builder_append_cstring(sb, "c_short"); break;
        case LAYE_AST_NODE_TYPE_C_USHORT: string_builder_append_cstring(sb, "c_ushort"); break;
        case LAYE_AST_NODE_TYPE_C_INT: string_builder_append_cstring(sb, "c_int"); break;
        case LAYE_AST_NODE_TYPE_C_UINT: string_builder_append_cstring(sb, "c_uint"); break;
        case LAYE_AST_NODE_TYPE_C_LONG: string_builder_append_cstring(sb, "c_long"); break;
        case LAYE_AST_NODE_TYPE_C_ULONG: string_builder_append_cstring(sb, "c_ulong"); break;
        case LAYE_AST_NODE_TYPE_C_LONGLONG: string_builder_append_cstring(sb, "c_longlong"); break;
        case LAYE_AST_NODE_TYPE_C_ULONGLONG: string_builder_append_cstring(sb, "c_ulonglong"); break;
        case LAYE_AST_NODE_TYPE_C_SIZE_T: string_builder_append_cstring(sb, "c_size_t"); break;
        case LAYE_AST_NODE_TYPE_C_PTRDIFF_T: string_builder_append_cstring(sb, "c_ptrdiff_t"); break;
        case LAYE_AST_NODE_TYPE_C_FLOAT: string_builder_append_cstring(sb, "c_float"); break;
        case LAYE_AST_NODE_TYPE_C_DOUBLE: string_builder_append_cstring(sb, "c_double"); break;
        case LAYE_AST_NODE_TYPE_C_LONGDOUBLE: string_builder_append_cstring(sb, "c_longdouble"); break;
        case LAYE_AST_NODE_TYPE_C_BOOL: string_builder_append_cstring(sb, "c_bool"); break;
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

static void laye_ast_fprint_name(ast_fprint_state state, string_view name, bool isLast)
{
    if (state.indents->count > 0)
    {
        fprintf(state.stream, "%.*s", (int)state.indents->count, cast(const char*)state.indents->memory);
    }

    const char* currentIndent = isLast ? "└ " : "├ ";

    fprintf(state.stream, "%s", currentIndent);
    PUTCOLOR(ANSI_COLOR_RED);
    fprintf(state.stream, STRING_VIEW_FORMAT, STRING_VIEW_EXPAND(name));
    RESETCOLOR;
}

static void laye_ast_fprint_template_parameter(ast_fprint_state state, laye_ast_template_parameter param, bool isLast)
{
    laye_ast_fprint_name(state, STRING_VIEW_LITERAL("TEMPLATE_PARAMETER"), isLast);

    switch (param.kind)
    {
        default: break;
        case LAYE_TEMPLATE_PARAM_VALUE:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Type: ");
            RESETCOLOR;
            string typeString = laye_ast_node_type_to_string(param.valueType);
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(typeString));
            string_deallocate(typeString);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } // fallthrough;

        case LAYE_TEMPLATE_PARAM_TYPE:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(param.name));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;
    }

    RESETCOLOR;
    fprintf(state.stream, "\n");
}

static void laye_ast_fprint_node(ast_fprint_state state, laye_ast_node* node, bool isLast);

static void laye_ast_fprint_template_argument(ast_fprint_state state, laye_ast_template_argument arg, bool isLast)
{
    laye_ast_fprint_name(state, STRING_VIEW_LITERAL("TEMPLATE_ARGUMENT"), isLast);

    switch (arg.kind)
    {
        default: break;
        case LAYE_TEMPLATE_PARAM_VALUE:
        {
            laye_ast_fprint_node(state, arg.value, true);
        } break;

        case LAYE_TEMPLATE_PARAM_TYPE:
        {
            laye_ast_fprint_name(state, STRING_VIEW_LITERAL("TYPE_PARAMETER"), isLast);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            string typeString = laye_ast_node_type_to_string(arg.value);
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(typeString));
            string_deallocate(typeString);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;
    }

    RESETCOLOR;
    fprintf(state.stream, "\n");
}

static void laye_ast_fprint_node(ast_fprint_state state, laye_ast_node* node, bool isLast)
{
    laye_ast_fprint_name(state, laye_ast_node_kind_name(node->kind), isLast);

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

        case LAYE_AST_NODE_STRUCT_DECLARATION:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->structDeclaration.name));
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

                string paramTypeString = laye_ast_node_type_to_string(parameterBinding->bindingDeclaration.declaredType);
                fprintf(state.stream, STRING_FORMAT" "STRING_FORMAT, STRING_EXPAND(paramTypeString), STRING_EXPAND(parameterBinding->bindingDeclaration.name));
                string_deallocate(paramTypeString);
            }

            fprintf(state.stream, ")");
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_PATH_RESOLVE:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Path: ");
            RESETCOLOR;
            if (node->pathLookup.isHeadless)
                fprintf(state.stream, "::");
            for (usize i = 0, iLen = arrlenu(node->pathLookup.path); i < iLen; i++)
            {
                if (i > 0)
                    fprintf(state.stream, "::");
                fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->pathLookup.path[i]));
            }
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
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->nameLookup.name));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_STRING:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Value: ");
            RESETCOLOR;
            fprintf(state.stream, "\""STRING_FORMAT"\"", STRING_EXPAND(node->literal.stringValue));
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

        case LAYE_AST_NODE_EXPRESSION_BINARY:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Operator: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->binary.operatorString));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;
    }

    RESETCOLOR;
    fprintf(state.stream, "\n");

    usize lastStringBuilderCount = state.indents->count;
    string_builder_append_cstring(state.indents, isLast ? "  " : "│ ");

    switch (node->kind)
    {
        default: break;

        case LAYE_AST_NODE_FUNCTION_DECLARATION:
        {
            usize templateParamsLen = arrlenu(node->functionDeclaration.templateParameters);

            for (usize i = 0; i < templateParamsLen; i++)
                laye_ast_fprint_template_parameter(state, node->functionDeclaration.templateParameters[i], i == templateParamsLen - 1 && node->functionDeclaration.body == nullptr);

            if (node->functionDeclaration.body != nullptr)
            {
                laye_ast_fprint_node(state, node->functionDeclaration.body, true);
            }
        } break;

        case LAYE_AST_NODE_STRUCT_DECLARATION:
        {
            usize templateParamsLen = arrlenu(node->structDeclaration.templateParameters);
            usize fieldsLen = arrlenu(node->structDeclaration.fieldBindings);

            for (usize i = 0; i < templateParamsLen; i++)
                laye_ast_fprint_template_parameter(state, node->structDeclaration.templateParameters[i], i == templateParamsLen - 1 && fieldsLen == 0);

            for (usize i = 0; i < fieldsLen; i++)
                laye_ast_fprint_node(state, node->structDeclaration.fieldBindings[i], i == fieldsLen - 1);
        } break;

        case LAYE_AST_NODE_BINDING_DECLARATION:
        {
            if (node->bindingDeclaration.initialValue != nullptr)
                laye_ast_fprint_node(state, node->bindingDeclaration.initialValue, true);
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

        case LAYE_AST_NODE_EXPRESSION_INVOKE:
        {
            usize argLen = arrlenu(node->invoke.arguments);
            laye_ast_fprint_node(state, node->invoke.target, argLen == 0);
            for (usize i = 0; i < argLen; i++)
                laye_ast_fprint_node(state, node->invoke.arguments[i], i == argLen - 1);
        } break;

        case LAYE_AST_NODE_EXPRESSION_BINARY:
        {
            laye_ast_fprint_node(state, node->binary.lhs, false);
            laye_ast_fprint_node(state, node->binary.rhs, true);
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

    string_view filePath = layec_context_get_file_full_path(context, ast->fileId);

    PUTCOLOR(ANSI_COLOR_RED);
    fprintf(stream, "ROOT");
    PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
    fprintf(stream, " "STRING_VIEW_FORMAT, STRING_VIEW_EXPAND(filePath));
    RESETCOLOR;
    fprintf(stream, "\n");

    usize importCount = arrlenu(ast->imports);
    usize topLevelNodeCount = arrlenu(ast->topLevelNodes);

    for (usize i = 0; i < importCount; i++)
    {
        laye_ast_import import = ast->imports[i];
        bool isLast = i == importCount - 1 && topLevelNodeCount == 0;
        laye_ast_fprint_name(state, STRING_VIEW_LITERAL("IMPORT"), isLast);
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
