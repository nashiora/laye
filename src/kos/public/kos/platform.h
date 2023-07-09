#ifndef KOS_PLATFORM_H
#define KOS_PLATFORM_H

#include "kos/primitives.h"
#include "kos/string.h"

#ifndef KOS_NO_SHORT_NAMES
#  define PLATFORM_PATH_SEPARATOR KOS_PLATFORM_PATH_SEPARATOR
#  define platform_read_file_status kos_platform_read_file_status
#  define platform_read_file(path, allocator, status) kos_platform_read_file(path, allocator, status)
#  define platform_full_path(path) kos_platform_full_path(path)
#  define platform_path_up(path) kos_platform_path_up(path)
#  define platform_path_combine(path0, path1) kos_platform_path_combine(path0, path1)
#endif // KOS_NO_SHORT_NAMES

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

kos_string kos_platform_read_file(const char* path, kos_allocator_function allocator, kos_platform_read_file_status* status);

kos_string kos_platform_full_path(kos_string_view path);

kos_string_view kos_platform_path_up(kos_string_view path);
kos_string kos_platform_path_combine(kos_string_view path0, kos_string_view path1);

#endif // PLATFORM_H
