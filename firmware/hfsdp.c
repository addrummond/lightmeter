#include <hfsdp.h>
#include <goetzel.h>
#include <debugging.h>

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s, int32_t f1_coscoeff, int32_t f1_sincoeff, int32_t f2_coscoeff, int32_t f2_sincoeff)
{
    s->calib_count = 0;
    s->min_f1 = 2147483647;
    s->max_f1 = -1;
    s->min_f2 = 2147483647;
    s->max_f2 = -1;
    s->avg = 0;
    s->f1_coscoeff = f1_coscoeff;
    s->f1_sincoeff = f1_sincoeff;
    s->f2_coscoeff = f2_coscoeff;
    s->f2_sincoeff = f2_sincoeff;
    s->count = 0;
    s->f1_less_than_f2 = -1;
}

bool hfsdp_check_start(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen)
{
    if (s->calib_count == 8*HFSDP_SAMPLE_MULTPLIER) { // Long enough to see both high and low values.
        s->avg /= sizeof(s->vs)/sizeof(int32_t);

        // We want to see a high standard deviation -- that indicates that
        // this is an actual square wave signal and not just noise.
        int32_t stddev = 0;
        unsigned i;
        for (i = 0; i < sizeof(s->vs)/sizeof(int32_t); ++i) {
            int32_t x = (s->vs[i] - s->avg);
            stddev += x*x;
        }

        if (stddev > 8000) { // Magic empirically-determined number.
            s->highest_f1_low = s->max_f1/8;
            s->highest_f2_low = s->max_f2/8;
            debugging_writec("YES!\n");
            return true;
        }
        else {
            s->calib_count = 0;
            s->min_f1 = 2147483647;
            s->max_f1 = -1;
            s->avg = 0;
            return false;
        }
    }

    goetzel_result_t r1, r2;
    int32_t p1, p2;
    goetzel2(buf, buflen, 0,
             s->f1_coscoeff, s->f2_sincoeff,
             s->f1_coscoeff, s->f2_sincoeff,
             &r1, &r2);
    p1 = goetzel_get_freq_power(&r1);
    p2 = goetzel_get_freq_power(&r2);

    if (s->calib_count < sizeof(s->vs)/sizeof(s->vs[0])) {
        s->vs[s->calib_count] = p1;
        s->avg += p1;
    }

    ++(s->calib_count);

    if (p1 > s->max_f1)
        s->max_f1 = p1;
    else if (p2 < s->min_f1)
        s->min_f1 = p1;

    if (p2 > s->max_f2)
        s->max_f2 = p2;
    else if (p2 < s->min_f2)
        s->min_f2 = p2;

    // We want to try and sample somewhere around the middle of each bit.
    if (s->f1_less_than_f2 == -2) {
        ;
    }
    else if (s->f1_less_than_f2 == -1) {
        s->f1_less_than_f2 = (p1 < p2);
    }
    else if ((s->f1_less_than_f2 && p2 >= p1) ||
             ((!s->f1_less_than_f2) && p1 < p2)) {
        s->count = 0;
        s->f1_less_than_f2 = -2;
    }
    ++(s->count);
    if (s->count == HFSDP_SAMPLE_MULTPLIER)
        s->count = 0;

    return false;
}

int32_t hfsdp_read_bit_debug_last_f1;
int32_t hfsdp_read_bit_debug_last_f2;

int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen)
{
    goetzel_result_t r1, r2;
    int32_t p1, p2;
    goetzel2(buf, buflen, 0,
             s->f1_coscoeff, s->f1_sincoeff,
             s->f2_coscoeff, s->f2_sincoeff,
             &r1, &r2);

    p1 = goetzel_get_freq_power(&r1);
    p2 = goetzel_get_freq_power(&r2);

    hfsdp_read_bit_debug_last_f1 = p1;
    hfsdp_read_bit_debug_last_f2 = p2;

    ++(s->count);
    if (s->count == HFSDP_SAMPLE_MULTPLIER/2)
        return (p1 < p2);

    if (s->count == HFSDP_SAMPLE_MULTPLIER)
        s->count = 0;

    return HFSDP_READ_BIT_NOTHING_READ;
}

#ifdef TEST
#include <stdlib.h>
#include <stdio.h>

#define SAMPLES_TO_SKIP_F_BITS 8
#define SAMPLES_TO_SKIP ((int)(((float)(44100 << SAMPLES_TO_SKIP_F_BITS)/(HFSDP_SAMPLE_FREQ*1.0))))
#define NSAMPLES 40

#define COSCOEFF1_  -0.9070590106
#define SINCOEFF1_  0.4210035051
#define COSCOEFF2_  -0.9576483160
#define SINCOEFF2_  0.2879404501
#define COSCOEFF1   GOETZEL_FLOAT_TO_FIX(COSCOEFF1_)
#define SINCOEFF1   GOETZEL_FLOAT_TO_FIX(SINCOEFF1_)
#define COSCOEFF2   GOETZEL_FLOAT_TO_FIX(COSCOEFF2_)
#define SINCOEFF2   GOETZEL_FLOAT_TO_FIX(SINCOEFF2_)

#define MAKE_QUIETER_BY 1

static void get_fake_adc_vals(const char *filename, int16_t **out, unsigned *length)
{
    FILE *fp = fopen(filename, "r");

    float *vals = malloc(sizeof(float) * 100);
    unsigned vals_len = 100;
    unsigned vals_i = 0;

    for (;;) {
        int r = fscanf(fp, "%f\n", vals + vals_i);
        if (r <= 0)
            break;

        if (vals[vals_i] > 1.0 || vals[vals_i] < -1.0) {
            printf("OUT OF RANGE\n");
            return;
        }

        ++vals_i;
        if (vals_i == vals_len) {
            vals_len *= 2;
            vals = realloc(vals, vals_len * sizeof(float));
        }
    }

    fclose(fp);

    int16_t *fake_adc_vals = malloc(sizeof(int16_t) * vals_i);
    unsigned i;
    int32_t min = 100000, max = -100000;
    for (i = 0; i < vals_i; ++i) {
        fake_adc_vals[i] = (int16_t)(((4096.0/2)*vals[i])/MAKE_QUIETER_BY);
        if (fake_adc_vals[i] > max)
            max = fake_adc_vals[i];
        else if (fake_adc_vals[i] < min)
            min = fake_adc_vals[i];
    }
    free(vals);

    printf("MIN = %i, MAX = %i\n", min, max);

    *length = vals_i;
    *out = fake_adc_vals;
}

#define OFFSET 0

static int test0(const char *filename)
{
    unsigned vals_i;
    int16_t *fake_adc_vals;
    get_fake_adc_vals(filename, &fake_adc_vals, &vals_i);

    unsigned i;
    int32_t ref_power = -1;
    printf("sts, pow, clock r,clock i,data r,data i,pow f1,pow f2\n");
    for (i = 0; i < (vals_i << SAMPLES_TO_SKIP_F_BITS)/SAMPLES_TO_SKIP - 1; ++i) {
        int32_t ii = (i * SAMPLES_TO_SKIP) >> SAMPLES_TO_SKIP_F_BITS;
        int32_t rm = ii % (1 << SAMPLES_TO_SKIP_F_BITS);
        if (rm <= -(1 << (SAMPLES_TO_SKIP_F_BITS-1)))
            ii -= 1;
        else if (rm >= (1 << (SAMPLES_TO_SKIP_F_BITS-1)))
            ii += 1;

        goetzel_result_t gr1, gr2;
        int32_t pow1, pow2;
        goetzel2(fake_adc_vals + ii, NSAMPLES, 0,
                 COSCOEFF1, SINCOEFF1,
                 COSCOEFF2, SINCOEFF2,
                 &gr1, &gr2);
        pow1 = goetzel_get_freq_power(&gr1);
        pow2 = goetzel_get_freq_power(&gr2);

        printf("%i,%i,%i,%i,%i,%i,%i,%i\n", SAMPLES_TO_SKIP, gr1.total_power, gr1.r, gr1.i, gr2.r, gr2.i, pow1, pow2);
    }

    return 0;
}

// TODO: Update with new names for coeffs.
/*static int test1(const char *filename)
{
    unsigned vals_i;
    int16_t *fake_adc_vals;
    get_fake_adc_vals(filename, &fake_adc_vals, &vals_i);

    hfsdp_read_bit_state_t s;
    init_hfsdp_read_bit_state(&s, CLOCK_COSCOEFF, CLOCK_SINCOEFF, DATA_COSCOEFF, DATA_SINCOEFF);
    bool started = true;
    unsigned i;
    for (i = 0; i < (vals_i << SAMPLES_TO_SKIP_F_BITS)/SAMPLES_TO_SKIP - 1; ++i) {
        int32_t ii = OFFSET + ((i * SAMPLES_TO_SKIP) >> SAMPLES_TO_SKIP_F_BITS);
        int32_t rm = ii % (1 << SAMPLES_TO_SKIP_F_BITS);
        if (rm <= -(1 << (SAMPLES_TO_SKIP_F_BITS-1)))
            ii -= 1;
        else if (rm >= (1 << (SAMPLES_TO_SKIP_F_BITS-1)))
            ii += 1;

        if (! started) {
            started = hfsdp_check_start(&s, fake_adc_vals + ii, NSAMPLES);
        }
        else {
            int r = hfsdp_read_bit(&s, fake_adc_vals + ii, NSAMPLES);

            if (r == HFSDP_READ_BIT_DECODE_ERROR)
                printf("DECODE ERROR\n");
            else if (r == HFSDP_READ_BIT_NOTHING_READ)
                ;//printf("NUTTIN\n");
            else
                printf("B: %i\n", r);
        }
    }

    return 0;
}*/

static void test2(const char *filename)
{
    unsigned vals_i;
    int16_t *fake_adc_vals;
    get_fake_adc_vals(filename, &fake_adc_vals, &vals_i);

    int16_t c1, c2, best_c1, best_c2;
    int32_t max_amp = -1;
    goetzel_result_t gr;
    for (c1 = -65536/2+1; c1 < 65536/2-1; c1 += 10) {
        for (c2 = -65536/2+1; c2 < 65536/2-1; c2 += 10) {
            goetzel_result_t gr;
            goetzel1(fake_adc_vals, 64, 0, c1, c2, &gr);
            int32_t amp = goetzel_get_freq_power(&gr);
            if (amp > max_amp) {
                max_amp = amp;
                best_c1 = c1;
                best_c2 = c2;
            }
        }
    }

    printf("Best c1 = %i, c2 = %i\n", best_c1, best_c2);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Bad arguments\n");
        exit(1);
    }

    test0(argv[1]);
}

#endif
