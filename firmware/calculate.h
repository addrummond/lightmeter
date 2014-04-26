#ifndef CALCULATE_H
#define CALCULATE_H

#include <state.h>

uint8_t count_bits_in_word(uint16_t word);
uint8_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage, uint8_t op_amp_resistor_stage);
uint8_t convert_from_reference_voltage(uint16_t adc_out);

#endif
