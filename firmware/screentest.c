#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <readbyte.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <bitmaps/bitmaps.h>

#include <display.h>

static void test_display()
{
    display_clear();

    /*uint8_t out[40];
    uint8_t x, y;
    for (x = 0, y = 0; x < 128-12; x += 12, ++y) {
        memset(out, 0, sizeof(out));
        display_bwrite_12x12_char(CHAR_12PX_0, out, 3, y & 7);
        display_write_page_array(out, 12, 3, x, y >> 3);
        memset(out, 0, sizeof(out));
        display_bwrite_12x12_char(CHAR_12PX_1, out, 3, (y+21) & 7);
        display_write_page_array(out, 12, 3, x, (y+21) >> 3);
        memset(out, 0, sizeof(out));
        display_bwrite_12x12_char(CHAR_12PX_2, out, 3, (y+41) & 7);
        display_write_page_array(out, 12, 3, x, (y+41) >> 3);
        }*/

    uint8_t out2[8];
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_0, out2, 1, 0);
    display_write_page_array(out2, 8, 1, 20, 4);
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_1, out2, 1, 0);
    display_write_page_array(out2, 8, 1, 28, 4);
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_2, out2, 1, 0);
    display_write_page_array(out2, 8, 1, 36, 4);
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_3, out2, 1, 0);
    display_write_page_array(out2, 8, 1, 44, 4);

    for (;;);
}

int main()
{
    display_init();

    test_display();

    for (;;);
}
