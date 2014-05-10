#include <ui.h>
#include <display.h>
#include <bitmaps/bitmaps.h>
#include <exposure.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mymemset.h>
#include <assert.h>

#include <basic_serial/basic_serial.h>

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

#define ms global_meter_state
#define tms global_transient_meter_state

void ui_top_status_line_at_6col(ui_top_status_line_state_t *func_state,
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
        if (ms.meter_mode == METER_MODE_REFLECTIVE) {
            display_bwrite_8px_char(CHAR_8PX_R, out, pages_per_col, 0);
        }
        else { // METER_MODE_INCIDENT
            display_bwrite_8px_char(CHAR_8PX_I, out, pages_per_col, 0);
        }
    }

    // We're now behind by two pixels from the right edge of the display.

    //
    // ISO
    //

    uint8_t iso_start_x = DISPLAY_LCDWIDTH - (ms.bcd_iso_length << 2) - (ms.bcd_iso_length << 1) - (4*CHAR_WIDTH_8PX);

    if (x >= iso_start_x - 2 && x < DISPLAY_LCDWIDTH) {
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

            if (di < ms.bcd_iso_length) {
                uint8_t digit = ms.bcd_iso_digits[di];
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
            for (j = 5, o = out + pages_per_col; j >= 4; --j, o -= pages_per_col)
                *o = func_state->charbuffer[j];
        }

        memset8_zero(func_state->charbuffer, sizeof(func_state->charbuffer));
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

void ui_main_reading_display_at_8col(ui_main_reading_display_state_t *func_state,
                                     uint8_t *out,
                                     uint8_t pages_per_col,
                                     uint8_t x)
{
    const uint8_t VOFFSET = 5;

    if (func_state->len == 0) { // State is unitialized; initialize it.
        // Copy because volatile.
        func_state->exposure_ready = tms.exposure_ready;
        if (! func_state->exposure_ready)
            return;

        uint8_t ss = tms.shutter_speed.ev;
        shutter_speed_to_string(ss, &(func_state->shutter_speed_string));
        aperture_to_string(tms.aperture, &(func_state->aperture_string), PRECISION_MODE_TENTH); // TODO PRECISION MODES

        // Total number of chars is the sum of the two plus one for a space plus one for
        // the 'f' we insert before the aperture.
        func_state->len = func_state->shutter_speed_string.length + func_state->aperture_string.length + 2;
        func_state->start_x = (DISPLAY_LCDWIDTH >> 1) - ((func_state->len << 3) >> 1);
        func_state->i = 0;
    }

    if (! func_state->exposure_ready)
        return;

    if (x >= func_state->start_x && func_state->i < func_state->len) {
        if (func_state->i < func_state->shutter_speed_string.length) {
            const uint8_t *shutstr = SHUTTER_STRING_OUTPUT_STRING(func_state->shutter_speed_string);
            uint8_t ascii = shutstr[func_state->i];
            display_bwrite_12px_char(ssa_get_12px_grid(ascii), out, pages_per_col, VOFFSET);
        }
        else if (func_state->i == func_state->shutter_speed_string.length + 1) {
            display_bwrite_12px_char(CHAR_12PX_F, out, pages_per_col, VOFFSET);
        }
        else if (func_state->i >= func_state->shutter_speed_string.length + 2) {
            const uint8_t *apstr = APERTURE_STRING_OUTPUT_STRING(func_state->aperture_string);
            uint8_t ascii = apstr[func_state->i - func_state->shutter_speed_string.length - 2];
            display_bwrite_12px_char(ssa_get_12px_grid(ascii), out, pages_per_col, VOFFSET);
       }

        ++(func_state->i);
    }
    else if (ms.precision_mode == PRECISION_MODE_TENTH &&
        func_state->i < func_state->len + 2) {
        // Write tenths if any.
    }
}

static void write_eighths_8px_chars(uint8_t *digits, uint8_t eighths)
{
    //    assert(eighths < 8 && eighths > 0);
    if (eighths == 1 || eighths == 2 || eighths == 4)
        digits[0] = CHAR_8PX_1_O;
    else if (eighths == 3 || eighths == 6)
        digits[0] = CHAR_8PX_3_O;
    else if (eighths == 5)
        digits[0] = CHAR_8PX_5_O;
    else // if (eighths == 7)
        digits[0] = CHAR_8PX_7_O;

    digits[1] = CHAR_8PX_SLASH_O;

    if (eighths & 1)
        digits[2] = CHAR_8PX_8_O;
    else if (eighths == 4)
        digits[2] = CHAR_8PX_2_O;
    else
        digits[2] = CHAR_8PX_4_O;
}

void ui_bttm_status_line_at_6col(ui_bttm_status_line_state_t *func_state,
                                 uint8_t *out,
                                 uint8_t pages_per_col,
                                 uint8_t x)
{
    if (func_state->expcomp_chars[0] == 0) { // State is not initialized; initialize it.
        // Cache because volatile.
        func_state->exposure_ready = tms.exposure_ready;

        if (! func_state->exposure_ready)
            return;

        // Compute strings for fractional EV values according to precision mode.
        uint8_t ev = tms.last_ev_with_tenths.ev;
        uint8_t tenths = tms.last_ev_with_tenths.tenths;
        uint8_t eighths = ev & 0b111;
        if (ev < 5*8) {
            eighths = 8-eighths;

            func_state->ev_length = 2;
            func_state->ev_chars[0] = CHAR_8PX_MINUS_O;
            func_state->ev_chars[1] = CHAR_8PX_0_O + CHAR_OFFSET_8PX(5 - (ev >> 3));
            func_state->ev_chars_ = func_state->ev_chars;
        }
        else {
            //func_state->ev_length = 2;
            //func_state->ev_chars[0] = CHAR_8PX_7_O;
            //func_state->ev_chars[1] = CHAR_8PX_8_O;
            //func_state->ev_chars_ = func_state->ev_chars;
            func_state->ev_chars_ = uint8_to_bcd((ev >> 3) - 5, func_state->ev_chars, 3);
            func_state->ev_length = bcd_length_after_op(func_state->ev_chars, 3, func_state->ev_chars_);
            uint8_t i;
            for (i = 0; i < func_state->ev_length; ++i)
                func_state->ev_chars_[i] = CHAR_OFFSET_8PX(func_state->ev_chars_[i]) + CHAR_8PX_0_O;
        }

        uint8_t l = func_state->ev_length;
        if (ms.precision_mode == PRECISION_MODE_EIGHTH && eighths != 0 && eighths != 8) {
            func_state->ev_chars_[l++] = CHAR_8PX_PLUS_O;
            write_eighths_8px_chars(func_state->ev_chars_ + l, eighths);
            l += 3;
        }
        else if (ms.precision_mode == PRECISION_MODE_TENTH && tenths != 0 && tenths != 10) {
            func_state->ev_chars_[l++] = CHAR_8PX_PERIOD_O;
            func_state->ev_chars_[l++] = CHAR_OFFSET_8PX(tenths) + CHAR_8PX_0_O;
        }

        func_state->ev_length = l;
    }

    if (! func_state->exposure_ready)
        return;

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
        func_state->expcomp_chars[i++] = CHAR_8PX_PLUS_O;
        write_eighths_8px_chars(func_state->expcomp_chars + i, eighths);
        i += 3;
    }

    func_state->expcomp_chars_length = i;
    func_state->start_x = DISPLAY_LCDWIDTH - (i << 2) - (i << 1);

    if (x == 0) {
        display_bwrite_8px_char(CHAR_8PX_E, out, pages_per_col, 0);
    }
    else if (x == 6) {
        display_bwrite_8px_char(CHAR_8PX_V, out, pages_per_col, 0);
    }
    else if (x >= 18 && x < 18 + (8*6)) {
        uint8_t off, j;
        for (off = 0, j = x; j > 18; j -= 6, ++off);

        if (off < func_state->ev_length) {
            uint8_t co = func_state->ev_chars_[off];
            display_bwrite_8px_char(CHAR_PIXELS_8PX + co, out, pages_per_col, 0);
        }
    }

    if (global_meter_state.exp_comp == 0)
        return;

    // We're now two pixels behind alignment with the right edge of the display.

    if (x >= func_state->start_x - 2) {
        // Write tail end of previous char if there was one.
        if (func_state->charbuffer_has_contents) {
            uint8_t j;
            uint8_t *o;
            for (j = 5, o = out + pages_per_col; j >= 4; --j, o -= pages_per_col)
                *o = func_state->charbuffer[j];
        }

        memset8_zero(func_state->charbuffer, sizeof(func_state->charbuffer));

        uint8_t index;
        for (index = 0; x > func_state->start_x - 2; x -= 6, ++index);

        uint8_t char_offset = func_state->expcomp_chars[index];
        display_bwrite_8px_char(CHAR_PIXELS_8PX + char_offset, func_state->charbuffer, 1, 0);
        func_state->charbuffer_has_contents = true;

        // Write beginning of that char.
        uint8_t j;
        uint8_t *o;
        for (j = 0, o = out + (pages_per_col << 1); j < 4; ++j, o += pages_per_col)
            *o = func_state->charbuffer[j];
    }
}
