#ifndef KOS_STRING_H
#define KOS_STRING_H

#include "kos/allocator.h"
#include "kos/primitives.h"

#define KOS_STRING_EMPTY ((kos_string){ 0 })
#define KOS_STRING_LITERAL(L) ((kos_string){ nullptr, L, (usize)((sizeof L) - 1) })
#define KOS_STRING_FORMAT "%.*s"
#define KOS_STRING_EXPAND(S) (cast(int) (S).count), (cast(const char*) (S).memory)
#define KOS_STRING_VIEW_EMPTY ((kos_string_view){ 0 })
#define KOS_STRING_VIEW_LITERAL(L) ((kos_string_view){ L, (usize)((sizeof L) - 1) })
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

#  define string_allocate(allocator, count) kos_string_allocate(allocator, count)
#  define string_create(allocator, memory, count) kos_string_create(allocator, memory, count)
#  define string_deallocate(s) kos_string_deallocate(s)
#  define string_to_cstring(s, allocator) kos_string_to_cstring(s, allocator)
#  define string_index_of_uchar(s, uc) kos_string_index_of_uchar(s, uc)

#  define string_slice(s, offset, length) kos_string_slice(s, offset, length)
#  define string_view_slice(sv, offset, length) kos_string_view_slice(sv, offset, length)
#  define string_view_to_cstring(sv, allocator) kos_string_view_to_cstring(sv, allocator)
#  define string_view_index_of_uchar(sv, uc) kos_string_view_index_of_uchar(sv, uc)

#  define string_view_equals(sv, other) kos_string_view_equals(sv, other)
#  define string_view_equals_constant(sv, constant) kos_string_view_equals_constant(sv, constant)
#  define string_view_ends_with(sv, other) kos_string_view_ends_with(sv, other)
#  define string_view_ends_with_constant(sv, constant) kos_string_view_ends_with_constant(sv, constant)
#endif // KOS_NO_SHORT_NAMES

/* owning UTF-8 string. */
typedef struct kos_string
{
    kos_allocator_function allocator;
    // the (owned) allocated memory for this string.
    const uchar* memory;
    // the total number of bytes in the allocated memory block.
    usize count;
} kos_string;

typedef struct kos_string_view
{
    // the (non-owned) allocated memory for this string.
    const uchar* memory;
    // the total number of bytes in the we're viewing in the memory block.
    usize count;
} kos_string_view;

kos_string kos_string_allocate(kos_allocator_function allocator, usize count);
kos_string kos_string_create(kos_allocator_function allocator, const uchar* memory, usize count);
kos_string kos_string_deallocate(kos_string s);
const char* kos_string_to_cstring(kos_string s, kos_allocator_function allocator);
isize kos_string_index_of_uchar(kos_string s, uchar uc);

kos_string_view kos_string_slice(string s, usize offset, usize count);
kos_string_view kos_string_view_slice(string_view sv, usize offset, usize count);
const char* kos_string_view_to_cstring(kos_string_view sv, kos_allocator_function allocator);
isize kos_string_view_index_of_uchar(kos_string_view sv, uchar uc);

bool kos_string_view_equals(kos_string_view sv, kos_string_view other);
bool kos_string_view_equals_constant(kos_string_view sv, const char* constant);
bool kos_string_view_ends_with(kos_string_view sv, kos_string_view other);
bool kos_string_view_ends_with_constant(kos_string_view sv, const char* constant);

#endif // KOS_STRING_H
