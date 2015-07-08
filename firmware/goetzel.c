#include <stdint.h>
#include <string.h>
#include <goetzel.h>
#include <myassert.h>

#ifdef TEST
#include <math.h>
#include <stdio.h>
#include <assert.h>
#endif

static inline int32_t MUL(int32_t x, int32_t y)
{
    return (x * y) >> (GOETZEL_ADC_VAL_BITS-1);
}

static inline int32_t DIV(int32_t x, int32_t y)
{
   return (x << (GOETZEL_ADC_VAL_BITS-1)) / y;
}

//
// The following macros generate a goetzelN function which calculates the power
// of N specified frequencies in the given sample buffer.
//

#define INLINE_COUNT 8
#define INLINE(N, M) M(N) M(N) M(N) M(N) M(N) M(N) M(N) M(N)

#define PARAMS(n)    , int32_t coeff ## n
#define DESTS(n)     , int32_t *dest ## n
#define PREVS(n)     int32_t prev1_ ## n = 0, prev2_ ## n = 0, s ##n;
#define GET_S(n)     s ## n = samplesi + MUL(coeff ## n, prev1_ ## n) - prev2_ ## n;
#define SET_PREVS(n) prev2_ ## n = prev1_ ## n ; prev1_ ## n = s ## n;
#define GET_R(n)     int32_t r ## n = DIV(MUL(prev2_ ## n,prev2_ ## n) + MUL(prev1_ ## n,prev1_ ## n) - MUL(MUL(coeff ## n,prev1_ ## n),prev2_ ## n), total_power);
#define ABS_R(n)     if (r ## n < 0) r ## n = -r ## n;
#define SETDEST(n)   *dest ## n = r ## n;

#define LOOP_BODY(N)                          \
    samplesi = samples[i];                    \
    total_power += MUL(samplesi, samplesi)/4; \
    N(GET_S)                                  \
    N(SET_PREVS)                              \
    ++i;

#define MAKE_GOETZEL_N(n, N)                                                         \
    void goetzel ## n (const int16_t *samples, unsigned length N(PARAMS) N(DESTS) )  \
    {                                                                                \
        assert((length % INLINE_COUNT) == 0);                                        \
                                                                                     \
        int32_t total_power = 0;                                                     \
                                                                                     \
        N(PREVS)                                                                     \
                                                                                     \
        int i;                                                                       \
        int32_t samplesi;                                                            \
        for (i = 0; i < length;) {                                                   \
            INLINE(N, LOOP_BODY)                                                     \
        }                                                                            \
                                                                                     \
        N(GET_R)                                                                     \
        N(ABS_R)                                                                     \
        N(SETDEST)                                                                   \
    }

#define ONE(N) N(1)
#define TWO(N) N(1) N(2)

MAKE_GOETZEL_N(1, ONE)
MAKE_GOETZEL_N(2, TWO)

/*
int32_t goetzel(const int16_t *samples, unsigned length, int32_t coeff)
{
    int32_t total_power = 0;

    int32_t prev1 = 0, prev2 = 0;
    unsigned i;
    for (i = 0; i < length; ++i) {
        int32_t samplesi = samples[i];
        total_power += MUL(samplesi,samplesi)/4;
        int32_t s = samplesi + MUL(coeff, prev1) - prev2;
        prev2 = prev1;
        prev1 = s;
    }

    int32_t r = MUL(prev2,prev2) + MUL(prev1,prev1) - MUL(MUL(coeff,prev1),prev2);

    r = DIV(r, total_power);

    // No point normalizing for length because we'll always be comparing buffers
    // of the same length.
    //
    // r = DIV(r, length);

    if (r < 0)
        r = -r;
    return r;
}
*/

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

    const float LOWEST_DETECTABLE = SAMPLE_FREQ/N_SAMPLES;

    int16_t samples[N_SAMPLES];
    unsigned i;
    for (i = 0; i < N_SAMPLES; ++i) {
        float t = (float)i/(float)SAMPLE_FREQ;
        float v = 0.24*(sin(2.0*M_PI*SIG1_FREQ*t)) +
                  0.24*(cos(2.0*M_PI*SIG2_FREQ*t));
        samples[i] = (int16_t)(v*(4096/2.0));
    }

    printf("bin,bin_freq,value\n");

    unsigned j;
    for (j = 1; j <= N_BINS; ++j) {
        float BIN_FREQ = ((float)j/(float)N_BINS)*(SAMPLE_FREQ/2);
        if (BIN_FREQ < LOWEST_DETECTABLE)
            continue;

        float NFREQ = BIN_FREQ/SAMPLE_FREQ;
        int32_t COEFF = GOETZEL_FLOAT_TO_FIX(2*cos(2*M_PI*NFREQ*1));

        int32_t p;
        goetzel1(samples, N_SAMPLES, COEFF, &p);

        unsigned nstars = (unsigned)(round((float)p/(4096.0/2) * 0.5));
        printf("%i,%.2fHz: ", j, BIN_FREQ);
        unsigned k;
        for (k = 0; k < nstars; ++k)
            printf("*");
        printf("\n");
    }
}

int main()
{
    test1();
}

#endif
