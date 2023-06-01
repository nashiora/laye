#include <stdio.h>

#include "layec/front/laye/front.h"

layec_front_end_status laye_front_end_entry(layec_context* context, list(layec_fileid) inputFiles)
{
    assert(context);
    
    printf("Hello, Laye front end!\n");
    
    if (arrlenu(inputFiles) == 0)
        return LAYEC_FRONT_SUCCESS;

    for (usize i = 0; i < arrlenu(inputFiles); i++)
    {
        string_view fileName = layec_context_get_file_name(context, inputFiles[i]);
        printf("  " STRING_VIEW_FORMAT "\n", STRING_VIEW_EXPAND(fileName));
        
        string fileSourceText = layec_context_get_file_source(context, inputFiles[i]);
        // TODO(local): do something with the input files
    }

    return LAYEC_FRONT_SUCCESS;
}
