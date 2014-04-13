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

    uint8_t out[40];
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
    }

    for (;;);
}

int main()
{
    display_init();

    test_display();

    for (;;);
}
