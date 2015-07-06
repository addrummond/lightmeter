#ifndef GOETZEL_H
#define GOETZEL_H

#include <stdint.h>

#define GOETZEL_ADC_VAL_BITS 12
#define GOETZEL_FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<(GOETZEL_ADC_VAL_BITS-1))))
#define GOETZEL_FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<(GOETZEL_ADC_VAL_BITS-1))))

void goetzel1(const int16_t *samples, int length, int32_t coeff, int32_t *dest);
void goetzel2(const int16_t *samples, int length, int32_t coeff1, int32_t coeff2, int32_t *dest1, int32_t *dest2);

#endif
