#ifndef PARSER_H
#define PARSER_H

#include "kos/kos.h"

#include "ast.h"
#include "token.h"

typedef enum laye_parse_result_status
{
    LAYE_PARSE_OK,
    LAYE_PARSE_FAILURE,
} laye_parse_result_status;

typedef struct laye_parse_result
{
    laye_parse_result_status status;
    laye_ast ast;
} laye_parse_result;

list(laye_token*) laye_lex(layec_context* context, layec_fileid fileId);
laye_parse_result laye_parse(layec_context* context, layec_fileid fileId);

#endif // PARSER_H
