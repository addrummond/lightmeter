#ifndef HFSDP_H
#define HFSDP_H

#include <stdint.h>
#include <goetzel.h>

#define HFSDP_SIGNAL_FREQ      126
#define HFSDP_SAMPLE_FREQ      (HFSDP_SIGNAL_FREQ*8)
#define HFSDP_SAMPLE_CYCLES    (48000000/HFSDP_SAMPLE_FREQ)
#define HFSDP_CLOCK_THRESHOLD  200
#define HFSDP_DATA_THRESHOLD   100

#define HFSDP_MASTER_CLOCK_HZ    20000.0
#define HFSDP_MASTER_CLOCK_COEFF GOETZEL_FLOAT_TO_FIX(-1.2748451909216907)
#define HFSDP_MASTER_DATA_HZ     19000.0
#define HFSDP_MASTER_DATA_COEFF  GOETZEL_FLOAT_TO_FIX(-1.0927858139146585)

typedef struct {
    int prev_clock_direction;
    int32_t prev_pclock, prev_pdata;
} hfsdp_read_bit_state_t;

void init_hfsdp_read_bit_state(hfsdp_read_bit_state_t *s);
int hfsdp_read_bit(hfsdp_read_bit_state_t *s, const int16_t *buf, unsigned buflen);

#define HFSDP_READ_BIT_DECODE_ERROR  -2
#define HFSDP_READ_BIT_NOTHING_READ  -1

#endif
