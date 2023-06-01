#ifndef LAYEC_FRONT_LAYE_TOKEN_H
#define LAYEC_FRONT_LAYE_TOKEN_H

#include "kos/kos.h"

#include "layec/front/front.h"

#include "layec/diagnostic.h"
#include "layec/compiler.h"

#define LAYE_TOKEN_KINDS \
    T(IDENTIFIER, "<identifier>") \
    T(INTEGER, "<integer>") \
    T(BX, "<sized bool type name>") \
    T(IX, "<sized signed integer type name>") \
    T(UX, "<sized unsigned integer type name>") \
    T(FX, "<sized floating point type name>") \
    K(RETURN, "return")

typedef enum laye_token_kind
{
    LAYE_TOKEN_UNKNOWN = 0,

#define CHAR(E, C, S) LAYE_TOKEN_ ## E = C,
LAYEC_CHAR_TOKENS
#undef CHAR

    LAYE_TOKEN_MULTIBYTE_START = 255,

#define T(E, S) LAYE_TOKEN_ ## E,
#define K(E, S) T(E, S)
LAYE_TOKEN_KINDS
#undef K
#undef T
} laye_token_kind;

typedef struct laye_token
{
    laye_token_kind kind;
    layec_location location;
    string_view atom;
} laye_token;

#endif // LAYEC_FRONT_LAYE_TOKEN_H
