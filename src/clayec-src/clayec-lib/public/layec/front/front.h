#ifndef LAYEC_FRONT_FRONT_H
#define LAYEC_FRONT_FRONT_H

#include "layec/compiler.h"

#define LAYEC_CHAR_TOKENS \
    CHAR(END_OF_FILE, '\0', "<eof>") \
    CHAR(BACKTICK, '`', "`") \
    CHAR(TILDE, '~', "~") \
    CHAR(BANG, '!', "!") \
    CHAR(AT, '@', "@") \
    CHAR(HASH, '#', "#") \
    CHAR(DOLLAR, '$', "$") \
    CHAR(PERCENT, '%', "%") \
    CHAR(CARET, '^', "^") \
    CHAR(AMPERSAND, '&', "&") \
    CHAR(STAR, '*', "*") \
    CHAR(OPEN_PAREN, '(', "(") \
    CHAR(CLOSE_PAREN, ')', ")") \
    CHAR(MINUS, '-', "-") \
    CHAR(UNDERSCORE, '_', "_") \
    CHAR(EQUAL, '=', "=") \
    CHAR(PLUS, '+', "+") \
    CHAR(OPEN_BRACKET, '[', "[") \
    CHAR(OPEN_BRACE, '{', "{") \
    CHAR(CLOSE_BRACKET, ']', "]") \
    CHAR(CLOSE_BRACE, '}', "}") \
    CHAR(BACKSLASH, '\\', "\\") \
    CHAR(PIPE, '|', "|") \
    CHAR(SEMI_COLON, ';', ";") \
    CHAR(SINGLE_QUOTE, '\'', "'") \
    CHAR(DOUBLE_QUOTE, '"', "\"") \
    CHAR(COMMA, ',', ",") \
    CHAR(LESS, '<', "<") \
    CHAR(DOT, '.', ".") \
    CHAR(GREATER, '>', ">") \
    CHAR(SLASH, '/', "/") \
    CHAR(QUESTION, '?', "?")

typedef enum layec_front_end_status
{
    LAYEC_FRONT_SUCCESS = 0,
    LAYEC_FRONT_NO_INPUT_FILES,
    LAYEC_FRONT_PARSE_FAILED,
} layec_front_end_status;

typedef layec_front_end_status (*layec_front_end_entry_function)(layec_context* context, list(layec_fileid) inputFiles);

#endif // LAYEC_FRONT_FRONT_H
