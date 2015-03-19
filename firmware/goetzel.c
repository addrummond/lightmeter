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

static inline int32_t MUL(int32_t x, int32_t y)
{
    return (x * y) >> (GOETZEL_ADC_VAL_BITS-1);
}

static inline int32_t DIV(int32_t x, int32_t y)
{
   return (x << (GOETZEL_ADC_VAL_BITS-1)) / y;
}

#define GOETZEL_NORMALIZE
int32_t goetzel(const uint16_t *samples, unsigned length, int32_t coeff)
{
#ifdef GOETZEL_NORMALIZE
    int32_t total_power = 0;
#endif
    int32_t prev1 = 0, prev2 = 0;
    unsigned i;
    for (i = 0; i < length; ++i) {
#ifdef GOETZEL_NORMALIZE
        total_power += MUL(samples[i],samples[i]);
#endif
        int32_t s = samples[i] + MUL(coeff, prev1) - prev2 - (4096.0/2.0);
        prev2 = prev1;
        prev1 = s;
    }
    int32_t r = MUL(prev2,prev2) + MUL(prev1,prev1) - MUL(MUL(coeff,prev1),prev2);
#ifdef GOETZEL_NORMALIZE
    // No point normalizing for length because we'll always be comparing buffers
    // of the same length.
    r = /*DIV(*/DIV(r, total_power)/*, length)*/;
#endif
    if (r < 0)
        r = -r;
    return r;
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

    uint16_t samples[N_SAMPLES];
    unsigned i;
    for (i = 0; i < N_SAMPLES; ++i) {
        float t = (float)i/(float)SAMPLE_FREQ;
        float v = 0.24*(sin(2.0*M_PI*SIG1_FREQ*t)) +
                  0.24*(cos(2.0*M_PI*SIG2_FREQ*t));
        samples[i] = (int32_t)(v*(4096/2.0)) - (4096/2.0);
    }

    printf("bin,bin_freq,value\n");

    unsigned j;
    for (j = 1; j <= N_BINS; ++j) {
        float BIN_FREQ = ((float)j/(float)N_BINS)*(SAMPLE_FREQ/2);
        float NFREQ = BIN_FREQ/SAMPLE_FREQ;
        int32_t COEFF = GOETZEL_FLOAT_TO_FIX(2*cos(2*M_PI*NFREQ*1));

        int32_t p = goetzel(samples, N_SAMPLES, COEFF);
        printf("%i,%f,%f\n", j, BIN_FREQ, (float)p);
    }
}

int main()
{
    test1();
}

#endif
