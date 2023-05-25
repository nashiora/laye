#include <stdlib.h>

#include "kos/builtins.h"
#include "kos/allocator.h"

void* kos_default_allocator(kos_allocator_action action, void* memory, usize count)
{
    switch (action)
    {
        case KOS_ALLOC_ALLOCATE: return malloc(count);
        case KOS_ALLOC_REALLOCATE: return realloc(memory, count);
        case KOS_ALLOC_DEALLOCATE: free(memory); break;
        default: unreachable;
    }
}

void* kos_allocate(kos_allocator_function allocator, usize count)
{
    return allocator(KOS_ALLOC_ALLOCATE, nullptr, count);
}

void* kos_reallocate(kos_allocator_function allocator, void* memory, usize count)
{
    return allocator(KOS_ALLOC_REALLOCATE, memory, count);
}

void  kos_deallocate(kos_allocator_function allocator, void* memory)
{
    allocator(KOS_ALLOC_DEALLOCATE, memory, 0);
}
