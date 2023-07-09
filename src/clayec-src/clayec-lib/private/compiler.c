#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "kos/ansi.h"
#include "kos/kos.h"
#include "kos/platform.h"

#include "layec/compiler.h"
#include "layec/diagnostic.h"

void layec_context_init(layec_context* context)
{
    context->constantArena = arena_create(default_allocator, 10 * 1024);
    assert(context->constantArena != nullptr);
}

static layec_fileid try_read_file(layec_context* context, string_view name, string_view fullPath)
{
    usize numFiles = arrlenu(context->files);
    for (usize i = 0; i < numFiles; i++)
    {
        layec_source_file_info info = context->files[i];
        if (string_view_equals(info.fullPath, fullPath))
            return i + 1;
    }

    platform_read_file_status readStatus = 0;
    string fileSource = platform_read_file(string_view_to_cstring(fullPath, nullptr), nullptr, &readStatus);

    if (readStatus != KOS_PLATFORM_READ_FILE_SUCCESS)
        return 0;

    layec_fileid nextId = 1 + cast(layec_fileid) numFiles;
    layec_source_file_info file = { name, fullPath, fileSource };
    arrput(context->files, file);

    return nextId;
}

layec_fileid layec_context_add_file(layec_context* context, string_view name, string_view relativeTo)
{
    if (relativeTo.count > 0)
    {
        string_view relativeParent = platform_path_up(relativeTo);
        string relativeFullPath = platform_path_combine(relativeParent, name);
        assert(relativeFullPath.count > 0);
        
        string_view relativeFullPathView = string_slice(relativeFullPath, 0, relativeFullPath.count);
        layec_fileid relativeFileId = try_read_file(context, name, relativeFullPathView);

        if (relativeFileId != 0)
            return relativeFileId;

        string_deallocate(relativeFullPath);
    }
    
    string cwdFullPath = platform_full_path(name);
    assert(cwdFullPath.count > 0);
    
    string_view cwdFullPathView = string_slice(cwdFullPath, 0, cwdFullPath.count);
    layec_fileid cwdFileId = try_read_file(context, name, cwdFullPathView);

    return cwdFileId;
}

layec_fileid layec_context_add_file_with_source(layec_context* context, string_view name, string source)
{
    usize numFiles = arrlenu(context->files);
    for (usize i = 0; i < numFiles; i++)
    {
        layec_source_file_info info = context->files[i];
        if (string_view_equals(info.fullPath, name))
        {
            return 0;
        }
    }

    layec_fileid nextId = 1 + cast(layec_fileid) numFiles;
    layec_source_file_info file = { name, name, source };
    arrput(context->files, file);
    return nextId;
}

string_view layec_context_get_file_name(layec_context* context, layec_fileid fileId)
{
    assert(fileId <= arrlenu(context->files));
    if (fileId == 0)
        return STRING_VIEW_LITERAL("<invalid file>");
    return context->files[fileId - 1].name;
}

string_view layec_context_get_file_full_path(layec_context* context, layec_fileid fileId)
{
    assert(fileId <= arrlenu(context->files));
    if (fileId == 0)
        return STRING_VIEW_LITERAL("<invalid file>");
    return context->files[fileId - 1].fullPath;
}

string layec_context_get_file_source(layec_context* context, layec_fileid fileId)
{
    assert(fileId <= arrlenu(context->files));
    if (fileId == 0)
        return STRING_EMPTY;
    return context->files[fileId - 1].source;
}

static const char *severityNames[SEV_COUNT] = {
    "Info",
    "Warning",
    "Error",
    "Internal Compiler Error",
    "Sorry, unimplemented",
};

static const char *severityColors[SEV_COUNT] = {
    ANSI_COLOR_BRIGHT_BLACK,
    ANSI_COLOR_YELLOW,
    ANSI_COLOR_RED,
    ANSI_COLOR_BRIGHT_RED,
    ANSI_COLOR_MAGENTA,
};

static void get_line_from_source(string source, layec_location loc,   
    u32* lineNumber, u32* lineStartOffset, u32* lineEndOffset)
{
    assert(source.memory);
    assert(lineNumber);
    assert(lineStartOffset);
    assert(lineEndOffset);

    /// Seek to the start of the line. Keep track of the line number.
    u32 line = 1, line_start = 0;
    for (u32 i = cast(u32) loc.offset; i > 0; --i) {
        if (source.memory[i] == '\n') {
            if (!line_start)
                line_start = i + 1;
            line++;
        }
    }

    /// Don’t include the newline in the line.
    if (source.memory[line_start] == '\n') ++line_start;

    /// Seek to the end of the line.
    u32 line_end = loc.offset;
    while (line_end < source.count && source.memory[line_end] != '\n')
        line_end++;

    /// Return the results.
    if (lineNumber) *lineNumber = line;
    if (lineStartOffset) *lineStartOffset = line_start;
    if (lineEndOffset) *lineEndOffset = line_end;
}

string_view layec_view_from_location(layec_context* context, layec_location loc)
{
    string source = layec_context_get_file_source(context, loc.fileId);
    return (string_view){ source.memory + loc.offset, loc.length };
}

// TODO(local): actually figure out a mechanism for interning strings
string layec_intern_string_view(layec_context* context, string_view view)
{
    char* memory = allocate(default_allocator, view.count + 1);
    memcpy(memory, view.memory, view.count);
    memory[view.count] = 0;

    string newIntern = (string){
        .allocator = default_allocator,
        .memory = cast(const uchar*) memory,
        .count = view.count,
    };

    return newIntern;
}

EXT_FORMAT(4, 5)
void layec_debugf(layec_context* context, const char* fmt, ...)
{
    assert(context != nullptr);
    assert(fmt != nullptr);
    va_list ap;
    va_start(ap, fmt);
    layec_vdebugf(context, fmt, ap);
    va_end(ap);
}

void layec_vdebugf(layec_context* context, const char* fmt, va_list ap)
{
    assert(context != nullptr);
    assert(fmt != nullptr);
    if (context->verbose)
    {
        fprintf(stderr, ANSI_COLOR_BRIGHT_BLACK "DEBUG: ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, ANSI_COLOR_RESET);
    }
}

void layec_issue_diagnostic(layec_context* context, layec_diagnostic_severity severity, layec_location loc, const char* fmt, ...)
{
    assert(context != nullptr);
    assert(fmt != nullptr);
    va_list ap;
    va_start(ap, fmt);
    layec_vissue_diagnostic(context, severity, loc, fmt, ap);
    va_end(ap);
}
    
void layec_vissue_diagnostic(layec_context* context, layec_diagnostic_severity severity, layec_location loc, const char* fmt, va_list ap)
{
    assert(context != nullptr);
    assert(fmt != nullptr);
    assert(severity >= 0 && severity < SEV_COUNT);
    if (severity >= SEV_ERROR) context->hasIssuedHighSeverityDiagnostic = true;

    string source = layec_context_get_file_source(context, loc.fileId);
    string_view fileName = layec_context_get_file_name(context, loc.fileId);

    static bool isFirstDiagnostic = true;
    if (isFirstDiagnostic)
        isFirstDiagnostic = false;
    else fprintf(stderr, "\n");

    if (source.memory && source.count)
    {
        if (loc.offset > source.count)
            loc.offset = source.count;
        if (loc.offset + loc.length > source.count)
            loc.length = source.count - loc.offset;

        u32 lineNumber, lineStartOffset, lineEndOffset;
        get_line_from_source(source, loc, &lineNumber, &lineStartOffset, &lineEndOffset);

        if (loc.offset + loc.length > lineEndOffset)
            loc.length = lineEndOffset - loc.offset;

        fprintf(stderr, STRING_VIEW_FORMAT ":%u:%zu: %s%s" ANSI_COLOR_RESET ": ", STRING_VIEW_EXPAND(fileName), lineNumber, 1 + loc.offset - lineStartOffset, severityColors[severity], severityNames[severity]);
        vfprintf(stderr, fmt, ap);

        if (loc.length != 0)
        {
            fprintf(stderr, "\n %u | ", lineNumber);
            for (u32 i = lineStartOffset; i < loc.offset; i++)
            {
                if (source.memory[i] == '\t')
                    fprintf(stderr, "    ");
                else fputc(source.memory[i], stderr);
            }

            fprintf(stderr, "%s", severityColors[severity]);
            for (u32 i = loc.offset; i < loc.offset + loc.length; i++)
            {
                if (source.memory[i] == '\t')
                    fprintf(stderr, "    ");
                else fputc(source.memory[i], stderr);
            }
            fprintf(stderr, ANSI_COLOR_RESET);
            
            for (u32 i = loc.offset + loc.length; i < lineEndOffset; i++)
            {
                if (source.memory[i] == '\t')
                    fprintf(stderr, "    ");
                else fputc(source.memory[i], stderr);
            }

            fputc('\n', stderr);

            usize nSpaces = log10(lineNumber) + 1;
            for (usize i = 0; i < nSpaces; i++)
                fputc(' ', stderr);
            fprintf(stderr, "  | %s", severityColors[severity]);
            for (u32 i = lineStartOffset; i < loc.offset; i++)
            {
                if (source.memory[i] == '\t')
                    fprintf(stderr, "    ");
                else fputc(' ', stderr);
            }

            for (u32 i = loc.offset; i < loc.offset + loc.length; i++)
            {
                if (source.memory[i] == '\t')
                    fprintf(stderr, "~~~~");
                else fputc('~', stderr);
            }
            fprintf(stderr, ANSI_COLOR_RESET);
        }
    }
    else
    {
        fprintf(stderr, STRING_VIEW_FORMAT ": %s%s" ANSI_COLOR_RESET ": ", STRING_VIEW_EXPAND(fileName), severityColors[severity], severityNames[severity]);
        vfprintf(stderr, fmt, ap);
    }

    fputc('\n', stderr);
}

