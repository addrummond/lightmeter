#ifndef HFSDP_H
#define HFSDP_H

#include <stdint.h>
#include <goetzel.h>

//
// We're sampling at 239.5 + 12.5 clock cycles. ADC is running on asynch 14MHz clock.
// Thus, our sample rate is 55555.6Hz.
//
// The carrier frequency for the clock signal is 20,000Hz.
// The carrier frequency for the data signal is 19,000Hz.
//
// Coefficients below calculated using calcoeffs.py
//

#define HFSDP_SIGNAL_FREQ        126
#define HFSDP_SAMPLE_FREQ        (HFSDP_SIGNAL_FREQ*4)
#define HFSDP_SAMPLE_CYCLES      (48000000/HFSDP_SAMPLE_FREQ)

#define HFSDP_MASTER_CLOCK_COSCOEFF_  -0.6393223606
#define HFSDP_MASTER_CLOCK_SINCOEFF_  0.7689388267
#define HFSDP_MASTER_DATA_COSCOEFF_   -0.5484583480
#define HFSDP_MASTER_DATA_SINCOEFF_   0.8361778761
#define HFSDP_MASTER_CLOCK_COSCOEFF   GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_CLOCK_COSCOEFF_)
#define HFSDP_MASTER_CLOCK_SINCOEFF   GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_CLOCK_SINCOEFF_)
#define HFSDP_MASTER_DATA_COSCOEFF    GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_DATA_COSCOEFF_)
#define HFSDP_MASTER_DATA_SINCOEFF    GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_DATA_SINCOEFF_)

typedef struct {
    int32_t calib_count;
    int32_t min_pclock, max_pclock;
    int32_t highest_low;
    int32_t prev_pclock;
    int32_t clock_coscoeff, data_coscoeff;
    int32_t clock_sincoeff, data_sincoeff;
} hfsdp_read_bit_state_t;

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s, int32_t clock_coscoeff, int32_t clock_sincoeff, int32_t data_coscoeff, int32_t data_sincoeff);
int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen,
                   int32_t *debug_clock_amp, int32_t *debug_data_amp, int32_t *debug_power);

#define HFSDP_READ_BIT_DECODE_ERROR  -2
#define HFSDP_READ_BIT_NOTHING_READ  -1

#endif
