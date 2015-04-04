// We don't pack the digits two per byte because when we're dealing
// with bcd quantities we always want to convert them to strings
// eventually. This is much easier if we can just overwrite each
// byte with its value plus 48.

#include <stdint.h>
#include <stdbool.h>
#include <bcd.h>
#include <mymemset.h>
#include <myassert.h>
#ifdef TEST
#include <stdio.h>
#include <string.h>
#endif

// 'digits' and 'dest' may point to same buffer.
unsigned bcd_to_string_fp(uint8_t *digits, uint_fast8_t length, uint8_t *dest, uint_fast8_t sigfigs, uint_fast8_t dps)
{
    uint_fast8_t i;
    for (i = 0; i < length && digits[i] == 0; ++i);
    if (i == length) {
        dest[0] = '0';
        dest[1] = '\0';
        return 1;
    }

    uint_fast8_t dot = 0;

    uint_fast8_t j;
    for (j = 0; i < length && j < sigfigs; ++i, ++j) {
        if (length - i == dps)
            dot = j++;
        dest[j] = '0' + digits[i];
    }

    if (i > length - 1)
        return j;

    // Rounding
    if (digits[i] >= 5) {
        uint_fast8_t k = j-1;
        do {
            if (dest[k] == '.')
                continue;

            ++dest[k];
            if (dest[k] < '0' + 10)
                break;
            else
                dest[k] = '0';
        } while (--k > 0);
    }

    // Trailing zeroes if any.
    for (; length - i > dps; ++i, ++j) {
        if (length - i == dps)
            dot = j++;
        dest[j] = '0';
    }

    if (dot != 0)
        dest[dot] = '.';

    dest[j] = '\0';

    return j;
}

#ifdef TEST

void debug_print_bcd(uint8_t *digits, uint_fast8_t length)
{
    uint8_t digits2[length + 1];
    uint_fast8_t i;
    for (i = 0; i < length; ++i)
        digits2[i] = digits[i];
    digits2[length] = 0;
    bcd_to_string(digits2, length);
    printf("%s", digits2);
}


static void bcd_to_string_fp_test()
{
    uint8_t digits[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0  };
    uint8_t dest[sizeof(digits)/sizeof(uint8_t) + 1 /* point */ + 1 /* zero terminator */];

    bcd_to_string_fp(digits, (sizeof(digits)/sizeof(uint8_t))-2, digits, 7, 3);

    printf("bcd_to_string_fp: %s\n", digits);
}

int main()
{
    bcd_to_string_fp_test();

    return 0;
}

#endif
