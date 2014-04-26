# See state.h.

from intelhex import IntelHex

eeprom = [
    # Initial unused 4 bytes.
    0, 0, 0, 0,

    1, 0, 0, 0, 0, 0, 0,                                           # bcd_iso
    3,                                                             # bcd_iso_length
    38,                                                            # iso 100

    1,                                                             # shutter_priority

    48,                                                            # aperture (f8)
    72,                                                            # shutter_speed (1/15)
    
    0,                                                             # exp comp

    1,                                                             # first op amp resistor stage.

    ord('8'), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                        # aperture_string_output.chars
    1,                                                             # aperture_string_output.length
    ord('1'), ord('/'), ord('1'), ord('2'), ord('5'), 0, 0, 0, 0,  # shutter_string_ouput.chars
    5,                                                             # shutter_string_output.length

    0,                                                             # UI_MODE_DEFAULT

    0,                                                             # METER_MODE_REFLECTIVE
]

ih = IntelHex()
for i in xrange(0, len(eeprom)):
    ih[i] = eeprom[i]
ih.write_hex_file('/tmp/lightmeter.eep')
