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

#define PIEZO_MIC_BUFFER_N_SAMPLES       256

//
// We're sampling at 239.5 clock cycles. ADC is running on asynch 14MHz clock.
// Thus, our sample rate is 58455Hz.
//
// The coefficient for the Goetzel algorithm is calculated using the following
// formula (JS code):
//
//     coeff = 2*Math.cos(2*Math.PI*(f/58455))
//

#define PIEZO_HFSDP_A_MODE_MASTER_CLOCK_HZ    5000.0
#define PIEZO_HFSDP_A_MODE_MASTER_CLOCK_COEFF GOETZEL_FLOAT_TO_FIX(1.7180463686839544)
#define PIEZO_HFSDP_A_MODE_MASTER_DATA_HZ     6000.0
#define PIEZO_HFSDP_A_MODE_MASTER_DATA_COEFF  GOETZEL_FLOAT_TO_FIX(1.5982892778942597)

#define PIEZO_HFSDP_A_MODE_MASTER_F1_HZ  5000.0
#define PIEZO_HFSDP_A_MODE_MASTER_F2_HZ  5500.0
#define PIEZO_HFSDP_A_MODE_SLAVE_F1_HZ   7000.0
#define PIEZO_HFSDP_A_MODE_SLAVE_F2_HZ   11000.0

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

bool piezo_hfsdp_listen_for_masters_init(void);

#endif
