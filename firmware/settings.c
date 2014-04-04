#include <avr/eeprom.h>
#include <stdint.h>
#include <stdbool.h>

#include <settings.h>

// EEPROM layout.
//
//     0-1  one little-endian word giving number of byte settings.
//     2-3  one little-endian word giving start address of next block (in bytes).
//     ...  pairs of bytes: (byte_setting_id, setting_value)

uint8_t read_byte_setting(byte_settings_names_t name)
{
    uint16_t n = eeprom_read_word((uint16_t *)0);

    uint16_t i;
    for (i = 4; i < (n << 1) + 4; i += 2) {
        if (eeprom_read_byte((uint8_t *)i) == name) {
            return eeprom_read_byte((uint8_t *)(i+1));
        }
    }
    return 0;
}

// Updates value if already exists, adds it otherwise. Returns true
// if value already existed or false otherwise.
bool write_byte_setting(byte_settings_names_t name, uint8_t value)
{
    uint16_t n = eeprom_read_word((uint16_t *)0);
    uint16_t next_block = eeprom_read_word((uint16_t *)1);

    uint16_t i;
    for (i = 4; i < (n << 1) + 4; i += 2) {
        if (eeprom_read_byte((uint8_t *)i) == name) {
            eeprom_write_byte((uint8_t *)(i+1), value);
            return true; // Value already existed
        }
    }

    if (i >= next_block) {
        // There's no space to write the value.
        return false;
    }

    // Value didn't already exist and there is space to write it, so do so.
    eeprom_write_byte((uint8_t *)i, name);
    eeprom_write_byte((uint8_t *)(i+1), value);
    ++n;
    eeprom_write_word((uint16_t *)0, n);
    return false;
}
