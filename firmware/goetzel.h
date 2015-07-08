#ifndef GOETZEL_H
#define GOETZEL_H

#include <stdint.h>

#define GOETZEL_ADC_VAL_BITS 12
#define GOETZEL_FLOAT_TO_FIX(x) ((int32_t)((x)*(float)(1<<(GOETZEL_ADC_VAL_BITS-1))))
#define GOETZEL_FIX_TO_FLOAT(x) (((float)(x))/((float)(1<<(GOETZEL_ADC_VAL_BITS-1))))

void goetzel1(const int16_t *samples, unsigned length, int32_t coscoeff, int32_t sincoeff, int32_t *dest, int32_t *power);
void goetzel2(const int16_t *samples, unsigned length, int32_t coscoeff1, int32_t sincoeff1, int32_t coscoeff2, int32_t sincoeff2, int32_t *dest1, int32_t *dest2, int32_t *power);

#endif
