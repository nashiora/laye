#ifndef KOS_PLATFORM_H
#define KOS_PLATFORM_H

#include "kos/primitives.h"
#include "kos/string.h"

#ifndef _WIN32
#define KOS_PLATFORM_PATH_SEPARATOR "/"
#else
#define KOS_PLATFORM_PATH_SEPARATOR "\\"
#endif

typedef enum kos_platform_read_file_status
{
    KOS_PLATFORM_READ_FILE_SUCCESS,
    KOS_PLATFORM_READ_FILE_NULL_PATH,
    KOS_PLATFORM_READ_FILE_DOES_NOT_EXIST,
    KOS_PLATFORM_READ_FILE_FAILED,
} kos_platform_read_file_status;

kos_string kos_platform_read_file(const uchar* path, kos_allocator_function allocator, kos_platform_read_file_status* status);

#endif // PLATFORM_H
