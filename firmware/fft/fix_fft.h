#ifndef FIX_FFT_H
#define FIX_FFT_H

#include <stdint.h>

int fix_fft(int16_t fr[], int16_t fi[], int16_t m, int16_t inverse);
int fix_fftr(int16_t f[], int m, int inverse);

#endif