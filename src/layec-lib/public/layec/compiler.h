#ifndef LAYEC_COMPILER_H
#define LAYEC_COMPILER_H

#include "kos/kos.h"

#include "layec/diagnostic.h"

typedef struct layec_source_file_info
{
    string_view name;
    string source;
} layec_source_file_info;

typedef struct layec_context
{
    bool hasIssuedHighSeverityDiagnostic;
    list(layec_source_file_info) files;
} layec_context;

layec_fileid layec_context_add_file(layec_context* context, string_view name, string source);
string_view layec_context_get_file_name(layec_context* context, layec_fileid fileId);
string layec_context_get_file_source(layec_context* context, layec_fileid fileId);

string_view layec_view_from_location(layec_context* context, layec_location loc);

EXT_FORMAT(4, 5)
void layec_issue_diagnostic(layec_context* context, layec_diagnostic_severity severity, layec_location loc, const char* fmt, ...);
    
void layec_vissue_diagnostic(layec_context* context, layec_diagnostic_severity severity, layec_location loc, const char* fmt, va_list ap);

#endif // LAYEC_COMPILER_H
