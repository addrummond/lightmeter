import math
import sys

##########
# Configuration values.
##########

reference_voltage              = 5500.0 # mV
op_amp_gain                    = 56.4/2.0
irradience_constant_adjustment = 0.0

##########

#
# EV table.
#

bv_to_voltage = ((1/256.0) * reference_voltage) / op_amp_gain

# http://www.vishay.com/docs/81521/bpw34.pdf, Fig 1, p. 2.
# Temp in C, RDC in log10 pA.
# Slope is 1/16 with y in log10.
# Constant is 1.
def temp_to_rdc(temp):
    return (temp / 16.0) + 3.0

# Slope of all lines is 62.5 with log10 irrad.
# See http://www.vishay.com/docs/80085/measurem.pdf, Fig 19, p. 7.
def ocv_and_rdc_to_irrad(ocv, rdc):
    l1 = math.log(10, 10)
    l2 = math.log(100, 10)
    l3 = math.log(1000, 10)
    l4 = math.log(5000, 10)
    l5 = math.log(30000, 10)

    slope = 62.5 # Negative slope

    # -20.0 because graph in measurem.pdf is not for BPW34,
    # which has a 20mV higher OCV listed on data sheet.
    # UPDATE: Adding the -20 appears to make it less accurate;
    # it seems that the graph is more-or-less correct for the BPW34 too.
    ica = irradience_constant_adjustment
    l1Val = (ocv - 275.0 + ica) / slope
    l2Val = (ocv - 212.5 + ica) / slope
    l3Val = (ocv - 160.0 + ica) / slope
    l4Val = (ocv - 100.0 + ica) / slope
    l5Val = (ocv - 62.5  + ica) / slope

#    print l1Val, l2Val, l3Val, l4Val, l5Val

    k = None
    if rdc <= l1:
        return l1Val
    elif rdc <= l2:
        d = (l2 - rdc) / (l2 - l1)
        return (d * l2Val) + ((1.0-d) * l1Val)
    elif rdc <= l3:
        d = (l3 - rdc) / (l3 - l2)
        return (d * l3Val) + ((1.0-d) * l2Val)
    elif rdc <= l4:
        d = (l4 - rdc) / (l4 - l3)
        return (d * l4Val) + ((1.0-d) * l3Val)
    elif rdc <= l5:
        d = (l5 - rdc) / (l5 - l4)
        return (d * l5Val) + ((1.0-d) * l4Val)
    else:
        return l5Val

# We can get log10 illum (lux) from log10 irrad by adding 3.
# This can be inferred from figs 3 and 4 on p. 3 of http://www.vishay.com/docs/81521/bpw34.pdf
def irrad_to_lux(irrad):
    return irrad + 2.7

# Convert lux to EV at ISO 100.
# See http://stackoverflow.com/questions/5401738/how-to-convert-between-lux-and-exposure-value
def lux_to_ev_at_100(lux):
    # TODO: Could probably do this all in log space for greater accuracy.
    lux = math.pow(10, lux)
    return math.log(lux/2.5, 2)

def temp_and_voltage_to_ev(temp, v):
    rdc = temp_to_rdc(temp)
    irrad = ocv_and_rdc_to_irrad(v, rdc)
    lux = irrad_to_lux(irrad)
    return lux_to_ev_at_100(lux)

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
def output_table():
    bitpatterns = [ ]
    vallist_abs = [ ]
    vallist_diffs = [ ]
    for t in xrange(0, 256, 16):
        temperature = (t * 0.4) - 51.0

        for sv in xrange(0, 256, 16):
            # Write the absolute 8-bit EV value.
            voltage = sv * bv_to_voltage
            ev = temp_and_voltage_to_ev(temperature, voltage)
            eight = int(round(ev * 8))
            if sv == 0 and t != 0:
                sys.stdout.write("      ")

            # Write the 1-bit differences (two bytes).
            prev = eight
            num = ""
            bpis = [ None, None ]
            for j in xrange(0, 2):
                eight2 = None
                o = ""
                for k in xrange(0, 8):
                    v = (sv + (j * 8.0) + k) * bv_to_voltage
                    ev2 = temp_and_voltage_to_ev(temperature, v)
                    eight2 = int(round(ev2 * 8))
#                    sys.stderr.write('TAVX ' + str(temperature) + ',' + str(v) + "," + str(eight2) +'\n')
                    assert eight2 - prev == 0 or not (j == 0 and k == 0)
                    assert eight2 - prev <= 1
                    assert eight2 - prev >= 0
#                    sys.stderr.write(str(eight2 - prev) + "\n")
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

    sys.stdout.write('const uint8_t TEMP_AND_VOLTAGE_TO_EV_BITPATTERNS[] = {\n')
    for p in bitpatterns:
        sys.stdout.write('0b%s,' % p)
    sys.stdout.write('\n};\n')

    sys.stdout.write('const uint8_t TEMP_AND_VOLTAGE_TO_EV_ABS[] = {')
    for i in xrange(len(vallist_abs)):
        if i % 32 == 0:
            sys.stdout.write('\n    ');
        sys.stdout.write('%i,' % vallist_abs[i])
    sys.stdout.write('\n};\n')
    sys.stdout.write('const uint8_t TEMP_AND_VOLTAGE_TO_EV_DIFFS[] = {')
    for i in xrange(len(vallist_diffs)):
        if i % 32 == 0:
            sys.stdout.write('\n    ');
        sys.stdout.write('%i,' % vallist_diffs[i])
    sys.stdout.write('\n};\n')

def output_full_table_as_comment():
    sys.stdout.write('// EV @ ISO 100 for given temp (C) and voltage (mV)\n')
    sys.stdout.write('//      ')
    for t in xrange(0, 256, 16):
        temperature = (t * 0.4) - 51.0
        s = None
        if temperature < 0:
            s = "%02d  " % abs(temperature)
        else:
            s = "%02d  " % temperature
        if s[0] != '-':
            s = ' ' + s
        sys.stdout.write(s)
    sys.stdout.write('\n')
    for v in xrange(0, 256):
        voltage = v * bv_to_voltage
        sys.stdout.write("// %03d   " % voltage)
        for t in xrange(0, 256, 16):
            temperature = (t * 0.4) - 51.0
            ev = temp_and_voltage_to_ev(temperature, voltage)
            sys.stdout.write('%02d   ' % ev)
        sys.stdout.write('\n')

# Straight up array that we use to test that the bitshifting logic is
# working correctly.
def output_test_table():
    sys.stdout.write('    { ')
    for t in xrange(0, 256, 16):
        temperature = (t * 0.4) - 51.0
        for v in xrange(0, 256):
            voltage = v * bv_to_voltage
#            sys.stderr.write('TAVY ' + str(temperature) + ',' + str(voltage) + '\n')
            ev = temp_and_voltage_to_ev(temperature, voltage)
            eight = int(round(ev * 8))
            if v == 0 and t != 0:
                sys.stdout.write("     ")
            sys.stdout.write("%i," % eight)
        sys.stdout.write('\n')
    sys.stdout.write('    }');

def test_output():
#    irrad = ocv_and_rdc_to_irrad(0, temp_to_rdc(20))
#    lux = irrad_to_lux(irrad)
#    print "ZERO", irrad, lux, lux_to_ev_at_100(lux)
#    return

    for t in xrange(0, 256, 16):
        print "%f, %f" % (t * 0.4 - 51.0, temp_to_rdc(t * 0.4 - 51.0))
    print
    print
    for t in xrange(0, 256, 16):
        temperature = t * 0.4 - 51.0
        for v in xrange(0, 256, 8):
            voltage = v * bv_to_voltage
            print "T %f V %f EV %f" % (temperature, voltage, temp_and_voltage_to_ev(temperature, voltage))

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
shutter_speeds_bitmap = [
    None, # Zero is reserved as a terminator for strings < 5 chars.
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '/', '+', 'S', 'X', 'M'
]
shutter_speeds = [
    '1M', ###
    '56S',
    '53S',
    '49S',
    '45S',
    '41S',
    '38S',
    '34S',
    '39S',
    '28S',
    '27S',
    '25S',
    '23S',
    '21S',
    '29S',
    '17S',
    '15S', ###
    '15S',
    '14S',
    '13S',
    '12S',
    '11S',
    '10S',
    '9S',
    '8S', ###
    '7+3/4', # 'S' preceding plus is not included.
    '7+1/2', 
    '7S',
    '6S',
    '5+1/2',
    '5S',
    '4+1/2',
    '4S', ###
    '3+3/4',
    '3+1/2',  
    '3+1/4',
    '3S',
    '2+3/4',
    '2+1/2',
    '2+1/4',
    '2S', ###
    '2S',
    '1+3/4',
    '1+1/2',
    '1+1/2',
    '1+1/2',
    '1+1/4',
    '1+1/4',
    '1S', ###
    '1S',
    '1S',
    '1S',
    '3/4',   # TODO Maybe add some extra detail in this region.
    '3/4',
    '3/4',
    '3/4',
    '1/2', ###
    '15/32',
    '7/16',
    '13/32',
    '3/8',
    '11/32',
    '5/16',
    '9/32',
    '1/4', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/8', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/15', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/30', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/60', ### 
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/125', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/250', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/500', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    # Last one zero not included from this point on.
    '1/100', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/200', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/400', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    '1/800', ###
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    # Last two zeros not included.
    '1/160', 
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX',
    'XXX'
]

apertures_bitmap = [
    None, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'X'
]
apertures = [
    '14', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '28', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '4', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '56', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '8', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '11', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '16', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '22', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    '32', ###
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
    'XX',
]

def output_shutter_speeds():
    sys.stdout.write('const uint8_t SHUTTER_SPEEDS[] = {\n')
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
    sys.stdout.write('const uint8_t SHUTTER_SPEEDS_BITMAP[] = { ')
    for c in shutter_speeds_bitmap:
        if c is None:
            sys.stdout.write("'\\0',")
        else:
            sys.stdout.write("'%s'," % c) # None of the chars need escaping.
    sys.stdout.write('};\n')

def output_apertures():
    sys.stdout.write('const uint8_t APERTURES[] = {\n')
    for a in apertures:
        n1 = apertures_bitmap.index(a[0])
        n2 = apertures_bitmap.index(a[1]) if len(a) > 1 else 0
        sys.stdout.write('0b' +
                         (n2 & 8 and '1' or '0') + (n2 & 4 and '1' or '0') + (n2 & 2 and '1' or '0') + (n2 & 1 and '1' or '0') +
                         (n1 & 8 and '1' or '0') + (n1 & 4 and '1' or '0') + (n1 & 2 and '1' or '0') + (n1 & 1 and '1' or '0') +
                         ',')
    sys.stdout.write('};\n')
    sys.stdout.write('const uint8_t APERTURES_BITMAP[] = { ')
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
    sys.stdout.write('#ifndef TABLES_H\n#define TABLES_H\n\n')
    sys.stdout.write("#include <stdint.h>\n")
    output_full_table_as_comment()
    output_table()
    sys.stdout.write('\n#ifdef TEST\n')
    sys.stdout.write('const uint8_t TEST_TEMP_AND_VOLTAGE_TO_EV[] =\n')
    output_test_table()
    sys.stdout.write(';\n#endif\n')
    output_shutter_speeds()
    output_apertures()
    sys.stdout.write('\n#endif')

if __name__ == '__main__':
#    test_output()
    output()



