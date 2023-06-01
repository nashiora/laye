#include <string.h>

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

const char* kos_string_to_cstring(kos_string s, kos_allocator_function allocator)
{
    if (allocator == nullptr)
        allocator = s.allocator;
    if (allocator == nullptr)
        allocator = kos_default_allocator;

    assert(allocator);

    char* result = allocate(allocator, s.count + 1);
    result[s.count] = 0;
    memcpy(result, s.memory, s.count);

    return result;
}

isize kos_string_index_of_uchar(kos_string s, uchar uc)
{
    for (isize i = 0; i < s.count; i++)
    {
        if (s.memory[i] == uc)
            return i;
    }

    return -1;
}

kos_string_view kos_string_slice(string s, usize offset, usize count)
{
    if (offset > s.count)
        offset = s.count;
    
    if (count > s.count - offset)
        count = s.count - offset;
    
    return (kos_string_view){
        .memory = s.memory + offset,
        .count = count,
    };
}

kos_string_view kos_string_view_slice(string_view sv, usize offset, usize count)
{
    if (offset > sv.count)
        offset = sv.count;
    
    if (count > sv.count - offset)
        count = sv.count - offset;
    
    return (kos_string_view){
        .memory = sv.memory + offset,
        .count = count,
    };
}

const char* kos_string_view_to_cstring(kos_string_view sv, kos_allocator_function allocator)
{
    if (allocator == nullptr)
        allocator = kos_default_allocator;

    assert(allocator);

    char* result = allocate(allocator, sv.count + 1);
    result[sv.count] = 0;
    memcpy(result, sv.memory, sv.count);

    return result;
}

isize kos_string_view_index_of_uchar(kos_string_view sv, uchar uc)
{
    for (isize i = 0; i < sv.count; i++)
    {
        if (sv.memory[i] == uc)
            return i;
    }

    return -1;
}

bool kos_string_view_equals(kos_string_view sv, kos_string_view other)
{
    if (sv.count != other.count)
        return false;
    
    return 0 == memcmp(sv.memory, other.memory, other.count);
}

bool kos_string_view_equals_constant(kos_string_view sv, const char* constant)
{
    usize constantLength = cast(usize) strlen(constant);
    if (sv.count != constantLength)
        return false;
    
    return 0 == memcmp(sv.memory, constant, constantLength);
}

bool kos_string_view_ends_with(kos_string_view sv, kos_string_view other)
{
    if (sv.count < other.count)
        return false;
    
    return 0 == memcmp(sv.memory + (sv.count - other.count), other.memory, other.count);
}

bool kos_string_view_ends_with_constant(kos_string_view sv, const char* constant)
{
    usize constantLength = cast(usize) strlen(constant);
    if (sv.count < constantLength)
        return false;
    
    return 0 == memcmp(sv.memory + (sv.count - constantLength), constant, constantLength);
}
