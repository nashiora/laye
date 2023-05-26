#include <stdio.h>
#include <string.h>

#include "kos/args.h"
#include "kos/stb_ds.h"

void kos_args_parse(kos_args_parser* parser, int argc, char** argv, void* input)
{
    kos_args_state state = {
        .input = input,
    };

    kos_args_parser_function parserFunction = parser->parserFunction;
    
    bool isOptionParsingDisabled = false;
    for (int i = 1; i < argc; i++)
    {
        char* arg = argv[i];
        usize argLength = cast(usize) strlen(arg);
        if (argLength == 0) continue;
        
        kos_string_view argView = (kos_string_view){ .memory = cast(const uchar*) arg, .count = argLength };

        if (arg[0] == '-')
        {
            if (argLength == 1)
            {
                kos_arg_parsed parsedArg = { .kind = KOS_ARG_VALUE, .value = argView };
                parserFunction(parsedArg, &state);
                continue;
            }

            if (arg[1] == '-')
            {
                if (argLength == 2)
                {
                    if (!isOptionParsingDisabled)
                    {
                        isOptionParsingDisabled = true;
                        continue;
                    }
                    else
                    {
                        kos_arg_parsed parsedArg = { .kind = KOS_ARG_VALUE, .value = argView };
                        parserFunction(parsedArg, &state);
                        continue;
                    }
                }

                kos_string_view longName = (kos_string_view){ .memory = cast(const uchar*) (arg + 2), .count = argLength - 2 };
                isize longOptionEqIndex = kos_string_view_index_of_uchar(longName, cast(uchar) '=');

                kos_string_view longOptionValue = KOS_STRING_VIEW_EMPTY;
                if (longOptionEqIndex >= 0)
                {
                    longOptionValue = kos_string_view_slice(longName, longOptionEqIndex + 1, longName.count - longOptionEqIndex - 1);
                    longName.count = longOptionEqIndex;
                }

                for (kos_arg_option* option = &parser->options[0]; option->name != nullptr || option->key != 0; option++)
                {
                    if (kos_string_view_equals_constant(longName, option->name))
                    {
                        kos_arg_parsed parsedArg = { .kind = KOS_ARG_LONG, .longOption = longName, .value = longOptionValue };

                        if (option->arg != nullptr && longOptionEqIndex < 0)
                        {
                            i++;
                            if (i >= argc)
                            {
                                fprintf(stderr, "The provided option `%s` requires an argument.\n", arg);
                                exit(1);
                                continue;
                            }
                            
                            parsedArg.value = (kos_string_view){
                                .memory = cast(const uchar*) argv[i],
                                .count = cast(usize) strlen(argv[i]),
                            };
                        }

                        parserFunction(parsedArg, &state);
                        goto continue_arg_loop;
                    }
                }

                fprintf(stderr, "The provided option `%s` is not recognized.\n", arg);
                exit(1);
                continue;
            }

            // -char
            if (argLength != 2)
            {
                fprintf(stderr, "The provided option `%s` is invalid: options preceded by a single hyphen must be single-character options. (e.g., `-o`, not `-out`)\n", arg);
                exit(1);
                continue;
            }

            int shortKey = cast(int) arg[1];
            for (kos_arg_option* option = &parser->options[0]; option->name != nullptr || option->key != 0; option++)
            {
                if (shortKey == option->key)
                {
                    kos_arg_parsed parsedArg = { .kind = KOS_ARG_SHORT, .shortOption = shortKey };

                    if (option->arg != nullptr)
                    {
                        i++;
                        if (i >= argc)
                        {
                            fprintf(stderr, "The provided option `%s` requires an argument.\n", arg);
                            exit(1);
                            continue;
                        }

                        parsedArg.value = (kos_string_view){
                            .memory = cast(const uchar*) argv[i],
                            .count = cast(usize) strlen(argv[i]),
                        };
                    }

                    parserFunction(parsedArg, &state);
                    goto continue_arg_loop;
                }
            }

            fprintf(stderr, "The provided option `%s` is not recognized.\n", arg);
            exit(1);
            continue;
        }
        else
        {
            kos_arg_parsed parsedArg = { .kind = KOS_ARG_VALUE, .value = argView };
            parserFunction(parsedArg, &state);
        }

    continue_arg_loop:;
        continue;
    }
}
