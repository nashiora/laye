#ifndef KOS_ALLOCATOR_H
#define KOS_ALLOCATOR_H

#include "kos/primitives.h"

#ifndef KOS_NO_SHORT_NAMES
#  define allocator_action kos_allocator_action
#  define allocator_function kos_allocator_function
#  define arena_allocator kos_arena_allocator
#  define default_allocator kos_default_allocator
#  define allocate(allocator, count) kos_allocate(allocator, count)
#  define reallocate(allocator, memory, count) kos_reallocate(allocator, memory, count)
#  define deallocate(allocator, memory) kos_deallocate(allocator, memory)
#  define arena_create(baseAllocator, blockSize) kos_arena_create(baseAllocator, blockSize)
#  define arena_destroy(arena) kos_arena_destroy(arena)
#  define arena_clear(arena) kos_arena_clear(arena)
#  define arena_push(arena, count) kos_arena_push(arena, count)
#endif // KOS_NO_SHORT_NAMES

typedef enum kos_allocator_action
{
    KOS_ALLOC_ALLOCATE,
    KOS_ALLOC_REALLOCATE,
    KOS_ALLOC_DEALLOCATE,
} kos_allocator_action;

typedef void* (*kos_allocator_function)(kos_allocator_action action, void* memory, usize count);

typedef struct kos_arena_allocator kos_arena_allocator;

void* kos_default_allocator(kos_allocator_action action, void* memory, usize count);

void* kos_allocate  (kos_allocator_function allocator, usize count);
void* kos_reallocate(kos_allocator_function allocator, void* memory, usize count);
void  kos_deallocate(kos_allocator_function allocator, void* memory);

kos_arena_allocator* kos_arena_create(kos_allocator_function baseAllocator, usize blockSize);
void kos_arena_destroy(kos_arena_allocator* arena);
void kos_arena_clear(kos_arena_allocator* arena);
void* kos_arena_push(kos_arena_allocator* arena, usize count);

#endif // KOS_ALLOCATOR_H
