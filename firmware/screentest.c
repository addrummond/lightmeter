#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <display_constants.h>

#include <bitmaps/bitmaps.h>

static void fast_write(uint8_t d)
{
    uint8_t bit;
    for (bit = 0x80; bit; bit >>= 1) {
        DISPLAY_CLK_PORT &= ~(1 << DISPLAY_CLK_BIT);
        if (d & bit)
            DISPLAY_DATA_PORT |= (1 << DISPLAY_DATA_BIT);
        else
            DISPLAY_DATA_PORT &= ~(1 << DISPLAY_DATA_BIT);
        DISPLAY_CLK_PORT |= (1 << DISPLAY_CLK_BIT);  
    }
}

static void display_command(uint8_t c)
{
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
    DISPLAY_DC_PORT &= ~(1 << DISPLAY_DC_BIT);
    DISPLAY_CS_PORT &= ~(1 << DISPLAY_CS_BIT);
    fast_write(c);
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
}

static void init_display()
{
    // Setup output ports.
    DISPLAY_DATA_DDR |= (1 << DISPLAY_DATA_BIT);
    DISPLAY_CS_DDR |= (1 << DISPLAY_CS_BIT);
    DISPLAY_CLK_DDR |= (1 << DISPLAY_CLK_BIT);
    DISPLAY_DC_DDR |= (1 << DISPLAY_DC_BIT);
    DISPLAY_RESET_DDR |= (1 << DISPLAY_RESET_BIT);

    // Trigger reset pin on screen.
    DISPLAY_RESET_PORT |= (1 << DISPLAY_RESET_BIT);
    _delay_ms(1);
    DISPLAY_RESET_PORT &= ~(1 << DISPLAY_RESET_BIT);
    _delay_ms(10);
    DISPLAY_RESET_PORT |= (1 << DISPLAY_RESET_BIT);

    // Display initialization sequence. No idea what most of this does.
    display_command(DISPLAY_DISPLAYOFF);                    // 0xAE
    display_command(DISPLAY_SETDISPLAYCLOCKDIV);            // 0xD5
    display_command(0x80);                                  // the suggested ratio 0x80
    display_command(DISPLAY_SETMULTIPLEX);                  // 0xA8
    display_command(0x3F);
    display_command(DISPLAY_SETDISPLAYOFFSET);              // 0xD3
    display_command(0x0);                                   // no offset
    display_command(DISPLAY_SETSTARTLINE | 0x0);            // line #0
    display_command(DISPLAY_CHARGEPUMP);                    // 0x8D
    display_command(0x14);
    display_command(DISPLAY_MEMORYMODE);                    // 0x20
    display_command(0x00);                                  // 0x0 act like ks0108
    display_command(DISPLAY_SEGREMAP | 0x1);
    display_command(DISPLAY_COMSCANDEC);
    display_command(DISPLAY_SETCOMPINS);                    // 0xDA
    display_command(0x12);
    display_command(DISPLAY_SETCONTRAST);                   // 0x81
    display_command(0xCF);
    display_command(DISPLAY_SETPRECHARGE);                  // 0xd9
    display_command(0xF1);
    display_command(DISPLAY_SETVCOMDETECT);                 // 0xDB
    display_command(0x40);
    display_command(DISPLAY_DISPLAYALLON_RESUME);           // 0xA4
    display_command(DISPLAY_NORMALDISPLAY);                 // 0xA6

    display_command(DISPLAY_DISPLAYON);
}

static void test_display()
{
    uint8_t out = 0xF0;
    for (;; out ^= 0xFF) {
        display_command(DISPLAY_COLUMNADDR);
        display_command(0);   // Column start address (0 = reset)
        display_command(127); // Column end address (127 = reset)
        
        display_command(DISPLAY_PAGEADDR);
        display_command(0); // Page start address (0 = reset)
        display_command((DISPLAY_LCDHEIGHT == 64) ? 7 : 3); // Page end address

        DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
        DISPLAY_DC_PORT |= (1 << DISPLAY_DC_BIT);
        DISPLAY_CS_PORT &= ~(1 << DISPLAY_CS_BIT);
    
        uint16_t i;
        for (i = 0; i < (DISPLAY_LCDWIDTH*DISPLAY_LCDHEIGHT/8); ++i) {
            fast_write(out);
        }
        DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);

        _delay_ms(100);
    }
}

int main()
{
    init_display();

    test_display();

    for (;;);
}
