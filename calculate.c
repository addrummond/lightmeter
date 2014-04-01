#include <stdint.h>
#include <assert.h>
#include <constants.h>
#ifdef TEST
#    include <stdio.h>
#endif

extern const uint8_t TEMP_AND_VOLTAGE_TO_EV[];
extern const uint8_t TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[];
#ifdef TEST
extern const uint8_t TEST_TEMP_AND_VOLTAGE_TO_EV[];
#endif

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
// This is explicitly implemented without using division or multiplcation,
// since the attiny doesn't have hardware division or multiplication.
// gcc would probably do most of these optimizations automatically, but since
// this code really definitely needs to run quickly, I'm doing it explicitly here.
uint8_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage)
{
    uint16_t row_start = ((uint16_t)temperature & ~15) << 1; // (temperature / 16) * 32 [32 bytes per row]
    uint16_t absval_i = row_start + ((voltage >> 3) & ~1); // ((v/2)/16)*2
    uint16_t bits_to_add = (voltage & 15) + 1; // (voltage % 16) + 1

    uint8_t bit_pattern_indices = TEMP_AND_VOLTAGE_TO_EV[absval_i + 1];
    uint16_t bits = TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[bit_pattern_indices >> 4] << 8;
    bits |= TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[bit_pattern_indices & 0x0F];
    bits &= (uint16_t)0xFFFF << (16 - bits_to_add);

    uint8_t c = count_bits_in_word(bits);
    printf("voltage = %i, count = %i, bitsper = %i, abs %i, absi = %i, r = %i\n", voltage, c, bits_to_add, TEMP_AND_VOLTAGE_TO_EV[absval_i], absval_i, TEMP_AND_VOLTAGE_TO_EV[absval_i] + c);

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
    // Test that compressed table is giving correct values by comparing to uncompressed table.
    for (int t = 0; t < 256; ++t) {
        printf("LOOP %i\n", t);
        for (int v = 0; v < 246; ++v) {
            uint8_t uncompressed = TEST_TEMP_AND_VOLTAGE_TO_EV[(256 * ((unsigned)t/16)) + (unsigned)v];
            uint8_t compressed = get_ev100_at_temperature_voltage((uint8_t)t, (uint8_t)v);
            if (uncompressed != compressed) {
                printf("Values not equal for t = %i, v = %i: compressed = %i, uncompressed = %i\n", (unsigned)t, (unsigned)v, (unsigned)compressed, (unsigned)uncompressed);
                return 1;
            }
        }
    }

    printf("\nTest completed successfully.\n");

    return 0;
}

#endif
