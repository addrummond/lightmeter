#include <stdint.h>
#ifdef TEST
#include <stdio.h>
#endif

uint16_t bitfiddle_uint16_approx_div_by_10(uint16_t n)
{
    // We divide by 8 then add/subtract another series of numbers to get the final result.
    // This gives a maximum error of 2 (and errors of either 0 or 1 most of the time).
    return   (n >> 3) // div by 8
           - (n >> 5) + (n >> 7) + (n >> 10) - (n >> 9) - (n >> 11) - (n >> 13);
}

// Approx div by 4th root of 2, i.e. 1.1892. Max error 3.15.
uint16_t bitfiddle_uint16_approx_div_by_4rt2(uint16_t n)
{
    return n - (n >> 3) - (n >> 5) - (n >> 7) + (n >> 8) + (n >> 9) - (n >> 10) + (n>>14);    
}

//
// Not currently used; commenting out to reduce code size.
//
/*uint16_t bitfiddle_uint16_approx_div_by_5(uint16_t n)
{
    // Similar to above. Max error is 3.
    return   (n >> 2) // div by 4
           - (n >> 4) + (n >> 6) - (n >> 8) + (n >> 10) - (n >> 12) + (n >> 14);
}

uint16_t bitfiddle_uint16_approx_div_by_100(uint16_t n)
{
    // Similar to above. Max error is 3. Addition of &~1 does not reduce max
    // error but reduces average error.
    return (   (n >> 6) // div by 64
             - (n >> 8) - (n >> 12) - (n >> 9) + (n >> 11) ) & ~1;
}*/

#ifdef TEST

int main()
{
    printf("Testing approx div by 10\n");
    for (unsigned i = 0; i < 65536; ++i) {
        uint16_t x = (uint16_t)i / 10;
        uint16_t y = bitfiddle_uint16_approx_div_by_10((uint16_t)i);

        uint16_t diff;
        if ((x >= y && (diff = (x - y)) > 3) || (y > x && (diff = (y - x)) > 3)) {
            printf("Not approx equal div/10 for i = %i: real value = %i, bithack value = %i\n", i, x, y);
            return 1;
        }

        if (i % 256 == 0) {
            printf("Error at div/10 i = %i: %i (real result = %i, approx result = %i)\n", i, diff, x, y);
        }
    }

    printf("Testing approx div by 4th root of 2\n");
    for (unsigned i = 0; i < 65536; ++i) {
        uint16_t x = (uint16_t)((float)i/1.1892);
        uint16_t y = bitfiddle_uint16_approx_div_by_4rt2((uint16_t)i);

        uint16_t diff;
        if ((x >= y && (diff = (x - y)) > 4) || (y > x && (diff = (y - x)) > 4)) {
            printf("Not approx equal div/4rt2 for i = %i: real value = %i, bithack value = %i\n", i, x, y);
            return 1;
        }

        if (i % 256 == 0) {
            printf("Error at div/4rt2 i = %i: %i (real result = %i, approx result = %i)\n", i, diff, x, y);
        }
    }

    /*printf("Testing approx div by 5\n");
    for (unsigned i = 0; i < 65536; ++i) {
        uint16_t x = (uint16_t)i / 5;
        uint16_t y = bitfiddle_uint16_approx_div_by_5((uint16_t)i);

        uint16_t diff;
        if ((x >= y && (diff = (x - y)) > 3) || (y > x && (diff = (y - x)) > 3)) {
            printf("Not approx equal div/5 for i = %i: real value = %i, bithack value = %i\n", i, x, y);
            return 1;
        }

        if (i % 256 == 0) {
            printf("Error at div/5 i = %i: %i (real result = %i, approx result = %i)\n", i, diff, x, y);
        }
    }

    printf("Testing approx div by 100\n");
    for (unsigned i = 0; i < 65536; ++i) {
        uint16_t x = (uint16_t)i / 100;
        uint16_t y = bitfiddle_uint16_approx_div_by_100((uint16_t)i);

        uint16_t diff;
        if ((x >= y && (diff = (x - y)) > 3) || (y > x && (diff = (y - x)) > 3)) {
            printf("Not approx equal div/100 for i = %i: real value = %i, bithack value = %i\n", i, x, y);
            return 1;
        }

        if (i % 256 == 0) {
            printf("Error at div/100 i = %i: %i (real result = %i, approx result = %i)\n", i, diff, x, y);
        }
    }*/

    return 0;
}

#endif
