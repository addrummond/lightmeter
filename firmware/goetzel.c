#include <stdint.h>
#include <string.h>
#include <goetzel.h>

#ifdef TEST
#include <math.h>
#include <stdio.h>
#endif

// ADC samples are 12-bit.
//
// This module assumes that the raw ADC value has been converted to a SIGNED
// 12-bit value.
//
// We add two additionl bits.
//

#define ADC_VAL_BITS 12
#define EXTRA_BITS 0

static int32_t ADC_TO_FIX(int32_t x)
{
    return x << EXTRA_BITS;
}

#define FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<15)))
#define FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<15)))

static int32_t MUL(int32_t x, int32_t y)
{
    return (x * y) >> (ADC_VAL_BITS-1+EXTRA_BITS);
}

static int32_t DIV(int32_t x, int32_t y)
{
   return (x << (ADC_VAL_BITS-1+EXTRA_BITS)) / y;
}

void goetzel_state_init(goetzel_state_t *st, int32_t coeff)
{
    memset(st, 0, sizeof(*st));
    st->coeff = coeff;
}

// Samples should be a signed 12-bit value.
uint32_t goetzel(goetzel_state_t *st, int32_t sample)
{
    int32_t s = ADC_TO_FIX(sample) + MUL(st->coeff, st->prev1) - st->prev2;
    st->prev2 = st->prev1;
    st->prev1 = s;
    ++(st->n);
    uint32_t t1 = MUL(st->prev2,st->prev2),
             t2 = MUL(st->prev1,st->prev1),
             t3 = MUL(st->coeff,MUL(st->prev1,st->prev2));
    uint32_t p = t1 + t2 - t3;
    st->power += MUL(p,p)   ;
    if (st->power == 0)
        st->power = 1;
    return DIV(p,DIV(st->power,st->n));
}

#ifdef TEST
int main()
{
    const float SAMPLE_FREQ = 40000;
    const float SIG_FREQ = 5000;
    const float NFREQ = SIG_FREQ/SAMPLE_FREQ;
    const int32_t COEFF = FLOAT_TO_FIX(2*cos(2*M_PI*NFREQ*2));

    goetzel_state_t gs;
    goetzel_state_init(&gs, COEFF);

    unsigned i;
    for (i = 0; i < 512; ++i) {
        float t = i/SAMPLE_FREQ;
        float sample_value = 0.45*(sin(2.0*M_PI*SIG_FREQ*t));
        int32_t sample12 = sample_value*(4096/2.0);
        uint32_t p = goetzel(&gs, sample12);
        //printf("Power at t=%f, sample=%f: %f\n", t, sample_value, sqrt((float)(p/(4096.0/2))));
        printf("%f,%f,%f\n", t, sample_value, FIX_TO_FLOAT(p));
    }
}
#endif
