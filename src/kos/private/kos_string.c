#include "kos/builtins.h"
#include "kos/primitives.h"
#include "kos/string.h"

kos_string kos_string_allocate(kos_allocator_function allocator, usize count)
{
    if (allocator == nullptr)   
        allocator = kos_default_allocator;
    
    const uchar* memory = kos_allocate(allocator, count);
    return kos_string_create(allocator, memory, count);
}

kos_string kos_string_create(kos_allocator_function allocator, const uchar* memory, usize count)
{
    assert(allocator != nullptr);
    return (kos_string){
        .allocator = allocator,
        .memory = memory,
        .count = count,
    };
}

kos_string kos_string_deallocate(kos_string s)
{
    kos_deallocate(s.allocator, cast(void*) s.memory);
}
