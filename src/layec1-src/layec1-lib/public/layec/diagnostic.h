#ifndef LAYEC_DIAGNOSTIC_H
#define LAYEC_DIAGNOSTIC_H

#include <stdarg.h>

#include "kos/kos.h"

#define ICE(...) (raise_fatal_error(__FILE__, PRETTY_FUNCTION, __LINE__, false, false, NULL, __VA_ARGS__), BUILTIN_UNREACHABLE())
#define TODO(...) (raise_fatal_error(__FILE__, PRETTY_FUNCTION, __LINE__, false, true, NULL, "" __VA_ARGS__), BUILTIN_UNREACHABLE())

typedef usize layec_fileid;

typedef enum layec_diagnostic_severity
{
    SEV_INFO,
    SEV_WARN,
    SEV_ERROR,
    SEV_ICE,
    SEV_SORRY,
    SEV_COUNT,
} layec_diagnostic_severity;

typedef struct layec_location
{
    layec_fileid fileId;
    usize offset, length;
} layec_location;

layec_location layec_location_combine(layec_location a, layec_location b);

#endif // LAYEC_DIAGNOSTIC_H
