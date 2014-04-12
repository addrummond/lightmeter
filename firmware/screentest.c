#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// PB0: Data
// PB1: CS
// PB2: Clk
// PB3: Rst
// PB4: DC

#define DISPLAY_LCDWIDTH                  128
#define DISPLAY_LCDHEIGHT                 64

#define DISPLAY_SETCONTRAST 0x81
#define DISPLAY_DISPLAYALLON_RESUME 0xA4
#define DISPLAY_DISPLAYALLON 0xA5
#define DISPLAY_NORMALDISPLAY 0xA6
#define DISPLAY_INVERTDISPLAY 0xA7
#define DISPLAY_DISPLAYOFF 0xAE
#define DISPLAY_DISPLAYON 0xAF

#define DISPLAY_SETDISPLAYOFFSET 0xD3
#define DISPLAY_SETCOMPINS 0xDA

#define DISPLAY_SETVCOMDETECT 0xDB

#define DISPLAY_SETDISPLAYCLOCKDIV 0xD5
#define DISPLAY_SETPRECHARGE 0xD9

#define DISPLAY_SETMULTIPLEX 0xA8

#define DISPLAY_SETLOWCOLUMN 0x00
#define DISPLAY_SETHIGHCOLUMN 0x10

#define DISPLAY_SETSTARTLINE 0x40

#define DISPLAY_MEMORYMODE 0x20
#define DISPLAY_COLUMNADDR 0x21
#define DISPLAY_PAGEADDR   0x22

#define DISPLAY_COMSCANINC 0xC0
#define DISPLAY_COMSCANDEC 0xC8

#define DISPLAY_SEGREMAP 0xA0

#define DISPLAY_CHARGEPUMP 0x8D

#define DISPLAY_EXTERNALVCC 0x1
#define DISPLAY_SWITCHCAPVCC 0x2

#define DISPLAY_ACTIVATE_SCROLL 0x2F
#define DISPLAY_DEACTIVATE_SCROLL 0x2E
#define DISPLAY_SET_VERTICAL_SCROLL_AREA 0xA3
#define DISPLAY_RIGHT_HORIZONTAL_SCROLL 0x26
#define DISPLAY_LEFT_HORIZONTAL_SCROLL 0x27
#define DISPLAY_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define DISPLAY_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

static void fast_write(uint8_t d)
{
    uint8_t bit;
    for (bit = 0x80; bit; bit >>= 1) {
        PORTB &= ~0b100;
        if (d & bit)
            PORTB |= 0b1;
        else
            PORTB &= ~0b1;
        PORTB |= 0b100;
    }
}

static void display_command(uint8_t c)
{
    PORTB |= 0b10;
    PORTB &= ~0b10000;
    PORTB &= ~0b10;
    fast_write(c);
    PORTB |= 0b10;
}

static void init_display()
{
    DDRB = 0b11111;

    // Trigger reset pin on screen.
    DDRB |= 0b1000;
    PORTB |= 0b1000;
    _delay_ms(1);
    PORTB &= ~0b1000;
    _delay_ms(10);
    PORTB |= 0b1000;

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
    display_command(DISPLAY_COLUMNADDR);
    display_command(0);   // Column start address (0 = reset)
    display_command(127); // Column end address (127 = reset)

    display_command(DISPLAY_PAGEADDR);
    display_command(0); // Page start address (0 = reset)
    display_command((DISPLAY_LCDHEIGHT == 64) ? 7 : 3); // Page end address

    // SPI
    PORTB |= 0b10;
    PORTB |= 0b10000;
    PORTB &= ~0b10;

    uint16_t i;
    for (i = 0; i < (DISPLAY_LCDWIDTH*DISPLAY_LCDHEIGHT/8); ++i) {
      fast_write(i & 1);
      //ssd1306_data(buffer[i]);
    }
    PORTB |= 0b10;
}

int main()
{
    init_display();

    test_display();

    for (;;);
}
