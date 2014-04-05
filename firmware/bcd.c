// We don't pack the digits two per byte because when we're dealing
// with bcd quantities we always want to convert them to strings
// eventually. This is much easier if we can just overwrite each
// byte with its value plus 48.

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef TEST
#include <stdio.h>
#include <string.h>
#endif

// First number must have at least as many digits as second.
// Assumes that there is an available byte at digits1[-1].
// Stores result in first number
// Returns either digits1 or (digits1-1).
uint8_t *bcd_add(uint8_t *digits1, uint8_t digits1_length,
                 uint8_t *digits2, uint8_t digits2_length)
{
    assert(digits1_length >= digits2_length);

    uint8_t end1 = digits1_length - 1;
    uint8_t end2 = digits2_length - 1;

    uint8_t carry = 0;
    carry = 0;

    uint8_t i, j;
    for (i = end1+1, j = end2+1; j > 0; --i, --j) {
        uint8_t r = digits1[i-1] + digits2[j-1] + carry;
        if (r > 9) {
            digits1[i-1] = r - 10;
            carry = 1;
        }
        else {
            carry = 0;
            digits1[i-1] = r;
        }
    }

    if (carry == 1) {
        if (i == 0) {
            --digits1;
            *digits1 = 1;
        }
        else {
            digits1[i-1] = 1;
        }
    }

    return digits1;
}

uint8_t bcd_length_after_add(uint8_t *oldptr, uint8_t oldlength, uint8_t *newptr)
{
    return oldlength + (oldptr - newptr);
}

void bcd_to_string(uint8_t *digits, uint8_t length)
{
    for (uint8_t i = 0; i < length; ++i)
        digits[i] += 48;
}

bool bcd_gt(uint8_t *digits1, uint8_t length1, uint8_t *digits2, uint8_t length2)
{
    uint8_t i, j;
    uint8_t zeroes1 = 0;
    uint8_t zeroes2 = 0;
    uint8_t lmax;
    if (length2 > length1) {
        zeroes1 = length2 - length1;
        lmax = length2;
    }
    else {
        zeroes2 = length1 - length2;
        lmax = length1;
    }

    i = 0, j = 0;
    for (; i < length1;) {
        uint8_t a, b;
        if (zeroes1) {
            a = 0;
            --zeroes1;
        }
        else {
            a = digits1[i];
            ++i;
        }

        if (zeroes2) {
            b = 0;
            --zeroes2;
        }
        else {
            b = digits2[j];
            ++j;
        }

        if (a < b)
            return false;
        if (a > b)
            return true;
    }

    // If we get here, they're equal.
    return false;
}

#ifdef TEST

static void add_test1()
{
    uint8_t digits1[] = { 1, 2, 3, '\0' };
    uint8_t digits2[] = { 4, 5, 6, '\0' };

    uint8_t *r = bcd_add(digits1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_add(digits1, 3, r));
    printf("123 + 456 = %s\n", r);
    assert(!strcmp((char *)r, "579"));
}

static void add_test2()
{
    uint8_t digits1[] = { '\0', 9, 7, 8, '\0' };
    uint8_t digits2[] = {       8, 8, 6, '\0' };

    uint8_t *r = bcd_add(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_add(digits1+1, 3, r));
    printf("978 + 886 = %s\n", r);
    assert(!strcmp((char *)r, "1864"));
}

static void add_test3()
{
    uint8_t digits1[] = { '\0', 0, 0, 8, '\0' };
    uint8_t digits2[] = {       8, 8, 6, '\0' };

    uint8_t *r = bcd_add(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_add(digits1+1, 3, r));
    printf("008 + 886 = %s\n", r);
    assert(!strcmp((char *)r, "894"));
}

static void add_test4()
{
    uint8_t digits1[] = { 1, '\0' };
    uint8_t digits2[] = { 2, '\0' };

    uint8_t *r = bcd_add(digits1, 1, digits2, 1);
    bcd_to_string(r, bcd_length_after_add(digits1, 1, r));
    printf("1 + 2 = %s\n", r);
    assert(!strcmp((char *)r, "3"));
}

static void gt_test1()
{
    uint8_t digits1[] = { 9, 7, 8, '\0' };
    uint8_t digits2[] = { 8, 8, 6, '\0' };

    bool v = bcd_gt(digits1, 3, digits2, 3);
    printf("978 > 886 = %s\n", v ? "true" : false);
    assert(v);
}

static void gt_test2()
{
    uint8_t digits1[] = { 9, 7, 8, '\0' };
    uint8_t digits2[] = { 9, 7, 8, '\0' };

    bool v = bcd_gt(digits1, 3, digits2, 3);
    printf("978 > 978 = %s\n", v ? "true" : "false");
    assert(!v);
}

static void gt_test3()
{
    uint8_t digits1[] = { 9, 7, 8, '\0' };
    uint8_t digits2[] = { 8, 8, 6, '\0' };

    bool v = bcd_gt(digits2, 3, digits1, 3);
    printf("886 > 978 = %s\n", v ? "true" : false);
    assert(!v);
}

int main()
{
    add_test1();
    add_test2();
    add_test3();
    add_test4();

    gt_test1();
    gt_test2();
}

#endif
