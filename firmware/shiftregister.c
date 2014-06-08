#include <avr/io.h>
#include <deviceconfig.h>

void setup_shift_register()
{
    SHIFT_REGISTER_OUTPUT_DDR |= (1 << SHIFT_REGISTER_OUTPUT_BIT);
    SHIFT_REGISTER_CLK_DDR |= (1 << SHIFT_REGISTER_CLK_BIT);
    SHIFT_REGISTER_CLK_PORT &= ~(1 << SHIFT_REGISTER_CLK_BIT);
}

// Low bit of 'out' is QA, high bit is QE.
volatile uint8_t shift_register_out__ = 0;
void set_shift_register_out()
{
    // Clear the register.
    // Appears not to be necessary if we're not worried about having outputs
    // in a weird state for a few moments.
    //
    //SHIFT_REGISTER_CLR_PORT &= ~(1 << SHIFT_REGISTER_CLR_BIT);
    //SHIFT_REGISTER_CLR_PORT |= (1 << SHIFT_REGISTER_CLR_BIT);

    uint8_t out = shift_register_out__;
    uint8_t j;
    for (j = 0; j < 8; ++j) {
        uint8_t bit = (out & 0b10000000) >> 7;
        out <<= 1;
        // Set the clock low.
        SHIFT_REGISTER_CLK_PORT &= ~(1 << SHIFT_REGISTER_CLK_BIT);
        // Output the bit.
        SHIFT_REGISTER_OUTPUT_PORT &= ~(1 << SHIFT_REGISTER_OUTPUT_BIT);
        SHIFT_REGISTER_OUTPUT_PORT |= (bit << SHIFT_REGISTER_OUTPUT_BIT);
        // Set the clock high.
        SHIFT_REGISTER_CLK_PORT |= (1 << SHIFT_REGISTER_CLK_BIT);
    }
}
