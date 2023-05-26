#ifndef KOS_ALLOCATOR_H
#define KOS_ALLOCATOR_H

#include "kos/primitives.h"

#ifndef KOS_NO_SHORT_NAMES
#  define allocator_action kos_allocator_action
#  define allocator_function kos_allocator_function
#  define default_allocator kos_default_allocator
#  define allocate kos_allocate
#  define reallocate kos_reallocate
#  define deallocate kos_deallocate
#endif // KOS_NO_SHORT_NAMES

typedef enum kos_allocator_action
{
    KOS_ALLOC_ALLOCATE,
    KOS_ALLOC_REALLOCATE,
    KOS_ALLOC_DEALLOCATE,
} kos_allocator_action;

typedef void* (*kos_allocator_function)(kos_allocator_action action, void* memory, usize count);

void* kos_default_allocator(kos_allocator_action action, void* memory, usize count);

void* kos_allocate  (kos_allocator_function allocator, usize count);
void* kos_reallocate(kos_allocator_function allocator, void* memory, usize count);
void  kos_deallocate(kos_allocator_function allocator, void* memory);

#endif // KOS_ALLOCATOR_H
