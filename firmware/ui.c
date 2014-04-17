#include <ui.h>
#include <display.h>
#include <bitmaps/bitmaps.h>

//
// Modularizing the display code takes a little bit of care when
// we're not buffering the entire screen area. We will often want
// have multiple ui widgets drawing to the same page. E.g., the
// top status line is 12px high, so that other ui widgets might
// want to draw on the remaining 4px of the second page. The basic
// strategy here is to have a function for each widget which
// renders some number of columns of the widget at the specified x value.
// In cases where multiple widgets are being displayed on overlapping
// pages, the corresponding functions can then each be called in
// each iteration of a loop.
//

static uint8_t iso_digits[] = { 1, 2, 3 };
static uint8_t iso_length = 3;
void ui_top_status_line_at_8col(const meter_state_t *ms,
                                uint8_t *out,
                                uint8_t pages_per_col,
                                uint8_t x)
{
    //
    // * Incident/reflective symbol (top left).
    // * ISO (top right).
    //


    //
    // Incident/reflective symbol.
    //
    if (x == 0) {
        switch (ms->meter_mode) {
        case METER_MODE_REFLECTIVE: {
            display_bwrite_8x8_char(CHAR_8PX_0, out, pages_per_col, 0);        
        } break;
        case METER_MODE_INCIDENT: {
            display_bwrite_8x8_char(CHAR_8PX_1, out, pages_per_col, 0);
        } break;
        default: { // FOR TEST PURPOSES
            display_bwrite_8x8_char(CHAR_8PX_2, out, pages_per_col, 0);
        } break;
        }

        return;
    }

    //
    // ISO
    //

    uint8_t iso_start_x = DISPLAY_LCDWIDTH - (iso_length << 3); //(ms->bcd_iso_length << 3/* *8 */);

    if (x >= iso_start_x) {
        uint8_t digit_index = (x - iso_start_x) >> 3;
        if (digit_index >= iso_length)
            return;
        uint8_t digit = iso_digits[digit_index];//ms->bcd_iso_digits[(x - iso_start_x) >> 3];
        const uint8_t *digit_px_grid = CHAR_PIXELS_8PX + CHAR_8PX_0_O + (digit << 3);

        display_bwrite_8x8_char(digit_px_grid, out, pages_per_col, 0);

        return;
    }
}
