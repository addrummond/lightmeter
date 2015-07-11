#ifndef HFSDP_H
#define HFSDP_H

#include <stdint.h>
#include <stdbool.h>
#include <goetzel.h>

//
// We're sampling at 44164.038Hz.
//
// Clock carrier frequency: 20000Hz
// Data carrier frequency:  18000Hz
//
// Coefficients below calculated using calcoeffs.py
//

#define HFSDP_SIGNAL_FREQ             126
#define HFSDP_SAMPLE_FREQ             (HFSDP_SIGNAL_FREQ*8)
#define HFSDP_SAMPLE_CYCLES           (48000000/HFSDP_SAMPLE_FREQ)

#define HFSDP_MASTER_CLOCK_COSCOEFF_  0.1455192152//-0.9564504300//0.1460830286//-0.9573194986//-0.6374225955//-0.6443143297//-0.7196861131//-0.5895992008//-0.5471463462//-0.6374225955
#define HFSDP_MASTER_CLOCK_SINCOEFF_  0.9893554255//0.2918948011//0.9892723330//0.2890317933//0.7705143962//0.7647607760//0.6942995742//0.8076959716//0.8370369620//0.7705143962
#define HFSDP_MASTER_DATA_COSCOEFF_   0.2845275866//-0.8360536734//0.2850192625//-0.8375280419//-0.4483817604//-0.5463929070//-0.5535161478// -0.4973092855//-0.4541908911//-0.5463929070
#define HFSDP_MASTER_DATA_SINCOEFF_   0.9586678530//0.5486476603// 0.9585217890//0.5463943439//0.89384215440//0.8375289793//0.8328384442//0.8675733251//0.8909043913//0.8375289793
#define HFSDP_MASTER_CLOCK_COSCOEFF   GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_CLOCK_COSCOEFF_)
#define HFSDP_MASTER_CLOCK_SINCOEFF   GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_CLOCK_SINCOEFF_)
#define HFSDP_MASTER_DATA_COSCOEFF    GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_DATA_COSCOEFF_)
#define HFSDP_MASTER_DATA_SINCOEFF    GOETZEL_FLOAT_TO_FIX(HFSDP_MASTER_DATA_SINCOEFF_)

typedef struct {
    int32_t calib_count;
    int32_t min_pclock, max_pclock;
    int32_t avg;
    int32_t vs[8];
    int32_t highest_low;
    int32_t prev_pclock;
    int32_t clock_coscoeff, data_coscoeff;
    int32_t clock_sincoeff, data_sincoeff;
} hfsdp_read_bit_state_t;

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s, int32_t clock_coscoeff, int32_t clock_sincoeff, int32_t data_coscoeff, int32_t data_sincoeff);
bool hfsdp_check_start(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen);
int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen,
                   int32_t *debug_clock_amp, int32_t *debug_data_amp, int32_t *debug_power);

#define HFSDP_READ_BIT_DECODE_ERROR  -2
#define HFSDP_READ_BIT_NOTHING_READ  -1

#endif
