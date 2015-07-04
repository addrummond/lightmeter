#ifndef GOETZEL_H
#define GOETZEL_H

#include <stdint.h>

#define GOETZEL_ADC_VAL_BITS 12
#define GOETZEL_FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<(GOETZEL_ADC_VAL_BITS-1))))
#define GOETZEL_FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<(GOETZEL_ADC_VAL_BITS-1))))

int32_t goetzel(const int16_t *samples, unsigned length, int32_t coeff) __attribute__((optimize("-O3")));

#endif
