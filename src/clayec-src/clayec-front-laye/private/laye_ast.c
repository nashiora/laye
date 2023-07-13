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

        case LAYE_AST_NODE_TYPE_ERROR:
        {
            if (typeNode->errorUnionType.errorPath != nullptr)
            {
                assert(typeNode->errorUnionType.errorPath->kind == LAYE_AST_NODE_TYPE_NAMED);
                type_to_string_builder(typeNode->errorUnionType.errorPath, sb);
            }

            string_builder_append_rune(sb, cast(rune) '!');
            assert(typeNode->errorUnionType.valueType != nullptr);
            type_to_string_builder(typeNode->errorUnionType.valueType, sb);
        } break;

        case LAYE_AST_NODE_TYPE_FUNCTION:
        {
            bool isErrorReturn = typeNode->functionType.returnType->kind == LAYE_AST_NODE_TYPE_ERROR;
            if (isErrorReturn) string_builder_append_rune(sb, cast(rune) '(');
            type_to_string_builder(typeNode->functionType.returnType, sb);
            if (isErrorReturn) string_builder_append_rune(sb, cast(rune) ')');
            string_builder_append_rune(sb, cast(rune) '(');
            bool isLayeVarargs = typeNode->functionType.varargsKind == LAYE_AST_VARARGS_LAYE;
            for (usize i = 0, iLen = arrlenu(typeNode->functionType.parameterTypes); i < iLen; i++)
            {
                if (i > 0)
                    string_builder_append_cstring(sb, ", ");
                if (i == iLen - 1 && isLayeVarargs)
                    string_builder_append_cstring(sb, "varargs ");
                type_to_string_builder(typeNode->functionType.parameterTypes[i], sb);
            }
            if (typeNode->functionType.varargsKind == LAYE_AST_VARARGS_C)
                string_builder_append_cstring(sb, ", varargs");
            string_builder_append_rune(sb, cast(rune) ')');
            if (typeNode->functionType.isNilable)
                string_builder_append_rune(sb, cast(rune) '?');
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
            if (typeNode->containerType.isNilable)
                string_builder_append_rune(sb, cast(rune) '?');
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
            if (typeNode->containerType.isNilable)
                string_builder_append_rune(sb, cast(rune) '?');
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
            if (typeNode->containerType.isNilable)
                string_builder_append_rune(sb, cast(rune) '?');
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
            if (typeNode->containerType.isNilable)
                string_builder_append_rune(sb, cast(rune) '?');
        } break;

        case LAYE_AST_NODE_TYPE_NAMED:
        {
            if (typeNode->lookupType.isHeadless)
                string_builder_append_cstring(sb, "::");
            for (usize i = 0, iLen = arrlenu(typeNode->lookupType.path); i < iLen; i++)
            {
                if (i > 0)
                    string_builder_append_cstring(sb, "::");
                string_builder_append_string(sb, typeNode->lookupType.path[i]);
            }
            if (arrlenu(typeNode->lookupType.templateArguments) > 0)
                template_args_to_string_builder(typeNode->lookupType.templateArguments, sb);
            if (typeNode->lookupType.isNilable)
                string_builder_append_rune(sb, cast(rune) '?');
        } break;

        case LAYE_AST_NODE_TYPE_INFER: string_builder_append_cstring(sb, "var"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_NORETURN: string_builder_append_cstring(sb, "noreturn"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_RAWPTR: string_builder_append_cstring(sb, "rawptr"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_VOID: string_builder_append_cstring(sb, "void"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_STRING:
            if (typeNode->primitiveType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, "readonly ");
            else if (typeNode->primitiveType.access == LAYE_AST_ACCESS_WRITEONLY)
                string_builder_append_cstring(sb, "writeonly ");
            string_builder_append_cstring(sb, "string");
            goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_RUNE: string_builder_append_cstring(sb, "rune"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_BOOL: string_builder_append_cstring(sb, "bool"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_INT: string_builder_append_cstring(sb, "int"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_UINT: string_builder_append_cstring(sb, "uint"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_FLOAT: string_builder_append_cstring(sb, "float"); goto check_primitive_nilable;

        case LAYE_AST_NODE_TYPE_BOOL_SIZED:
            string_builder_append_cstring(sb, "b");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            goto check_primitive_nilable;

        case LAYE_AST_NODE_TYPE_INT_SIZED:
            string_builder_append_cstring(sb, "i");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            goto check_primitive_nilable;

        case LAYE_AST_NODE_TYPE_UINT_SIZED:
            string_builder_append_cstring(sb, "u");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            goto check_primitive_nilable;

        case LAYE_AST_NODE_TYPE_FLOAT_SIZED:
            string_builder_append_cstring(sb, "f");
            string_builder_append_uint(sb, typeNode->primitiveType.size);
            goto check_primitive_nilable;

        case LAYE_AST_NODE_TYPE_C_STRING:
            if (typeNode->primitiveType.access == LAYE_AST_ACCESS_READONLY)
                string_builder_append_cstring(sb, "readonly ");
            else if (typeNode->primitiveType.access == LAYE_AST_ACCESS_WRITEONLY)
                string_builder_append_cstring(sb, "writeonly ");
            string_builder_append_cstring(sb, "c_string");
            goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_CHAR: string_builder_append_cstring(sb, "c_char"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_SCHAR: string_builder_append_cstring(sb, "c_schar"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_UCHAR: string_builder_append_cstring(sb, "c_uchar"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_SHORT: string_builder_append_cstring(sb, "c_short"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_USHORT: string_builder_append_cstring(sb, "c_ushort"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_INT: string_builder_append_cstring(sb, "c_int"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_UINT: string_builder_append_cstring(sb, "c_uint"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_LONG: string_builder_append_cstring(sb, "c_long"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_ULONG: string_builder_append_cstring(sb, "c_ulong"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_LONGLONG: string_builder_append_cstring(sb, "c_longlong"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_ULONGLONG: string_builder_append_cstring(sb, "c_ulonglong"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_SIZE_T: string_builder_append_cstring(sb, "c_size_t"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_PTRDIFF_T: string_builder_append_cstring(sb, "c_ptrdiff_t"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_FLOAT: string_builder_append_cstring(sb, "c_float"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_DOUBLE: string_builder_append_cstring(sb, "c_double"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_LONGDOUBLE: string_builder_append_cstring(sb, "c_longdouble"); goto check_primitive_nilable;
        case LAYE_AST_NODE_TYPE_C_BOOL: string_builder_append_cstring(sb, "c_bool"); goto check_primitive_nilable;
    }

    return;
check_primitive_nilable:;
    if (typeNode->primitiveType.isNilable)
        string_builder_append_rune(sb, cast(rune) '?');
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
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(param.name));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, "> <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Type: ");
            RESETCOLOR;
            string typeString = laye_ast_node_type_to_string(param.valueType);
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(typeString));
            string_deallocate(typeString);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

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

    RESETCOLOR;
    fprintf(state.stream, "\n");

    usize lastStringBuilderCount = state.indents->count;
    string_builder_append_cstring(state.indents, isLast ? "  " : "│ ");

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

    string_builder_set_count(state.indents, lastStringBuilderCount);
}

static void laye_ast_fprint_struct_variant(ast_fprint_state state, laye_ast_struct_variant variant, bool isLast)
{
    laye_ast_fprint_name(state, STRING_VIEW_LITERAL("STRUCT_VARIANT"), isLast);

    if (variant.isVoid)
    {
        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
        fprintf(state.stream, " <");
        PUTCOLOR(ANSI_COLOR_BLUE);
        fprintf(state.stream, "Void");
        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
        fprintf(state.stream, ">");
    }
    else
    {
        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
        fprintf(state.stream, " <");
        PUTCOLOR(ANSI_COLOR_BLUE);
        fprintf(state.stream, "Name: ");
        RESETCOLOR;
        fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(variant.name));
        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
        fprintf(state.stream, ">");
    }

    RESETCOLOR;
    fprintf(state.stream, "\n");

    usize lastStringBuilderCount = state.indents->count;
    string_builder_append_cstring(state.indents, isLast ? "  " : "│ ");
    
    usize variantsLen = arrlenu(variant.fieldBindings);
    for (usize i = 0; i < variantsLen; i++)
        laye_ast_fprint_node(state, variant.fieldBindings[i], i == variantsLen - 1);
    
    string_builder_set_count(state.indents, lastStringBuilderCount);
}

static void laye_ast_fprint_enum_variant(ast_fprint_state state, laye_ast_enum_variant variant, bool isLast)
{
    laye_ast_fprint_name(state, STRING_VIEW_LITERAL("ENUM_VARIANT"), isLast);

    PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
    fprintf(state.stream, " <");
    PUTCOLOR(ANSI_COLOR_BLUE);
    fprintf(state.stream, "Name: ");
    RESETCOLOR;
    fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(variant.name));
    PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
    fprintf(state.stream, ">");

    RESETCOLOR;
    fprintf(state.stream, "\n");
    
    if (variant.value != nullptr)
    {
        usize lastStringBuilderCount = state.indents->count;
        string_builder_append_cstring(state.indents, isLast ? "  " : "│ ");
        laye_ast_fprint_node(state, variant.value, true);
        string_builder_set_count(state.indents, lastStringBuilderCount);
    }
}

static void laye_ast_fprint_constructor_value(ast_fprint_state state, laye_ast_constructor_value value, bool isLast)
{
    laye_ast_fprint_name(state, STRING_VIEW_LITERAL("FIELD_ASSIGN"), isLast);

    PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
    fprintf(state.stream, " <");
    PUTCOLOR(ANSI_COLOR_BLUE);
    fprintf(state.stream, "Name: ");
    RESETCOLOR;
    fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(value.name));
    PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
    fprintf(state.stream, ">");

    RESETCOLOR;
    fprintf(state.stream, "\n");
    
    usize lastStringBuilderCount = state.indents->count;
    string_builder_append_cstring(state.indents, isLast ? "  " : "│ ");
    laye_ast_fprint_node(state, value.value, true);
    string_builder_set_count(state.indents, lastStringBuilderCount);
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

        case LAYE_AST_NODE_ENUM_DECLARATION:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->enumDeclaration.name));
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

            bool isLayeVarargs = node->functionDeclaration.varargsKind == LAYE_AST_VARARGS_LAYE;
            for (usize i = 0, len = arrlenu(node->functionDeclaration.parameterBindings); i < len; i++)
            {
                if (i > 0)
                    fprintf(state.stream, ", ");

                if (i == len - 1 && isLayeVarargs)
                    fprintf(state.stream, "varargs ");

                laye_ast_node* parameterBinding = node->functionDeclaration.parameterBindings[i];

                string paramTypeString = laye_ast_node_type_to_string(parameterBinding->bindingDeclaration.declaredType);
                fprintf(state.stream, STRING_FORMAT" "STRING_FORMAT, STRING_EXPAND(paramTypeString), STRING_EXPAND(parameterBinding->bindingDeclaration.name));
                string_deallocate(paramTypeString);
            }

            if (node->functionDeclaration.varargsKind == LAYE_AST_VARARGS_C)
                fprintf(state.stream, ", varargs");

            fprintf(state.stream, ")");
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");

            usize modifiersLen = arrlen(node->functionDeclaration.modifiers);
            for (usize i = 0; i < modifiersLen; i++)
            {
                laye_ast_modifier modifier = node->functionDeclaration.modifiers[i];
                switch (modifier.kind)
                {
                    default: break;
                    case LAYE_AST_MODIFIER_EXPORT:
                    {
                        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
                        fprintf(state.stream, " <");
                        PUTCOLOR(ANSI_COLOR_BLUE);
                        fprintf(state.stream, "Export");
                        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
                        fprintf(state.stream, ">");
                    } break;
                    
                    case LAYE_AST_MODIFIER_INLINE:
                    {
                        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
                        fprintf(state.stream, " <");
                        PUTCOLOR(ANSI_COLOR_BLUE);
                        fprintf(state.stream, "Inline");
                        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
                        fprintf(state.stream, ">");
                    } break;
                    
                    case LAYE_AST_MODIFIER_FOREIGN:
                    {
                        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
                        fprintf(state.stream, " <");
                        PUTCOLOR(ANSI_COLOR_BLUE);
                        fprintf(state.stream, "Foreign");
                        if (modifier.foreignName.count > 0)
                        {
                            fprintf(state.stream, ": ");
                            RESETCOLOR;
                            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(modifier.foreignName));
                        }
                        PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
                        fprintf(state.stream, ">");
                    } break;
                }
            }
        } break;

        case LAYE_AST_NODE_EXPRESSION_LOOKUP:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Path: ");
            RESETCOLOR;
            if (node->lookupType.isHeadless)
                fprintf(state.stream, "::");
            for (usize i = 0, iLen = arrlenu(node->lookupType.path); i < iLen; i++)
            {
                if (i > 0)
                    fprintf(state.stream, "::");
                fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->lookupType.path[i]));
            }
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
            // TODO(local): it's actually awkward to print newlines, should we do anything about that?
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

        case LAYE_AST_NODE_EXPRESSION_BOOL:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Value: ");
            RESETCOLOR;
            fprintf(state.stream, "%s", node->literal.boolValue ? "true" : "false");
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

        case LAYE_AST_NODE_EXPRESSION_CONSTRUCTOR:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Type: ");
            RESETCOLOR;
            string typeString = laye_ast_node_type_to_string(node->constructor.typeName);
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(typeString));
            string_deallocate(typeString);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_NEW:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Type: ");
            RESETCOLOR;
            string typeString = laye_ast_node_type_to_string(node->new.type);
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(typeString));
            string_deallocate(typeString);
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_FIELD_INDEX:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            fprintf(state.stream, "Field Name: ");
            RESETCOLOR;
            fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->field_index.name));
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, ">");
        } break;

        case LAYE_AST_NODE_EXPRESSION_CATCH:
        {
            PUTCOLOR(ANSI_COLOR_BRIGHT_BLACK);
            fprintf(state.stream, " <");
            PUTCOLOR(ANSI_COLOR_BLUE);
            if (node->catch.captureName.count > 0)
            {
                fprintf(state.stream, "Capture Name: ");
                RESETCOLOR;
                fprintf(state.stream, STRING_FORMAT, STRING_EXPAND(node->catch.captureName));
            }
            else fprintf(state.stream, "No Capture");
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
            usize variantsLen = arrlenu(node->structDeclaration.variants);

            for (usize i = 0; i < templateParamsLen; i++)
                laye_ast_fprint_template_parameter(state, node->structDeclaration.templateParameters[i], i == templateParamsLen - 1 && fieldsLen == 0 && variantsLen == 0);

            for (usize i = 0; i < fieldsLen; i++)
                laye_ast_fprint_node(state, node->structDeclaration.fieldBindings[i], i == fieldsLen - 1 && variantsLen == 0);

            for (usize i = 0; i < variantsLen; i++)
                laye_ast_fprint_struct_variant(state, node->structDeclaration.variants[i], i == variantsLen - 1);
        } break;

        case LAYE_AST_NODE_ENUM_DECLARATION:
        {
            usize variantsLen = arrlenu(node->enumDeclaration.variants);
            for (usize i = 0; i < variantsLen; i++)
                laye_ast_fprint_enum_variant(state, node->enumDeclaration.variants[i], i == variantsLen - 1);
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
        case LAYE_AST_NODE_STATEMENT_YIELD:
        case LAYE_AST_NODE_STATEMENT_YIELD_RETURN:
        {
            if (node->returnValue != nullptr)
                laye_ast_fprint_node(state, node->returnValue, true);
        } break;

        case LAYE_AST_NODE_STATEMENT_IF:
        {
            usize condLen = arrlenu(node->_if.conditionals);
            for (usize i = 0; i < condLen; i++)
            {
                laye_ast_fprint_node(state, node->_if.conditionals[i].condition, false);
                laye_ast_fprint_node(state, node->_if.conditionals[i].body, i == condLen - 1 && node->_if.fail == nullptr);
            }

            if (node->_if.fail != nullptr)
                laye_ast_fprint_node(state, node->_if.fail, true);
        } break;

        case LAYE_AST_NODE_STATEMENT_WHILE:
        {
            laye_ast_fprint_node(state, node->_while.condition, false);
            laye_ast_fprint_node(state, node->_while.body, node->_while.fail == nullptr);

            if (node->_if.fail != nullptr)
                laye_ast_fprint_node(state, node->_while.fail, true);
        } break;

        case LAYE_AST_NODE_STATEMENT_DO_WHILE:
        {
            laye_ast_fprint_node(state, node->_while.body, false);
            laye_ast_fprint_node(state, node->_while.condition, true);
        } break;

        case LAYE_AST_NODE_EXPRESSION_INVOKE:
        {
            usize argLen = arrlenu(node->invoke.arguments);
            laye_ast_fprint_node(state, node->invoke.target, argLen == 0);
            for (usize i = 0; i < argLen; i++)
                laye_ast_fprint_node(state, node->invoke.arguments[i], i == argLen - 1);
        } break;

        case LAYE_AST_NODE_EXPRESSION_SLICE:
        {
            laye_ast_fprint_node(state, node->slice.target, node->slice.offset == nullptr && node->slice.length == nullptr);
            if (node->slice.offset != nullptr)
                laye_ast_fprint_node(state, node->slice.offset, node->slice.length == nullptr);
            if (node->slice.length != nullptr)
                laye_ast_fprint_node(state, node->slice.length, true);
        } break;

        case LAYE_AST_NODE_EXPRESSION_INDEX:
        {
            usize argLen = arrlenu(node->container_index.arguments);
            laye_ast_fprint_node(state, node->container_index.target, argLen == 0);
            for (usize i = 0; i < argLen; i++)
                laye_ast_fprint_node(state, node->container_index.arguments[i], i == argLen - 1);
        } break;

        case LAYE_AST_NODE_EXPRESSION_FIELD_INDEX:
        {
            laye_ast_fprint_node(state, node->container_index.target, true);
        } break;

        case LAYE_AST_NODE_EXPRESSION_BINARY:
        {
            laye_ast_fprint_node(state, node->binary.lhs, false);
            laye_ast_fprint_node(state, node->binary.rhs, true);
        } break;

        case LAYE_AST_NODE_EXPRESSION_CONSTRUCTOR:
        {
            usize fieldsLen = arrlenu(node->constructor.values);
            for (usize i = 0; i < fieldsLen; i++)
                laye_ast_fprint_constructor_value(state, node->constructor.values[i], i == fieldsLen - 1);
        } break;

        case LAYE_AST_NODE_EXPRESSION_NEW:
        {
            usize fieldsLen = arrlenu(node->new.values);
            if (node->new.allocator != nullptr)
                laye_ast_fprint_node(state, node->new.allocator, fieldsLen == 0);
            for (usize i = 0; i < fieldsLen; i++)
                laye_ast_fprint_constructor_value(state, node->new.values[i], i == fieldsLen - 1);
        } break;

        case LAYE_AST_NODE_EXPRESSION_TRY:
        {
            laye_ast_fprint_node(state, node->try.target, true);
        } break;

        case LAYE_AST_NODE_EXPRESSION_CATCH:
        {
            laye_ast_fprint_node(state, node->catch.target, false);
            laye_ast_fprint_node(state, node->catch.body, true);
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
