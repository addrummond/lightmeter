typedef enum byte_settings_names {
    BYTE_SETTING_TEMPERATURE_OFFSET
} byte_settings_names_t;

uint8_t read_byte_setting(byte_settings_names_t name, uint8_t values);
bool write_byte_setting(byte_settings_names_t name, uint8_t value)
