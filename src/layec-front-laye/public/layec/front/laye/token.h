#ifndef LAYEC_FRONT_LAYE_TOKEN_H
#define LAYEC_FRONT_LAYE_TOKEN_H

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
} laye_token;

#endif // LAYEC_FRONT_LAYE_TOKEN_H
