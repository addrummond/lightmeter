#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>
#include <stdbool.h>
#include <goetzel.h>

#define PIEZO_MIC_BUFFER_N_SAMPLES       256

#define PIEZO_HFSDP_A_MODE_MASTER_CLOCK_HZ    5000.0
#define PIEZO_HFSDP_A_MODE_MASTER_CLOCK_COEFF GOETZEL_FLOAT_TO_FIX(1.314337413286371)
#define PIEZO_HFSDP_A_MODE_MASTER_DATA_HZ     6000.0
#define PIEZO_HFSDP_A_MODE_MASTER_DATA_COEFF  GOETZEL_FLOAT_TO_FIX(1.0390727525380696)

#define PIEZO_HFSDP_A_MODE_MASTER_F1_HZ  5000.0
#define PIEZO_HFSDP_A_MODE_MASTER_F2_HZ  5500.0
#define PIEZO_HFSDP_A_MODE_SLAVE_F1_HZ   7000.0
#define PIEZO_HFSDP_A_MODE_SLAVE_F2_HZ   11000.0
// Estimated by testing. Each element in the sequence outputted by the FFT
// appears to correspond to about 217.4Hz when ADC samples at 239.5 clock
// cycles (and processor is at 48MHz). From this we can deduce that our
// current sample rate is about 36798Hz. (We'll want to bump this up to
// detect ultrasound.)
#define PIEZO_HZ_TO_FTF(hz)              (((unsigned)((hz)/217.4))+1)
#define PIEZO_HFSDP_A_MODE_MASTER_F1_FTF PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_MASTER_F1_HZ)
#define PIEZO_HFSDP_A_MODE_MASTER_F2_FTF PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_MASTER_F2_HZ)
#define PIEZO_HFSDP_A_MODE_SLAVE_F1_FTF  PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_SLAVE_F1_HZ)
#define PIEZO_HFSDP_A_MODE_SLAVE_F2_FTF  PIEZO_HZ_TO_FTF(PIEZO_HFSDP_A_MODE_SLAVE_F2_HZ)

void piezo_mic_init(void);
void piezo_mic_deinit(void);

void piezo_out_init(void);
void piezo_set_period(unsigned channels, uint16_t period);
void piezo_turn_on(unsigned channels);
void piezo_pause(unsigned channels);
void piezo_unpause(unsigned channels);
void piezo_out_deinit(void);

bool piezo_hfsdp_listen_for_masters_init(void);

#endif
