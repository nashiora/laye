#include "kos/utf8.h"

int kos_utf8_calc_encoded_byte_count(u8 byte1)
{
    // 0x80 = 1000_0000
    if ((byte1 & 0x80) == 0)
        return 1; // it's in the lower 7 bits, ASCII space
    // 0xE0 = 1110_0000, 0xC0 = 1100_0000
    else if ((byte1 & 0xE0) == 0xC0)
        return 2;
    // 0xF0 = 1111_0000, 0xE0 = 1110_0000
    else if ((byte1 & 0xF0) == 0xE0)
        return 3;
    // 0xF8 = 1111_1000, 0xF0 = 1111_0000
    else if ((byte1 & 0xF8) == 0xF0)
        return 4;
    // not a valid UTF-8 byte1
    else return 0;
}

int kos_utf8_calc_rune_required_byte_count(rune value)
{
    if (value <= 0x007F)
        return 1;
    else if (value <= 0x07FF)
        return 2;
    else if (value <= 0xFFFF)
        return 3;
    else return 4; // up to 0x10FFFF
}

kos_utf8_decode_result kos_utf8_decode_rune(const char* data, usize offset, usize dataLength)
{
    kos_utf8_decode_result result = { 0 };
    if (data == nullptr)
    {
        result.kind = KOS_UTF8_DECODE_NOT_ENOUGH_BYTES;
        return result;
    }

    isize remainingByteCount = cast(isize) dataLength - cast(isize) offset;
    if (remainingByteCount <= 0)
    {
        result.kind = KOS_UTF8_DECODE_NOT_ENOUGH_BYTES;
        return result;
    }

    u8 byte1 = cast(u8) data[offset];

    int expectedByteCount = kos_utf8_calc_encoded_byte_count(byte1);
    if (expectedByteCount == 0)
    {
        result.kind = KOS_UTF8_DECODE_INVALID_START_BYTE;
        return result;
    }

    if (expectedByteCount > remainingByteCount)
    {
        result.kind = KOS_UTF8_DECODE_NOT_ENOUGH_BYTES;
        return result;
    }

// 0x3F = 0011_1111
#define CONTINUE_MASK 0x3F
#define CONTINUE_BIT_COUNT 6

    result.kind = KOS_UTF8_DECODE_OK;
    if (expectedByteCount == 1)
        result.value = cast(rune) data[offset];
    else if (expectedByteCount == 2)
    {
        // 0x1F = 0001_1111
        rune byte1 = (cast(rune) data[offset + 0] & 0x1F) << CONTINUE_BIT_COUNT;
        rune byte2 = (cast(rune) data[offset + 1] & CONTINUE_MASK);

        result.value = byte1 | byte2;
    }
    else if (expectedByteCount == 3)
    {
        // 0x0F = 0000_1111
        rune byte1 = (cast(rune) data[offset + 0] & 0X0F) << (2 * CONTINUE_BIT_COUNT);
        rune byte2 = (cast(rune) data[offset + 1] & CONTINUE_MASK) << CONTINUE_BIT_COUNT;
        rune byte3 = (cast(rune) data[offset + 2] & CONTINUE_MASK);

        result.value = byte1 | byte2 | byte3;
    }
    else// if (expectedByteCount == 4)
    {
        assert(expectedByteCount == 4);

        // 0x07 = 0000_0111
        rune byte1 = (cast(rune) data[offset + 0] & 0X07) << (3 * CONTINUE_BIT_COUNT);
        rune byte2 = (cast(rune) data[offset + 1] & CONTINUE_MASK) << (2 * CONTINUE_BIT_COUNT);
        rune byte3 = (cast(rune) data[offset + 2] & CONTINUE_MASK) << CONTINUE_BIT_COUNT;
        rune byte4 = (cast(rune) data[offset + 3] & CONTINUE_MASK);

        result.value = byte1 | byte2 | byte3 | byte4;
    }

#undef CONTINUE_BIT_COUNT
#undef CONTINUE_MASK

    return result;
}

kos_utf8_decode_result kos_utf8_decode_rune_at_string_position(kos_string s, usize position)
{
    return kos_utf8_decode_rune(s.memory, position, s.count);
}

kos_utf8_decode_result kos_utf8_decode_rune_at_string_view_position(kos_string_view sv, usize position)
{
    return kos_utf8_decode_rune(sv.memory, position, sv.count);
}

kos_utf8_encode_result kos_utf8_encode_rune(char* dest, usize destLength, rune value)
{
    int requiredByteCount = utf8_calc_rune_required_byte_count(value);
    if (cast(usize) requiredByteCount > destLength)
        return (kos_utf8_encode_result){ KOS_UTF8_ENCODE_NOT_ENOUGH_BYTES, 0 };
    
    switch (requiredByteCount)
    {
        case 1:
            //dest[0] = cast(char) (value & 0x7F);
            dest[0] = cast(char) (value & 0b01111111);
            return (kos_utf8_encode_result){ KOS_UTF8_ENCODE_OK, 1 };
        case 2:
            dest[0] = cast(char) (((value >>  0) & 0x1F) | 0xC0);
            dest[1] = cast(char) (((value >>  5) & 0x3F) | 0x80);
            return (kos_utf8_encode_result){ KOS_UTF8_ENCODE_OK, 2 };
        case 3:
            dest[0] = cast(char) (((value >>  0) & 0x0F) | 0xE0);
            dest[1] = cast(char) (((value >>  5) & 0x3F) | 0x80);
            dest[2] = cast(char) (((value >> 11) & 0x3F) | 0x80);
            return (kos_utf8_encode_result){ KOS_UTF8_ENCODE_OK, 3 };
        case 4:
            dest[0] = cast(char) (((value >>  0) & 0x07) | 0xF0);
            dest[1] = cast(char) (((value >>  5) & 0x3F) | 0x80);
            dest[2] = cast(char) (((value >> 11) & 0x3F) | 0x80);
            dest[3] = cast(char) (((value >> 17) & 0x3F) | 0x80);
            return (kos_utf8_encode_result){ KOS_UTF8_ENCODE_OK, 4 };
        default: unreachable;
    }
}
