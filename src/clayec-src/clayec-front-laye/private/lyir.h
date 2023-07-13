#ifndef LAYE_LYIR_H
#define LAYE_LYIR_H

#include "kos/kos.h"

#include "layec/front/front.h"

#include "layec/diagnostic.h"
#include "layec/compiler.h"

#define LYIR_TYPE_KINDS(T) \
    T(VOID) \
    T(INT) \
    T(FLOAT) \
    T(POINTER) \
    T(STRUCT) \
    T(FUNCTION) \
    T(ARRAY)

typedef enum lyir_constant_kind
{
    LYIR_CONST_INTEGER,
    LYIR_CONST_FLOAT,
    LYIR_CONST_STRING,
} lyir_constant_kind;

typedef struct lyir_constant
{
    lyir_constant_kind kind;
    union
    {
        u64 integerValue;
        long double floatValue;
        string stringValue;
    };
} lyir_constant;

typedef struct lyir_type lyir_type;

typedef enum lyir_type_kind
{
    LYIR_TYPE_INVALID,

#define T(E) LYIR_TYPE_ ## E,
LYIR_TYPE_KINDS(T)
#undef T
} lyir_type_kind;

typedef enum lyir_calling_convention
{
    LYIR_CC_LAYE,
    LYIR_CC_CDECL,
    LYIR_CC_STDCALL,
    LYIR_CC_FASTCALL,
} lyir_calling_convention;

typedef struct lyir_type_flags
{
    bool isVarargs  : 1;
} lyir_type_flags;

struct lyir_type
{
    lyir_type_kind kind;
    int width;

    lyir_type_flags flags;
    string foreignName;

    union
    {
        struct
        {
            // the name of this struct; should assert that is is nul terminated
            string name;
            list(lyir_type*) fieldTypes;
            list(lyir_type*) variantTypes;
        } _struct;

        struct
        {
            lyir_calling_convention callingConvention;
            lyir_type* returnType;
            list(lyir_type*) parameterTypes;
        } function;

        struct
        {
            lyir_type* elementType;
            list(usize) ranks;
        } array;
    };
};

typedef struct lyir_symbol_flags
{
    bool isFunction : 1;
    bool isGlobal   : 1;
    bool isConstant : 1;
    bool isForeign  : 1;
    bool isInline   : 1;
    bool isExported : 1;
} lyir_symbol_flags;

typedef struct lyir_symbol
{
    // the name of this symbol; should assert that is is nul terminated
    string name;
    lyir_symbol_flags flags;
    lyir_type* type;
} lyir_symbol;

#define LYIR_OP_KINDS(O) \
    O(LOAD, "load")

typedef enum lyir_op_kind
{
    LYIR_OP_NOP,

#define O(E, S) LYIR_OP_ ## E,
LYIR_OP_KINDS(O)
#undef O

    LYIR_OP_COUNT,
} lyir_op_kind;

typedef struct lyir_block lyir_block;

typedef struct lyir_op
{
    lyir_op_kind kind;
    string name;
} lyir_op;

struct lyir_block
{
    string name;
    list(lyir_op*) ops;
};

typedef struct lyir_function
{
    lyir_symbol* symbol;
    list(lyir_block*) blocks;
} lyir_function;

typedef struct lyir_binding
{
    lyir_symbol* symbol;
    lyir_constant value;
} lyir_binding;

typedef struct lyir_module
{
    layec_context* context;

    arena_allocator* symbolArena;
    strmap(lyir_symbol*) symbols;

    arena_allocator* opsArena;
    strmap(lyir_function) functions;
    strmap(lyir_binding) globals;
} lyir_module;

#endif // LAYE_LYIR_H
