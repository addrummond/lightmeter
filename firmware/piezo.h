#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>

#define PIEZO_MIC_BUFFER_SIZE_BYTES            512
#define PIEZO_SHIFT_1_BY_THIS_TO_GET_N_SAMPLES 8

void piezo_mic_init(void);
void piezo_mic_read_buffer(int8_t *samples);
uint32_t piezo_mic_buffer_get_sqmag(int8_t *samples);
void piezo_mic_buffer_fft(int8_t *samples);
void piezo_fft_buffer_get_12formants(int8_t *samples, unsigned *first_formant, unsigned *second_formant);

void piezo_out_init(void);
void piezo_set_period(unsigned channels, uint16_t period);
void piezo_turn_on(unsigned channels);
void piezo_pause(unsigned channels);
void piezo_unpause(unsigned channels);
void piezo_out_deinit(void);

#endif
