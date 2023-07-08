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
