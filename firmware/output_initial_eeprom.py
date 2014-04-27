# See state.h.

from intelhex import IntelHex

eeprom = [
    # Initial unused 4 bytes.
    0, 0, 0, 0,

    1, 0, 0, 0, 0, 0, 0,                                           # bcd_iso
    3,                                                             # bcd_iso_length
    38,                                                            # iso 100

    1,                                                             # shutter_priority

    9,                                                             # exp comp

    1,                                                             # first op amp resistor stage.

    0,                                                             # UI_MODE_DEFAULT

    0,                                                             # METER_MODE_REFLECTIVE
]

ih = IntelHex()
for i in xrange(0, len(eeprom)):
    ih[i] = eeprom[i]
ih.write_hex_file('/tmp/lightmeter.eep')
