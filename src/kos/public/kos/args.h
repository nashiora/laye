#ifndef KOS_ARGS_H
#define KOS_ARGS_H

#include "kos/builtins.h"
#include "kos/primitives.h"
#include "kos/string.h"

#ifndef KOS_NO_SHORT_NAMES
#  define args_parser_function kos_args_parser_function
#  define args_parse_status kos_args_parse_status
#  define arg_kind kos_arg_kind
#  define arg_option kos_arg_option
#  define arg_parsed kos_arg_parsed
#  define args_parser kos_args_parser
#  define args_state kos_args_state
#  define args_parse(parser, argc, argv, input) kos_args_parse(parser, argc, argv, input)
#endif // KOS_NO_SHORT_NAMES

typedef struct kos_arg_parsed kos_arg_parsed;
typedef struct kos_args_state kos_args_state;

typedef enum kos_args_parse_status (*kos_args_parser_function)(struct kos_arg_parsed arg, struct kos_args_state* state);

typedef enum kos_args_parse_status
{
    KOS_ARGS_PARSED_OK = 0,
    KOS_ARGS_PARSED_ERR_UNKNOWN,
} kos_args_parse_status;

typedef enum kos_arg_kind
{
    KOS_ARG_LONG,
    KOS_ARG_SHORT,
    KOS_ARG_VALUE,
} kos_arg_kind;

typedef struct kos_arg_option
{
    const char* name;
    int key;
    const char* arg;
    const char* doc;
    int group;
} kos_arg_option;

struct kos_arg_parsed
{
    kos_arg_kind kind;
    kos_string_view value;
    kos_string_view longOption;
    uchar shortOption;
};

typedef struct kos_args_parser
{
    kos_arg_option* options;
    kos_args_parser_function parserFunction;
    const char* description;
    const char* usage;
} kos_args_parser;

struct kos_args_state
{
    void* input;
};

void kos_args_parse(kos_args_parser* parser, int argc, char** argv, void* input);

#endif // KOS_ARGS_H
