#include <ui.h>
#include <display.h>
#include <bitmaps/bitmaps.h>
#include <exposure.h>
#include <stdlib.h>

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

uint8_t ui_top_status_line_at_6col(const meter_state_t *ms,
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
            display_bwrite_8px_char(CHAR_8PX_R, out, pages_per_col, 0);        
        } break;
        case METER_MODE_INCIDENT: {
            display_bwrite_8px_char(CHAR_8PX_I, out, pages_per_col, 0);
        } break;
        default: { // FOR TEST PURPOSES
            display_bwrite_8px_char(CHAR_8PX_0, out, pages_per_col, 0);
        } break;
        }

        // Now we can skip two pixels so that we're 6-aligned with the end of the display.
        return 2;
    }

    //
    // ISO
    //

    uint8_t iso_start_x = DISPLAY_LCDWIDTH - (ms->bcd_iso_length << 2) - (ms->bcd_iso_length << 1) - (4*CHAR_WIDTH_8PX);

    if (x >= iso_start_x && DISPLAY_LCDWIDTH - x >= 6) {
        const uint8_t *px_grid;

        bool dont_write = false;
        if (x == iso_start_x) {
            px_grid = CHAR_8PX_I;
        }
        else if (x == iso_start_x + 6) {
            px_grid = CHAR_8PX_S;
        }
        else if (x == iso_start_x + 12) {
            px_grid = CHAR_8PX_O;
        }
        else if (x >= iso_start_x + 24) {
            uint8_t di;
            for (di = 0; x > iso_start_x + 24; x -= 6, ++di);

            if (di < ms->bcd_iso_length) {
                uint8_t digit = ms->bcd_iso_digits[di];
                px_grid = CHAR_PIXELS_8PX + CHAR_8PX_0_O + CHAR_OFFSET_8PX(digit);
            }
            else {
                px_grid = NULL;
                dont_write = true;
            }
        }
        else {
            dont_write = true;
        }

        if (! dont_write)
            display_bwrite_8px_char(px_grid, out, pages_per_col, 0);

        return 0;
    }

    return 0;
}

// For shutter speeds and apertures (hence 'ssa').
static const uint8_t *ssa_get_12px_grid(uint8_t ascii)
{
    uint8_t m;
    if (ascii >= '0' && ascii <= '9') {
        m = '0';
        goto diff;
    }
    else if (ascii >= 'A' && ascii <= 'Z') {
        m = 'A';
        goto diff;
    }
    else if (ascii >= 'a' && ascii <= 'z') {
        m = 'a';
        goto diff;
    }
    else if (ascii == '/') {
        return CHAR_12PX_SLASH;
    }
    else {
        // Should never get here. Useful for running test code before full
        // character set has been created. (The 0 is just a dummy char,
        // could have chosen anything.)
        return CHAR_12PX_0;
    }

diff:
    return CHAR_12PX_GRIDS + CHAR_12PX_0_O + CHAR_OFFSET_12PX(ascii - m);
}

typedef struct main_reading_state {
    shutter_string_output_t sso;
    aperture_string_output_t aso;
    uint8_t len;
    uint8_t start_x;
    uint8_t i;
} main_reading_state_t;
size_t ui_main_reading_display_at_8col_state_size() { return sizeof(main_reading_state_t); }
void ui_main_reading_display_at_8col(void *func_state_,
                                      const meter_state_t *ms,
                                      const transient_meter_state_t *tms,
                                      uint8_t *out,
                                      uint8_t pages_per_col,
                                      uint8_t x)
{
    main_reading_state_t *func_state = func_state_;

    if (func_state->sso.length == 0) { // State is unitialized; initialize it.
        shutter_speed_to_string(tms->shutter_speed, &(func_state->sso));
        aperture_to_string(tms->aperture, &(func_state->aso));
        // Total number of chars is the sum of the two plus one for a space plus one for
        // the 'f' we insert before the aperture.
        func_state->len = func_state->sso.length + func_state->aso.length + 2;
        func_state->start_x = (DISPLAY_LCDWIDTH >> 1) - ((func_state->len << 3) >> 1);
        func_state->i = 0;
    }

    if (x >= func_state->start_x && func_state->i < func_state->len) {
        if (func_state->i < func_state->sso.length) {
            uint8_t ascii = SHUTTER_STRING_OUTPUT_STRING(func_state->sso)[func_state->i];
            display_bwrite_12px_char(ssa_get_12px_grid(ascii), out, pages_per_col, 0);
        }
        else if (func_state->i == func_state->sso.length + 1) {
            display_bwrite_12px_char(CHAR_12PX_F, out, pages_per_col, 0);
        }
        else if (func_state->i >= func_state->sso.length + 2) {
            uint8_t ascii = APERTURE_STRING_OUTPUT_STRING(func_state->aso)[func_state->i - func_state->sso.length - 2];
            display_bwrite_12px_char(ssa_get_12px_grid(ascii), out, pages_per_col, 0);
       }

        ++(func_state->i);
    }
}
