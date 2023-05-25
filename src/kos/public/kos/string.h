#ifndef KOS_STRING_H
#define KOS_STRING_H

#include "kos/allocator.h"
#include "kos/primitives.h"

#define KOS_STRING_EMPTY ((kos_string){ 0 })
#define KOS_STRING_FORMAT "%.*s"
#define KOS_STRING_EXPAND(S) (cast(int) (S).count), (cast(const char*) (S).memory)

/* owning UTF-8 string. */
typedef struct kos_string
{
    kos_allocator_function allocator;
    // the (owned) allocated memory for this string.
    const uchar* memory;
    // the total number of bytes in the allocated memory block.
    usize count;
} kos_string;

kos_string kos_string_allocate(kos_allocator_function allocator, usize count);
kos_string kos_string_create(kos_allocator_function allocator, const uchar* memory, usize count);
kos_string kos_string_deallocate(kos_string s);

#endif // KOS_STRING_H
