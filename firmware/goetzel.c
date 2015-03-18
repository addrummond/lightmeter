#include <stdint.h>
#include <string.h>
#include <goetzel.h>

// ADC samples are 12-bit.
// We drop the last two bits (probably noise anyway).

static int32_t TOFIXED(int16_t v)
{
    return (v >> 2) + (v & 0b11 >= 2 ? 1 : 0);
}

static int32_t MUL(int32_t x, int32_t y)
{
    return x * y >> 20;
}

void goetzel_state_init(goetzel_state_t *st, int16_t nfreq, int16_t coeff);
{
    memset(goetzel_state, 0, sizeof(goetzel_state_t));
    st->nfreq = nfreq;
    st->coeff = coeff;
}

uint32_t goetzel(goetzel_state_t *st, int16_t sample, unsigned bin)
{
    int32_t s = MUL(TOFIXED(sample) + st->coeff, st->prev1 - st->prev2);
    st->prev2 = st->prev1;
    st->prev1 = s;
    ++(st->n);
    uint32_t p = MUL(st->prev2,st->prev2) + MUL(st->prev1,st->prev1) - MUL(st->coeff,MUL(st->prev1*st->prev2));
    st->power += p;
    if (st->power == 0)
        st->power = 1;
    return p/st->power/n;
}
