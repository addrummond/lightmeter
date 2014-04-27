#include <readbyte.h>
#include <stdint.h>
#include <assert.h>
#include <divmulutils.h>
#include <calculate.h>
#include <tables.h>
#ifdef TEST
#    include <stdio.h>
#endif

#ifdef TEST
extern const uint8_t TEST_VOLTAGE_TO_EV[];
#endif

// See comments in calculate_tables.py for info on the way
// temp/voltage are encoded.
// This is explicitly implemented without using division or multiplcation,
// since the attiny doesn't have hardware division or multiplication.
// gcc would probably do most of these optimizations automatically, but since
// this code really definitely needs to run quickly, I'm doing it explicitly here.
//
// 'voltage' is in 1/256ths of the reference voltage.
uint8_t get_ev100_at_temperature_voltage(uint8_t temperature, uint8_t voltage, uint8_t op_amp_resistor_stage)
{
    const uint8_t *ev_abs = NULL, *ev_diffs = NULL, *ev_bitpatterns = NULL;
#define ASSIGN(n) (ev_abs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_ABS,                 \
                   ev_diffs = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_DIFFS,             \
                   ev_bitpatterns = STAGE ## n ## _LIGHT_VOLTAGE_TO_EV_BITPATTERNS)
    switch (op_amp_resistor_stage) {
    case 1: ASSIGN(1); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 1
    case 2: ASSIGN(2); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 2
    case 3: ASSIGN(3); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 3
    case 4: ASSIGN(4); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 4
    case 5: ASSIGN(5); break;
#if NUM_OP_AMP_RESISTOR_STAGES > 5
#error "Too many op amp stages"
#endif
#endif
#endif
#endif
#endif
    }
#undef ASSIGN

    if (voltage < VOLTAGE_TO_EV_ABS_OFFSET)
        voltage = 0;
    else
        voltage -= VOLTAGE_TO_EV_ABS_OFFSET;

    uint8_t v16 = voltage >> 4;

    uint8_t absi = v16;
    uint8_t bits_to_add = (voltage & 15) + 1; // (voltage % 16) + 1

    uint8_t bit_pattern_indices = pgm_read_byte(ev_diffs + absi);
    uint8_t bits1 = pgm_read_byte(ev_bitpatterns + (bit_pattern_indices >> 4));
    uint8_t bits2 = pgm_read_byte(ev_bitpatterns + (bit_pattern_indices & 0x0F));
    if (bits_to_add < 8)
        bits1 &= 0xFF << (8 - bits_to_add);
    if (bits_to_add < 16)
        bits2 &= 0xFF << (16 - bits_to_add);

    // See http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    uint8_t r = pgm_read_byte(ev_abs + absi);
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
           voltage, c, bits_to_add, pgm_read_byte(&NORMAL_LIGHT_VOLTAGE_TO_EV_ABS[absi]),
           absi, pgm_read_byte(ev_abs + absi) + c);
#endif

    // Compensate for effect of ambient temperature.
    int8_t adj = TEMP_EV_ADJUST[temperature >> 2];
    int16_t withcomp = r + adj;
    if (withcomp < 0)
        return 0;
    else if (withcomp > 255)
        return 255;
    else
        return (uint8_t)withcomp;

#undef INCC
}

#ifdef TEST

int main()
{
    // Test that compressed table is giving correct values by comparing to uncompressed table.
    printf("OFFSET %i\n", VOLTAGE_TO_EV_ABS_OFFSET);
    uint8_t t = 227; // ~=40C; should track reference_temperature in calculate_tables.py
    for (int v_ = 0; v_ < 246; ++v_) {
        int v = v_ - VOLTAGE_TO_EV_ABS_OFFSET;
        if (v < 0)
            v = 0;
        uint8_t uncompressed = pgm_read_byte(&TEST_VOLTAGE_TO_EV[(unsigned)v]);
        uint8_t compressed = get_ev100_at_temperature_voltage((uint8_t)t, (uint8_t)v_, NORMAL_GAIN);
        if (uncompressed != compressed) {
            printf("Values not equal for t = %i, v = %i: compressed = %i, uncompressed = %i\n", (unsigned)t, (unsigned)v_, (unsigned)compressed, (unsigned)uncompressed);
                        return 1;
        }
    }

    printf("\nTest completed successfully.\n");

    return 0;
}

#endif
