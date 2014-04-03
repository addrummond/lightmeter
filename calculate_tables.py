import math
import sys

##########
# Configuration values.
##########

reference_voltage = 5500.0 # mV
op_amp_gain       = 100.0

##########

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
    l1Val = (ocv - 155.0 - 20.0) / slope
    l2Val = (ocv - 100.0 - 20.0) / slope
    l3Val = (ocv - 47.0 - 20.0)  / slope
    l4Val = (ocv - 12.0 - 20.0)  / slope
    l5Val = (ocv - 5.0 - 20.0)   / slope

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
def irrad_to_illum(irrad):
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
    lux = irrad_to_illum(irrad)
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
    for t in xrange(0, 256, 16):
        print "%f, %f" % (t * 0.4 - 51.0, temp_to_rdc(t * 0.4 - 51.0))
    print
    print
    for t in xrange(0, 256, 16):
        temperature = t * 0.4 - 51.0
        for v in xrange(0, 256, 8):
            voltage = v * bv_to_voltage
            print "T %f V %f EV %f" % (temperature, voltage, temp_and_voltage_to_ev(temperature, voltage))

def output():
    sys.stdout.write("#include <stdint.h>\n")
    output_full_table_as_comment()
    output_table()
    sys.stdout.write('\n#ifdef TEST\n')
    sys.stdout.write('const uint8_t TEST_TEMP_AND_VOLTAGE_TO_EV[] =\n')
    output_test_table()
    sys.stdout.write(';\n#endif\n')

if __name__ == '__main__':
#    test_output()
    output()



