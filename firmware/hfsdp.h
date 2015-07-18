#ifndef HFSDP_H
#define HFSDP_H

#include <stdint.h>
#include <stdbool.h>
#include <goetzel.h>

//
// We're sampling at 88888.89Hz.
// (This is our approximation to 88400Hz (= 2*44100Hz) using the 8MHz clock.)
//
// Clock carrier frequency: 20000Hz
// Data carrier frequency:  18000Hz
//
// Coefficients below calculated using calcoeffs.py
//

#define HFSDP_SIGNAL_FREQ             126
#define HFSDP_SAMPLE_MULTPLIER        8
#define HFSDP_SAMPLE_FREQ             (HFSDP_SIGNAL_FREQ*HFSDP_SAMPLE_MULTPLIER)
#define HFSDP_SAMPLE_CYCLES           (8000000/HFSDP_SAMPLE_FREQ)

#define HFSDP_MASTER_CLOCK_COSCOEFF_  0.1564330687
#define HFSDP_MASTER_CLOCK_SINCOEFF_  0.9876885617
#define HFSDP_MASTER_DATA_COSCOEFF_   0.2940391091
#define HFSDP_MASTER_DATA_SINCOEFF_   0.9557933889
#define HFSDP_MASTER_CLOCK_COSCOEFF   GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_CLOCK_COSCOEFF_)
#define HFSDP_MASTER_CLOCK_SINCOEFF   GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_CLOCK_SINCOEFF_)
#define HFSDP_MASTER_DATA_COSCOEFF    GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_DATA_COSCOEFF_)
#define HFSDP_MASTER_DATA_SINCOEFF    GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_DATA_SINCOEFF_)

#define HFSDP_HIGH_PASS_N 12

typedef struct {
    int32_t calib_count;
    int32_t min_pclock, max_pclock;
    int32_t min_pdata, max_pdata;
    int32_t avg;
    int32_t vs[8];
    int32_t highest_clock_low;
    int32_t highest_data_low;
    int32_t prev_pclock;
    int32_t clock_coscoeff, data_coscoeff;
    int32_t clock_sincoeff, data_sincoeff;
} hfsdp_read_bit_state_t;

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s, int32_t clock_coscoeff, int32_t clock_sincoeff, int32_t data_coscoeff, int32_t data_sincoeff);
bool hfsdp_check_start(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen);
int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen);

#define HFSDP_READ_BIT_DECODE_ERROR  -2
#define HFSDP_READ_BIT_NOTHING_READ  -1

extern int32_t hfsdp_read_bit_debug_last_pclock;
extern int32_t hfsdp_read_bit_debug_last_pdata;

#endif
