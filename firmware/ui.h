#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stddef.h>
#include <state.h>

void ui_show_interface();

typedef struct ui_top_status_line_state {
    uint8_t charbuffer[6];
    bool charbuffer_has_contents;
} ui_top_status_line_state_t;
void ui_top_status_line_at_6col(ui_top_status_line_state_t *func_state,
                                uint8_t *out,
                                uint8_t pages_per_col,
                                uint8_t x);

typedef struct ui_main_reading_display_state {
    uint8_t len;
    uint8_t start_x;
    uint8_t i;

    bool exposure_ready;

    aperture_string_output_t aperture_string;
    shutter_string_output_t shutter_speed_string;
} ui_main_reading_display_state_t;
void ui_main_reading_display_at_8col(ui_main_reading_display_state_t *func_state,
                                     uint8_t *out,
                                     uint8_t pages_per_col,
                                     uint8_t x);

typedef struct ui_bttm_status_line_state {
    // EV chars. TODO: Currently we're just displaying EV at ISO 100. If we
    // keep this for the final product, it should display the EV at the current
    // ISO.
    uint8_t ev_chars[7]; // Max length example: "-10 1/8"
    uint8_t ev_length;

    uint8_t expcomp_chars[7]; // Max length example: "+14 1/8"
    uint8_t expcomp_chars_length;
    uint8_t start_x;

    uint8_t charbuffer[6];
    bool charbuffer_has_contents;

    bool exposure_ready;
} ui_bttm_status_line_state_t;
void ui_bttm_status_line_at_6col(ui_bttm_status_line_state_t *func_state,
                                 uint8_t *out,
                                 uint8_t pages_per_col,
                                 uint8_t x);

#endif
