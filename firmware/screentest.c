#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <i2cmaster.h>

// PB 2: RST
// PB 1: Clck
// PB 4: Data

#define READ  1
#define WRITE 0

#define SSD1306_I2C_ADDRESS   0b01111000 //0x3C	// 011110+SA0+RW - 0x3C or 0x3D
#define SSD1306_LCDWIDTH                  128
#define SSD1306_LCDHEIGHT                 64

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

static void display_command(uint8_t c)
{
    i2c_start(SSD1306_I2C_ADDRESS | WRITE);
    i2c_write(0x00);
    i2c_write(c);
    i2c_stop();
}

static void init_display()
{
    display_command(SSD1306_DISPLAYOFF);                    // 0xAE
    display_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    display_command(0x80);                                  // the suggested ratio 0x80
    display_command(SSD1306_SETMULTIPLEX);                  // 0xA8
    display_command(0x3F);
    display_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    display_command(0x0);                                   // no offset
    display_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
    display_command(SSD1306_CHARGEPUMP);                    // 0x8D
    //display_command(0x10); // external VCC
    display_command(0x14); // internal VCC
    display_command(SSD1306_MEMORYMODE);                    // 0x20
    display_command(0x00);                                  // 0x0 act like ks0108
    display_command(SSD1306_SEGREMAP | 0x1);
    display_command(SSD1306_COMSCANDEC);
    display_command(SSD1306_SETCOMPINS);                    // 0xDA
    display_command(0x12);
    display_command(SSD1306_SETCONTRAST);                   // 0x81
    //    display_command(0x9F); // external VCC
    display_command(0xCF); // internal VCC
    display_command(SSD1306_SETPRECHARGE);                  // 0xd9
    //display_command(0x22); // internal VCC
    display_command(0xF1); // internal VCC
    display_command(SSD1306_SETVCOMDETECT);                 // 0xDB
    display_command(0x40);
    display_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    display_command(SSD1306_NORMALDISPLAY);                 // 0xA6

    display_command(SSD1306_DISPLAYON);
}

static void test_display()
{
    display_command(SSD1306_COLUMNADDR);
    display_command(0);   // Column start address (0 = reset)
    display_command(127); // Column end address (127 = reset)

    display_command(SSD1306_PAGEADDR);
    display_command(0); // Page start address (0 = reset)
    display_command(7); // Page end address

    i2c_start(SSD1306_I2C_ADDRESS | WRITE);
    uint16_t i;
    for (i = 0; i < 128*64/8; ++i) {
        i2c_write(i > 500 ? 0xFF : 0x00);
    }
    i2c_stop();
}

int main()
{
    // LED FLASH
    DDRB |= 0b10;
    PORTB |= 0b10;
    _delay_ms(1000);
    PORTB &= ~0b10;
    _delay_ms(500);
    
    i2c_init();

    // Trigger reset pin on screen.
    DDRB |= 0b1000;
    PORTB |= 0b1000;
    _delay_ms(1);
    PORTB &= ~0b1000;
    _delay_ms(10);
    PORTB |= 0b1000;

    // LED FLASH
    DDRB |= 0b10;
    PORTB |= 0b10;
    _delay_ms(1000);
    PORTB &= ~0b10;

    _delay_ms(500);

    init_display();

    // LED FLASH
    DDRB |= 0b10;
    PORTB |= 0b10;
    _delay_ms(2000);
    PORTB &= ~0b10;

    test_display();

    // LED FLASH
    DDRB |= 0b10;
    PORTB |= 0b10;
    _delay_ms(2000);
    PORTB &= ~0b10;

    for (;;);
}
