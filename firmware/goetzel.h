#ifndef GOETZEL_H
#define GOETZEL_H

#include <stdint.h>

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
