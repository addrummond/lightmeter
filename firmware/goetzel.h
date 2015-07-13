#ifndef GOETZEL_H
#define GOETZEL_H

#include <stdint.h>

#define GOETZEL_ADC_VAL_BITS 12
#define GOETZEL_FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<(GOETZEL_ADC_VAL_BITS-1))))
#define GOETZEL_FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<(GOETZEL_ADC_VAL_BITS-1))))

typedef struct {
    int32_t r;
    int32_t i;
    int32_t sin_coeff;
    int32_t cos_coeff;
    int32_t total_power;
} goetzel_result_t;

int32_t goetzel_get_freq_power(const goetzel_result_t *gr);

void goetzel1(const int16_t *samples, unsigned length, unsigned offset, int32_t coscoeff, int32_t sincoeff, goetzel_result_t *dest);
void goetzel2(const int16_t *samples, unsigned length, unsigned offset, int32_t coscoeff1, int32_t sincoeff1, int32_t coscoeff2, int32_t sincoeff2, goetzel_result_t *dest1, goetzel_result_t *dest2);

void high_pass(int32_t *buf, unsigned buflen, int n);

#endif
