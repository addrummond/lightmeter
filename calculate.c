#include <readbyte.h>
#include <stdint.h>
#include <assert.h>
#include <divmulutils.h>
#ifdef TEST
#    include <stdio.h>
#endif

extern const uint8_t TEMP_AND_VOLTAGE_TO_EV_ABS[];
extern const uint8_t TEMP_AND_VOLTAGE_TO_EV_DIFFS[];
extern const uint8_t TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[];
#ifdef TEST
extern const uint8_t TEST_TEMP_AND_VOLTAGE_TO_EV[];
#endif

// See comments in calculate_tables.py for info on the way
// temp/voltage are encoded.
// This is explicitly implemented without using division or multiplcation,
// since the attiny doesn't have hardware division or multiplication.
// gcc would probably do most of these optimizations automatically, but since
// this code really definitely needs to run quickly, I'm doing it explicitly here.
//
// 'voltage' is in 1/256ths of the reference voltage.
uint8_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage)
{
    uint8_t row_start = ((uint16_t)temperature & ~15); // (temperature / 16) * 16 [16 bytes per row]
    uint8_t i = row_start + (voltage >> 4); // voltage/16
    uint8_t bits_to_add = (voltage & 15) + 1; // (voltage % 16) + 1

    uint8_t bit_pattern_indices = pgm_read_byte(&TEMP_AND_VOLTAGE_TO_EV_DIFFS[i]);
    uint8_t bits1 = pgm_read_byte(&TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[bit_pattern_indices >> 4]);
    uint8_t bits2 = pgm_read_byte(&TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[bit_pattern_indices & 0x0F]);
    if (bits_to_add < 8)
        bits1 &= 0xFF << (8 - bits_to_add);
    if (bits_to_add < 16)
        bits2 &= 0xFF << (16 - bits_to_add);

    // See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    uint8_t r = pgm_read_byte(&TEMP_AND_VOLTAGE_TO_EV_ABS[i]);
#ifdef TEST
    uint8_t c = 0;
#define INCC ,++c
#else
#define INCC
#endif
    for (; bits1; ++r INCC)
        bits1 &= bits1 - 1;
    for (; bits2; ++r INCC)
        bits2 &= bits2 - 1;

#ifdef TEST
    printf("voltage = %i, count = %i, bitsper = %i, abs %i, absi = %i, r = %i\n",
           voltage, c, bits_to_add, pgm_read_byte(&TEMP_AND_VOLTAGE_TO_EV_ABS[i]),
           i, pgm_read_byte(&TEMP_AND_VOLTAGE_TO_EV_ABS[i]) + c);
#endif

    return r;

#undef INCC
}

#ifdef TEST

int main()
{
    // Test that compressed table is giving correct values by comparing to uncompressed table.
    for (int t = 0; t < 256; ++t) {
        for (int v = 0; v < 246; ++v) {
          uint8_t uncompressed = pgm_read_byte(&TEST_TEMP_AND_VOLTAGE_TO_EV[(256 * ((unsigned)t/16)) + (unsigned)v]);
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
