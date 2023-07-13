#ifndef KOS_STRING_H
#define KOS_STRING_H

#include "kos/allocator.h"
#include "kos/primitives.h"

#define KOS_STRING_EMPTY ((kos_string){ 0 })
#define KOS_STRING_LITERAL(L) ((kos_string){ nullptr, cast(const uchar*) (L), (usize)((sizeof L) - 1), .isNulTerminated = true })
#define KOS_STRING_FORMAT "%.*s"
#define KOS_STRING_EXPAND(S) (cast(int) (S).count), (cast(const char*) (S).memory)
#define KOS_STRING_VIEW_EMPTY ((kos_string_view){ 0 })
#define KOS_STRING_VIEW_LITERAL(L) ((kos_string_view){ cast(const uchar*) (L), (usize)((sizeof L) - 1) })
#define KOS_STRING_VIEW_FORMAT "%.*s"
#define KOS_STRING_VIEW_EXPAND(S) (cast(int) (S).count), (cast(const char*) (S).memory)

#ifndef KOS_NO_SHORT_NAMES
#  define STRING_EMPTY KOS_STRING_EMPTY
#  define STRING_LITERAL KOS_STRING_LITERAL
#  define STRING_FORMAT KOS_STRING_FORMAT
#  define STRING_EXPAND KOS_STRING_EXPAND
#  define STRING_VIEW_EMPTY KOS_STRING_VIEW_EMPTY
#  define STRING_VIEW_LITERAL KOS_STRING_VIEW_LITERAL
#  define STRING_VIEW_FORMAT KOS_STRING_VIEW_FORMAT
#  define STRING_VIEW_EXPAND KOS_STRING_VIEW_EXPAND
#  define string kos_string
#  define string_view kos_string_view
#  define string_builder kos_string_builder

#  define string_allocate(allocator, count) kos_string_allocate(allocator, count)
#  define string_create(allocator, memory, count) kos_string_create(allocator, memory, count)
#  define string_deallocate(s) kos_string_deallocate(s)
#  define string_to_cstring(s, allocator) kos_string_to_cstring(s, allocator)
#  define string_index_of_uchar(s, uc) kos_string_index_of_uchar(s, uc)

#  define string_to_view(s) kos_string_to_view(s)
#  define string_slice(s, offset, length) kos_string_slice(s, offset, length)
#  define string_view_slice(sv, offset, length) kos_string_view_slice(sv, offset, length)
#  define string_view_to_cstring(sv, allocator) kos_string_view_to_cstring(sv, allocator)
#  define string_view_index_of_uchar(sv, uc) kos_string_view_index_of_uchar(sv, uc)

#  define string_view_equals(sv, other) kos_string_view_equals(sv, other)
#  define string_view_equals_constant(sv, constant) kos_string_view_equals_constant(sv, constant)
#  define string_view_ends_with(sv, other) kos_string_view_ends_with(sv, other)
#  define string_view_ends_with_constant(sv, constant) kos_string_view_ends_with_constant(sv, constant)

#  define string_builder_init(sb, allocator) kos_string_builder_init(sb, allocator)
#  define string_builder_deallocate(sb) kos_string_builder_deallocate(sb)
#  define string_builder_to_string(sb, allocator) kos_string_builder_to_string(sb, allocator)
#  define string_builder_to_string_arena(sb, arena) kos_string_builder_to_string_arena(sb, arena)
#  define string_builder_append_rune(sb, value) kos_string_builder_append_rune(sb, value)
#  define string_builder_append_uint(sb, value) kos_string_builder_append_uint(sb, value)
#  define string_builder_append_string(sb, s) kos_string_builder_append_string(sb, s)
#  define string_builder_append_view(sb, value) kos_string_builder_append_view(sb, value)
#  define string_builder_append_cstring(sb, s) kos_string_builder_append_cstring(sb, s)
#  define string_builder_set_count(sb, count) kos_string_builder_set_count(sb, count)
#endif // KOS_NO_SHORT_NAMES

/* owning UTF-8 string. */
typedef struct kos_string
{
    kos_allocator_function allocator;
    // the (owned) allocated memory for this string.
    const uchar* memory;
    // the total number of bytes in the allocated memory block.
    usize count;
    // true if there is a 0 terminating this string, even if it's not part of `count`.
    bool isNulTerminated;
} kos_string;

typedef struct kos_string_view
{
    // the (non-owned) allocated memory for this string.
    const uchar* memory;
    // the total number of bytes in the we're viewing in the memory block.
    usize count;
} kos_string_view;

typedef struct kos_string_builder
{
    kos_allocator_function allocator;
    uchar* memory;
    usize capacity;
    usize count;
} kos_string_builder;

kos_string kos_string_allocate(kos_allocator_function allocator, usize count);
kos_string kos_string_create(kos_allocator_function allocator, const uchar* memory, usize count);
void kos_string_deallocate(kos_string s);
const char* kos_string_to_cstring(kos_string s, kos_allocator_function allocator);
isize kos_string_index_of_uchar(kos_string s, uchar uc);

kos_string_view kos_string_to_view(string s);
kos_string_view kos_string_slice(string s, usize offset, usize count);
kos_string_view kos_string_view_slice(string_view sv, usize offset, usize count);
const char* kos_string_view_to_cstring(kos_string_view sv, kos_allocator_function allocator);
isize kos_string_view_index_of_uchar(kos_string_view sv, uchar uc);

bool kos_string_view_equals(kos_string_view sv, kos_string_view other);
bool kos_string_view_equals_constant(kos_string_view sv, const char* constant);
bool kos_string_view_ends_with(kos_string_view sv, kos_string_view other);
bool kos_string_view_ends_with_constant(kos_string_view sv, const char* constant);

void kos_string_builder_init(kos_string_builder* sb, kos_allocator_function allocator);
void kos_string_builder_deallocate(kos_string_builder* sb);
kos_string kos_string_builder_to_string(kos_string_builder* sb, kos_allocator_function allocator);
kos_string kos_string_builder_to_string_arena(kos_string_builder* sb, kos_arena_allocator* arena);

void kos_string_builder_append_rune(kos_string_builder* sb, rune value);
void kos_string_builder_append_uint(kos_string_builder* sb, usize value);
void kos_string_builder_append_string(kos_string_builder* sb, kos_string s);
void kos_string_builder_append_view(kos_string_builder* sb, kos_string_view value);
void kos_string_builder_append_cstring(kos_string_builder* sb, const char* s);
void kos_string_builder_set_count(kos_string_builder* sb, usize count);

#endif // KOS_STRING_H
