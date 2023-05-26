#include <stdio.h>

#include "kos/args.h"
#include "kos/kos.h"

typedef struct layec_file_info
{
    string_view fileName;
    string_view language;
} layec_file_info;

typedef struct layec_args
{
    string_view outputFileName;
    list(layec_file_info) files;
} layec_args;

static const char layec_desc[] = "Laye Programming Language Compiler";
static const char layec_usage[] = "layec [options...] [files...]";

static args_parse_status layec_args_parser(arg_parsed arg, args_state* state);

static arg_option options[] = {
    { "out", 'o', "file", "Write output to <file>" },
    { 0    , 'x', "language", "Treat the subsequent input files as having type <language>" },
    { 0 },
};

static args_parser parser = { options, layec_args_parser, layec_desc, layec_usage };

static args_parse_status layec_args_parser(arg_parsed arg, args_state* state)
{
    static string_view overrideLanguage = { 0 };

    layec_args* args = state->input;
    if (arg.kind == KOS_ARG_VALUE)
    {
        layec_file_info fileInfo = {
            .fileName = arg.value,
            .language = overrideLanguage,
        };
        arrput(args->files, fileInfo);
    }
    else if (arg.kind == KOS_ARG_SHORT)
    {
        switch (arg.shortOption)
        {
            case 'o': args->outputFileName = arg.value; break;
            case 'x':
            {
                if (string_view_equals_constant(arg.value, "auto"))
                    overrideLanguage = STRING_VIEW_EMPTY;
                else overrideLanguage = arg.value;
            } break;
            default: return KOS_ARGS_PARSED_ERR_UNKNOWN;
        }
    }
    else if (arg.kind == KOS_ARG_LONG)
    {
        if (string_view_equals_constant(arg.longOption, "out"))
            args->outputFileName = arg.value;
        else return KOS_ARGS_PARSED_ERR_UNKNOWN;
    }
    
    return 0;
}

int main(int argc, char** argv)
{
    layec_args args = { 0 };
    args_parse(&parser, argc, argv, &args);

    printf("output file name: "STRING_VIEW_FORMAT"\n", STRING_VIEW_EXPAND(args.outputFileName));
    printf("input files:\n");
    for (usize i = 0; i < arrlenu(args.files); i++)
    {
        layec_file_info fileInfo = args.files[i];
        printf("    "STRING_VIEW_FORMAT, STRING_VIEW_EXPAND(fileInfo.fileName));
        if (fileInfo.language.count != 0)
            printf(" ("STRING_VIEW_FORMAT")", STRING_VIEW_EXPAND(fileInfo.language));
        printf("\n");
    }

    return 0;
}
