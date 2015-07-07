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

/*bool piezo_read_data(uint8_t *buffer, unsigned nbits)
{
    int prev_clock_direction = 0; // -1 -> unknown, 0 -> rising, 1 -> falling.
    int32_t prev_pclock = -1, prev_pdata = -1;
    unsigned nreceived = 0;
    for (;;) {
        uint32_t tn = SysTick->VAL;
        uint32_t te;
        if (tn >= SAMPLE_CYCLES)
            te = tn - SAMPLE_CYCLES;
        else
            te = SYS_TICK_MAX - (SAMPLE_CYCLES - tn);

        piezo_mic_read_buffer();
        int32_t pclock, pdata;
        goetzel2((const int16_t *)piezo_mic_buffer, PIEZO_MIC_BUFFER_N_SAMPLES,
                 PIEZO_HFSDP_A_MODE_MASTER_CLOCK_COEFF,
                 PIEZO_HFSDP_A_MODE_MASTER_DATA_COEFF,
                 &pclock, &pdata);

        unsigned clock_direction;
        if (prev_pclock != -1) {
            int32_t pclockdiff = pclock - prev_pclock;
            if (pclockdiff >= CLOCK_THRESHOLD)
                clock_direction = 0;
            else if (pclockdiff <= -CLOCK_THRESHOLD)
                clock_direction = 1;
            else
                clock_direction = -1;
        }
        else {
            clock_direction = -1;
        }

        // debugging_writec("C: ");
        // debugging_write_int32(clock_direction);
        // debugging_writec("\n");

        // Has the clock changed direction, indicating that we should read a
        // data bit?
        if (clock_direction != -1 && clock_direction != prev_clock_direction) {
            prev_clock_direction = clock_direction;

            if (prev_pdata == -1) {
                prev_pdata = pdata;
            }
            else {
                // At this point, we'd better have a significant difference between
                // prev_pdata and pdata. If not, we return false to indicate that
                // the signal could not be decoded.
                int32_t pdatadiff = pdata - prev_pdata;
                if (pdatadiff > -DATA_THRESHOLD && pdatadiff < DATA_THRESHOLD)
                    return false;

                buffer[nreceived++] = (prev_pdata <= pdata);

                prev_pdata = pdata;

                if (nreceived == nbits)
                    return true;
            }
        }

        prev_pclock = pclock;

        // Wait until it's time to take the next sample.
        if (te < tn)
            while (SysTick->VAL > te);
        else
            while (SysTick->VAL <= te);

        // int32_t elapsed = tn - SysTick->VAL;
        // debugging_writec("EE: ");
        // debugging_write_uint32(elapsed);
        // debugging_writec("\n");
    }
}*/
