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

// Assumes that there are sufficient available bytes at digits1[-1] onwards.
// Stores result in digits1.
// Returns a pointer which may be equal to, less than or greater than digits1.
// This is used to implement both bcd_add and bcd_sub (which are now macros).
uint8_t *bcd_add_(uint8_t *digits1, uint_fast8_t digits1_length,
                  uint8_t *digits2, uint_fast8_t digits2_length,
                  uint_fast8_t xoradd)
{
    uint_fast8_t end1 = digits1_length - 1;
    uint_fast8_t end2 = digits2_length - 1;

    int_fast8_t carry = 0;

    int_fast8_t i = end1, j = end2;
    do {
        int_fast8_t x = 0;
        if (j >= 0)
            x = digits2[j];
        int_fast8_t y = 0;
        if (i >= 0)
            y = digits1[i];
        x = (x ^ xoradd) + (xoradd & 1);
        int_fast8_t r = x + y + carry;
        if (r < 0) {
            digits1[i] = r + 10;
            carry = -1;
        }
        else if (r > 9) {
            digits1[i] = r - 10;
            carry = 1;
        }
        else {
            carry = 0;
            digits1[i] = r;
        }
    } while (--j, --i, i >= 0 || j >= 0);

    ++i;
    if (carry != 0)
        --i;

    if (i < 0)
        digits1 += i;

    if (carry != 0) {
        if (i < 0)
            *digits1 = carry;
        else
            digits1[i] += carry;
    }

    // Strip leading zeroes.
    for (i = 0; digits1[i] == 0 && i < digits1_length-1; ++i);
    digits1 += i;

    return digits1;
}

// Save some code space by implementing <, <=, >, >=, = in one function.
// This function is called by macros bcd_lt, bcd_gteq, etc.
bool bcd_cmp(const uint8_t *digits1, uint_fast8_t length1, const uint8_t *digits2, uint_fast8_t length2,
             uint_fast8_t which) // 0 == eq, 1 = lteq, 2 = gteq, 3 = lt, 4 = gt)
{
    uint_fast8_t i, j;
    uint_fast8_t zeroes1 = 0;
    uint_fast8_t zeroes2 = 0;
    if (length2 > length1)
        zeroes1 = length2 - length1;
    else
        zeroes2 = length1 - length2;

    i = 0, j = 0;
    for (; i < length1;) {
        int_fast8_t a, b;
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

        int_fast8_t diff = a - b;
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

uint8_t *bcd_mul(uint8_t *digits1, uint_fast8_t length1, const uint8_t *digits2, uint_fast8_t length2)
{
    uint_fast8_t l = length1 + length2;
    uint8_t tmp1[l], tmp2[l];
    memset8_zero(tmp1, l);
    memset8_zero(tmp2, l);

    uint8_t *result_digits = tmp2;

    // Multiply by each digit separately.
    uint_fast8_t j = length2-1;
    uint_fast8_t result_digits_length = l;
    uint_fast8_t zeroes = 0;
    do {
        uint8_t *mulres_start = tmp1 + l - 1;
        // Add trailing zeroes as appropriate.
        uint_fast8_t z;
        for (z = 0; z < zeroes; ++z)
            *(mulres_start--) = 0;
        ++zeroes;

        uint_fast8_t carry = 0;
        uint_fast8_t i = length1-1;
        do {
            uint_fast8_t r = (digits1[i] * digits2[j]) + carry;
            *(mulres_start--) = r % 10;
            carry = r / 10;
        } while (i-- > 0);
        if (carry != 0) {
            *mulres_start = carry;
        }


        uint_fast8_t mulres_length = l - (mulres_start - tmp1);

        //printf("(M) ADDING ");
        //uint8_t x;
        //for (x = 0; x < result_digits_length; ++x)
        //    printf("%c", result_digits[x] + '0');
        //printf(" TO ");
        //for (x = 0; x < mulres_length; ++x)
        //    printf("%c", mulres_start[x] + '0');

        // Add to total.
        uint8_t *result_digits_ = bcd_add(result_digits, result_digits_length, mulres_start, mulres_length);
        result_digits_length = bcd_length_after_op(result_digits, result_digits_length, result_digits_);
        result_digits = result_digits_;

        //printf(" = ");
        //for (x = 0; x < result_digits_length; ++x)
        //    printf("%c", result_digits[x] + '0');
        //printf("\n");
    } while (j-- > 0);

    uint8_t *start = digits1 + length1;
    --result_digits_length;
    do {
        *(--start) = result_digits[result_digits_length];
    } while (result_digits_length-- > 0);

    return start;
}

static const uint8_t TEN[] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };
uint8_t *bcd_div_by_lt10(uint8_t *digits, uint_fast8_t length, uint_fast8_t by)
{
    assert(by < 10);

    uint_fast8_t n = digits[0];
    uint_fast8_t outi;
    uint_fast8_t rem = 0;
    for (outi = 0; outi < length; ++outi) {
        n = digits[outi];
        uint_fast8_t rem8 = rem << 3;
        n += rem8;
        n += rem + rem;
        if (n < by && outi != length - 1) {
            n = TEN[n];
            n += digits[outi+1];
            digits[outi++] = 0;
        }

        uint_fast8_t c;
        for (c = 0; n >= by; ++c, n -= by);
        rem = n;

        digits[outi] = c;
    }

    // Strip leading zeroes.
    for (outi = 0; digits[outi] == 0 && outi < length-1; ++outi);
    digits += outi;

    return digits;
}

static uint_fast8_t first_nonzero_index(uint8_t *digits, uint_fast8_t length)
{
    uint_fast8_t i;
    for (i = 0; i < length && digits[i] == 0; ++i);
    return i;
}

#if BCD_EXP2_PRECISION == 0
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6)
#elif BCD_EXP2_PRECISION == 1
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6) ,d1l
#elif BCD_EXP2_PRECISION == 2
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6) ,d1,d2l
#elif BCD_EXP2_PRECISION == 3
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6) ,d1,d2,d3l
#elif BCD_EXP2_PRECISION == 4
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6) ,d1,d2,d3,d4l
#elif BCD_EXP2_PRECISION == 5
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6) ,d1,d2,d3,d4,d5l
#elif BCD_EXP2_PRECISION == 6
#define DIGITS(d1,d1l,d2,d2l,d3,d3l,d4,d4l,d5,d5l,d6) ,d1,d2,d3,d4,d5,d6
#else
#error "Bad value for BCD_EXP2_PRECISION"
#endif
// First element is number of leading zeroes.
// TODO: Might make more sense to have first element give length of array.
static const uint8_t TWO_10[]      = { 2,  0,0,1,0,2,4  DIGITS(0,0, 0,0, 0,0, 0,0, 0,0, 0) };
static const uint8_t TWO_4[]       = { 4,  0,0,0,0,1,6  DIGITS(0,0, 0,0, 0,0, 0,0, 0,0, 0) };
static const uint8_t TWO_1[]       = { 5,  0,0,0,0,0,2  DIGITS(0,0, 0,0, 0,0, 0,0, 0,0, 0) };
static const uint8_t TWO_0_PT_5[]  = { 5,  0,0,0,0,0,1  DIGITS(4,4, 1,1, 4,4, 2,2, 1,1, 4) };
static const uint8_t TWO_0_PT_1[]  = { 5,  0,0,0,0,0,1  DIGITS(0,1, 7,7, 1,2, 7,8, 7,7, 3) };
static const uint8_t TWO_0_PT_05[] = { 5,  0,0,0,0,0,1  DIGITS(0,0, 3,4, 5,5, 2,3, 6,6, 5) };
static const uint8_t TWO_0_PT_01[] = { 5,  0,0,0,0,0,1  DIGITS(0,0, 0,1, 6,7, 9,9, 5,6, 5) };
#undef DIGITS
// x!=10 condition is added to make it easy for GCC to optimize out the check following the && in this case.
#define GTEQ(n,l,minlen,fnzi,i,j,x) ((l) >= BCD_EXP2_PRECISION+(minlen) &&                       \
                                     ((fnzi) <= (l)-BCD_EXP2_PRECISION-1+(i) ||                  \
                                      ((x) != 10 && (n)[(l)-BCD_EXP2_PRECISION-1+(j)] >= (x))))
#define GTEQ_30(n,l,fnzi)         GTEQ((n), (l),  1, (fnzi), -2,  -1, 3)
#define GTEQ_10(n,l,fnzi)         GTEQ((n), (l),  1, (fnzi), -1,  0,  10)
#define GTEQ_4(n,l,fnzi)          GTEQ((n), (l),  1, (fnzi), -1,  0,  4)
#define GTEQ_1(n,l,fnzi)          GTEQ((n), (l),  1, (fnzi),  0,  0,  1)
#define GTEQ_0_PT_5(n,l,fnzi)     GTEQ((n), (l),  0, (fnzi),  0,  1,  5)
#define GTEQ_0_PT_1(n,l,fnzi)     GTEQ((n), (l),  0, (fnzi),  0,  1,  1)
#define GTEQ_0_PT_05(n,l,fnzi)    GTEQ((n), (l), -1, (fnzi),  1,  2,  5)
#define GTEQ_0_PT_01(n,l,fnzi)    GTEQ((n), (l), -1, (fnzi),  1,  2,  1)
#define N_DIGITS                  ((sizeof(TWO_4)/sizeof(uint8_t)) - 1)
#define N_WHOLE_DIGITS            ((sizeof(TWO_4)/sizeof(uint8_t)) - BCD_EXP2_PRECISION - 1)
uint8_t *bcd_exp2(uint8_t *digits, uint_fast8_t length)
{
#ifdef TEST
    assert(!GTEQ_30(digits, length, first_nonzero_index(digits, length)));
#endif

    uint8_t log_digits_[length];
    uint8_t *log_digits = log_digits_;
    uint_fast8_t i;
    for (i = 0; i < length; ++i)
        log_digits[i] = digits[i];

    uint8_t sub_digits[length];

    uint_fast8_t result_length = length;
    bool first_loop;
    uint_fast8_t fnzi;
    for (first_loop = true;
         fnzi = first_nonzero_index(log_digits, length), GTEQ_0_PT_01(log_digits, length, fnzi);
         first_loop = false) {
        memset8_zero(sub_digits, sizeof(sub_digits));

        //printf("RDIGS ");
        //uint8_t x;
        //for (x = 0; x < length; ++x)
        //    printf("%c", log_digits[x] + '0');
        //printf("\n");

        const uint8_t *mulby_digits;
        if (GTEQ_10(log_digits, length, fnzi)) {
            //printf("MB 10\n");
            sub_digits[length-BCD_EXP2_PRECISION-2] = 1;
            mulby_digits = TWO_10;
        }
        else if (GTEQ_4(log_digits, length, fnzi)) {
            //printf("MB 4\n");
            sub_digits[length-BCD_EXP2_PRECISION-1] = 4;
            mulby_digits = TWO_4;
        }
        else if (GTEQ_1(log_digits, length, fnzi)) {
            //printf("MB 1\n");
            sub_digits[length-BCD_EXP2_PRECISION-1] = 1;
            mulby_digits = TWO_1;
        }
        else if (GTEQ_0_PT_5(log_digits, length, fnzi)) {
            //printf("MB 0.5\n");
            sub_digits[length-BCD_EXP2_PRECISION] = 5;
            mulby_digits = TWO_0_PT_5;
        }
        else if (GTEQ_0_PT_1(log_digits, length, fnzi)) {
            //printf("MB 0.1\n");
            sub_digits[length-BCD_EXP2_PRECISION] = 1;
            mulby_digits = TWO_0_PT_1;
        }
        else if (GTEQ_0_PT_05(log_digits, length, fnzi)) {
            //printf("MB 0.05\n");
            sub_digits[length-BCD_EXP2_PRECISION+1] = 5;
            mulby_digits = TWO_0_PT_05;
        }
        else { //if (GTEQ_0_PT_01(log_digits, length, fnzi)) {
            //printf("MB 0.01\n");
            sub_digits[length-BCD_EXP2_PRECISION+1] = 1;
            mulby_digits = TWO_0_PT_01;
        }

        uint_fast8_t trailing_zeroes = mulby_digits[0];
        ++mulby_digits;

        if (first_loop) {
            i = N_DIGITS-1;
            digits += result_length;
            result_length = 0;
            do {
                *(--digits) = mulby_digits[i];
                ++result_length;
            } while (i-- > trailing_zeroes);
        }
        else {
            //printf("MULTIPLYING ");
            //uint8_t x;
            //for (x = 0; x < result_length; ++x)
            //    printf("%c", digits[x] + '0');
            //printf(" * ");
            //for (x = 0; x < N_DIGITS; ++x)
            //    printf("%c", mulby_digits[x] + '0');

            uint8_t *digits_n = bcd_mul(digits, result_length, mulby_digits, N_DIGITS);
            result_length = bcd_length_after_op(digits, result_length, digits_n);
            digits = digits_n;

            //printf(" = ");
            //for (x = 0; x < result_length; ++x)
            //    printf("%c", digits[x] + '0');
            //printf("\n");

            // Round and remove digits from end to get back to BCD_EXP2_PRECISION.
            // Lazy rounding -- we don't bother propagating.
            if (digits[result_length-BCD_EXP2_PRECISION] >= 5 && digits[result_length-BCD_EXP2_PRECISION-1] < 9)
                ++digits[result_length-BCD_EXP2_PRECISION-1];
            i = result_length - 1;
            uint_fast8_t j = result_length - 1 - BCD_EXP2_PRECISION;
            do {
                digits[i] = digits[j];
            } while (--i, j-- > 0);
            digits += BCD_EXP2_PRECISION;
            result_length -= BCD_EXP2_PRECISION;
        }

        uint8_t *log_digits_n = bcd_sub(log_digits, length, sub_digits, length);
        length = bcd_length_after_op(log_digits, length, log_digits_n);
        log_digits = log_digits_n;
    }

    if (first_loop) {
        // The number was very small. Set appopriate approx result (0).
        digits += result_length - 1;
        *digits = 0;
    }

    return digits;
}
#undef GTEQ
#undef GTEQ_10
#undef GTEQ_4
#undef GTEQ_1
#undef GTEQ_0_PT_5
#undef GTEQ_0_PT_1
#undef GTEQ_0_PT_05
#undef GTEQ_0_PT_01
#undef N_DIGITS
#undef N_WHOLE_DIGITS

uint8_t *uint32_to_bcd(uint32_t n, uint8_t *digits, uint_fast8_t length)
{
    uint_fast8_t i = length-1;
    while (n >= 10) {
        uint32_t v = n/10;
        //printf("  %i/%i = %i\n", n, 10, v);
        uint_fast8_t rem = n - (v*10);
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

// 'digits' and 'dest' may point to same buffer.
uint_fast8_t bcd_to_string_fp(uint8_t *digits, uint_fast8_t length, uint8_t *dest, uint_fast8_t sigfigs, uint_fast8_t dps)
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

static void uint32_to_bcd_test_(uint32_t val)
{
    uint8_t digits[8];
    digits[7] = '\0';
    uint8_t *digits_ = uint32_to_bcd(val, digits, 7);
    uint_fast8_t i;
    for (i = 0; i < 7 - (digits_ - digits); ++i)
        digits_[i] += '0';
    printf("%i -> '%s' (%i)\n", val, digits_, (int)bcd_length_after_op(digits, 7, digits_));
}

static void uint32_to_bcd_test()
{
    uint32_to_bcd_test_(1);
    uint32_to_bcd_test_(10);
    uint32_to_bcd_test_(100);
    uint32_to_bcd_test_(0);
    uint32_to_bcd_test_(124);
    uint32_to_bcd_test_(12);
    uint32_to_bcd_test_(255);
    uint32_to_bcd_test_(26);
    uint32_to_bcd_test_(27);
    uint32_to_bcd_test_(65536);
    printf("\n");
}

static void bcd_to_string_fp_test()
{
    uint8_t digits[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0  };
    uint8_t dest[sizeof(digits)/sizeof(uint8_t) + 1 /* point */ + 1 /* zero terminator */];

    bcd_to_string_fp(digits, (sizeof(digits)/sizeof(uint8_t))-2, digits, 7, 3);

    printf("bcd_to_string_fp: %s\n", digits);
}

static void exp2_test1()
{
    uint8_t digits[] = { 0, 1, 0, 0, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+1, 4);
    bcd_to_string(r, bcd_length_after_op(digits+1, 4, r));
    printf("1^2 = %s/1000\n", r);
    assert(!strcmp("2000", (char *)r));
}

static void exp2_test2()
{
    uint8_t digits[] = { 0, 0, 0, 2, 0, 0, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+3, 4);
    bcd_to_string(r, bcd_length_after_op(digits+3, 4, r));
    printf("2^2 = %s/1000\n", r);
    assert(!strcmp("4000", (char *)r));
}

static void exp2_test3()
{
    uint8_t digits[] = { 0, 0, 0, 1, 5, 0, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+3, 4);
    bcd_to_string(r, bcd_length_after_op(digits+3, 4, r));
    printf("[%i], 2^1.5 = %s/1000\n", bcd_length_after_op(digits+3, 4, r), r);
    assert(!strcmp("2828", (char *)r));
}

static void exp2_test4()
{
    uint8_t digits[] = { 0, 0, 0, 1, 6, 0, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+3, 4);
    bcd_to_string(r, bcd_length_after_op(digits+3, 4, r));
    printf("[%i], 2^1.6 = %s/1000\n", bcd_length_after_op(digits+3, 4, r), r);
    assert(!strcmp("3032", (char *)r));
}

static void exp2_test5()
{
    uint8_t digits[] = { 0, 0, 0, 1, 6, 1, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+3, 4);
    bcd_to_string(r, bcd_length_after_op(digits+3, 4, r));
    printf("[%i], 2^1.61 = %s/1000\n", bcd_length_after_op(digits+3, 4, r), r);
    assert(!strcmp("3053", (char *)r));
}

static void exp2_test6()
{
    uint8_t digits[] = { 0, 0, 0, 0, 1, 5, 6, 1, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+4, 5);
    bcd_to_string(r, bcd_length_after_op(digits+4, 5, r));
    printf("[%i], 2^15.61 = %s/1000\n", bcd_length_after_op(digits+4, 5, r), r);
    assert(!strcmp("50017687", (char *)r));
}

static void exp2_test7()
{
    uint8_t digits[] = { 0, 0, 0, 0, 0, 6, 0, 0, 0, '\0' };
    uint8_t *r = bcd_exp2(digits+5, 4);
    bcd_to_string(r, bcd_length_after_op(digits+5, 4, r));
    printf("[%i], 2^6 = %s/1000\n", bcd_length_after_op(digits+5, 4, r), r);
    assert(!strcmp("64000", (char *)r));
}

static void mul_test1()
{
    uint8_t digits1[] = { 0, 0, 0, 1, 9, 3, '\0' };
    uint8_t digits2[] = { 2, '\0' };

    uint8_t *r = bcd_mul(digits1+3, 3, digits2, 1);
    bcd_to_string(r, bcd_length_after_op(digits1+3, 3, r));
    printf("193 * 2 = %s\n", r);
    assert(!strcmp((char *)r, "386"));
}

static void mul_test2()
{
    uint8_t digits1[] = { 0, 0, 0, 1, 9, 3, '\0' };
    uint8_t digits2[] = { 2, 8, '\0' };

    uint8_t *r = bcd_mul(digits1+3, 3, digits2, 2);
    bcd_to_string(r, bcd_length_after_op(digits1+3, 3, r));
    printf("193 * 28 = %s\n", r);
    assert(!strcmp((char *)r, "5404"));
}

static void mul_test3()
{
    uint8_t digits1[] = { 0, 0, 0, 1, 0, 0, 0, '\0' };
    uint8_t digits2[] = { 0, 0, 0, 0, 0, 3, 1, 6, 2, '\0' };

    uint8_t *r = bcd_mul(digits1+3, 4, digits2+5, 4);
    bcd_to_string(r, bcd_length_after_op(digits1+3, 4, r));
    printf("1000 * 000003162 = %s\n", r);
    assert(!strcmp((char *)r, "3162000"));
}

static void mul_test4()
{
    uint8_t digits1[] = { 0, 0, 0, 6, 0, 2, 7, 3, '\0' };
    uint8_t digits2[] = { 2, 5, '\0' };

    uint8_t *r = bcd_mul(digits1+3, 5, digits2, 2);
    bcd_to_string(r, bcd_length_after_op(digits1+3, 5, r));
    printf("60273 * 25 = %s\n", r);
    assert(!strcmp((char *)r, "1506825"));
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

static void sub_test3()
{
    uint8_t digits1[] = { '\0', 1, 5, 0, '\0' };
    uint8_t digits2[] = {       1, 0, 0, '\0' };

    uint8_t *r = bcd_sub(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("150 - 100 = %s\n", r);
    assert(!strcmp((char *)r, "50"));
}

static void sub_test4()
{
    uint8_t digits1[] = { '\0', 1, 5, 0, '\0' };
    uint8_t digits2[] = {       1, 5, 0, '\0' };

    uint8_t *r = bcd_sub(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("150 - 150 = %s\n", r);
    assert(!strcmp((char *)r, "0"));
}

static void sub_test5()
{
    uint8_t digits1[] = { '\0', 5, 0, 0, '\0' };
    uint8_t digits2[] = {       5, 0, 0, '\0' };

    uint8_t *r = bcd_sub(digits1+1, 3, digits2, 3);
    bcd_to_string(r, bcd_length_after_op(digits1+1, 3, r));
    printf("500 - 500 = %s\n", r);
    assert(!strcmp((char *)r, "0"));
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
    uint32_to_bcd_test();

    bcd_to_string_fp_test();

    exp2_test1();
    exp2_test2();
    exp2_test3();
    exp2_test4();
    exp2_test5();
    exp2_test6();
    exp2_test7();

    mul_test1();
    mul_test2();
    mul_test3();
    mul_test4();

    add_test1();
    add_test2();
    add_test3();
    add_test4();

    sub_test1();
    sub_test2();
    sub_test3();
    sub_test4();
    sub_test5();

    gt_test1();
    gt_test2();
    gt_test3();
    gt_test4();
    gt_test5();

    div_by_test1();
    div_by_test2();
    div_by_test3();
    div_by_test4();

    return 0;
}

#endif
