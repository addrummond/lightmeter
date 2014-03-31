#include <stdint.h>
#include <assert.h>
#include <constants.h>
#ifdef TEST
#    include <stdio.h>
#endif

extern const uint8_t TEMP_AND_VOLTAGE_TO_EV[];

// See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
uint8_t count_bits_in_word(uint16_t word)
{
    uint8_t c = 0;
    for (; word; ++c)
        word &= word - 1;
    return c;
}

// See comments in calculate_tables.py for info on the way
// temp/voltage are encoded.
uint8_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage)
{
    uint16_t row_start = (uint16_t)temperature;
    row_start += row_start/2; // temperature / 16 * 24 [24 bytes per row]
    uint16_t absval_i = row_start + (uint16_t)(voltage / 16); // / 2 / 8
    uint16_t bits_to_add = voltage % 16;

    uint16_t bits = TEMP_AND_VOLTAGE_TO_EV[absval_i + 1];
    bits <<= 8;
    bits |= TEMP_AND_VOLTAGE_TO_EV[absval_i + 2];
    bits &= (uint16_t)0xFF << (16 - bits_to_add);

    uint8_t c = count_bits_in_word(bits);

    return TEMP_AND_VOLTAGE_TO_EV[absval_i] + c;
}

uint8_t convert_from_reference_voltage(uint16_t adc_out)
{
#if REFERENCE_VOLTAGE_MV == 5000
    return adc_out / 4;
#elif REFERENCE_VOLTAGE_MV == 3300
    return (adc_out / 2) + (adc_out / 10);
#elif REFERENCE_VOLTAGE_MV == 2500
    return adc_out / 8;
#elif REFERENCE_VOLTAGE_MV == 1650
    return adc_out / 16;
#else
#error "Can't handle that reference voltage"
    return 0;
#endif
}

#ifdef TEST

int main()
{
    for (uint8_t temp = 0; temp <= 255; ++temp) {
        for (uint8_t voltage = 0; voltage <= 255; ++voltage) {
            printf("temp = %i, voltage = %i, ev*8 = %i\n", temp, voltage, get_ev100_at_temperature_voltage(temp, voltage));
        }
    }

    return 0;
}

#endif
