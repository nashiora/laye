#ifndef LAYEC_DIAGNOSTIC_H
#define LAYEC_DIAGNOSTIC_H

#include <stdarg.h>

#include "kos/kos.h"

#ifndef LAYEC_NO_SHORT_NAMES
//#  define fileid layec_fileid
//#  define diagnostic_severity layec_diagnostic_severity
//#  define location layec_location

//#  define view_from_location layec_view_from_location
#  define issue_diagnostic layec_issue_diagnostic
#  define vissue_diagnostic layec_vissue_diagnostic
#endif // LAYEC_NO_SHORT_NAMES

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

#endif // LAYEC_DIAGNOSTIC_H
