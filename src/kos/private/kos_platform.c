#ifndef _WIN32
#include <limits.h> /* PATH_MAX */
#else
# define WIN32_LEAN_AND_MEAN
# include "Windows.h"
# include "fileapi.h"
#endif

#include <stdio.h>

#include "kos/allocator.h"
#include "kos/builtins.h"
#include "kos/primitives.h"
#include "kos/platform.h"
#include "kos/string.h"

// TODO(local): kos_platform_read_file is actually just libc calls right now
kos_string kos_platform_read_file(const char* path, kos_allocator_function allocator, kos_platform_read_file_status* status)
{
    if (path == nullptr)
    {
        if (status) *status = KOS_PLATFORM_READ_FILE_NULL_PATH;
        return KOS_STRING_EMPTY;
    }

    if (allocator == nullptr)
        allocator = kos_default_allocator;

    FILE* fileHandle = fopen(cast(const char*) path, "r");
    if (fileHandle == nullptr)
    {
        if (status) *status = KOS_PLATFORM_READ_FILE_FAILED;
        return KOS_STRING_EMPTY;
    }

    fseek(fileHandle, 0, SEEK_END);
    usize fileLength = cast(usize) ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_SET);

    uchar* sourceText = kos_allocate(allocator, fileLength);
    fread(sourceText, sizeof(uchar), fileLength, fileHandle);

    fclose(fileHandle);

    if (status) *status = KOS_PLATFORM_READ_FILE_SUCCESS;
    return kos_string_create(allocator, sourceText, fileLength);
}

kos_string kos_platform_full_path(kos_string_view path)
{
    TCHAR nameBuffer[1024] = { 0 };
    memcpy(nameBuffer, path.memory, path.count);
    
    usize outBufferCount = 1024;
    char* outBuffer = allocate(default_allocator, outBufferCount);

#ifndef _WIN32
    char* out = realpath(nameBuffer, outBuffer);
    if (out == NULL)
    {
        return KOS_STRING_EMPTY;
    }
    else
    {
        return kos_string_create(default_allocator, cast(const uchar*) outBuffer, cast(usize) strlen(outBuffer));
    }
#else
    DWORD x = GetFullPathNameA(nameBuffer, outBufferCount, outBuffer, NULL);
    if (x == 0)
    {
        return KOS_STRING_EMPTY;
    }
    else if (x < outBufferCount)
    {
        return kos_string_create(default_allocator, cast(const uchar*) outBuffer, cast(usize) x);
    }
    else
    {
        outBuffer = kos_reallocate(default_allocator, outBuffer, cast(usize) x);
        DWORD y = GetFullPathNameA(nameBuffer, x, outBuffer, NULL);
        if (y == 0)
        {
            return KOS_STRING_EMPTY;
        }

        assert(y > x);
        return kos_string_create(default_allocator, cast(const uchar*) outBuffer, cast(usize) y);
    }
#endif
}

kos_string_view kos_platform_path_up(kos_string_view path)
{
    for (isize i = path.count - 2; i >= 0; i--)
    {
        uchar c = path.memory[i];
        if (c == '/' || c == '\\')
        {
            return kos_string_view_slice(path, 0, i);
        }
    }

    return KOS_STRING_VIEW_EMPTY;
}

kos_string kos_platform_path_combine(kos_string_view path0, kos_string_view path1)
{
    kos_string_builder sb = { 0 };
    kos_string_builder_init(&sb, default_allocator);

    if (path0.count > 0)
    {
        kos_string_builder_append_view(&sb, path0);
        if (path0.memory[path0.count - 1] != '/' && path0.memory[path0.count - 1] != '\\')
        {
            kos_string_builder_append_cstring(&sb, KOS_PLATFORM_PATH_SEPARATOR);
        }
    }
    
    kos_string_builder_append_view(&sb, path1);

    string result = kos_string_builder_to_string(&sb, default_allocator);
    kos_string_builder_deallocate(&sb);

    return result;
}
