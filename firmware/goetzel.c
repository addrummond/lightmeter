#include <stdint.h>
#include <string.h>
#include <goetzel.h>

#ifdef TEST
#include <math.h>
#include <stdio.h>
#include <assert.h>
#endif

// ADC samples are 12-bit.
//
// This module assumes that the raw ADC value has been converted to a SIGNED
// 12-bit value.
//

#define ADC_VAL_BITS 12

static int32_t ADC_TO_FIX(int32_t x)
{
    return x;
}

#define FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<(ADC_VAL_BITS-1))))
#define FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<(ADC_VAL_BITS-1))))

static int32_t MUL(int32_t x, int32_t y)
{
    return (x * y) >> (ADC_VAL_BITS-1);
}

static int32_t DIV(int32_t x, int32_t y)
{
   return (x << (ADC_VAL_BITS-1)) / y;
}

void goetzel_state_init(goetzel_state_t *st, int32_t coeff)
{
    memset(st, 0, sizeof(*st));
    st->coeff = coeff;
}

int32_t goetzel_step(goetzel_state_t *st, int32_t sample)
{
#ifdef TEST
    assert(-2048 < sample && sample < 2048);
#endif

    int32_t s = ADC_TO_FIX(sample) + MUL(st->coeff, st->prev1) - st->prev2;
    st->prev2 = st->prev1;
    st->prev1 = s;
    ++(st->n);

    int32_t p = MUL(st->prev2,st->prev2) + MUL(st->prev1,st->prev1) - MUL(MUL(st->coeff,st->prev1),st->prev2);
    st->power += MUL(sample,sample);
    if (st->power == 0)
        st->power = 1;
    return DIV(DIV(p,st->power),st->n);
}

#ifdef TEST

//
// * Signal is the sum of a 1000Hz sin wave and a 9000Hz cos wave.
// * Sample rate is 40,000Hz.
// * Outputs values for frequency bins in CSV format.
// * Note that number of frequency bins is independent of number of samples,
//   since we are redoing the entire computation for every frequency of interest.
//
static void test1()
{
    unsigned N_SAMPLES = 512;
    unsigned N_BINS = 400;

    const float SAMPLE_FREQ = 40000;
    const float SIG1_FREQ = 1000;
    const float SIG2_FREQ = 9000;

    printf("bin,bin_freq,value\n");

    goetzel_state_t gs;
    unsigned i, j;
    for (j = 1; j <= N_BINS; ++j) {
        float BIN_FREQ = ((float)j/(float)N_BINS)*(SAMPLE_FREQ/2);
        float NFREQ = BIN_FREQ/SAMPLE_FREQ;
        int32_t COEFF = FLOAT_TO_FIX(2*cos(2*M_PI*NFREQ*1));

        goetzel_state_init(&gs, COEFF);

        // It's a bit inefficient to recompute samples for each outer loop, but
        // this doesn't really matter in test code.
        uint32_t p;
        for (i = 0; i < N_SAMPLES; ++i) {
            float t = i/SAMPLE_FREQ;
            float sample_value = 0.24*(sin(2.0*M_PI*SIG1_FREQ*t)) +
                                 0.24*(cos(2.0*M_PI*SIG2_FREQ*t));
            int32_t sample12 = (int32_t)(sample_value*(4096/2.0));
            p = goetzel_step(&gs, sample12);
        }
        printf("%i,%f,%f\n", j, BIN_FREQ, (float)p);
    }
}

int main()
{
    test1();
}

#endif
