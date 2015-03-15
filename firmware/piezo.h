#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>
#include <stdbool.h>

// (1 << PIEZO_SHIFT_1_BY_THIS_TO_GET_N_SAMPLES) == PIEZO_MIC_BUFFER_SIZE_BYTES/2
#define PIEZO_MIC_BUFFER_SIZE_BYTES            512
#define PIEZO_SHIFT_1_BY_THIS_TO_GET_N_SAMPLES 8

#define PIEZO_HFSDP_A_MODE_MASTER_F1_HZ  5000.0
#define PIEZO_HFSDP_A_MODE_MASTER_F2_HZ  9000.0
#define PIEZO_HFSDP_A_MODE_SLAVE_F1_HZ   7000.0
#define PIEZO_HFSDP_A_MODE_SLAVE_F2_HZ   11000.0
// Estimated by testing. Each element in the sequence outputted by the FFT
// appears to correspond to about 217.4Hz when ADC samples at 239.5 clock
// cycles (and processor is at 8MHz).
#define PIEZO_HZ_TO_FTF(hz)              (((unsigned)((hz)/217.4))+1)
#define PIEZO_HFSDP_A_MODE_MASTER_F1_FTF PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_MASTER_F1_HZ)
#define PIEZO_HFSDP_A_MODE_MASTER_F2_FTF PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_MASTER_F2_HZ)
#define PIEZO_HFSDP_A_MODE_SLAVE_F1_FTF  PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_MASTER_F3_HZ)
#define PIEZO_HFSDP_A_MODE_SLAVE_F2_FTF  PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_MASTER_F4_HZ)

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

bool piezo_hfsdp_listen_for_masters_init(void);

#endif
