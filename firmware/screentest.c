#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <readbyte.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <bitmaps/bitmaps.h>

#include <display.h>
#include <ui.h>

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

    /*uint8_t out2[16];
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_0, out2, 2, 0);
    display_write_page_array(out2, 8, 2, 20, 4);
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_1, out2, 2, 1);
    display_write_page_array(out2, 8, 2, 28, 4);
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_2, out2, 2, 2);
    display_write_page_array(out2, 8, 2, 36, 4);
    memset(out2, 0, sizeof(out2));
    display_bwrite_8x8_char(CHAR_8PX_3, out2, 2, 3);
    display_write_page_array(out2, 8, 2, 44, 4);*/

    uint8_t i;
    uint8_t out[6];
    for (i = 0; DISPLAY_LCDWIDTH - i >= 6; i += 6) {
        memset(out, 0, sizeof(out));
        uint8_t offset = ui_top_status_line_at_6col(&global_meter_state, out, 1, i);
        display_write_page_array(out, 6, 1, i, 0);
        i += offset;
    }

    global_transient_meter_state.shutter_speed = 80;
    global_transient_meter_state.aperture = 80;

    uint8_t out2[24];
    size_t sz = ui_main_reading_display_at_12col_state_size();
    uint8_t state[sz];
    memset(state, 0, sz);
    for (i = 0; i < DISPLAY_LCDWIDTH; i += 12) {
        memset(out2, 0, sizeof(out2));
        ui_main_reading_display_at_12col(state, &global_meter_state, &global_transient_meter_state, out2, 2, i);
        display_write_page_array(out2, 12, 2, i, 3);
    }

    for (;;);
}

int main()
{
    initialize_global_meter_state();
    
    display_init();

    test_display();

    for (;;);
}
