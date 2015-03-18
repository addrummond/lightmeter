#ifndef GOETZEL_H
#define GOETZEL_H

#include <stdint.h>

#define GOETZEL_ADC_VAL_BITS 12
#define GOETZEL_FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<(GOETZEL_ADC_VAL_BITS-1))))
#define GOETZEL_FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<(GOETZEL_ADC_VAL_BITS-1))))

typedef struct goetzel_state {
    int32_t prev1;
    int32_t prev2;
    int32_t last_power;
    int32_t total_power;
    unsigned n;
    int32_t coeff;
} goetzel_state_t;

void goetzel_state_init(goetzel_state_t *st, int32_t coeff);
void goetzel_step(goetzel_state_t *st, int32_t sample);
int32_t goetzel_get_normalized_power(goetzel_state_t *st);

#endif
