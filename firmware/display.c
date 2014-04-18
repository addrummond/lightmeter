#include <display.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <readbyte.h>
#include <assert.h>
#include <bitmaps/bitmaps.h>

void display_write_byte(uint8_t d)
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

void display_command(uint8_t c)
{
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
    DISPLAY_DC_PORT &= ~(1 << DISPLAY_DC_BIT);
    DISPLAY_CS_PORT &= ~(1 << DISPLAY_CS_BIT);
    display_write_byte(c);
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
}

void display_write_data_start()
{
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
    DISPLAY_DC_PORT |= (1 << DISPLAY_DC_BIT);
    DISPLAY_CS_PORT &= ~(1 << DISPLAY_CS_BIT);
}

void display_write_data_end()
{
    DISPLAY_CS_PORT |= (1 << DISPLAY_CS_BIT);
}

void display_init()
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

// Writes a slightly oddly-formatted array of columns, where each column is some number of pages tall.
// For example, if 'pages_per_col' is 3, then pages[0] gives the column at x+0, page_y,
// pages[1] gives the column at x+0, page_y+1, pages[2] gives the column at x+0, page_y+2,
// and page[3] gives the column at x+1, page_y.
void display_write_page_array(const uint8_t *pages, uint8_t ncols, uint8_t pages_per_col, uint8_t x, uint8_t page_y)
{
    uint8_t col_start_low = x & 0x0F;
    uint8_t col_start_high = x >> 4;

    uint8_t p;
    for (p = 0; p < pages_per_col; ++p) {
        display_command(DISPLAY_SET_COL_START_LOW + col_start_low);
        display_command(DISPLAY_SET_COL_START_HIGH + col_start_high);
        display_command(DISPLAY_SET_PAGE_START + page_y + p);
        
        DISPLAY_WRITE_DATA {
            uint8_t c;
            uint8_t cc;
            for (c = 0, cc = p; c < ncols; ++c, cc += pages_per_col) {
                display_write_byte(pages[cc]);
            }
        }
    }
}

void display_bwrite_8px_char(const uint8_t *px_grid, uint8_t *out, uint8_t pages_per_col, uint8_t voffset)
{
    uint8_t page_voffset = voffset >> 3;
    uint8_t pixel_voffset = voffset & 7;

    uint8_t i;
    out += page_voffset;
    for (i = 0; i < CHAR_WIDTH_8PX; ++i, out += pages_per_col) {
        uint8_t px = pgm_read_byte(&px_grid[i]);
        out[0] |= px << pixel_voffset;
        if (pixel_voffset > 0)
            out[1] |= px >> (8 - pixel_voffset);
    }
}

static uint8_t flip_nibble(uint8_t nibble)
{
    return ((nibble & 1) << 3) | ((nibble & 2) << 1) | ((nibble & 4) >> 1) | ((nibble & 8) >> 3);
}

// Each byte of out is an 8px (i.e. one-page) column. 'pages_per_col' gives the number of (stacked) columns.
// 'voffeset' is the pixel offset of the top of each character from the top of the highest column.
// 'bwrite' is short for "buffered write".
void display_bwrite_12px_char(const uint8_t *char_grid, uint8_t *out, uint8_t pages_per_col, uint8_t voffset)
{
    uint8_t page_voffset = voffset >> 3;
    uint8_t pixel_voffset = voffset & 7;

    out += page_voffset;
    uint8_t i;
    for (i = 0; i < 8/CHAR_12PX_BLOCK_SIZE; ++i) {
        // Top block.
        uint8_t rawtopi = pgm_read_byte(&char_grid[(16/CHAR_12PX_BLOCK_SIZE)+i]);
        const uint8_t *top = CHAR_BLOCKS_12PX + ((rawtopi >> 2) << 1);
        // Middle block.
        uint8_t rawmiddlei = pgm_read_byte(&char_grid[(8/CHAR_12PX_BLOCK_SIZE)+i]);
        const uint8_t *middle = CHAR_BLOCKS_12PX + ((rawmiddlei >> 2) << 1);
        // Bottom block.
        uint8_t rawbttmi = pgm_read_byte(&char_grid[(0/CHAR_12PX_BLOCK_SIZE)+i]);
        const uint8_t *bttm = CHAR_BLOCKS_12PX + ((rawbttmi >> 2) << 1);
        
        // One loop for each column.
        uint8_t j;
        for (j = 0; j < CHAR_12PX_BLOCK_SIZE; ++j, out += pages_per_col) {
            uint8_t bi = j >> 1;
            uint8_t bm = (~(j & 1)) << 2;

            uint8_t flip_j = CHAR_12PX_BLOCK_SIZE-1-j;
            uint8_t flip_bi = flip_j >> 1;
            uint8_t flip_bm = (~(flip_j & 1)) << 2;
 
#define BI(x) (raw ## x ## i & 2 ? flip_bi : bi)
#define BM(x) (raw ## x ## i & 2 ? flip_bm : bm)
            uint8_t top_bits = (pgm_read_byte(&top[BI(top)]) >> BM(top)) & 0x0F;
            if (rawtopi & 1)
                top_bits = flip_nibble(top_bits);
            uint8_t middle_bits = (pgm_read_byte(&middle[BI(middle)]) >> BM(middle)) & 0x0F;
            if (rawmiddlei & 1)
                middle_bits = flip_nibble(middle_bits);
            uint8_t bttm_bits = (pgm_read_byte(&bttm[BI(bttm)]) >> BM(bttm)) & 0x0F;
            if (rawbttmi & 1)
                bttm_bits = flip_nibble(bttm_bits);

            uint8_t topmiddle_bits = top_bits | (middle_bits << 4);

            // Column consists of topmiddle_bits and then bttm_bits.
            out[0] |= topmiddle_bits << pixel_voffset;
            out[1] |= (bttm_bits << pixel_voffset) | (topmiddle_bits >> (8 - pixel_voffset));
            if (pixel_voffset > 4) {
                out[2] |= bttm_bits >> (8 - pixel_voffset);
            }
#undef BI
#undef BM
        }
    }
}

void display_clear()
{
    display_command(DISPLAY_HORIZONTALADDR);

    DISPLAY_WRITE_DATA {
        uint16_t i;
        for (i = 0; i < (DISPLAY_LCDWIDTH*DISPLAY_LCDHEIGHT/8); ++i) {
            display_write_byte(0x00);
        }
    }

    display_command(DISPLAY_PAGEADDR);
}

// See DISPLAY_WRITE_DATA macro in display.h
uint8_t i___;
