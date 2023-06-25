#ifndef KOS_UTF8_H
#define KOS_UTF8_H

#include "kos/builtins.h"
#include "kos/string.h"

#ifndef KOS_NO_SHORT_NAMES
#  define utf8_decode_result_kind kos_utf8_decode_result_kind
#  define UTF8_DECODE_OK KOS_UTF8_DECODE_OK
#  define UTF8_DECODE_INVALID_START_BYTE KOS_UTF8_DECODE_INVALID_START_BYTE
#  define UTF8_DECODE_NOT_ENOUGH_BYTES KOS_UTF8_DECODE_NOT_ENOUGH_BYTES

#  define utf8_decode_result kos_utf8_decode_result

#  define utf8_calc_encoded_byte_count kos_utf8_calc_encoded_byte_count
#  define utf8_calc_rune_required_byte_count kos_utf8_calc_rune_required_byte_count
#  define utf8_decode_rune kos_utf8_decode_rune
#  define utf8_decode_rune_at_string_position kos_utf8_decode_rune_at_string_position
#  define utf8_decode_rune_at_string_view_position kos_utf8_decode_rune_at_string_view_position
#endif

typedef enum kos_utf8_decode_result_kind
{
    KOS_UTF8_DECODE_OK,
    KOS_UTF8_DECODE_INVALID_START_BYTE,
    KOS_UTF8_DECODE_NOT_ENOUGH_BYTES,
} kos_utf8_decode_result_kind;

typedef struct kos_utf8_decode_result
{
    kos_utf8_decode_result_kind kind;
    rune value;
    int nBytesRead;
} kos_utf8_decode_result;

typedef enum kos_utf8_encode_result_kind
{
    KOS_UTF8_ENCODE_OK,
    KOS_UTF8_ENCODE_NOT_ENOUGH_BYTES,
} kos_utf8_encode_result_kind;

typedef struct kos_utf8_encode_result
{
    kos_utf8_encode_result_kind kind;
    int nBytesWritten;
} kos_utf8_encode_result;

int kos_utf8_calc_encoded_byte_count(u8 byte1);
int kos_utf8_calc_rune_required_byte_count(rune value);

kos_utf8_decode_result kos_utf8_decode_rune(const char* data, usize offset, usize dataLength);
kos_utf8_decode_result kos_utf8_decode_rune_at_string_position(kos_string s, usize position);
kos_utf8_decode_result kos_utf8_decode_rune_at_string_view_position(kos_string_view sv, usize position);

kos_utf8_encode_result kos_utf8_encode_rune(char* dest, usize destLength, rune value);

#endif // KOS_UTF8_H
