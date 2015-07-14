//
// This module implements hamming coding for sequences of 32-bits. The first 21
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
#include <stdint.h>
#include <stdbool.h>
#include <myassert.h>
#ifdef TEST
#include <stdio.h>
#endif
#endif

#ifdef JAVASCRIPT
#define static
#define uint32_t var
#define unsigned var
#define int32_t var
#define const
#define assert(x) do { if (! (x)) { throw new Error("ASSERTION ERROR"); } } while (0)
#define BIN(v) parseInt(#v.substr(2), 2)
#define FUNC(rettype) function
#define ARG(type)
#else
#define BIN(x) x
#define ARG(type) type
#define FUNC(rettype) rettype
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

FUNC(uint32_t) hammingify_uint32(ARG(uint32_t) n)
{
    assert(n < (1 << (31-5)));

    // Make space for parity bits.
    n = ((n & BIN(0b1)) << 2)                            |
        ((n & BIN(0b1110)) << 3)                         |
        ((n & BIN(0b11111110000)) << 4)                  |
        ((n & BIN(0b11111111111111100000000000)) << 5);

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

    // Set additional parity bit.
    if (bits_set_in_uint32(n) % 2 == 0)
        n ^= (1 << (32-1));

    return n;
}

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
        // Error in parity bits, cannot decode.
        return -1;
    }
    else if (error_bit_index) {
        // The bit at error_bit_index is flipped, so correct it.
        n ^= (1 << (error_bit_index-1));
    }

    // Is the corrected number consistent with the final parity bit?
    if (bits_set_in_uint32(n) % 2 == 0)
        return -1;

    // If we get here, we have the correct data in n together with the parity
    // bits. Now we just need to remove the parity bits.
    return ((n & BIN(0b100)) >> 2)                              |
           ((n & BIN(0b1110000)) >> 3)                          |
           ((n & BIN(0b111111100000000)) >> 4)                  |
           ((n & BIN(0b1111111111111110000000000000000)) >> 5);
}

#if defined TEST || defined JAVASCRIPT

FUNC(int)
#ifdef TEST
main(int argc, char **argv)
#endif
#ifdef JAVASCRIPT
test()
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
