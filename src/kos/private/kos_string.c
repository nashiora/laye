#include <string.h>

#include "kos/builtins.h"
#include "kos/primitives.h"
#include "kos/string.h"
#include "kos/utf8.h"

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
    assert(s.allocator);
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

static void kos_string_builder_ensure_capacity(kos_string_builder* sb, usize minCapacity)
{
    assert(sb->allocator);

    if (sb->memory == nullptr)
    {
        assert(sb->capacity == 0);
        assert(sb->count == 0);

        sb->memory = kos_allocate(sb->allocator, minCapacity);
        assert(sb->memory);

        sb->capacity = minCapacity;

        return;
    }

    assert(sb->memory);
    assert(sb->capacity != 0);

    if (sb->capacity >= minCapacity)
        return;

    usize newCapacity = sb->capacity * 2;
    while (newCapacity < minCapacity)
        newCapacity = newCapacity * 2;

    sb->memory = kos_reallocate(sb->allocator, sb->memory, newCapacity);
    assert(sb->memory);

    sb->capacity = newCapacity;
}

void kos_string_builder_init(kos_string_builder* sb, kos_allocator_function allocator)
{
    if (allocator == nullptr)
        allocator = default_allocator;
    
    sb->allocator = allocator;
    sb->memory = nullptr;
    sb->count = 0;
}

void kos_string_builder_deallocate(kos_string_builder* sb)
{
    assert(sb->allocator);
    kos_deallocate(sb->allocator, cast(void*) sb->memory);
}

kos_string kos_string_builder_to_string(kos_string_builder* sb, kos_allocator_function allocator)
{
    if (allocator == nullptr)
        allocator = sb->allocator;
    assert(allocator);

    string result = { 0 };
    result.allocator = allocator;
    result.count = sb->count;

    uchar* memory = kos_allocate(result.allocator, result.count + 1);
    memory[result.count] = 0; // doesn't hurt to nul terminate anyway
    memcpy(memory, sb->memory, result.count);

    result.memory = memory;
    return result;
}

kos_string kos_string_builder_to_string_arena(kos_string_builder* sb, kos_arena_allocator* arena)
{
    assert(arena);

    string result = { 0 };
    result.allocator = nullptr;
    result.count = sb->count;

    uchar* memory = kos_arena_push(arena, result.count + 1);
    memory[result.count] = 0; // doesn't hurt to nul terminate anyway
    memcpy(memory, sb->memory, result.count);
    
    result.memory = memory;
    return result;
}

void kos_string_builder_append_rune(kos_string_builder* sb, rune value)
{
    int runeByteCount = kos_utf8_calc_rune_required_byte_count(value);
    kos_string_builder_ensure_capacity(sb, sb->count + runeByteCount);

    uchar* runeStart = sb->memory + sb->count;
    kos_utf8_encode_result encodeResult = kos_utf8_encode_rune(cast(char*) runeStart, sb->capacity - sb->count, value);

    if (encodeResult.kind == KOS_UTF8_ENCODE_OK)
    {
        sb->count += runeByteCount;
        return;
    }

    runeStart[0] = '?';
    sb->count++;
}
