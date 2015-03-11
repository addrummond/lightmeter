#ifndef FIX_FFT_H
#define FIX_FFT_H

#include <stdint.h>

int fix_fft(int8_t fr[], int8_t fi[], int m, int inverse);
int fix_fftr(int8_t f[], int m, int inverse);

#endif
