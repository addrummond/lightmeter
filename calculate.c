#include <stdint.h>
#include <assert.h>
#include <constants.h>

extern const uint8_t *TEMP_AND_VOLTAGE_TO_EV;

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
    uint16_t ti = temperature / 16;
    uint16_t vi = voltage / 2;

    uint16_t row_start = ti * 128;
    uint16_t absval_i = vi / 8;
    uint16_t bits_to_add = vi % 8;

    uint16_t bits = TEMP_AND_VOLTAGE_TO_EV[row_start + absval_i + 1];
    bits <<= 8;
    bits |= TEMP_AND_VOLTAGE_TO_EV[row_start + absval_i + 2];
    static const uint16_t all_set = 0xFFFF;
    bits &= all_set << (16 - bits_to_add);

    uint8_t c = count_bits_in_word(bits);

    return TEMP_AND_VOLTAGE_TO_EV[row_start + absval_i] + c;
}

// refv_v is in 10ths of a volt.
uint8_t convert_from_reference_voltage(uint16_t adc_out)
{
#if REFERENCE_VOLTAGE_TENTHS == 50
    return adc_out / 4;
#elif REFERENCE_VOLTAGE_TENTHS == 33
    return (adc_out / 2) + (adc_out / 10);
#elif REFERENCE_VOLTAGE_TENTHS == 25
    return adc_out / 8;
#elif REFERENCE_VOLTAGE_TENTHS == 16 // Actually 1.65
    return adc_out / 16;
#else
#error "Can't handle that reference voltage"
    return 0;
#endif
}
