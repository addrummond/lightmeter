#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <state.h>

void ui_top_status_line_at_8col(const meter_state_t *ms,
                                uint8_t *out,
                                uint8_t pages_per_col,
                                uint8_t x);

#endif
