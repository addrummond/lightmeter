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
#define HFSDP_SAMPLE_MULTPLIER        4
#define HFSDP_SAMPLE_FREQ             (HFSDP_SIGNAL_FREQ*HFSDP_SAMPLE_MULTPLIER)
#define HFSDP_SAMPLE_CYCLES           (8000000/HFSDP_SAMPLE_FREQ)

#define HFSDP_COSCOEFF1_ 0.2155701619
#define HFSDP_SINCOEFF1_ 0.9764883539
#define HFSDP_COSCOEFF2_ 0.1455192152
#define HFSDP_SINCOEFF2_ 0.9893554255

#define HFSDP_COSCOEFF1 GOETZEL_FLOAT_TO_FIX(HFSDP_COSCOEFF1_)
#define HFSDP_SINCOEFF1 GOETZEL_FLOAT_TO_FIX(HFSDP_SINCOEFF1_)
#define HFSDP_COSCOEFF2 GOETZEL_FLOAT_TO_FIX(HFSDP_COSCOEFF2_)
#define HFSDP_SINCOEFF2 GOETZEL_FLOAT_TO_FIX(HFSDP_SINCOEFF2_)
#define HFSDP_COSCOEFF3 GOETZEL_FLOAT_TO_FIX(HFSDP_COSCOEFF3_)
#define HFSDP_SINCOEFF3 GOETZEL_FLOAT_TO_FIX(HFSDP_SINCOEFF3_)
#define HFSDP_COSCOEFF4 GOETZEL_FLOAT_TO_FIX(HFSDP_COSCOEFF4_)
#define HFSDP_SINCOEFF4 GOETZEL_FLOAT_TO_FIX(HFSDP_SINCOEFF4_)

typedef struct {
    int32_t calib_count;
    int32_t min_f1, max_f1;
    int32_t min_f2, max_f2;
    int32_t avg;
    int32_t vs[8];
    int32_t highest_f1_low;
    int32_t highest_f2_low;
    int32_t f1_coscoeff, f1_sincoeff;
    int32_t f2_coscoeff, f2_sincoeff;
    int f1_less_than_f2; // -1 when first initialized, -2 when we've syncd, otherwise 0 or 1
    unsigned count;
} hfsdp_read_bit_state_t;

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s, int32_t f1_coscoeff, int32_t f1_sincoeff, int32_t f2_coscoeff, int32_t f2_sincoeff);
bool hfsdp_check_start(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen);
int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen);

#define HFSDP_READ_BIT_DECODE_ERROR  -2
#define HFSDP_READ_BIT_NOTHING_READ  -1

extern int32_t hfsdp_read_bit_debug_last_f1;
extern int32_t hfsdp_read_bit_debug_last_f2;

#endif
