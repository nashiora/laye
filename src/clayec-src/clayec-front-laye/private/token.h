#ifndef TOKEN_H
#define TOKEN_H

#include "kos/kos.h"

#include "layec/front/front.h"

#include "layec/diagnostic.h"
#include "layec/compiler.h"

#define LAYE_TOKEN_ALLOCATOR default_allocator

#define LAYE_TOKEN_KINDS \
    T(IDENTIFIER, "<identifier>") \
    T(LITERAL_INTEGER, "<integer>") \
    T(LITERAL_REAL, "<real>") \
    T(LITERAL_STRING, "<string>") \
    T(LESS_LESS, "<<") \
    T(GREATER_GREATER, ">>") \
    T(EQUAL_EQUAL, "==") \
    T(BANG_EQUAL, "!=") \
    T(PLUS_EQUAL, "+=") \
    T(MINUS_EQUAL, "-=") \
    T(SLASH_EQUAL, "/=") \
    T(STAR_EQUAL, "*=") \
    T(PERCENT_EQUAL, "%=") \
    T(LESS_EQUAL, "<=") \
    T(GREATER_EQUAL, ">=") \
    T(LES_LESS_EQUAL, "<<=") \
    T(GREATER_GREATER_EQUAL, ">>=") \
    T(EQUAL_GREATER, "=>") \
    T(COLON_COLON, "::") \
    T(BX, "<sized bool type name>") \
    T(IX, "<sized signed integer type name>") \
    T(UX, "<sized unsigned integer type name>") \
    T(FX, "<sized floating point type name>") \
    K(TRUE, "true") \
    K(FALSE, "false") \
    K(IF, "if") \
    K(THEN, "then") \
    K(ELSE, "else") \
    K(WHILE, "while") \
    K(DO, "do") \
    K(FOR, "for") \
    K(EACH, "each") \
    K(SWITCH, "switch") \
    K(CASE, "case") \
    K(DEFAULT, "default") \
    K(RETURN, "return") \
    K(BREAK, "break") \
    K(CONTINUE, "continue") \
    K(YIELD, "yield") \
    K(GOTO, "goto") \
    K(STRUCT, "struct") \
    K(ENUM, "enum") \
    K(USE, "use") \
    K(IMPORT, "import") \
    K(EXPORT, "export") \
    K(OPERATOR, "operator") \
    K(VOID, "void") \
    K(VAR, "var") \
    K(NORETURN, "noreturn") \
    K(RAWPTR, "rawptr") \
    K(BOOL, "bool") \
    K(STRING, "string") \
    K(RUNE, "rune") \
    K(INT, "int") \
    K(UINT, "uint") \
    K(FLOAT, "float") \
    K(ISIZE, "isize") \
    K(USIZE, "usize") \
    K(C_CHAR, "c_char") \
    K(C_STRING, "c_string") \
    K(C_SCHAR, "c_schar") \
    K(C_UCHAR, "c_uchar") \
    K(C_SHORT, "c_short") \
    K(C_USHORT, "c_ushort") \
    K(C_INT, "c_int") \
    K(C_UINT, "c_uint") \
    K(C_LONG, "c_long") \
    K(C_ULONG, "c_ulong") \
    K(C_LONGLONG, "c_longlong") \
    K(C_ULONGLONG, "c_ulonglong") \
    K(C_SIZE_T, "c_size_t") \
    K(C_PTRDIFF_T, "c_ptrdiff_t") \
    K(C_FLOAT, "c_float") \
    K(C_DOUBLE, "c_double") \
    K(C_LONGDOUBLE, "c_longdouble") \
    K(C_BOOL, "c_bool") \
    K(CAST, "cast") \
    K(DISCARD, "discard") \
    K(SIZEOF, "sizeof") \
    K(ALIGNOF, "alignof") \
    K(OFFSETOF, "offsetof") \
    K(NOT, "not") \
    K(AND, "and") \
    K(OR, "or") \
    K(XOR, "xor") \
    K(CONST, "const") \
    K(FOREIGN, "foreign") \
    K(INLINE, "inline") \
    K(CALLCONV, "callconv")

typedef enum laye_token_kind
{
    LAYE_TOKEN_INVALID,

#define CHAR(E, C, S) LAYE_TOKEN_ ## E = C,
LAYEC_CHAR_TOKENS
#undef CHAR

    LAYE_TOKEN_MULTIBYTE_START = 255,

#define T(E, S) LAYE_TOKEN_ ## E,
#define K(E, S) LAYE_TOKEN_ ## E,
LAYE_TOKEN_KINDS
#undef K
#undef T

    LAYE_TOKEN_UNKNOWN,
    LAYE_TOKEN_MAX,
} laye_token_kind;

typedef struct laye_token
{
    laye_token_kind kind;
    layec_location location;
    string_view atom;
    union
    {
        int sizeParameter;
        u64 integerValue;
        string stringValue;
    };
} laye_token;

#endif // TOKEN_H
