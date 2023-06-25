#ifndef KOS_PRIMITIVES_H
#define KOS_PRIMITIVES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kos/builtins.h"

#ifndef KOS_NO_SHORT_NAMES
#  define bigint_word kos_bigint_word
#  define bigint kos_bigint

#  define bigint_addto_word(l, r) kos_bigint_addto_word(l, r)
#  define bigint_multo_word(l, r) kos_bigint_multo_word(l, r)
#endif // KOS_NO_SHORT_NAMES

typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;

typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;

typedef ptrdiff_t     isize;
typedef size_t        usize;

typedef int32_t       rune;

typedef   signed char ichar;
typedef unsigned char uchar;

#define U64_MAX (18446744073709551615ull)

#endif // KOS_PRIMITIVES_H
