#include <hfsdp.h>
#include <goetzel.h>

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s)
{
    s->prev_clock_direction = 0; // -1 -> unknown, 0 -> rising, 1 -> falling.
    s->prev_pclock = -1;
    s->prev_pdata = -1;
}

int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen)
{
    int32_t pclock, pdata;
    goetzel2(buf, buflen,
             HFSDP_MASTER_CLOCK_COEFF,
             HFSDP_MASTER_DATA_COEFF,
             &pclock, &pdata);

    unsigned clock_direction;
    if (s->prev_pclock != -1) {
        int32_t pclockdiff = pclock - s->prev_pclock;
        if (pclockdiff >= HFSDP_CLOCK_THRESHOLD)
            clock_direction = 0;
        else if (pclockdiff <= -HFSDP_CLOCK_THRESHOLD)
            clock_direction = 1;
        else
            clock_direction = -1;
    }
    else {
        clock_direction = -1;
    }

    // Has the clock changed direction, indicating that we should read a
    // data bit?
    if (clock_direction != -1 && clock_direction != s->prev_clock_direction) {
        s->prev_clock_direction = clock_direction;

        if (s->prev_pdata == -1) {
            s->prev_pdata = pdata;
        }
        else {
            // At this point, we'd better have a significant difference between
            // prev_pdata and pdata. If not, we return an error code to indicate that
            // the signal could not be decoded.
            int32_t pdatadiff = pdata - s->prev_pdata;
            if (pdatadiff > -HFSDP_DATA_THRESHOLD && pdatadiff < HFSDP_DATA_THRESHOLD)
                return HFSDP_READ_BIT_DECODE_ERROR;

            int bit = (s->prev_pdata <= pdata);
            s->prev_pdata = pdata;

            return bit;
        }
    }

    s->prev_pclock = pclock;

    return HFSDP_READ_BIT_NOTHING_READ;
}
