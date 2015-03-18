#include <stdint.h>
#include <string.h>
#include <goetzel.h>

#ifdef TEST
#include <math.h>
#include <stdio.h>
#endif

// ADC samples are 12-bit.

static int32_t TOFIXED(int16_t v)
{
    return v;
}

static int32_t MUL(int32_t x, int32_t y)
{
    return (x * y) >> 12;
}

static int32_t DIV(int32_t x, int32_t y)
{
    if (x > (1 << 29))
        return (x/y) << 12;
    else
        return (x << 12)/y;
}

void goetzel_state_init(goetzel_state_t *st, int32_t coeff)
{
    memset(st, 0, sizeof(*st));
    st->coeff = coeff;
}

uint32_t goetzel(goetzel_state_t *st, int16_t sample)
{
    int32_t s = TOFIXED(sample) + MUL(st->coeff, st->prev1) - st->prev2;
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
    const float COEFF_ = 2*cos(2*M_PI*NFREQ);
    const int32_t COEFF = (int32_t)(COEFF_*4096.0);

    goetzel_state_t gs;
    goetzel_state_init(&gs, COEFF);

    unsigned i;
    for (i = 0; i < 512; ++i) {
        float t = i/SAMPLE_FREQ;
        float sample_value = 0.45*(sin(2.0*M_PI*SIG_FREQ*t) + 1.0);
        int16_t sample12 = sample_value*4096.0;
        uint32_t p = goetzel(&gs, sample12);
        //printf("Power at t=%f, sample=%f: %f\n", t, sample_value, sqrt((float)(p/4096.0)));
        printf("%f,%f,%f\n", t, sample_value, sqrt((float)(p/4096.0)));
    }
}
#endif
