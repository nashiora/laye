#include <stdio.h>

#include "kos/args.h"
#include "kos/kos.h"

typedef struct layec_file_info
{
    string_view fileKind;
    string_view fileName;
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
    { "out", 'o', "fileName", "The name of the output file to generate." },
    { 0 },
};

static args_parser parser = { options, layec_args_parser, layec_desc, layec_usage };

static args_parse_status layec_args_parser(arg_parsed arg, args_state* state)
{
    static string_view fileKind = { 0 };

    layec_args* args = state->input;
    if (arg.kind == KOS_ARG_VALUE)
    {
        layec_file_info fileInfo = {
            .fileKind = STRING_VIEW_EMPTY,
            .fileName = arg.value,
        };
        arrput(args->files, fileInfo);
    }
    else if (arg.kind == KOS_ARG_SHORT)
    {
        switch (arg.shortOption)
        {
            case 'o': args->outputFileName = arg.value; break;
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
        printf("    "STRING_VIEW_FORMAT"\n", STRING_VIEW_EXPAND(args.files[i].fileName));
    }

    return 0;
}
