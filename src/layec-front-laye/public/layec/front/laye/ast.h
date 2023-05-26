#ifndef LAYEC_FRONT_LAYE_AST_H
#define LAYEC_FRONT_LAYE_AST_H

#include "kos/kos.h"

#define LAYE_AST_NODE_KINDS \
    A(LOOKUP)

typedef enum laye_ast_node_kind
{
#define A(E) LAYE_AST_NODE_ ## E,
LAYE_AST_NODE_KINDS
#undef T
} laye_ast_node_kind;

typedef struct laye_ast_node
{
    laye_ast_node_kind kind;
} laye_ast_node;

#endif // LAYEC_FRONT_LAYE_AST_H
