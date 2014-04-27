#include <ui.h>
#include <display.h>
#include <bitmaps/bitmaps.h>
#include <exposure.h>
#include <stdlib.h>
#include <stdbool.h>

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

typedef struct ui_top_status_line_state {
    uint8_t charbuffer[6];
    bool charbuffer_has_contents;
} ui_top_status_line_state_t;
size_t ui_top_status_line_at_6col_state_size() { return sizeof(ui_top_status_line_state_t); }
void ui_top_status_line_at_6col(void *func_state_,
                                const meter_state_t *ms,
                                uint8_t *out,
                                uint8_t pages_per_col,
                                uint8_t x)
{
    //
    // * Incident/reflective symbol (top left).
    // * ISO (top right).
    //

    ui_top_status_line_state_t *func_state = func_state_;

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
    }

    // We're now behind by two pixels from the right edge of the display.

    //
    // ISO
    //

    uint8_t iso_start_x = DISPLAY_LCDWIDTH - (ms->bcd_iso_length << 2) - (ms->bcd_iso_length << 1) - (4*CHAR_WIDTH_8PX);

    if (x == iso_start_x - 2 && DISPLAY_LCDWIDTH - x >= 6) {
        const uint8_t *px_grid;

        bool dont_write = false;
        if (x == iso_start_x - 2) {
            px_grid = CHAR_8PX_I;
        }
        else if (x == iso_start_x - 2 + 6) {
            px_grid = CHAR_8PX_S;
        }
        else if (x == iso_start_x - 2 + 12) {
            px_grid = CHAR_8PX_O;
        }
        else if (x >= iso_start_x - 2 + 24) {
            uint8_t di;
            for (di = 0; x > iso_start_x - 2 + 24; x -= 6, ++di);

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

        // Write tail end of last char if there was one.
        if (func_state->charbuffer_has_contents) {
            uint8_t j;
            uint8_t *o;
            for (j = 5, o = out; j >= 4; --j, o += pages_per_col)
                *o = func_state->charbuffer[j];
        }

        if (! dont_write) {
            display_bwrite_8px_char(px_grid, func_state->charbuffer, 1, 0);
            func_state->charbuffer_has_contents = true;

            // Write beginning of that char.
            uint8_t j;
            uint8_t *o;
            for (j = 0, o = out + (pages_per_col << 1); j < 4; ++j, o += pages_per_col)
                *o = func_state->charbuffer[j];
        }
    }
}

// For shutter speeds and apertures (hence 'ssa').
static const uint8_t *ssa_get_12px_grid(uint8_t ascii)
{
    uint8_t m;
    if (ascii >= '0' && ascii <= '9') {
        m = '0';
        goto diff;
    }
    else if (ascii == '/') {
        return CHAR_12PX_SLASH;
    }
    else if (ascii == 'F') {
        return CHAR_12PX_F;
    }
    else if (ascii == '.') {
        return CHAR_12PX_PERIOD;
    }
    else if (ascii == '+') {
        return CHAR_12PX_PLUS;
    }
    else if (ascii == '-') {
        return CHAR_12PX_MINUS;
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
    if (! tms->exposure_ready)
        return;
    
    const uint8_t VOFFSET = 5;
    main_reading_state_t *func_state = func_state_;

    if (func_state->len == 0) { // State is unitialized; initialize it.
        // Total number of chars is the sum of the two plus one for a space plus one for
        // the 'f' we insert before the aperture.
        func_state->len = tms->shutter_speed_string.length + tms->aperture_string.length + 2;
        func_state->start_x = (DISPLAY_LCDWIDTH >> 1) - ((func_state->len << 3) >> 1);
        func_state->i = 0;
    }

    if (x >= func_state->start_x && func_state->i < func_state->len) {
        if (func_state->i < tms->shutter_speed_string.length) {
            uint8_t ascii = SHUTTER_STRING_OUTPUT_STRING(tms->shutter_speed_string)[func_state->i];
            display_bwrite_12px_char(ssa_get_12px_grid(ascii), out, pages_per_col, VOFFSET);
        }
        else if (func_state->i == tms->shutter_speed_string.length + 1) {
            display_bwrite_12px_char(CHAR_12PX_F, out, pages_per_col, VOFFSET);
        }
        else if (func_state->i >= tms->shutter_speed_string.length + 2) {
            uint8_t ascii = APERTURE_STRING_OUTPUT_STRING(tms->aperture_string)[func_state->i - tms->shutter_speed_string.length - 2];
            display_bwrite_12px_char(ssa_get_12px_grid(ascii), out, pages_per_col, VOFFSET);
       }

        ++(func_state->i);
    }
}

typedef struct bttm_status_line_state {
    // Max length: "+14 1/8"
    uint8_t expcomp_chars[7];
    uint8_t expcomp_chars_length;
    uint8_t start_x;
} bttm_status_line_state_t;
size_t ui_bttm_status_line_at_6col_state_size() { return sizeof(bttm_status_line_state_t); }
uint8_t ui_bttm_status_line_at_6col(void *func_state_,
                                    const meter_state_t *ms,
                                    uint8_t *out,
                                    uint8_t pages_per_col,
                                    uint8_t x)
{
    if (global_meter_state.exp_comp == 0)
        return 0;

    bttm_status_line_state_t *func_state = func_state_;

    if (func_state->expcomp_chars[0] == 0) { // State is not initialized; initialize it.
        // Exposure compensation is stored in 1/8 stops in an int8_t.
        // This means that if we ignore the eighths, it can only be up to +/- 16 stops.
        // We can therefore avoid doing a proper conversion to BCD,
        // since it's easy to convert binary values from 0-16 to a two-digit decimal.

        uint8_t i = 0;

        int8_t exp_comp = global_meter_state.exp_comp;
        if (exp_comp & 128) { // It's negative.
            exp_comp = ~exp_comp + 1;
            func_state->expcomp_chars[i++] = CHAR_8PX_MINUS_O;
        }
        else {
            func_state->expcomp_chars[i++] = CHAR_8PX_PLUS_O;
        }

        uint8_t full_comp = global_meter_state.exp_comp >> 3;
        uint8_t full_d1 = 0, full_d2 = 0;
        if (full_comp > 9) {
            full_d1 = 1;
            full_comp -= 10;
        }
        full_d2 = full_comp;

        if (full_d1)
            func_state->expcomp_chars[i++] = CHAR_8PX_0_O + CHAR_OFFSET_8PX(full_d1);
        func_state->expcomp_chars[i++] = CHAR_8PX_0_O + CHAR_OFFSET_8PX(full_d2);

        uint8_t eighths = global_meter_state.exp_comp & 0b111;
        if (eighths) {
            func_state->expcomp_chars[i++] = 255; // We use this special value to represent a space.

            switch (eighths) {
            case 1: case 2: case 4: {
                func_state->expcomp_chars[i++] = CHAR_8PX_1_O;
            } break;
            case 3: case 6: {
                func_state->expcomp_chars[i++] = CHAR_8PX_3_O;
            } break;
            case 5: {
                func_state->expcomp_chars[i++] = CHAR_8PX_5_O;
            } break;
            case 7: {
                func_state->expcomp_chars[i++] = CHAR_8PX_7_O;                                                    
            } break;
            }

            func_state->expcomp_chars[i++] = CHAR_8PX_SLASH_O;

            if (eighths & 1)
                func_state->expcomp_chars[i++] = CHAR_8PX_8_O;
            else if (eighths == 4)
                func_state->expcomp_chars[i++] = CHAR_8PX_2_O;
            else
                func_state->expcomp_chars[i++] = CHAR_8PX_4_O;
        }

        func_state->expcomp_chars_length = i;
        func_state->start_x = DISPLAY_LCDWIDTH - (i << 2) - (i << 1);

        // Skip two pixels so that we're 6-aligned with the right edge of the display.
        return 2;
    }

    if (x >= func_state->start_x) {
        uint8_t index;
        for (index = 0; x > func_state->start_x; x -= 6, ++index);

        uint8_t char_offset = func_state->expcomp_chars[index];
        if (char_offset != 255 /*space*/) {
            display_bwrite_8px_char(CHAR_PIXELS_8PX + char_offset, out, pages_per_col, 0);
        }
    }

    return 0;
}
