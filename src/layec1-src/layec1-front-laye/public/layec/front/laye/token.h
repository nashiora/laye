#ifndef LAYEC_FRONT_LAYE_TOKEN_H
#define LAYEC_FRONT_LAYE_TOKEN_H

#include "kos/kos.h"

#include "layec/diagnostic.h"
#include "layec/compiler.h"

#define LAYE_TOKEN_KINDS \
    T(IDENTIFIER)

typedef enum laye_token_kind
{
#define T(E) LAYE_TOKEN_ ## E,
LAYE_TOKEN_KINDS
#undef T
} laye_token_kind;

typedef struct laye_token
{
    laye_token_kind kind;
    layec_location location;
    string_view atom;
} laye_token;

#endif // LAYEC_FRONT_LAYE_TOKEN_H
