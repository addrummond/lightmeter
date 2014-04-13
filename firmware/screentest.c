#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <readbyte.h>
#include <assert.h>

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

static void display_write_data_start()
{
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
    DISPLAY_DC_PORT |= (1 << DISPLAY_DC_BIT);
    DISPLAY_CS_PORT &= ~(1 << DISPLAY_CS_BIT);
}

static void display_write_data_end()
{
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
}

static uint8_t i___;
#define DISPLAY_WRITE_DATA for (i___ = 0, display_write_data_start(); i___ < 1; ++i___, display_write_data_end())

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

// Each byte of out is an 8px (i.e. one-page) column. 'npages' gives the number of columns.
// 'voffeset' is the pixel offset of the top of each character from the top of the highest column.
static void bwrite_12x12_char(const uint8_t *char_grid, uint8_t *out, uint8_t npages, uint8_t voffset)
{
    uint8_t page_voffset = voffset >> 3;
    uint8_t pixel_voffset = voffset & 7;

    out = out + page_voffset;
    uint8_t i;
    for (i = 0; i < 12/CHAR_12PX_BLOCK_SIZE; ++i) {
        // Top block.
        const uint8_t *top = CHAR_BLOCKS_12PX + pgm_read_byte(&char_grid[(24/CHAR_12PX_BLOCK_SIZE)+i]);
        // Middle block.
        const uint8_t *middle = CHAR_BLOCKS_12PX + pgm_read_byte(&char_grid[(12/CHAR_12PX_BLOCK_SIZE)+i]);
        // Bottom block.
        const uint8_t *bttm = CHAR_BLOCKS_12PX + pgm_read_byte(&char_grid[(12/CHAR_12PX_BLOCK_SIZE)+i]);
        
        // One loop for each column.
        uint8_t j;
        for (j = 0; j < CHAR_12PX_BLOCK_SIZE; ++j, out += npages) {
            uint8_t bi = j >> 1;
            uint8_t bm = (~(j & 1)) << 2;
 
            uint8_t top_bits = (pgm_read_byte(&top[bi]) >> bm) & 0x0F;
            uint8_t middle_bits = (pgm_read_byte(&middle[bi]) >> bm) & 0x0F;
            uint8_t bttm_bits = (pgm_read_byte(&bttm[bi]) >> bm) &0x0F;

            uint8_t bits_to_go = 12;
            uint8_t bout = top_bits << pixel_voffset;
            out[0] |= top_bits;
            bits_to_go -= 4;
            if (pixel_voffset > 4)
                bits_to_go += 4 - pixel_voffset;

            bout = top_bits >> (bits_to_go - 8);
            bout |= middle_bits << (bits_to_go - 8);
            out[1] |= bout;
            bits_to_go -= 4;

            bout = middle_bits >> (bits_to_go - 4);
            bout |= bttm_bits << (bits_to_go - 4);
            out[2] |= bout;
            bits_to_go -= 4;

            if (bits_to_go > 0)
                out[3] = bttm_bits >> bits_to_go;
        }
    }
}

static void write_12x12_character(const uint8_t *char_grid, uint8_t x, uint8_t y)
{
    // Doesn't handle non-page-aligned y values. (No point in fixing this because
    // it's just for test purposes.)

    assert(CHAR_12PX_BLOCK_SIZE == 4);

    uint8_t low_col_start = x & 0xF;
    uint8_t high_col_start = x >> 4;
    display_command(DISPLAY_SET_PAGE_START + (y >> 3));
    display_command(DISPLAY_SET_COL_START_LOW + low_col_start);
    display_command(DISPLAY_SET_COL_START_HIGH + high_col_start);  

    int8_t i, j;

    DISPLAY_WRITE_DATA {
        for (i = 0; i < 12/CHAR_12PX_BLOCK_SIZE; ++i) {
            // Top block.
            const uint8_t *top = CHAR_BLOCKS_12PX + pgm_read_byte(&char_grid[(24/CHAR_12PX_BLOCK_SIZE)+i]);
            // Middle block.
            const uint8_t *middle = CHAR_BLOCKS_12PX + pgm_read_byte(&char_grid[(12/CHAR_12PX_BLOCK_SIZE)+i]);

            // One loop for each column.
            uint8_t j;
            for (j = 0; j < CHAR_12PX_BLOCK_SIZE; ++j) {
                uint8_t bi = j >> 1;
                uint8_t bm = (~(j & 1)) << 2;

                uint8_t top_bits = (pgm_read_byte(&top[bi]) >> bm) & 0x0F;
                uint8_t middle_bits = (pgm_read_byte(&middle[bi]) >> bm) & 0x0F;

                fast_write(~(top_bits | (middle_bits << 4)));
            }
        }
    }

    display_command(DISPLAY_SET_PAGE_START + (y >> 3) + 1);
    display_command(DISPLAY_SET_COL_START_LOW + low_col_start);
    display_command(DISPLAY_SET_COL_START_HIGH + high_col_start);

    DISPLAY_WRITE_DATA {
        for (i = 0; i < 12/CHAR_12PX_BLOCK_SIZE; ++i) {
            // Bottom block.
            const uint8_t *bottom = CHAR_BLOCKS_12PX + pgm_read_byte(&char_grid[(0/CHAR_12PX_BLOCK_SIZE)+i]);

            // One loop for each pair of columns.
            for (j = 0; j < CHAR_12PX_BLOCK_SIZE; ++j) {
                uint8_t bi = j >> 1;
                uint8_t bm = (~(j & 1)) << 2;

                uint8_t bottom_bits = (pgm_read_byte(&bottom[bi]) >> bm) & 0x0F;

                fast_write((~bottom_bits) & 0x0F);
            }
        }
    }
}

static void clear_display()
{
    display_command(DISPLAY_HORIZONTALADDR);

    DISPLAY_WRITE_DATA {
        uint16_t i;
        for (i = 0; i < (DISPLAY_LCDWIDTH*DISPLAY_LCDHEIGHT/8); ++i) {
            fast_write(0x00);
        }
    }

    display_command(DISPLAY_PAGEADDR);
}

static void test_display()
{
    /*    uint8_t out = 0xF0;
    for (;; out ^= 0xFF) {
        clear_display();

        write_12x12_character(CHAR_12PX_F, 50, 8);

        _delay_ms(1000);

        clear_display();

        _delay_ms(500);
        }*/

    clear_display();
    write_12x12_character(CHAR_12PX_I, 8, 8);
    write_12x12_character(CHAR_12PX_S, 20, 8);
    write_12x12_character(CHAR_12PX_0, 32, 8);

    write_12x12_character(CHAR_12PX_F, 50, 8);
    write_12x12_character(CHAR_12PX_F, 62, 8);
    write_12x12_character(CHAR_12PX_9, 70, 26);
    for (;;);
}

int main()
{
    init_display();

    test_display();

    for (;;);
}
