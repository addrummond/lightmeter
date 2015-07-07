#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>
#include <stdbool.h>
#include <goetzel.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_dma.h>
#include <stm32f0xx_adc.h>

#define USE_DMA
//#undef USE_DMA

#define PIEZO_MIC_BUFFER_N_SAMPLES       64

//
// We're sampling at 239.5 + 12.5 clock cycles. ADC is running on asynch 14MHz clock.
// Thus, our sample rate is 55555.6Hz.
//
// The coefficient for the Goetzel algorithm is calculated using the following
// formula (JS code):
//
//     coeff = 2*Math.cos(2*Math.PI*(f/55555.6))
//

void piezo_mic_init(void);
void piezo_mic_deinit(void);

void piezo_mic_read_buffer();
int32_t piezo_get_magnitude();

void piezo_out_init(void);
void piezo_set_period(unsigned channels, uint16_t period);
void piezo_turn_on(unsigned channels);
void piezo_pause(unsigned channels);
void piezo_unpause(unsigned channels);
void piezo_out_deinit(void);

bool piezo_read_data(uint8_t *buffer, unsigned nbits);
bool piezo_hfsdp_listen_for_masters_init(void);

#endif
