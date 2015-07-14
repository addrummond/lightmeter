#include <stdint.h>
#include <stdbool.h>
#include <myassert.h>
#ifdef TEST
static void print_bin_backwards(uint32_t n, bool with_parity);
#include <stdio.h>
#endif

static unsigned bits_set_in_uint32(uint32_t n)
{
    // See http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
    unsigned count;
    for (count = 0; n; ++count, n &= n-1);
    return count;
}

#define PMASK1  0b01010101010101010101010101010101
#define PMASK2  0b01100110011001100110011001100110
#define PMASK4  0b01111000011110000111100001111000
#define PMASK8  0b01111111100000000111111110000000
#define PMASK16 0b01111111111111111000000000000000

// SECDED. Odd parity.
uint32_t hammingify_uint32(uint32_t n)
{
    assert(n < (1 << (31-5)));

    // Make space for parity bits.
    n = ((n & 0b1) << 2)                            |
        ((n & 0b1110) << 3)                         |
        ((n & 0b11111110000) << 4)                  |
        ((n & 0b11111111111111100000000000) << 5);

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
int32_t dehammingify_uint32(uint32_t n)
{
    unsigned pb1 = n & PMASK1;
    unsigned pb2 = n & PMASK2;
    unsigned pb4 = n & PMASK4;
    unsigned pb8 = n & PMASK8;
    unsigned pb16 = n & PMASK16;
    unsigned final = n & (1 << (31-1));

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
    return ((n & 0b100) >> 2)                              |
           ((n & 0b1110000) >> 3)                          |
           ((n & 0b111111100000000) >> 4)                  |
           ((n & 0b1111111111111110000000000000000) >> 5);
}

#ifdef TEST

static void print_bin_backwards(uint32_t n, bool with_parity)
{
    unsigned i;
    for (i = 1; n; ++i) {
        if (with_parity && (i == 1 || i == 2 || i == 4 || i == 8 || i == 16 || i == 32))
            printf("[");

        if (n & 1)
            printf("1");
        else
            printf("0");

        if (with_parity && (i == 1 || i == 2 || i == 4 || i == 8 || i == 16 || i == 32))
            printf("]");

        n >>= 1;
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    /*const uint32_t n = 8801;

    unsigned i;
    for (i = 0; i <= 32; ++i) {
        printf("Bad bit (0 means no bad bit): %i\n", i);
        printf("N = %i, ", n);
        print_bin_backwards(n, false);
        uint32_t h = hammingify_uint32(n);

        if (i != 0) {
            h ^= (1 << (i-1));
        }

        printf("H(N) = %i, ", h);
        print_bin_backwards(h, true);
        uint32_t r = dehammingify_uint32(h);
        printf("H-1(H(N)) = %i, ", r);
        print_bin_backwards(r, false);

        printf("\n");
    }
    return 0;*/

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
                    printf("FAILED for i = %i, n = %i, r = %i\n", i, n, r);
                }
            }
        }
    }

    return 0;
}

#endif
