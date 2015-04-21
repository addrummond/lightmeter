#ifndef DISPLAY__H
#define DISPLAY__H

#include <stdint.h>

void display_write_byte(uint8_t d);
void display_command(uint8_t c);
void display_write_data_start(void);
void display_write_data_end(void);
void display_reset(void);
void display_init(void);
void display_ramp_down_contrast(uint32_t ticks_per_grade);
void display_set_contrast(uint8_t contrast);
void display_write_page_array(const uint8_t *pages, uint_fast8_t ncols, uint_fast8_t pages_per_col, uint_fast8_t x, uint_fast8_t page_y);
void display_bwrite_8px_char(const uint8_t *char_grid, uint8_t *out, uint_fast8_t pages_per_col, uint_fast8_t voffset);
void display_bwrite_12px_char(const uint8_t *char_grid, uint8_t *out, uint_fast8_t pages_per_col, uint_fast8_t voffset);
void display_clear(void);

extern uint_fast8_t i___;
#define DISPLAY_WRITE_DATA for (i___ = 0, display_write_data_start(); i___ < 1; ++i___, display_write_data_end())

#define DISPLAY_LCDWIDTH                  128
#define DISPLAY_LCDHEIGHT                 64
#define DISPLAY_NUM_PAGES                 (DISPLAY_LCDHEIGHT/8)

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
#define DISPLAY_HORIZONTALADDR 0x20
#define DISPLAY_COLUMNADDR 0x21
#define DISPLAY_PAGEADDR   0x22

#define DISPLAY_SET_PAGE_START 0xB0
#define DISPLAY_SET_COL_START_LOW  0x00
#define DISPLAY_SET_COL_START_HIGH 0x10

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

#endif
