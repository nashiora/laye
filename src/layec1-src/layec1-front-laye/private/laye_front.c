#include <stdio.h>

#include "layec/front/laye/front.h"

#include "ast.h"
#include "token.h"
#include "parser.h"

layec_front_end_status laye_front_end_entry(layec_context* context, list(layec_fileid) inputFiles)
{
    assert(context);
    
    printf("Hello, Laye front end!\n");
    
    if (arrlenu(inputFiles) == 0)
        return LAYEC_FRONT_NO_INPUT_FILES;

    list(laye_ast) asts = nullptr;
    for (usize i = 0; i < arrlenu(inputFiles); i++)
    {
        laye_parse_result parseResult = laye_parse(context, inputFiles[i]);
        if (parseResult.status != LAYE_PARSE_OK)
            return LAYEC_FRONT_PARSE_FAILED;
        
        arrput(asts, parseResult.ast);
    }

    return LAYEC_FRONT_SUCCESS;
}
