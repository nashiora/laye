#include <stdio.h>

#include "kos/args.h"
#include "kos/kos.h"
#include "kos/platform.h"

#include "layec/compiler.h"

#include "layec/front/front.h"

#include "layec/front/laye/front.h"

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

static void debug_print_args(layec_args args)
{
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
}

typedef struct language_front_end_info
{
    const char* name;
    const char* extension;
    layec_front_end_entry_function entryFunction;
} language_front_end_info;

static language_front_end_info frontEndInfos[] = {
    { "laye", ".laye", laye_front_end_entry },
    { 0 },
};

typedef struct front_end_data
{
    layec_front_end_entry_function entryFunction;
    list(layec_fileid) files;
} front_end_data;

int main(int argc, char** argv)
{
    layec_args args = { 0 };
    args_parse(&parser, argc, argv, &args);

    debug_print_args(args);

    layec_context context = { 0 };
    list(front_end_data*) frontEndsToInvoke = nullptr;

    for (usize i = 0; i < arrlenu(args.files); i++)
    {
        layec_file_info fileInfo = args.files[i];
        platform_read_file_status readStatus = 0;
        string fileSource = platform_read_file(string_view_to_cstring(fileInfo.fileName, nullptr), nullptr, &readStatus);

        if (readStatus != KOS_PLATFORM_READ_FILE_SUCCESS)
        {
            layec_location loc = { 0, 0, 0 };
            layec_issue_diagnostic(&context, SEV_ERROR, loc, "Failed to read source file `"STRING_VIEW_FORMAT"`.", STRING_VIEW_EXPAND(fileInfo.fileName));
            continue;
        }
        
        layec_fileid fileId = layec_context_add_file(&context, fileInfo.fileName, fileSource);

        for (usize j = 0; frontEndInfos[j].name != NULL; j++)
        {
            language_front_end_info frontEndInfo = frontEndInfos[j];
            if (fileInfo.language.count != 0)
            {
                if (!string_view_equals_constant(fileInfo.language, frontEndInfo.name))
                {
                    continue;
                }
            }
            else if (!string_view_ends_with_constant(fileInfo.fileName, frontEndInfo.extension))
            {
                continue;
            }

            front_end_data* frontEndData = nullptr;
            for (usize k = 0; k < arrlenu(frontEndsToInvoke); k++)
            {
                if (frontEndsToInvoke[k]->entryFunction == frontEndInfo.entryFunction)
                {
                    frontEndData = frontEndsToInvoke[k];
                    break;
                }
            }

            if (frontEndData == nullptr)
            {
                frontEndData = calloc(1, sizeof(front_end_data));
                frontEndData->entryFunction = frontEndInfo.entryFunction;
                arrput(frontEndsToInvoke, frontEndData);
            }

            arrput(frontEndData->files, fileId);
            break;
        }
    }

    for (usize k = 0; k < arrlenu(frontEndsToInvoke); k++)
    {
        front_end_data* frontEndData = frontEndsToInvoke[k];
        frontEndData->entryFunction(&context, frontEndData->files);
    }

    return 0;
}