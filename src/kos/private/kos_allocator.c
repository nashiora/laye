#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "kos/ansi.h"
#include "kos/builtins.h"
#include "kos/allocator.h"

typedef struct kos_arena_block
{
    void* data;
    usize blockSize;
    usize allocated;
} kos_arena_block;

struct kos_arena_allocator
{
    kos_allocator_function baseAllocator;
    kos_arena_block* blocks;
    usize blockSize;
    usize blockCount;
};

void* kos_default_allocator(kos_allocator_action action, void* memory, usize count)
{
    switch (action)
    {
        case KOS_ALLOC_ALLOCATE: return malloc(count);
        case KOS_ALLOC_REALLOCATE: return realloc(memory, count);
        case KOS_ALLOC_DEALLOCATE: free(memory); return nullptr;
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

static void arena_add_block(kos_arena_allocator* arena)
{
    arena->blockCount++;
    kos_arena_block* newBlockMemory = kos_reallocate(arena->baseAllocator, arena->blocks, sizeof(kos_arena_block) * arena->blockCount);
    memcpy(newBlockMemory, arena->blocks, arena->blockCount - 1);

    kos_arena_block newBlock = {
        .data = kos_allocate(arena->baseAllocator, arena->blockSize),
        .blockSize = arena->blockSize,
        .allocated = 0,
    };
    newBlockMemory[arena->blockCount - 1] = newBlock;

    arena->blocks = newBlockMemory;
}

kos_arena_block* arena_current_block(kos_arena_allocator* arena)
{
    return arena->blocks + (arena->blockCount - 1);
}

kos_arena_allocator* kos_arena_create(kos_allocator_function baseAllocator, usize blockSize)
{
    if (baseAllocator == nullptr)
        baseAllocator = default_allocator;
    
    kos_arena_allocator* arena = kos_allocate(baseAllocator, sizeof(kos_arena_allocator));
    arena->baseAllocator = baseAllocator;
    arena->blocks = nullptr;
    arena->blockSize = blockSize;
    arena->blockCount = 0;

    arena_add_block(arena);
    return arena;
}

void kos_arena_destroy(kos_arena_allocator* arena)
{
    for (usize i = 0; i < arena->blockCount; i++)
        kos_deallocate(arena->baseAllocator, arena->blocks[i].data);
    kos_deallocate(arena->baseAllocator, arena->blocks);
    kos_deallocate(arena->baseAllocator, arena);
}

void kos_arena_clear(kos_arena_allocator* arena)
{
    assert(arena->blockCount > 0);

    for (usize i = 1; i < arena->blockCount; i++)
        kos_deallocate(arena->baseAllocator, arena->blocks[i].data);
    
    memset(arena->blocks[0].data, 0, arena->blockSize);
    arena->blockCount = 1;
}

void* kos_arena_push(kos_arena_allocator* arena, usize count)
{
    assert(arena != nullptr && "kos_arena_push got nullptr arena");

    if (count > arena->blockSize)
    {
        KOS_INTERNAL_PROGRAM_ERROR("attempt to allocate %zu bytes in an arena with a block size of %zu.", count, arena->blockSize);
        return nullptr;
    }

    kos_arena_block* currentBlock = arena_current_block(arena);
    assert(currentBlock != nullptr);
    if (count > currentBlock->blockSize - currentBlock->allocated)
    {
        arena_add_block(arena);
        currentBlock = arena_current_block(arena);
    }

    void* data = cast(char*) currentBlock->data + currentBlock->allocated;
    currentBlock->allocated += count;

    memset(data, 0, count);
    return data;
}
