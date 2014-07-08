// We don't pack the digits two per byte because when we're dealing
// with bcd quantities we always want to convert them to strings
// eventually. This is much easier if we can just overwrite each
// byte with its value plus 48.

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <bcd.h>
#include <readbyte.h>
#include <mymemset.h>
#ifdef TEST
#include <stdio.h>
#include <string.h>
#endif

// First number must have at least as many digits as second.
// Assumes that there is an available byte at digits1[-1].
// Stores result in digits1.
// Returns either digits1 or (digits1-1).
uint8_t *bcd_add(uint8_t *digits1, uint8_t digits1_length,
                 uint8_t *digits2, uint8_t digits2_length)
{
    assert(digits1_length >= digits2_length);

    uint8_t end1 = digits1_length - 1;
    uint8_t end2 = digits2_length - 1;

    uint8_t carry = 0;

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
            ++digits1_length;
        }
        else {
            digits1[i-1] = 1;
        }
    }

    // Strip leading zeroes.
    i = 0;
    for (; *digits1 == 0 && i < digits1_length; ++digits1, ++i)
        printf("STRIP\n");

    return digits1;
}

uint8_t *bcd_sub(uint8_t *digits1, uint8_t digits1_length, uint8_t *digits2, uint8_t digits2_length)
{
    assert(digits1_length >= digits2_length);

    uint8_t i_, j_, i, j;
    for (i_ = digits1_length, j_ = digits2_length; j_ > 0; --i_, --j_) {
        i = i_-1;
        j = j_-1;

        if (digits1[i] < digits2[j]) {
            // Borrow
            assert(i > 0);

            uint8_t k, l;
            for (k = i - 1; digits1[k] == 0; --k); // Given that digits1 >= digits2 we must find a non-zero eventually.
            for (l = k; l < i; ++l) {
                --digits1[l];
                digits1[l+1] += 10;
            }
        }

        digits1[i] -= digits2[j];
    }

    // Strip leading zeroes.
    // TODO BUG won't work if all 0.
    //for (; *digits1 == 0; ++digits1);

    return digits1;
}

void bcd_to_string(uint8_t *digits, uint8_t length)
{
    uint8_t i;
    for (i = 0; i < length; ++i)
        digits[i] += 48;
}

void string_to_bcd(uint8_t *digits, uint8_t length)
{
    uint8_t i;
    for (i = 0; i < length; ++i)
        digits[i] -= 48;
}

#if !defined(__AVR__)
void debug_print_bcd(uint8_t *digits, uint8_t length)
{
    uint8_t digits2[length + 1];
    uint8_t i;
    for (i = 0; i < length; ++i)
        digits2[i] = digits[i];
    digits2[length] = 0;
    bcd_to_string(digits2, length);
    printf("%s", digits2);
}
#endif

// Save some code space by implementing <, <=, >, >=, = in one function.
// This function is called by macros bcd_lt, bcd_gteq, etc.
bool bcd_cmp(const uint8_t *digits1, uint8_t length1, const uint8_t *digits2, uint8_t length2,
             uint8_t which) // 0 == eq, 1 = lteq, 2 = gteq, 3 = lt, 4 = gt)
{
    uint8_t i, j;
    uint8_t zeroes1 = 0;
    uint8_t zeroes2 = 0;
    if (length2 > length1)
        zeroes1 = length2 - length1;
    else
        zeroes2 = length1 - length2;

    i = 0, j = 0;
    for (; i < length1;) {
        int8_t a, b;
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

        int8_t diff = a - b;
        if (diff != 0) {
            if (which == 0) {
                return false;
            }
            else if (which == 1) {
                return diff < 0;
            }
            else if (which == 2) {
                return diff > 0;
            }
            else if (which == 3) {
                return diff < 0;
            }
            else if (which == 4) {
                return diff > 0;
            }
        }
    }

    return (which >= 0 && which <= 2);
}

uint8_t *bcd_mul(uint8_t *digits1, uint8_t length1, const uint8_t *digits2, uint8_t length2)
{
    assert(length1 >= length2);

    uint8_t l = length1 + length2;
    uint8_t tmp1[l], tmp2[l];
    memset8_zero(tmp1, l);
    memset8_zero(tmp2, l);

    uint8_t *result_digits = tmp2;

    // Multiply by each digit separately.
    uint8_t j = length2-1;
    uint8_t result_digits_length = l;
    uint8_t *mulres_start = tmp1 + l - 1;
    do {
        printf("OUT LOOP\n");
        uint8_t carry = 0;
        uint8_t i = length1-1;
        do {
            printf("IN LOOP\n");
            uint8_t r = (digits1[i] * digits2[j]) + carry;
            printf("R %i\n", r);
            printf("I %i\n", i + length1);
            *(mulres_start--) = r % 10;
            carry = r / 10;
        } while (i-- > 0);
        if (carry != 0) {
            *mulres_start = carry;
        }

        uint8_t mulres_length = l - (mulres_start - tmp1);
            printf("ADDING ");
            uint8_t k;
            for (k = 0; k < result_digits_length; ++k)
                printf("%i", result_digits[k]);
            printf(" TO ");
            for (k = 0; k < mulres_length; ++k)
                printf("%i", mulres_start[k]);
            printf("\n");
        // Add to total.
        uint8_t *result_digits_ = bcd_add(result_digits, result_digits_length, mulres_start, mulres_length);
        printf("PTR %p %p\n", result_digits, result_digits_);
        result_digits_length = bcd_length_after_op(result_digits, result_digits_length, result_digits_);
        result_digits = result_digits_;
        printf("RDL %i\n", result_digits_length);
    } while (j-- > 0);

    uint8_t *start = digits1 + length1;
    --result_digits_length;
    do {
        *(start--) = result_digits[result_digits_length];
    } while (result_digits_length-- > 0);

    printf("PTRD %p %p\n", digits1, start);

    return start;
}

static const uint8_t TEN[] PROGMEM = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };
uint8_t *bcd_div_by_lt10(uint8_t *digits, uint8_t length, uint8_t by)
{
    assert(by < 10);

    uint8_t n = digits[0];
    uint8_t outi;
    uint8_t rem = 0;
    for (outi = 0; outi < length; ++outi) {
        n = digits[outi];
        uint8_t rem8 = rem << 3;
        n += rem8;
        n += rem + rem;
        if (n < by && outi != length - 1) {
            n = pgm_read_byte(TEN + n);
            n += digits[outi+1];
            digits[outi++] = 0;
        }

        uint8_t c;
        for (c = 0; n >= by; ++c, n -= by);
        rem = n;

        digits[outi] = c;
    }

    // Strip leading zeroes.
    for (; digits < (digits + length) && *digits == 0; ++digits);

    return digits;
}

uint8_t *uint8_to_bcd(uint8_t n, uint8_t *digits, uint8_t length)
{
    uint8_t i = length-1;
    while (n >= 10) {
        uint8_t v = n/10;
        //printf("  %i/%i = %i\n", n, 10, v);
        uint8_t rem = n - (v << 3) - (v << 1);
        //printf("  %i%%%i = %i\n", n, 10, rem);
        n = v;

        digits[i] = rem;

        if (i == 0)
            break;
        --i;
    }
    digits[i] = n;

    return digits + i;
}

#ifdef TEST

static void uint8_to_bcd_test_(uint8_t val)
{
    uint8_t digits[4];
    digits[3] = '\0';
    uint8_t *digits_ = uint8_to_bcd(val, digits, 3);
    uint8_t i;
    for (i = 0; i < 3 - (digits_ - digits); ++i)
        digits_[i] += '0';
    printf("%i -> '%s' (%i)\n", val, digits_, (int)bcd_length_after_op(digits, 3, digits_));
}

static void uint8_to_bcd_test()
{
    uint8_to_bcd_test_(1);
    uint8_to_bcd_test_(10);
    uint8_to_bcd_test_(100);
    uint8_to_bcd_test_(0);
    uint8_to_bcd_test_(124);
    uint8_to_bcd_test_(12);
    uint8_to_bcd_test_(255);
    uint8_to_bcd_test_(26);
    uint8_to_bcd_test_(27);
    printf("\n");
}

static void mul_test1()
{
    uint8_t digits1[] = { 0, 0, 0, 1, 2, 3, '\0' };
    uint8_t digits2[] = { 2, '\0' };

    uint8_t *r = bcd_mul(digits1+3, 3, digits2, 1);
    bcd_to_string(r, bcd_length_after_op(digits1+3, 3, r));
    printf("[%i] 123 * 456 = %s\n", bcd_length_after_op(digits1+3, 3, r), r);
    assert(!strcmp((char *)r, "56088"));
}

static void add_test1()
{
    uint8_t digits1[] = { 1, 2, 3, '\0' };
    uint8_t digits2[] = { 4, 5, 6, '\0' };

    uint8_t *r = bcd_add(digits1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_op(digits1, 3, r));
    printf("123 + 456 = %s\n", r);
    assert(!strcmp((char *)r, "579"));
}

static void add_test2()
{
    uint8_t digits1[] = { '\0', 9, 7, 8, '\0' };
    uint8_t digits2[] = {       8, 8, 6, '\0' };

    uint8_t *r = bcd_add(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("978 + 886 = %s\n", r);
    assert(!strcmp((char *)r, "1864"));
}

static void add_test3()
{
    uint8_t digits1[] = { '\0', 0, 0, 8, '\0' };
    uint8_t digits2[] = {       8, 8, 6, '\0' };

    uint8_t *r = bcd_add(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("008 + 886 = %s\n", r);
    assert(!strcmp((char *)r, "894"));
}

static void add_test4()
{
    uint8_t digits1[] = { 1, '\0' };
    uint8_t digits2[] = { 2, '\0' };

    uint8_t *r = bcd_add(digits1, 1, digits2, 1);
    bcd_to_string(r, bcd_length_after_op(digits1, 1, r));
    printf("1 + 2 = %s\n", r);
    assert(!strcmp((char *)r, "3"));
}

static void sub_test1()
{
    uint8_t digits1[] = { '\0', 8, 8, 6, '\0' };
    uint8_t digits2[] = {             8, '\0' };

    uint8_t *r = bcd_sub(digits1+1, 3, digits2, 1);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("886 - 8 = %s\n", r);
    assert(!strcmp((char *)r, "878"));
}

static void sub_test2()
{
    uint8_t digits1[] = { '\0', 1, 5, 2, '\0' };
    uint8_t digits2[] = {          1, 2, '\0' };

    uint8_t *r = bcd_sub(digits1+1, 3, digits2, 2);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("152 - 12 = %s\n", r);
    assert(!strcmp((char *)r, "140"));
}

static void gt_test1()
{
    uint8_t digits1[] = { 9, 7, 8, '\0' };
    uint8_t digits2[] = { 8, 8, 6, '\0' };

    bool v = bcd_gt(digits1, 3, digits2, 3);
    printf("978 > 886 = %s\n", v ? "true" : "false");
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
    printf("886 > 978 = %s\n", v ? "true" : "false");
    assert(!v);
}

static void gt_test4()
{
    uint8_t digits1[] = { 9, 7, 8, '\0' };
    uint8_t digits2[] = { 8, 6, '\0' };

    bool v = bcd_gt(digits1, 3, digits2, 2);
    printf("978 > 86 = %s\n", v ? "true" : "false");
    assert(v);
}

static void gt_test5()
{
    uint8_t digits1[] = { 9, 7, 8, '\0' };
    uint8_t digits2[] = { 8, 6, '\0' };

    bool v = bcd_gt(digits2, 2, digits1, 3);
    printf("86 > 978 = %s\n", v ? "true" : "false");
    assert(!v);
}

static void div_by_test1()
{
    uint8_t digits[] = { 8, '\0' };

    uint8_t *r = bcd_div_by_lt10(digits, 1, 4);
    bcd_to_string(r, bcd_length_after_op(digits, 1, r));
    printf("8 / 4 = %s\n", r);
    assert(!strcmp((char *)r, "2"));
}

static void div_by_test2()
{
    uint8_t digits[] = { 9, 9, 9, '\0' };
    uint8_t *r = bcd_div_by_lt10(digits, 3, 4);
    bcd_to_string(r, bcd_length_after_op(digits, 3, r));
    printf("999 / 4 = %s\n", r);
    assert(!strcmp((char *)r, "249"));
}

static void div_by_test3()
{
    uint8_t digits[] = { 3, 2, 1, '\0' };
    uint8_t *r = bcd_div_by_lt10(digits, 3, 9);
    bcd_to_string(r, bcd_length_after_op(digits, 3, r));
    printf("321 / 9 = %s\n", r);
    assert(!strcmp((char *)r, "35"));
}

static void div_by_test4()
{
    uint8_t digits[] = { 2, 0, 0, '\0' };
    uint8_t *r = bcd_div_by_lt10(digits, 3, 8);
    bcd_to_string(r, bcd_length_after_op(digits, 3, r));
    printf("200 / 8 = %s\n", r);
    assert(!strcmp((char *)r, "25"));
}

int main()
{
    uint8_to_bcd_test();

    mul_test1();

    add_test1();
    add_test2();
    add_test3();
    add_test4();

    sub_test1();
    sub_test2();

    gt_test1();
    gt_test2();
    gt_test3();
    gt_test4();
    gt_test5();

    div_by_test1();
    div_by_test2();
    div_by_test3();
    div_by_test4();
}

#endif
