#include <stdint.h>
#include <string.h>
#include <goetzel.h>
#include <myassert.h>
#include <debugging.h>

#ifdef TEST
#include <math.h>
#include <stdio.h>
#include <assert.h>
#endif

static inline int32_t MUL(int32_t x, int32_t y)
{
    return (x * y) >> (GOETZEL_ADC_VAL_BITS-1);
}

static inline int64_t MUL64(int64_t x, int64_t y)
{
    return (x * y) >> (GOETZEL_ADC_VAL_BITS-1);
}

//
// The following macros generate a goetzelN function which calculates the power
// of N specified frequencies in the given sample buffer.
//

#define INLINE_COUNT 8
#define INLINE(N, M) M(N) M(N) M(N) M(N) M(N) M(N) M(N) M(N)

#define PARAMS(n)    , int32_t coscoeff ## n , int32_t sincoeff ## n

#define DESTS(n)     , goetzel_result_t *dest ## n

#define PREVS(n)     int32_t prev1_ ## n = 0, prev2_ ## n = 0, s ## n;

#define COS2(n)      int32_t coscoeff ## n ## _mul2 = 2 * coscoeff ## n;

#define GET_S(n)     s ## n = samplesi + MUL(coscoeff ## n ## _mul2, prev1_ ## n) - prev2_ ## n;

#define SET_PREVS(n) prev2_ ## n = prev1_ ## n ; prev1_ ## n = s ## n;

#define GET_R(n)     int32_t rr ## n = MUL(prev1_ ## n, coscoeff ## n) - prev2_ ## n; \
                     int32_t ri ## n = MUL(prev1_ ## n, sincoeff ## n);

#define SETDEST(n)   dest ## n ->r = rr ## n; \
                     dest ## n ->i = ri ## n;

#define SETCOEFFSANDP(n) dest ## n ->cos_coeff   = coscoeff ## n; \
                         dest ## n ->sin_coeff   = sincoeff ## n; \
                         dest ## n ->total_power = total_power; \
                         dest ## n ->length = length;

#define LOOP_BODY(N)                            \
    i = i_ % length;                            \
    samplesi = samples[i];                      \
    total_power += MUL(samplesi, samplesi);     \
    N(GET_S)                                    \
    N(SET_PREVS)                                \
    ++i_;

#define MAKE_GOETZEL_N(n, N)                                                                       \
    void goetzel ## n (const int16_t *samples, unsigned length, unsigned offset N(PARAMS) N(DESTS))\
    {                                                                                              \
        int32_t total_power = 0;                                                                   \
                                                                                                   \
        N(PREVS)                                                                                   \
        N(COS2)                                                                                    \
                                                                                                   \
        int i_, i;                                                                                 \
        int32_t samplesi;                                                                          \
        for (i_ = offset; i_ < offset + length;) {                                                 \
            INLINE(N, LOOP_BODY)                                                                   \
        }                                                                                          \
        for (; i_ < offset + length;) {                                                            \
            LOOP_BODY(N);                                                                          \
        }                                                                                          \
        N(GET_R)                                                                                   \
        N(SETDEST)                                                                                 \
        N(SETCOEFFSANDP)                                                                           \
    }

#define ONE(N) N(1)
#define TWO(N) N(1) N(2)
#define FOUR(N) N(1) N(2) N(3) N(4)

MAKE_GOETZEL_N(1, ONE)
MAKE_GOETZEL_N(2, TWO)

int32_t goetzel_get_freq_power(const goetzel_result_t *gr)
{
    int64_t r = gr->r;
    int64_t i = gr->i;
    int64_t r2 = MUL64((int64_t)r, (int64_t)r);
    int64_t i2 = MUL64((int64_t)i, (int64_t)i);
    int64_t pow64 = (r2 + i2)/gr->length;

    return (int32_t)(pow64);
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
    unsigned N_SAMPLES = 80;
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
        int32_t COSCOEFF = GOETZEL_FLOAT_TO_FIX(cos(2*M_PI*NFREQ*1));
        int32_t SINCOEFF = GOETZEL_FLOAT_TO_FIX(sin(2*M_PI*NFREQ*1));

        int32_t total_power;
        goetzel_result_t gr1, gr2;
        goetzel2(samples, N_SAMPLES, COSCOEFF, SINCOEFF, 100, 100, &gr1, &gr2, &total_power);
        int32_t fp = goetzel_get_freq_power(&gr1, total_power);

        unsigned nstars = (unsigned)(round(((float)fp/(4096.0/2))*0.2));

        printf("%i,%.2f,%iHz: ", j, BIN_FREQ, fp);
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
