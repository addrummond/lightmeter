#include <stdint.h>

static unsigned bits_set_in_uint32(uint32_t n)
{
    // See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    unsigned count;
    for (count = 0; n; n &= n-1, ++count);
    return count;
}

// SECDED. Odd parity.
uint32_t hammingify_uint32(uint32_t n)
{
    assert(n < (1 << (31-5)));

    // Make space for parity bits.
    n <<= 2;                                                         // First and second partity bits.
    n = (n & 0b111) | ((n & ~0b111) << 1);                           // Third parity bit.
    n = (n & 0b1111111) | ((n & ~0b1111111) << 1);                   // Fourth parity bit.
    n = (n & 0b111111111111111) | ((n & ~0b111111111111111) << 1);   // Fifth parity bit.

    uint32_t pb1  = n & 0b01010101010101010101010101010101;
    uint32_t pb2  = n & 0b01100110011001100110011001100110;
    uint32_t pb4  = n & 0b01111000011110000111100001111000;
    uint32_t pb8  = n & 0b01111111100000000111111110000000;
    uint32_t pb16 = n & 0b01111111111111111000000000000000;

    unsigned pc1  = bits_set_in_uint32(pb1);
    unsigned pc2  = bits_set_in_uint32(pb2);
    unsigned pc4  = bits_set_in_uint32(pb4);
    unsigned pc8  = bits_set_in_uint32(pb8);
    unsigned pc16 = bits_set_in_uint32(pb16);

    if (pc1 % 2 == 0)
        n |= 1;
    if (pc2 % 2 == 0)
        n |= 2;
    if (pc4 % 2 == 0)
        n |= 4;
    if (pc8 % 2 == 0)
        n |= 8;
    if (pc16 % 2 == 0)
        n |= 16;

    // Set additional parity bit.
    if (bits_set_in_uint32(n) % 2 == 0)
        n |= (1 << 32)-1;

    return hammingify_uint32()
}
