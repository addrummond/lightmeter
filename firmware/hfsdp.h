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

#define HFSDP_COSCOEFF1_ 0.2674099894
#define HFSDP_SINCOEFF1_ 0.9635828442
#define HFSDP_COSCOEFF2_ 0.2329258084
#define HFSDP_SINCOEFF2_ 0.9724945078
#define HFSDP_COSCOEFF3_ 0.1981461432
#define HFSDP_SINCOEFF3_ 0.9801724878
#define HFSDP_COSCOEFF4_ 0.1631151144
#define HFSDP_SINCOEFF4_ 0.9866070441

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
