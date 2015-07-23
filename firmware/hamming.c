//
// This module implements hamming coding for sequences of 32-bits. The first 31
// bits are encoded according to Hamming(31,26) with odd parity. The 32nd bit
// is an additional odd parity bit.
//
// This file can also be used to generate equivalent Javascript code by running
// it through the C preprocessor with -DJAVASCRIPT and then stripping lines
// beginning with '#' from the output.
//
// It is best to use 'gcc -E' or 'clang -E' rather than 'cpp', since 'cpp' on
// OS X runs in "traditional" mode, and hence does not recognize the '#'
// stringification operator. E.g.:
//
//     clang -E hamming.c | grep -v '^#' > hamming.js
//

#ifndef JAVASCRIPT
#include <hamming.h>
#include <myassert.h>
#ifdef TEST
#include <stdio.h>
#endif
#endif

#ifdef JAVASCRIPT
#define static
#define uint32_t var
#define unsigned var
#define int var
#define int32_t var
#define uint8_t var
#define bool var
#define const
#define assert(x) do { if (! (x)) { throw new Error("ASSERTION ERROR"); } } while (0)
#define BIN(v) parseInt(#v.substr(2), 2)
#define FUNC(rettype) function
#define ARG(type)
#define INT(x) parseInt(x)
#else
#define BIN(x) x
#define ARG(type) type
#define FUNC(rettype) rettype
#define INT(x) x
#endif

static FUNC(unsigned) bits_set_in_uint32(ARG(uint32_t) n)
{
    // See http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
    unsigned count;
    for (count = 0; n; ++count, n &= n-1);
    return count;
}

static const uint32_t PMASK1  = BIN(0b01010101010101010101010101010101);
static const uint32_t PMASK2  = BIN(0b01100110011001100110011001100110);
static const uint32_t PMASK4  = BIN(0b01111000011110000111100001111000);
static const uint32_t PMASK8  = BIN(0b01111111100000000111111110000000);
static const uint32_t PMASK16 = BIN(0b01111111111111111000000000000000);

static const uint32_t M1 = BIN(0b1);
static const uint32_t M2 = BIN(0b1110);
static const uint32_t M3 = BIN(0b11111110000);
static const uint32_t M4 = BIN(0b11111111111111100000000000);
FUNC(uint32_t) hammingify_uint32(ARG(uint32_t) n)
{
    assert(n < (1 << (31-5)));

    // Make space for parity bits.
    n = ((n & M1) << 2)    |
        ((n & M2) << 3)    |
        ((n & M3) << 4)    |
        ((n & M4) << 5);

    uint32_t pb1  = n & PMASK1;
    uint32_t pb2  = n & PMASK2;
    uint32_t pb4  = n & PMASK4;
    uint32_t pb8  = n & PMASK8;
    uint32_t pb16 = n & PMASK16;

    unsigned pc1  = bits_set_in_uint32(pb1);
    unsigned pc2  = bits_set_in_uint32(pb2);
    unsigned pc4  = bits_set_in_uint32(pb4);
    unsigned pc8  = bits_set_in_uint32(pb8);
    unsigned pc16 = bits_set_in_uint32(pb16);

    if (pc1 % 2 == 0)
        n ^= (1 << (1-1));
    if (pc2 % 2 == 0)
        n ^= (1 << (2-1));
    if (pc4 % 2 == 0)
        n ^= (1 << (4-1));
    if (pc8 % 2 == 0)
        n ^= (1 << (8-1));
    if (pc16 % 2 == 0)
        n ^= (1 << (16-1));

    // Set additional parity bit at bit 32.
    if (bits_set_in_uint32(n) % 2 == 0)
        n ^= (1 << (32-1));

    return n;
}

static const uint32_t IM1 = BIN(0b100);
static const uint32_t IM2 = BIN(0b1110000);
static const uint32_t IM3 = BIN(0b111111100000000);
static const uint32_t IM4 = BIN(0b1111111111111110000000000000000);
// Returns -1 if could not be decoded.
FUNC(int32_t) dehammingify_uint32(ARG(uint32_t) n)
{
    unsigned pb1 = n & PMASK1;
    unsigned pb2 = n & PMASK2;
    unsigned pb4 = n & PMASK4;
    unsigned pb8 = n & PMASK8;
    unsigned pb16 = n & PMASK16;

    unsigned pc1  = bits_set_in_uint32(pb1);
    unsigned pc2  = bits_set_in_uint32(pb2);
    unsigned pc4  = bits_set_in_uint32(pb4);
    unsigned pc8  = bits_set_in_uint32(pb8);
    unsigned pc16 = bits_set_in_uint32(pb16);

    unsigned error_bit_index = 0;
    if (pc1 % 2 == 0)
        error_bit_index += 1;
    if (pc2 % 2 == 0)
        error_bit_index += 2;
    if (pc4 % 2 == 0)
        error_bit_index += 4;
    if (pc8 % 2 == 0)
        error_bit_index += 8;
    if (pc16 % 2 == 0)
        error_bit_index += 16;

    if (error_bit_index == 1 || error_bit_index == 2 || error_bit_index == 4 ||
        error_bit_index == 8 || error_bit_index == 16) {
        // Error in one of the parity bits. We flip an abritrarily chosen
        // parity bit because we need to correct the parity of the entire 32-bit word.
        n ^= 1;
    }
    else if (error_bit_index) {
        // The bit at error_bit_index is flipped, so correct it.
        n ^= (1 << (error_bit_index-1));
    }

    // Is the corrected number consistent with the final parity bit? If not, we
    // have an uncorrectable error.
    if (bits_set_in_uint32(n) % 2 == 0)
        return -1;

    // If we get here, we have the correct data in n together with the parity
    // bits. Now we just need to remove the parity bits.
    return ((n & IM1) >> 2)   |
           ((n & IM2) >> 3)   |
           ((n & IM3) >> 4)   |
           ((n & IM4) >> 5);
}

FUNC(uint32_t) hamming_get_init_sequence_byte_length()
{
    return 4;
}

FUNC(uint32_t) hamming_get_encoded_message_byte_length(ARG(uint32_t) len) {
    uint32_t x = len*4;
    x += x % 3;
    return x / 3;
}

FUNC(uint32_t) hamming_get_encoded_message_byte_length_with_init_sequences(ARG(uint32_t) len, ARG(unsigned) num_init_sequences) {
    return (num_init_sequences*hamming_get_init_sequence_byte_length()) +  hamming_get_encoded_message_byte_length(len);
}

FUNC(uint32_t) hamming_get_max_output_length_given_input_length(ARG(uint32_t) len)
{
    uint32_t x = len*3;
    x += x % 4;
    return (x / 4);
}

#define MAGIC_NUMBER 24826601
static uint32_t MAGIC_NUMBER_HAMMING = 0;

FUNC(void) hamming_encode_message(ARG(const uint8_t *) input,
#ifndef JAVASCRIPT
unsigned length_,
#endif
ARG(uint8_t *) out,
ARG(unsigned) num_init_sequences)
{
#ifdef JAVASCRIPT
#define length input.length
#else
#define length length_
#endif

    if (MAGIC_NUMBER_HAMMING == 0)
        MAGIC_NUMBER_HAMMING = hammingify_uint32(MAGIC_NUMBER);

    uint32_t i = 0;
    uint32_t ilen = hamming_get_init_sequence_byte_length()*num_init_sequences;
    for (; i < ilen;) {
        out[i++] = (MAGIC_NUMBER_HAMMING & 0xFF);
        out[i++] = (MAGIC_NUMBER_HAMMING & 0xFF00) >> 8;
        out[i++] = (MAGIC_NUMBER_HAMMING & 0xFF0000) >> 16;
        out[i++] = (MAGIC_NUMBER_HAMMING & 0xFF000000) >> 24;
    }

    uint32_t j;
    for (j = 0; j < length; j += 3) {
        uint32_t v = input[j]                                    |
                     (j + 1 >= length ? 0 : (input[j+1] << 8))   |
                     (j + 2 >= length ? 0 : (input[j+2] << 16));
        uint32_t h = hammingify_uint32(v);
        out[i++] = (h & 0xFF);
        out[i++] = (h & 0xFF00) >> 8;
        out[i++] = (h & 0xFF0000) >> 16;
        out[i++] = (h & 0xFF000000) >> 24;
    }

#undef length
}

// nbits < 8. If nbits > 0, assumes that there is an available byte at buffer[length].
FUNC(void) hamming_bitshift_buffer_forward(ARG(uint8_t *) buffer, ARG(unsigned) length, ARG(unsigned) nbits)
{
    assert(nbits < 8);

    if (length == 0 || nbits == 0)
        return;

    uint8_t backup = 0;
    unsigned i;
    for (i = 0; i < length; ++i) {
        uint8_t r = (buffer[i] << nbits);
        r |= (backup >> (8 - nbits));
#ifdef JAVASCRIPT
        r &= 0xFF;
#endif
        backup = buffer[i];
        buffer[i] = r;
    }
    buffer[i] = (backup >> (8 - nbits));
#ifdef JAVASCRIPT
    buffer[i] &= 0xFF;
#endif
}

// Scans a buffer from all bit offsets for a sequence of magic numbers. Returns
// the bit index of the first sequence and the count.
FUNC(hamming_scan_for_init_sequence_result_t) hamming_scan_for_init_sequence(ARG(const uint8_t *) input
#ifndef JAVASCRIPT
, unsigned length_
#endif
)
{
#ifdef JAVASCRIPT
#define length input.length
    var result = { };
#else
#define length length_
    hamming_scan_for_init_sequence_result_t result;
#endif

    int magic_start_bit_index = -1;
    unsigned magic_count = 0;
    unsigned bit_index;
    for (bit_index = 0; bit_index < (length*8) - 32;) {
        unsigned byte = INT(bit_index / 8);
        unsigned bit = bit_index % 8;
        uint8_t b1 = ((input[byte+0] >> bit) | (input[byte+1] << (8-bit)));
        uint8_t b2 = ((input[byte+1] >> bit) | (input[byte+2] << (8-bit)));
        uint8_t b3 = ((input[byte+2] >> bit) | (input[byte+3] << (8-bit)));
        uint8_t b4 = ((input[byte+3] >> bit) | (input[byte+4] << (8-bit)));
#ifdef JAVASCRIPT
        b1 &= 0xFF;
        b2 &= 0xFF;
        b3 &= 0xFF;
        b4 &= 0xFF;
#endif

        uint32_t v = dehammingify_uint32(b1 | (b2 << 8) | (b3 << 16) | (b4 << 24));
        if (v == MAGIC_NUMBER) {
            if (magic_start_bit_index == -1)
                magic_start_bit_index = bit_index;
            ++magic_count;
            bit_index += 32;
        }
        else {
            if (magic_start_bit_index != -1) {
                result.bit_index = magic_start_bit_index;
                result.count = magic_count;
                return result;
            }
            ++bit_index;
        }
    }

    result.bit_index = magic_start_bit_index;
    result.count = magic_count;
    return result;

#undef length
}

// Returns byte length of decoded message if no error or negated index of first
// error in input buffer.
FUNC(int32_t) hamming_decode_message(ARG(const uint8_t *) input,
#ifndef JAVASCRIPT
unsigned length_,
#endif
ARG(uint8_t *) out)
{
#ifdef JAVASCRIPT
#define length input.length
#else
#define length length_
#endif

    uint32_t i, oi = 0;
    for (i = 0; i < length; i += 4) {
        uint32_t v = dehammingify_uint32(input[i] | (input[i+1] << 8) | (input[i+2] << 16) | (input[i+3] << 24));

        if (v == -1)
            return -i;

        out[oi++] = v & 0xFF;
        out[oi++] = (v & 0xFF00) >> 8;
        out[oi++] = (v & 0xFF0000) >> 16;
    }

    return oi;

#undef length
}

#if defined TEST || defined JAVASCRIPT

FUNC(int)
#ifdef TEST
main(int argc, char **argv)
#endif
#ifdef JAVASCRIPT
hmming_test()
#endif
{
    // Test hammingify_uint32 and dehammingify_uint32 for all possible values.

    uint32_t n;
    for (n = 0; n < (1 << (31-5)); ++n) {
        uint32_t h = hammingify_uint32(n);

        unsigned i;
        for (i = 0; i <= 32; ++i) {
            if (i != 0) {
                uint32_t h2 = h ^ (1 << (i-1));

                uint32_t r = dehammingify_uint32(h2);
                if (! (((r == -1 && (i == 1 || i == 2 || i == 4 || i == 8 || i == 16 || i == 32)) ||
                       (r == n)))) {
#ifdef TEST
                    printf("FAILED for i = %i, n = %i, r = %i\n", i, n, r);
#endif
#ifdef JAVASCRIPT
                    console.log("FAILED FOR i = ", i, "n = ", n, "r = ", r);
#endif
                }
            }
        }
    }

    return 0;
}

#endif
