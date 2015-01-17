#include <stdint.h>
#include <display.h>
#include <myassert.h>
#include <bitmaps/bitmaps.h>
#include <deviceconfig.h>
#include <stm32f0xx_gpio.h>
#include <debugging.h>

// Looking at LM75 data sheet an example I2C code indicates that 7-bit address
// should be left aligned.
#define DISPLAY_I2C_ADDR (0b0111101 << 1)
#define FLAG_TIMEOUT     ((uint32_t)0x1000)

static void timed_out(const char *msg, uint32_t length)
{
    debugging_writec("I2C Timeout: ");
    debugging_write(msg, length);
    debugging_writec("\n");
}

#define WAIT_ON_FLAG_(flag, msg, op, ret, brk)                           \
    do {                                                                 \
        uint32_t to = FLAG_TIMEOUT;                                      \
        while (I2C_GetFlagStatus(I2C_I2C, (flag)) op RESET) {            \
            if (to-- == 0)                                               \
                { ret timed_out((msg), sizeof(msg)/sizeof(char)); brk; } \
        }                                                                \
    } while (0)
#define WAIT_ON_FLAG_NO_RESET(flag, msg)       WAIT_ON_FLAG_((flag), (msg), !=, return, )
#define WAIT_ON_FLAG_RESET(flag, msg)          WAIT_ON_FLAG_((flag), (msg), ==, return, )
#define WAIT_ON_FLAG_NO_RESET_NORET(flag, msg) WAIT_ON_FLAG_((flag), (msg), !=, ,break)
#define WAIT_ON_FLAG_RESET_NORET(flag, msg)    WAIT_ON_FLAG_((flag), (msg), ==, ,break)

void display_write_data_start()
{
    ;
}

void display_write_byte(uint8_t b)
{
    WAIT_ON_FLAG_NO_RESET(I2C_ISR_BUSY, "display_write_byte 1");
    I2C_TransferHandling(I2C_I2C, DISPLAY_I2C_ADDR, 2, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);
    WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "TIMEOUT!");
    I2C_SendData(I2C_I2C, 0);
    WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "display_write_byte 3");
    I2C_SendData(I2C_I2C, b);
    WAIT_ON_FLAG_RESET(I2C_ISR_STOPF, "display_write_byte 4");
    I2C_ClearFlag(I2C_I2C, I2C_ICR_STOPCF);
}

void display_write_data_end()
{
    ;
}

void display_command(uint8_t c)
{
    WAIT_ON_FLAG_NO_RESET(I2C_ISR_BUSY, "display_command 1");
    I2C_TransferHandling(I2C_I2C, DISPLAY_I2C_ADDR, 2, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);
    WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "TIMEOUT!");
    // Send control byte.
    I2C_SendData(
        I2C_I2C,
          (1 << 7) // Not just data bytes
        | (0 << 6) // Command (not data)
    );
    WAIT_ON_FLAG_RESET(I2C_ISR_TXIS, "display_command 3");
    // Send the command byte itself (finally!)
    I2C_SendData(I2C_I2C, c);
    WAIT_ON_FLAG_RESET(I2C_ISR_STOPF, "display_command 4");
    I2C_ClearFlag(I2C_I2C, I2C_ICR_STOPCF);
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
    for (i = 0; i < 4000; ++i);
    debugging_writec("LOW\n");
    GPIO_WriteBit(DISPLAY_RESET_GPIO_PORT, DISPLAY_RESET_PIN, 0);
    for (i = 0; i < 4000; ++i);
    debugging_writec("HIGH\n");
    GPIO_WriteBit(DISPLAY_RESET_GPIO_PORT, DISPLAY_RESET_PIN, 1);
}

void display_init()
{
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
void display_write_page_array(const uint8_t *pages, uint_fast8_t ncols, uint_fast8_t pages_per_col, uint_fast8_t x, uint_fast8_t page_y)
{
    uint_fast8_t col_start_low = x & 0x0F;
    uint_fast8_t col_start_high = x >> 4;

    // Don't go off the right edge.
    if (ncols + x > DISPLAY_LCDWIDTH)
        ncols = DISPLAY_LCDWIDTH - x;

    uint_fast8_t p;
    for (p = 0; p < pages_per_col; ++p) {
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

    DISPLAY_WRITE_DATA {
        uint_fast16_t i;
        for (i = 0; i < (DISPLAY_LCDWIDTH*DISPLAY_LCDHEIGHT/8); ++i) {
            display_write_byte(0x00);
        }
    }

    display_command(DISPLAY_PAGEADDR);
}

// See DISPLAY_WRITE_DATA macro in display.h
uint_fast8_t i___;
