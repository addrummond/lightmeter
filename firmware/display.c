#include <stdint.h>
#include <stdlib.h>
#include <display.h>
#include <myassert.h>
#include <bitmaps/bitmaps.h>
#include <deviceconfig.h>
#include <stm32f0xx_gpio.h>
#include <debugging.h>
#include <i2c.h>

#define DISPLAY_I2C_ADDR (0b0111100 << 1)

// We send data in 32-byte chunks to reduce I2C overheads to acceptable level.
static uint8_t write_buffer[32];
static unsigned write_buffer_i = 0;

static void flush_write_buffer()
{
    if (write_buffer_i == 0)
        return;

    I2C_WAIT_ON_FLAG_NO_RESET(I2C_ISR_BUSY, "flush_write_buffer 1");
    I2C_TransferHandling(I2C_I2C, DISPLAY_I2C_ADDR, write_buffer_i+1, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "flush_write_buffer 2");
    I2C_SendData(I2C_I2C, 0b01000000); // Control byte.
    unsigned i;
    for (i = 0; i < write_buffer_i; ++i) {
        I2C_WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "flush_write_buffer 3");
        I2C_SendData(I2C_I2C, write_buffer[i]);
    }
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_STOPF, "flush_write_buffer 4");
}

void display_write_data_start()
{
    write_buffer_i = 0;
}

void display_write_data_end()
{
    flush_write_buffer();
    write_buffer_i = 0;
}

void display_write_byte(uint8_t b)
{
    write_buffer[write_buffer_i++] = b;
    if (write_buffer_i == sizeof(write_buffer)/sizeof(uint8_t)) {
        flush_write_buffer();
        write_buffer_i = 0;
    }
}

void display_command(uint8_t c)
{
    I2C_WAIT_ON_FLAG_NO_RESET(I2C_ISR_BUSY, "display_command 1");
    I2C_TransferHandling(I2C_I2C, DISPLAY_I2C_ADDR, 2, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "display_command 2");
    // Send control byte.
    I2C_SendData(
        I2C_I2C,
          (1 << 7) // Not just data bytes
        | (0 << 6) // Command (not data)
    );
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "display_command 3");
    // Send the command byte itself (finally!)
    I2C_SendData(I2C_I2C, c);
    I2C_WAIT_ON_FLAG_RESET(I2C_ISR_STOPF, "display_command 4");
}

void display_reset()
{
    // Configure reset pin.
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = DISPLAY_RESET_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DISPLAY_RESET_GPIO_PORT, &gpi);

    // Reset pin is pulled high in normal operation, set low to reset.
    GPIO_WriteBit(DISPLAY_RESET_GPIO_PORT, DISPLAY_RESET_PIN, 1);
    uint32_t i;
    for (i = 0; i < 400000; ++i);
    GPIO_WriteBit(DISPLAY_RESET_GPIO_PORT, DISPLAY_RESET_PIN, 0);
    for (i = 0; i < 40000; ++i);
    GPIO_WriteBit(DISPLAY_RESET_GPIO_PORT, DISPLAY_RESET_PIN, 1);
}

void display_set_contrast(uint8_t contrast)
{
    display_command(DISPLAY_SETCONTRAST);
    display_command(contrast);
}

void display_init()
{
    // Turn on power MOSFET for display.
    GPIO_InitTypeDef gpi;
    gpi.GPIO_Pin = DISPLAY_POWER_PIN;
    gpi.GPIO_Mode = GPIO_Mode_OUT;
    gpi.GPIO_Speed = GPIO_Speed_Level_1;
    gpi.GPIO_OType = GPIO_OType_PP;
    gpi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(DISPLAY_POWER_GPIO_PORT, &gpi);
    // Turn on power to screen.
    GPIO_WriteBit(DISPLAY_POWER_GPIO_PORT, DISPLAY_POWER_PIN, 1);

    display_reset();

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
    display_set_contrast(255);
    display_command(DISPLAY_SETPRECHARGE);                  // 0xd9
    display_command(0xF1);
    display_command(DISPLAY_SETVCOMDETECT);                 // 0xDB
    display_command(0x40);
    display_command(DISPLAY_DISPLAYALLON_RESUME);           // 0xA4
    display_command(DISPLAY_NORMALDISPLAY);                 // 0xA6

    display_command(DISPLAY_DISPLAYON);
}

void display_ramp_down_contrast(uint32_t ticks_per_grade)
{
    uint8_t val = 255;
    do {
        display_set_contrast(val);
        uint32_t e = ((SysTick->VAL << 8) - (ticks_per_grade << 8)) >> 8;
        while (SysTick->VAL < e); // Deal with case where it overflowed.
        while (SysTick->VAL >= e);
    } while (val-- > 0);
}

// Writes a slightly oddly-formatted array of columns, where each column is some number of pages tall.
// For example, if 'pages_per_col' is 3, then pages[0] gives the column at x+0, page_y,
// pages[1] gives the column at x+0, page_y+1, pages[2] gives the column at x+0, page_y+2,
// and page[3] gives the column at x+1, page_y.
//
// Automatically clips page arrays that extend beyond the bottom of the screen.
void display_write_page_array(const uint8_t *pages, uint_fast8_t ncols, uint_fast8_t pages_per_col, uint_fast8_t x, uint_fast8_t page_y)
{
    assert(page_y < DISPLAY_LCDHEIGHT);
    uint8_t col_start_low = x & 0x0F;
    uint8_t col_start_high = x >> 4;

    // Don't go off the right edge.
    if (ncols + x > DISPLAY_LCDWIDTH)
        ncols = DISPLAY_LCDWIDTH - x;

    display_command(DISPLAY_HORIZONTALADDR);
    display_command(DISPLAY_SET_PAGE_START + 0);

    uint_fast8_t p;
    for (p = 0; p < pages_per_col && p+page_y < DISPLAY_NUM_PAGES; ++p) {
        display_command(DISPLAY_SET_COL_START_LOW + col_start_low);
        display_command(DISPLAY_SET_COL_START_HIGH + col_start_high);
        display_command(DISPLAY_SET_PAGE_START + page_y + p);

        DISPLAY_WRITE_DATA {
            uint_fast8_t c;
            uint_fast8_t cc;
            for (c = 0, cc = p; c < ncols; ++c, cc += pages_per_col) {
                display_write_byte(pages[cc]);
            }
        }
    }
}

void display_bwrite_8px_char(const uint8_t *px_grid, uint8_t *out, uint_fast8_t pages_per_col, uint_fast8_t voffset)
{
    uint_fast8_t page_voffset = voffset >> 3;
    uint_fast8_t pixel_voffset = voffset & 7;

    uint_fast8_t i;
    out += page_voffset;
    for (i = 0; i < CHAR_WIDTH_8PX; ++i, out += pages_per_col) {
        uint8_t px = px_grid[i];
        out[0] |= px << pixel_voffset;
        if (pixel_voffset > 0)
            out[1] |= px >> (8 - pixel_voffset);
    }
}

// Each byte of out is an 8px (i.e. one-page) column. 'pages_per_col' gives the number of (stacked) columns.
// 'voffeset' is the pixel offset of the top of each character from the top of the highest column.
// 'bwrite' is short for "buffered write".
void display_bwrite_12px_char(const uint8_t *char_grid, uint8_t *out, uint_fast8_t pages_per_col, uint_fast8_t voffset)
{
    uint_fast8_t page_voffset = voffset >> 3;
    uint_fast8_t pixel_voffset = voffset & 7;

    out += page_voffset;
    uint_fast8_t i;
    for (i = 0; i < 8/CHAR_12PX_BLOCK_SIZE; ++i) {
        // Top block.
        uint_fast8_t topi = char_grid[(16/CHAR_12PX_BLOCK_SIZE)+i];
        const uint8_t *top = CHAR_BLOCKS_12PX + (topi << 1);
        // Middle block.
        uint_fast8_t middlei = char_grid[(8/CHAR_12PX_BLOCK_SIZE)+i];
        const uint8_t *middle = CHAR_BLOCKS_12PX + (middlei << 1);
        // Bottom block.
        uint_fast8_t bttmi = char_grid[(0/CHAR_12PX_BLOCK_SIZE)+i];
        const uint8_t *bttm = CHAR_BLOCKS_12PX + (bttmi << 1);

        // One loop iteration for each column.
        uint_fast8_t j;
        for (j = 0; j < CHAR_12PX_BLOCK_SIZE; ++j, out += pages_per_col) {
            uint_fast8_t bi = j >> 1;
            uint_fast8_t bm = ((j & 1) ^ 1) << 2;

            uint8_t top_bits = (top[bi] >> bm) & 0x0F;
            uint8_t middle_bits = (middle[bi] >> bm) & 0x0F;
            uint8_t bttm_bits = (bttm[bi] >> bm) & 0x0F;

            uint8_t topmiddle_bits = top_bits | (middle_bits << 4);

            // Column consists of topmiddle_bits and then bttm_bits.
            out[0] |= topmiddle_bits << pixel_voffset;
            out[1] |= (bttm_bits << pixel_voffset) | (topmiddle_bits >> (8 - pixel_voffset));
            if (pixel_voffset > 4) {
                out[2] |= bttm_bits >> (8 - pixel_voffset);
            }
        }
    }
}

void display_clear()
{
    display_command(DISPLAY_HORIZONTALADDR);
    display_command(DISPLAY_SET_PAGE_START + 0);

    DISPLAY_WRITE_DATA {
        uint_fast16_t i;
        for (i = 0; i < (DISPLAY_LCDWIDTH*DISPLAY_LCDHEIGHT/8); ++i) {
            display_write_byte(0x00);
        }
    }
}

// See DISPLAY_WRITE_DATA macro in display.h
uint_fast8_t i___;
