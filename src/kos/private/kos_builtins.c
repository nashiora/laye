#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kos/ansi.h"
#include "kos/builtins.h"
#include "kos/platform.h"

static const char* mystrstr(const char* haystack, const char* needle)
{
    u32 term = needle[0] << 24 | needle[1] << 16 | needle[2] << 8 | needle[3];
    u32 walk = haystack[0] << 24 | haystack[1] << 16 | haystack[2] << 8 | haystack[3];

    do
    {
        if (term == walk) return haystack;
        haystack++;
        walk = (walk << 8) | haystack[3];
    } while (haystack[3]);

    return nullptr;
}

noreturn void kos_raise_fatal_error(
    const char* file, const char* func, int line,
    bool isSignalOrException, bool isSorry,
    const char* assertCondition,
    const char* fmt, ...)
{
    const char* fileName = file;

    const char* srcPrefix;
    while (srcPrefix = mystrstr(fileName, "src" KOS_PLATFORM_PATH_SEPARATOR))
        fileName = srcPrefix + 4;

    const char* const color = isSorry ? ANSI_COLOR_BRIGHT_CYAN : ANSI_COLOR_BRIGHT_RED;

    if (isSorry && strcmp(fmt, "") == 0)
    {
        fprintf(stderr, "%s:%d: %sSorry! :( This is not implemented: Function '%s'\n",
            fileName, line, color, func);
    }
    else
    {
        if (isSignalOrException)
        {
            fprintf(stderr, "%s: In function '%s'\n%s:%d: %s",
                fileName, func, fileName, line, color);
        }
        else fprintf(stderr, "%s", color);

        if (assertCondition)
            fprintf(stderr, "Assertion failed" ANSI_COLOR_RESET ": '%s': ", assertCondition);
        else fprintf(stderr, "%s" ANSI_COLOR_RESET ": ", (isSorry ? "Sorry, Unimplemented" : "Internal Program Error"));

        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fputc('\n', stderr);
    }

    exit(1);
}
