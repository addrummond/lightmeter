# See state.h.

from intelhex import IntelHex

eeprom = [
    # Initial unused 4 bytes.
    0, 0, 0, 0,

    1, 0, 0, 0, 0, 0, 0,                                           # bcd_iso
    3,                                                             # bcd_iso_length
    32,                                                            # iso 100

    1,                                                             # shutter_priority

    0,                                                             # exp comp

    0,                                                             # UI_MODE_DEFAULT
    0,                                                             # METER_MODE_REFLECTIVE.
    10,                                                            # PRECISION_MODE_TENTH

    48,                                                            # priority_aperture = f8
    64                                                             # priority_shutter_speed = 1/4
]

ih = IntelHex()
for i in xrange(0, len(eeprom)):
    ih[i] = eeprom[i]
ih.write_hex_file('/tmp/lightmeter.eep')
