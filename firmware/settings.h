#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum byte_settings_names {
    BYTE_SETTING_ADC_TEMPERATURE_OFFSET
} byte_settings_names_t;

uint8_t read_byte_setting(byte_settings_names_t name);
bool write_byte_setting(byte_settings_names_t name, uint8_t value);

#endif
