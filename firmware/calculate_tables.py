import math
import sys

##########
# Configuration values.
##########

reference_voltage                = 5000.0  # mV
op_amp_normal_resistor           = 220.0   #kOhm
op_amp_low_light_resistor        = 220.0   #kOhm
# Table cells not calculated for voltages lower than this.
# This is because in the region just above v = 0, the changes
# between different voltage values lead to large discontinuities
# in EV values which the table compression mechanism can't handle.
voltage_offset                   = 150     # mV

# The graph in measurem.pdf is not for the BPW34.
# The BPW34 has a 20mV higher OCV listed on its data sheet compared
# to the diode graphed in measurem.pdf. However, adding a 20mV
# offset here appears to decrease rather than increase accuracy.
irradience_constant_adjustment = 0.0

##########


#
# EV table.
#

bv_to_voltage = ((1/256.0) * reference_voltage)

# http://www.vishay.com/docs/81521/bpw34.pdf, p. 2 Fig 2
# Temp in C.
def temp_to_rrlc(temp): # Temperature to relative reverse light current
    return 1.0 # TODO

# Log10 reverse light current microamps to log10 lux.
# http://www.vishay.com/docs/81521/bpw34.pdf, p. 3 Fig 4
# Useful link for calculating equations from lines (I *always* mess this up doing it by hand):
# http://www.mathportal.org/calculators/analytic-geometry/two-point-form-calculator.php
def rlc_to_lux(rlc):
    k = -1.254
    slope = 1.054
    return (rlc - k) / slope

# Convert log10 lux to EV at ISO 100.
# See http://stackoverflow.com/questions/5401738/how-to-convert-between-lux-and-exposure-value
def lux_to_ev_at_100(lux):
    # TODO: Could probably do this all in log space for greater accuracy.
    lux = math.pow(10, lux)
    ev = math.log(lux/2.5, 2)
    return ev

# Voltage (mV) and op amp resitor value (kOhm) to EV at the reference temp,
# which we see from Fig 2 on p.2 of http://www.vishay.com/docs/81521/bpw34.pdf
# is 40C.
def voltage_and_oa_resistor_to_ev(v, r):
    # v = ir => i = v/r
    v /= 1000.0 # v, mV -> V
    r *= 1000.0 # kOhm -> ohm

    if v < 1e-06:
        v = 1e-06

    v = math.log(v, 10)
    r = math.log(r, 10)
    i = v - r

    # Get i in microamps.
    i += math.log(1000000, 10)

    lux = rlc_to_lux(i)
    ev = lux_to_ev_at_100(lux)
    return ev

# Temperature, EV and voltage are all represented as unsigned 8-bit
# values on the microcontroller.
#
# Temperature: from -51C to 51C in 0.4C intervals (0 = -51C)
# Voltage: from 0 up in 1/256ths of the reference voltage.
# EV: from -5 to 26EV in 1/8 EV intervals.
#
# The table is a 2D array of bytes mapping (temperature, voltage) to
# EV*8.  Temperature goes up in steps of 16 (in terms of the 8-bit
# value).  Voltage goes up in steps of 1 (in terms of the 8-bit
# value).
#
# The resulting table would take up (256/1)*(256/16) = 4096 bytes =
# 4K, which would just about fit in the attiny85's 8K of
# flash. However, to make the table more compact, each row (series of
# voltages) is stored with the first 8-bit value followed by a series
# of (compacted) 1-bit values, where each 1-bit value indicates the
# (always positive) difference with the previous value.
#
# The initial idea was to use the following layout for each row. 16
# groups of three bytes, where the first byte in each group is an
# absolute EV value and the next two bytes give 1-bit
# differences. (For simplicity, the first bit is always 0, i.e., it
# gives the difference of abs_ev with abs_ev+0).  The value of any
# given cell can therefore be determined by doing at most 15
# additions. Each addition takes 1 clock cycle. If we conservatively
# assume 10 cycles for each iteration of the loop, that's 150 cycles,
# which at 8MHz is about 1/150th of a second.
#
# In this layout each row is 48 bytes, meaning that the table as a whole takes
# up a more managable 768 bytes.
#
#             voltage
#               (0)                 (16)
#  temp  (0)    ev  diffs diffs     ev   diffs diffs ...
#        (16)   ev  diffs diffs     ev   diffs diffs ...
#        (32)   ev  diffs diffs     ev   diffs diffs ...
#
# There are only 14 distinct diffs 8-bit patterns. We can therefore
# replace each [diffs diffs] sequence with the sequence [ev i], where
# i is a single byte containing two 4-bit indices (one for each 8-bit
# bit pattern) into an array of 14 bytes. This gives us a table where
# each row is 32 bytes, so together with the 12-byte bit pattern
# array, the entire thing takes up 524 bytes.
#
# Finally, since the array itself now takes up 512 bytes, we can split
# it into two arrays (one for the absolute value bytes and one for the
# diff bit pattern index bytes) so that each separate array can be
# indexed by a single byte.

def output_temp_table():
    sys.stdout.write("const int8_t TEMP_EV_ADJUST[] PROGMEM = {\n");
    # TODO: currently specify no adjustment for any temp
    for t in xrange(0,256,4):
        sys.stdout.write("0,")
    sys.stdout.write("};\n")

def output_ev_table(level): # level == 'NORMAL' or level == 'LOW'
    bitpatterns = [ ]
    vallist_abs = [ ]
    vallist_diffs = [ ]
    oar = op_amp_normal_resistor
    if level == 'LOW':
        oar = op_amp_low_light_resistor
    for sv in xrange(0, 256, 16):
        # Write the absolute 8-bit EV value.
        voltage = (sv * bv_to_voltage) + voltage_offset
        assert voltage >= 0
        if voltage > reference_voltage:
            break
        ev = voltage_and_oa_resistor_to_ev(voltage, oar)
        eight = int(round((ev+5.0) * 8.0))
        if eight < 0:
            eight = 0

        # Write the 1-bit differences (two bytes).
        prev = eight
        num = ""
        bpis = [ None, None ]
        for j in xrange(0, 2):
            eight2 = None
            o = ""
            for k in xrange(0, 8):
                v = ((sv + (j * 8.0) + k) * bv_to_voltage) + voltage_offset
                ev2 = voltage_and_oa_resistor_to_ev(v, oar)
                eight2 = int(round((ev2+5.0) * 8.0))
                if eight2 < 0:
                    eight2 = 0
#                print ">>>", eight2, prev
                assert eight2 - prev == 0 or not (j == 0 and k == 0)
                assert eight2 - prev <= 1
                assert eight2 - prev >= 0
                if eight2 - prev == 0:
                    o += '0'
                elif eight2 - prev == 1:
                    o += '1'
                else:
                    assert False
                prev = eight2
            ix = None
            try:
                ix = bitpatterns.index(o)
            except ValueError:
                bitpatterns.append(o)
                ix = len(bitpatterns)-1
            assert ix < 16
            bpis[j] = ix

        vallist_abs.append(eight)
        vallist_diffs.append(bpis[0] << 4 | bpis[1])

#    for v in xrange(0, 16):
#        for t in xrange(0, 256, 16):
#            sys.stderr.write(str(vallist_abs[t + v]) + " ")
#        sys.stderr.write("\n")

    sys.stdout.write('const uint8_t ' + level + '_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[] PROGMEM = {\n')
    for p in bitpatterns:
        sys.stdout.write('0b%s,' % p)
    sys.stdout.write('\n};\n')

    sys.stdout.write('const uint8_t ' + level + '_LIGHT_VOLTAGE_TO_EV_ABS[] PROGMEM = {')
    for i in xrange(len(vallist_abs)):
        if i % 32 == 0:
            sys.stdout.write('\n    ');
        sys.stdout.write('%i,' % vallist_abs[i])
    sys.stdout.write('\n};\n')
    sys.stdout.write('const uint8_t ' + level + '_LIGHT_VOLTAGE_TO_EV_DIFFS[] PROGMEM = {')
    for i in xrange(len(vallist_diffs)):
        if i % 32 == 0:
            sys.stdout.write('\n    ');
        sys.stdout.write('%i,' % vallist_diffs[i])
    sys.stdout.write('\n};\n')

# This is useful for santiy checking calculations. It outputs a graph of
# amplified voltage against EV which can be compared with voltage readings
# over the two input pins.
def output_sanity_graph():
    f = open("santitygraph.csv", "w")
    for v in xrange(0, 256):
        voltage = (v * bv_to_voltage) + voltage_offset
        ev = voltage_and_oa_resistor_to_ev(voltage, op_amp_normal_resistor)
        f.write("%f,%f\n" % (voltage, ev))
    f.close()

# Straight up array that we use to test that the bitshifting logic is
# working correctly. (Test will currently only be performed for normal light table.)
def output_test_table():
    sys.stdout.write('    { ')
    for v in xrange(0, 256):
        voltage = (v * bv_to_voltage) + voltage_offset
#            sys.stderr.write('TAVY ' + str(temperature) + ',' + str(voltage) + '\n')
        ev = voltage_and_oa_resistor_to_ev(voltage, op_amp_normal_resistor)
        eight = int(round((ev+5.0) * 8.0))
        sys.stdout.write("%i," % eight)
    sys.stdout.write('\n')
    sys.stdout.write('    }');


#
# Shutter speed and aperture tables.
#

#
# Shutter speeds are represented using 13 characters: 0-9, '/', '+' and 'S'.
# E.g.:
#
#     1/8    One eigth of a second
#     5s     Five seconds
#     1s+1/4 One and one quarter seconds.
#
# We want a way to represent each character using 4 bits. There are max 5
# characters per speed, so we have a total of (168*20)/8 = 420 bytes.
#
# Special case chars:
#
#     A -> 16
#     B -> 32
#
shutter_speeds_bitmap = [
    None, # Zero is reserved as a terminator for strings < 5 chars.
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '/', '+', '-',
    'A', # 14
    'B', # 15
]
assert len(shutter_speeds_bitmap) <= 16
shutter_speeds = [
    '1', ### FROM HERE ....
    '57',
    '53',
    '49',
    '45',
    '42',
    '38',
    '34',
    '30', ###
    '28',
    '26',
    '24',
    '23',
    '21',
    '19',
    '17',
    '15', ### ... TO HERE IS A MESS: CORRECT
    '15',
    '14',
    '13',
    '12',
    '11',
    '10',
    '9',
    '8',  ###
    '8-/4', # 'S' preceding +/- is not included.
    '7+/2', 
    '7',
    '6',
    '5+/2',
    '5',
    '4+/2',
    '4', ###
    '3+/2', # can't display this differently
    '3+/2',  
    '3+/4',
    '3',
    '2+/4', # can't display this differently.
    '2+/2',
    '2+/4',
    '2', ###
    '2',
    '2-/4',
    '1+/2',   # TODO: Check this region again, looks wrong.
    '1+/2',
    '1+/2',
    '1+/4',
    '1+/8',
    '1', ###
    '1',
    '1',
    '1',
    '3/4',   # TODO Maybe add some extra detail in this region.
    '3/4',
    '3/4',
    '3/4',
    '/2', ###
    '/2', # No sensible way of displaying this differently.
    '/2-/A', # i.e. 7/16. special case, A -> 16
    '/4+/8', # No sensible way of displaying this differently. 
    '/4+/8', # i.e. 3/8 
    '/4+/A', # No sensible way of displaying this differently.
    '/4+/A', # i.e. 5/16
    '/4',    # No sensible way of displaying this differently.
    '/4', ###
    '/4',    # |
    '/8+/A', # | No sensible way of displaying these differently.
    '/8+/A', # |
    '/8+/A', # i.e. 3/16
    '/8+/A',
    '/8+/B', # special case, B -> 32
    '/8',
    '/8', ###
    '/9',
    '/10',
    '/11',
    '/12',
    '/13',
    '/14',
    '/15', # special case because the next speed should really be 1/16.
    '/15', ###
    '/17',
    '/19',
    '/21',
    '/23',
    '/25',
    '/27',
    '/28',
    '/30', ###
    '/34',
    '/38',
    '/42',
    '/45',
    '/49',
    '/53',
    '/56',
    '/60', ### 
    '/68',
    '/75',
    '/83',
    '/90',
    '/98',
    '/105',
    '/113',
    '/125', ###
    '/141',
    '/156',
    '/172',
    '/188',
    '/204',
    '/219',
    '/235',
    '/250', ###
    '/281',
    '/313',
    '/344',
    '/375',
    '/406',
    '/438',
    '/469',
    '/500', ###
    '/563',
    '/625',
    '/688',
    '/750',
    '/813',
    '/875',
    '/938',
    '/100', ### # Last zero not included from this point on.
    '/130',
    '/125',
    '/138',
    '/150',
    '/163',
    '/175',
    '/188',
    '/200', ###
    '/225',
    '/250',
    '/275',
    '/300',
    '/325',
    '/350',
    '/375',
    '/400', ###
    '/450',
    '/500',
    '/550',
    '/600',
    '/650',
    '/700',
    '/750',
    '/800', ###
    '/900',
    '/1000',
    '/1100',
    '/1200',
    '/1300',
    '/1400',
    '/1500',
    '/1600', ### 
    '/1600',
    '/1600',
    '/1600',
    '/1600',
    '/1600',
    '/1600',
    '/1600'
]
assert(max(map(len, shutter_speeds)) == 5)

#
# Apertures are represented using unsigned 8-bit values. They go from
# 1.0 to 32 in 1/8th stop steps.
#
# The table is quarter-stop resolution only, since eighth-stop apertures are unwieldy.
# Instead, we display something like "1.1+1/8 stop" (for the f-stop 3/8ths of a stop above 1.0).
#
# To keep each entry two two characters, decimal points are not included. We rely on the fact
# that all two-digit apertures <= 9.6 have a decimal point in the middle while all other
# apertures do not.
#
# Each entry in the table is 1-byte (two 4-byte characters), so the whole table is 40 bytes.
#

apertures_bitmap = [
    None, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'X'
]
apertures = [
    '10', ###
    '11',
    '12',
    '13',
    '14', ###
    '15',
    '17',
    '18',
    '2', ###
    '22',
    '24',
    '26',
    '28', ###
    '31',
    '34',
    '37',
    '4', ###
    '44',
    '48',
    '52',
    '56', ###
    '62',
    '67',
    '73',
    '8', ###
    '87',
    '95',
    '10',
    '11', ###
    '12',
    '14',
    '15',
    '16', ###
    '17',
    '19',
    '21',
    '22', ###
    '24', # made up, TODO CHECK
    '27', # made up, TODO CHECK
    '30', # made up, TODO CHECK
    '32'
]

def output_shutter_speeds():
    sys.stdout.write('const uint8_t SHUTTER_SPEEDS[] PROGMEM = {\n')
    bits = []
    for speed in shutter_speeds:
        x = 0
        for c in speed:
            x += 1
            n = shutter_speeds_bitmap.index(c)
            bits.append((n & 8 and '1' or '0') + (n & 4 and '1' or '0') + (n & 2 and '1' or '0') + (n & 1 and '1' or '0'))
        for i in xrange(5 - x):
            bits.append('0000')
    for i in xrange(0, len(bits)-1, 2):
        if i < len(bits) - 1:
            sys.stdout.write('0b' + bits[i+1] + bits[i] + ',')
        else:
            sys.stdout.write('0b' + bits[i] + ',')
    sys.stdout.write('};\n')
#    sys.stdout.write('// ' + ' '.join(bits));
    sys.stdout.write('const uint8_t SHUTTER_SPEEDS_BITMAP[] PROGMEM = { ')
    for c in shutter_speeds_bitmap:
        if c is None:
            sys.stdout.write("'\\0',")
        else:
            sys.stdout.write("'%s'," % c) # None of the chars need escaping.
    sys.stdout.write('};\n')

def output_apertures():
    sys.stdout.write('const uint8_t APERTURES[] PROGMEM = {\n')
    for a in apertures:
        n1 = apertures_bitmap.index(a[0])
        n2 = apertures_bitmap.index(a[1]) if len(a) > 1 else 0
        sys.stdout.write('0b' +
                         (n2 & 8 and '1' or '0') + (n2 & 4 and '1' or '0') + (n2 & 2 and '1' or '0') + (n2 & 1 and '1' or '0') +
                         (n1 & 8 and '1' or '0') + (n1 & 4 and '1' or '0') + (n1 & 2 and '1' or '0') + (n1 & 1 and '1' or '0') +
                         ',')
    sys.stdout.write('};\n')
    sys.stdout.write('const uint8_t APERTURES_BITMAP[] PROGMEM = { ')
    for a in apertures_bitmap:
        if a is None:
            sys.stdout.write("'\\0',")
        else:
            sys.stdout.write("'%s'," % a)
    sys.stdout.write('};\n')

#
# Final output generation.
#

def output():
    output_sanity_graph()

    sys.stdout.write('#ifndef TABLES_H\n#define TABLES_H\n\n')
    sys.stdout.write("#include <stdint.h>\n")
    sys.stdout.write("#include <readbyte.h>\n")
    output_ev_table('NORMAL')
    output_ev_table('LOW')
    sys.stdout.write('const uint8_t VOLTAGE_TO_EV_ABS_OFFSET = ' + str(int(round((voltage_offset/reference_voltage)*256))) + ';\n')
    output_temp_table()
    sys.stdout.write('\n#ifdef TEST\n')
    sys.stdout.write('const uint8_t TEST_VOLTAGE_TO_EV[] PROGMEM =\n')
    output_test_table()
    sys.stdout.write(';\n#endif\n')
    output_shutter_speeds()
    output_apertures()
    sys.stdout.write('\n#endif')

if __name__ == '__main__':
    output()



