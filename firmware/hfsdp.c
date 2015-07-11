#include <hfsdp.h>
#include <goetzel.h>
#include <debugging.h>

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s, int32_t clock_coscoeff, int32_t clock_sincoeff, int32_t data_coscoeff, int32_t data_sincoeff)
{
    s->calib_count = 0;
    s->prev_pclock = -1;
    s->min_pclock = 2147483647;
    s->max_pclock = -1;
    s->avg = 0;
    s->clock_coscoeff = clock_coscoeff;
    s->clock_sincoeff = clock_sincoeff;
    s->data_coscoeff = data_coscoeff;
    s->data_sincoeff = data_sincoeff;
}

bool hfsdp_check_start(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen)
{
    if (s->calib_count == sizeof(s->vs)/sizeof(int32_t)) {
        s->avg /= sizeof(s->vs)/sizeof(int32_t);

        // We want to see a high standard deviation -- that indicates that
        // this is an actual square wave signal and not just noise.
        int32_t stddev = 0;
        unsigned i;
        for (i = 0; i < sizeof(s->vs)/sizeof(int32_t); ++i) {
            int32_t x = (s->vs[i] - s->avg);
            stddev += x*x;
        }

#ifndef TEST
        debugging_writec("STD: ");
        debugging_write_int32(stddev);
        debugging_writec("\n");
#endif

        if (stddev > 100000) {
            // The max clock level is typically only just above
            // the lowest level of the data line in its high state. We therefore can
            // set highest_low to around 1/2 of this level.
            s->highest_low = s->max_pclock/2;
#ifndef TEST
            debugging_writec("HL: ");
            debugging_write_int32(s->highest_low);
            debugging_writec("\n");
#endif
            return true;
        }
        else {
            s->calib_count = 0;
            s->min_pclock = 2147483647;
            s->max_pclock = -1;
            s->avg = 0;
            return false;
        }
    }

    int32_t pclock;
    goetzel1(buf, buflen,
             s->clock_coscoeff, s->clock_sincoeff,
             &pclock,
             0);

    s->prev_pclock = pclock;

    s->vs[s->calib_count] = pclock;
    s->avg += pclock;
    ++(s->calib_count);

    if (pclock > s->max_pclock)
        s->max_pclock = pclock;
    else if (pclock < s->min_pclock)
        s->min_pclock = pclock;

    return false;
}

#define HFSDP_CLOCK_THRESHOLD 200
#define HFSDP_DATA_THRESHOLD  200
int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen,
                   int32_t *debug_clock_amp, int32_t *debug_data_amp, int32_t *debug_power)
{
    int32_t pclock, pdata, power;
    goetzel2(buf, buflen,
             s->clock_coscoeff, s->clock_sincoeff,
             s->data_coscoeff, s->data_sincoeff,
             &pclock, &pdata,
             &power);

    if (debug_clock_amp)
        *debug_clock_amp = pclock;
    if (debug_data_amp)
        *debug_data_amp = pdata;
    if (debug_power)
        *debug_power = power;

    // No change in the clock level, so we don't read a bit.
    if (! ((pclock <= s->highest_low && s->prev_pclock > s->highest_low) ||
           (pclock > s->highest_low && s->prev_pclock <= s->highest_low))) {
        return HFSDP_READ_BIT_NOTHING_READ;
    }

    // Clock level has changed, so we read a bit.
    s->prev_pclock = pclock;
    return (pdata > s->highest_low);
}

#ifdef TEST
#include <stdlib.h>
#include <stdio.h>

#define SAMPLES_TO_SKIP_F_BITS 8
#define SAMPLES_TO_SKIP ((int)(((float)(44100 << SAMPLES_TO_SKIP_F_BITS)/(HFSDP_SAMPLE_FREQ*1.0))))

#define CLOCK_COSCOEFF_  -0.9576483160
#define CLOCK_SINCOEFF_  0.2879404501
#define DATA_COSCOEFF_   -0.8380881049
#define DATA_SINCOEFF_   0.5455349012
#define CLOCK_COSCOEFF   GOETZEL_FLOAT_TO_FIX(CLOCK_COSCOEFF_)
#define CLOCK_SINCOEFF   GOETZEL_FLOAT_TO_FIX(CLOCK_SINCOEFF_)
#define DATA_COSCOEFF    GOETZEL_FLOAT_TO_FIX(DATA_COSCOEFF_)
#define DATA_SINCOEFF    GOETZEL_FLOAT_TO_FIX(DATA_SINCOEFF_)

#define MAKE_QUIETER_BY 5.0

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

        ++vals_i;
        if (vals_i == vals_len) {
            vals_len *= 2;
            vals = realloc(vals, vals_len * sizeof(float));
        }
    }

    fclose(fp);

    int16_t *fake_adc_vals = malloc(sizeof(int16_t) * vals_i);
    unsigned i;
    for (i = 0; i < vals_i; ++i) {
        fake_adc_vals[i] = (int16_t)(((4096.0/2)*vals[i])/MAKE_QUIETER_BY);
    }
    free(vals);

    *length = vals_i;
    *out = fake_adc_vals;
}

static int test1(const char *filename)
{
    unsigned vals_i;
    int16_t *fake_adc_vals;
    get_fake_adc_vals(filename, &fake_adc_vals, &vals_i);

    hfsdp_read_bit_state_t s;
    init_hfsdp_read_bit_state(&s, CLOCK_COSCOEFF, CLOCK_SINCOEFF, DATA_COSCOEFF, DATA_SINCOEFF);
    bool started = false;
    unsigned i;
    for (i = 0; i < (vals_i << SAMPLES_TO_SKIP_F_BITS)/SAMPLES_TO_SKIP; ++i) {
        int32_t clock_amp, data_amp, powr;

        int32_t ii = (i * SAMPLES_TO_SKIP) >> SAMPLES_TO_SKIP_F_BITS;
        int32_t rm = ii % (1 << SAMPLES_TO_SKIP_F_BITS);
        if (rm <= -(1 << (SAMPLES_TO_SKIP_F_BITS-1)))
            ii -= 1;
        else if (rm >= (1 << (SAMPLES_TO_SKIP_F_BITS-1)))
            ii += 1;

        if (! started) {
            started = hfsdp_check_start(&s, fake_adc_vals + ii, (SAMPLES_TO_SKIP>>SAMPLES_TO_SKIP_F_BITS)-5);
        }
        else {
            int r = hfsdp_read_bit(&s, fake_adc_vals + ii, SAMPLES_TO_SKIP>>SAMPLES_TO_SKIP_F_BITS, &clock_amp, &data_amp, &powr);
            printf("%i, %i, %i\n", clock_amp, data_amp, powr);
            continue;

            if (r == HFSDP_READ_BIT_DECODE_ERROR)
                printf("DECODE ERROR\n");
            else if (r == HFSDP_READ_BIT_NOTHING_READ)
                ;//printf("NUTTIN\n");
            else
                printf("B: %i\n", r);
        }
    }

    return 0;
}

static void test2(const char *filename)
{
    unsigned vals_i;
    int16_t *fake_adc_vals;
    get_fake_adc_vals(filename, &fake_adc_vals, &vals_i);

    int16_t c1, c2, best_c1, best_c2;
    int32_t max_amp = -1;
    int32_t amp, power;
    for (c1 = -65536/2+1; c1 < 65536/2-1; c1 += 10) {
        for (c2 = -65536/2+1; c2 < 65536/2-1; c2 += 10) {
            goetzel1(fake_adc_vals, 64, c1, c2, &amp, &power);
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

    test1(argv[1]);
}

#endif
