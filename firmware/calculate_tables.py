import math
import sys

##########
# Configuration values.
##########

reference_voltage                = 5000.0  # mV
op_amp_resistor_stages = [ # In (kOhm,gain) pairs
    # For BPW 34
    #(320.0,1), # Very low light
    #(12.0,1)  # Fairly low light (brightish rooms, that kind of thing)
    #(0.50,1)  # Bright light (goes up to a bright sunny day)
    #(0.05,1),  # Extremely bright light

    # For VTB8440,8441
    #(258,1),
    #(42,1),
    #(1.7,1)
    #(1,0.075)

    # For BPW21R
    (330,20), # Was 350; 330 is more common size.
    (330,1),
    (22,1),
    (1.2,1)
]

op_amp_normal_resistor = op_amp_resistor_stages[1][0] * op_amp_resistor_stages[1][1]

# Table cells not calculated for voltages lower than this.
# This is because in the region just above v = 0, the changes
# between different voltage values lead to large discontinuities
# in EV values which the table compression mechanism can't handle.
voltage_offset                   = 250.0   # mV

# For BPW34
reference_temperature            = 30.0    # C

##########


b_voltage_offset = int(round((voltage_offset/reference_voltage)*256))
# So that we don't introduce any rounding error into calculations.
voltage_offset = (b_voltage_offset/256.0)*reference_voltage


for s in op_amp_resistor_stages:
    assert s[1] == 1 or s[1] == 20

#
# EV table.
#

bv_to_voltage = ((1/256.0) * reference_voltage)

# http://www.vishay.com/docs/81521/bpw34.pdf, p. 2 Fig 2
# Temp in C.
# For BPW34
#def temp_to_rrlc(temp): # Temperature to relative reverse light current
#    slope = 0.002167
#    k = 0.97
#    return (temp*slope) + k
#
# For BPW21
def temp_to_rrlc(temp):
    return ((19.0/22500.0)*temp) + (97.0/100.0)


# Log10 reverse light current microamps to log10 lux.
# http://www.vishay.com/docs/81521/bpw34.pdf, p. 3 Fig 4
# Useful link for calculating equations from lines (I *always* mess this up doing it by hand):
# http://www.mathportal.org/calculators/analytic-geometry/two-point-form-calculator.php
# For BPW34
#def rlc_to_lux(rlc):
#    k = -1.254
#    slope = 1.054
#    return (rlc - k) / slope
#
# For VTB8440, 8441
# See table on p. 22 of PDF datasheet included in git repo.
#def rlc_to_lux(rlc):
#    return math.log(10.763910417,10) + rlc
#
# For BPW21R. See Fig 3 of PDF datasheet included in git repo.
def log_rlc_to_log_lux(rlc):
    return (rlc + 1.756) / 0.806

# Convert log10 lux to EV at ISO 100.
# See http://stackoverflow.com/questions/5401738/how-to-convert-between-lux-and-exposure-value
# http://en.wikipedia.org/wiki/Exposure_value#EV_as_a_measure_of_luminance_and_illuminance
# This is (implicitly) using C=250.
def log_illuminance_to_ev_at_100(log_lux):
    # TODO: Could probably do this all in log space for greater accuracy.
    lux = math.pow(10, log_lux)
    ev = math.log(lux/2.5, 2)
    return ev

# Implicitly using the Sekonic calibration constant of 12.5.
def log_luminance_to_ev_at_100(log_lux):
    lux = math.pow(10, log_lux)
    ev_minus_3 = math.log(lux, 2)
    return ev_minus_3 + 3.0

# Difference between illuminance and irradience will be a constant in log space.
# We store illuminance values in the table then add extra if we're measuring
# irradience. This calculates how much extra.
LUMINANCE_COMPENSATION = log_luminance_to_ev_at_100(10) - log_illuminance_to_ev_at_100(10)
assert LUMINANCE_COMPENSATION >= 4.0 and LUMINANCE_COMPENSATION <= 5.0

# Voltage (mV) and op amp resitor value (kOhm) to EV at the reference temp,
# which we see from Fig 2 on p.2 of http://www.vishay.com/docs/81521/bpw34.pdf
# is 40C.
def voltage_and_oa_resistor_to_ev(v, r, TADJ = 0.0):
    # v = ir => i = v/r
    v /= 1000.0 # v, mV -> V
    r *= 1000.0 # kOhm -> ohm

    if v < 1e-06:
        v = 1e-06

    v = math.log(v, 10)
    r = math.log(r, 10)
    i = v - r + TADJ

    # Get i in microamps.
    i += math.log(1000000, 10)

    log_lux = log_rlc_to_log_lux(i)
    ev = log_illuminance_to_ev_at_100(log_lux)
    return ev

# If rlc_to_lux(rlc) is linear (as we falsely assume), then each +1 C yields
# a constant decrease in EV. The value of the op amp resistor makes no difference.
# Rather than crunching through the equations, we just choose a couple of arbitrary values to calculate this.
ev_change_for_every_1_6_c_rise = None
for x in xrange(1): # Just here to get a new scope
    tadj = math.log(temp_to_rrlc(reference_temperature-10), 10)
    r1 = voltage_and_oa_resistor_to_ev(reference_voltage/2.0, op_amp_normal_resistor, tadj)
    tadj = math.log(temp_to_rrlc(reference_temperature), 10)
    r2 = voltage_and_oa_resistor_to_ev(reference_voltage/2.0, op_amp_normal_resistor, tadj)
    ev_change_for_every_1_6_c_rise = r1 - r2

#
# Temperature, EV and voltage are all represented as unsigned 8-bit
# values on the microcontroller.
#
# Temperature: from -51C to 51C in 0.4C intervals (0 = -51C)
# Voltage: from 0 up in 1/256ths of the reference voltage.
# EV: from -5 to 26EV in 1/8 EV intervals.
#
# The table is an array mapping voltage to EV*8 at the reference
# temperature.  The resulting table would take up 256 bytes.  To make
# the table more compact, the series of voltages is stored with the
# first 8-bit value followed by a series of 16 (compacted) 1-bit
# values, where each 1-bit value indicates the (always positive)
# difference with the previous value.  The table is therefore grouped
# into triples of the form [ev diffs diffs].
#
# As it turns out, there are only a small number of distinct 8-bit
# diffs patterns. We can therefore replace each [diffs diffs] sequence
# with the sequence [ev i], where i is a single byte containing two
# 4-bit indices (one for each 8-bit bit pattern) into an array.
#
# Finally, we split the array into two arrays (one for the absolute
# value bytes and one for the diff bit pattern index bytes). (This was
# previously necessary because the monolithic array was too large to
# index with a single byte; may not be necessary now.)
#
# EV values for very low voltages are not stored. The
# VOLTAGE_TO_EV_ABS_OFFSET constant indicates the voltage for the
# first element of the table.
#
# A separate table of int8_t values maps temperatures to compensation
# in EV@100*8.
#

# The temperature adjustment curve is very flat, so we store it as
# follows. #define TEMP_EV_ADJUST_AT_T0 gives the EV compensation
# value for t=0 (i.e. -51C). Then we store the temperatures at which the
# EV compensation value goes down by 1 (i.e. 1/8EV) in TEMP_EV_ADJUST_CHANGE_TEMPS.
def output_temp_table(ofc, ofh):
    firstNibble = True
    lastEight = None
    arrayLen = 0
    for t in xrange(256):
        temperature = -51.0 + (t * 0.4)
        dtemp = temperature - reference_temperature
        comp = ev_change_for_every_1_6_c_rise * (dtemp / 1.6)
        eight = int(round(comp * 8.0))
        if eight < -127:
            eight = -127
        if eight > 127:
            eight = 127

        if t == 0:
            ofh.write("#define TEMP_EV_ADJUST_AT_T0 %i\n" % eight)
        else:
            if t == 1:
                ofc.write("const uint8_t TEMP_EV_ADJUST_CHANGE_TEMPS[] PROGMEM = { ")

            if eight == lastEight:
                pass
            else:
                arrayLen += 1
                ofc.write("%i," % t)

        lastEight = eight
    ofc.write("};\n")
    ofh.write("#define TEMP_EV_ADJUST_CHANGE_TEMPS_LENGTH %i\n" % arrayLen)

def get_tenth_bit(ev, test=False):
    """Given a representation of a voltage, return either 0 or 1,
       indicating respectively whether the voltage is nearest to the
       tenth immediately below/equal to the eighth or to the tenth
       immediately above it. Storing these additional "tenth bits"
       makes it possible to report exposures to a 1/10EV resolution
       with full accuracy.
    """
    eighths = int(round(ev*8.0))
    if test:
        sys.stdout.write("Original %f, eighths %i" % (ev, eighths))
    ee = eighths % 8
    evv = ev % 1.0
    nearest_tenth = 0.0
    while abs(evv - nearest_tenth) > 0.05:
        nearest_tenth += 0.1
    if test:
        sys.stdout.write("; nearest tenth = %f" % nearest_tenth)
    r = (1 if nearest_tenth > evv else 0)
    if test:
        sys.stdout.write("; r = %i\n" % r)
    return r

def output_ev_table(of, name_prefix, op_amp_resistor):
    bitpatterns = [ ]
    vallist_abs = [ ]
    vallist_diffs = [ ]
    oar = op_amp_resistor
    tenth_bits = [ ]
    for sv in xrange(0, 256-b_voltage_offset, 16):
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
#               sys.stderr.write(">>> %i %i\n" % (eight2, prev))
                if not (eight2 - prev == 0 or not (j == 0 and k == 0)) or \
                   not (eight2 - prev <= 1) or \
                   not (eight2 - prev >= 0):
                    return False, (prev, eight2)
                if eight2 - prev == 0:
                    o += '0'
                elif eight2 - prev == 1:
                    o += '1'
                prev = eight2

                tenth_bits.append(get_tenth_bit(ev2))

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

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[] PROGMEM = {\n')
    for p in bitpatterns:
        of.write('0b%s,' % p)
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_ABS[] PROGMEM = {')
    for i in xrange(len(vallist_abs)):
        if i % 32 == 0:
            of.write('\n    ');
        of.write('%i,' % vallist_abs[i])
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_DIFFS[] PROGMEM = {')
    for i in xrange(len(vallist_diffs)):
        if i % 32 == 0:
            of.write('\n    ');
        of.write('%i,' % vallist_diffs[i])
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_TENTHS[] PROGMEM = { ')
    bits = ['1' if x == 1 else '0' for x in tenth_bits]
    bytes = [ ]
    for i in xrange(0, len(bits), 8):
        bytes.append('0b' + ''.join(bits[i:i+8]))
    of.write(', '.join(bytes))
    of.write(' };\n')

    return True, ()

# This is useful for santiy checking calculations. It outputs a graph of
# amplified voltage against EV which can be compared with voltage readings
# over the two input pins (or between the input pin and ground if we're
# using single input mode).
def output_sanity_graph():
    f = open("sanitygraph.csv", "w")
    f.write("v," + ','.join(["s" + str(n+1) for n in xrange(len(op_amp_resistor_stages))]) + ',')
    f.write(','.join(["b" + str(n+1) for n in xrange(len(op_amp_resistor_stages))]) + '\n')
    for v in xrange(b_voltage_offset, 256):
        f.write("%i" % v)
        voltage = (v * bv_to_voltage)
        bins = [ ]
        for stage in xrange(len(op_amp_resistor_stages)):
            ev = voltage_and_oa_resistor_to_ev(voltage, op_amp_resistor_stages[stage][0] * op_amp_resistor_stages[stage][1])
            bins.append(int(round((ev + 5.0) *8.0)))
            f.write(",%f" % ev)
        f.write("," + ",".join(map(str,bins)))
        f.write("\n")
    f.close()

    f = open("luxtoev.csv", "w")
    f.write("lux,loglux,logillum,loglum\n")
    lux = 5
    while lux <= 656000:
        f.write("%i,%f,%f,%f\n" % (lux, math.log(lux,10), log_illuminance_to_ev_at_100(math.log(lux, 10)), log_luminance_to_ev_at_100(math.log(lux,10))))
        lux += lux
    f.close()

# Straight up array that we use to test that the bitshifting logic is
# working correctly. (Test will currently only be performed for normal light table.)
def output_test_table(of):
    of.write('    { ')
    for v in xrange(b_voltage_offset, 256):
        voltage = (v * bv_to_voltage)
#            sys.stderr.write('TAVY ' + str(temperature) + ',' + str(voltage) + '\n')
        ev = voltage_and_oa_resistor_to_ev(voltage, op_amp_normal_resistor)
        eight = int(round((ev+5.0) * 8.0))
        of.write("%i," % eight)
    of.write('\n')
    of.write('    }');

def test_get_tenth_bit():
    assert get_tenth_bit(0.5, True)    == 0
    assert get_tenth_bit(0.4, True)    == 0
    assert get_tenth_bit(0.875, True ) == 1
    assert get_tenth_bit(99.875, True) == 1
    assert get_tenth_bit(1.625, True)  == 0

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
# We store compacted tables giving EV -> aperture in 1/3, 1/8 and 1/10 steps.
# Decimal points are not included because these can be inserted in the appropriate
# place via a simple comparison on the EV value.
#
# Initial attempts to calculate aperture strings dynamically on the microcontroller
# proved fruitless. Even using 16-bit fixed point arithmetic, the code for doing
# the calculations comes out as big as (or maybe even bigger than) the tables.
# Since the tables are much easier to debug, they seem to be the better option.
#

apertures_third = [
    '10','11','12',
    '14','16','18',
    '20','22','25',
    '28','32','35',
    '40','45','50',
    '56','63','71',
    '80','90','10',
    '11','13','14',
    '16','18','20',
    '22','25','29',
    '32'
]
apertures_eighth = [
    '100', '104', '109', '114', '119', '124', '130', '135',
    '140', '148', '154', '161', '168', '176', '183', '192',
    '200', '209', '218', '228', '238', '248', '259', '271',
    '280', '295', '308', '322', '336', '351', '367', '383',
    '400', '418', '436', '456', '476', '497', '519', '542',
    '560', '591', '617', '644', '673', '703', '734', '766',
    '800', '835', '872', '911', '951', '993', '103', '108',
    '110', '118', '123', '129', '135', '141', '147', '153',
    '160', '167', '174', '182', '190', '199', '207', '217',
    '220', '236', '247', '258', '269', '281', '293', '306',
    '320'
]
apertures_tenth = [
    '100', '104', '107', '111', '115', '119', '123', '127', '132', '137',
    '140', '146', '152', '157', '162', '168', '174', '180', '187', '193',
    '200', '207', '214', '222', '230', '238', '246', '255', '264', '273',
    '280', '293', '303', '314', '325', '336', '348', '361', '373', '386',
    '400', '414', '429', '444', '459', '476', '492', '510', '528', '546',
    '560', '586', '606', '628', '650', '673', '696', '721', '746', '773',
    '800', '828', '857', '888', '919', '951', '985', '102', '106', '109',
    '110', '117', '121', '126', '130', '135', '139', '144', '149', '155',
    '160', '166', '171', '178', '184', '190', '197', '204', '211', '219',
    '220', '234', '243', '251', '260', '269', '279', '288', '299', '309',
    '320'
]

def output_shutter_speeds(of):
    of.write('const uint8_t SHUTTER_SPEEDS[] PROGMEM = {\n')
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
            of.write('0b' + bits[i+1] + bits[i] + ',')
        else:
            of.write('0b' + bits[i] + ',')
    of.write('};\n')
#    sys.stdout.write('// ' + ' '.join(bits));
    of.write('const uint8_t SHUTTER_SPEEDS_BITMAP[] PROGMEM = { ')
    for c in shutter_speeds_bitmap:
        if c is None:
            of.write("'\\0',")
        else:
            of.write("'%s'," % c) # None of the chars need escaping.
    of.write('};\n')

def output_apertures(of):
    of.write('const uint8_t APERTURES_THIRD[] PROGMEM = {\n')
    for a in apertures_third:
        of.write("0b{:04b}{:04b},".format(ord(a[1]) - ord('0'), ord(a[0]) - ord('0')))
    of.write('};\n')

    of.write('const uint8_t APERTURES_TENTH[] PROGMEM = {\n')
    odd = [1]
    def wn(n):
        n1s = "{:04b}".format(ord(a[0]) - ord('0'))
        n2s = "{:04b}".format(ord(a[1]) - ord('0') if len(a) > 1 else 0)
        n3s = "{:04b}".format(ord(a[2]) - ord('0') if len(a) > 2 else 0)
        if odd[0]:
            of.write('0b' + n1s + n2s + ',0b' + n3s)
        else:
            of.write(n1s + ",0b" + n2s + n3s + ",")
        odd[0] ^= 1
    i = 0
    for a in apertures_tenth:
        wn(a)
        if i % 10 == 9:
            of.write("\n")
        i += 1
    of.write('};\n')

    of.write('const uint8_t APERTURES_EIGHTH[] PROGMEM = {\n')
    odd = [1]
    i = 0
    for a in apertures_eighth:
        wn(a)
        if i % 10 == 9:
            of.write("\n")
        i += 1
    of.write('};\n')

#
# Final output generation.
#

def output():
    ofc = open("tables.c", "w")
    ofh = open("tables.h", "w")

    ofh.write("#ifndef TABLES_H\n")
    ofh.write("#define TABLES_H\n\n")
    ofh.write("#include <stdint.h>\n\n")

    ofh.write("#define NUM_OP_AMP_RESISTOR_STAGES %i\n" % len(op_amp_resistor_stages))

    ofc.write("#include <stdint.h>\n")
    ofc.write("#include <readbyte.h>\n")

    e, pr = None, None
    for i in xrange(len(op_amp_resistor_stages)):
        e, pr = output_ev_table(ofc, 'STAGE' + str(i+1), op_amp_resistor_stages[i][0] * op_amp_resistor_stages[i][1])
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_ABS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_DIFFS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_TENTHS[];\n" % (i+1))
        if not e:
            sys.stderr.write("R ERROR %.3f: (%.3f, %.3f)\n" % (op_amp_resistor_stages[i][0] * op_amp_resistor_stages[i][1], pr[0], pr[1]))
            break

    ofh.write("#define VOLTAGE_TO_EV_ABS_OFFSET " + str(b_voltage_offset) + '\n')
    ofh.write("#define LUMINANCE_COMPENSATION " + str(int(round(LUMINANCE_COMPENSATION*8.0))) + '\n')

    output_temp_table(ofc, ofh)
    ofc.write('\n#ifdef TEST\n')
    ofc.write('const uint8_t TEST_VOLTAGE_TO_EV[] PROGMEM =\n')
    ofh.write('extern const uint8_t TEST_VOLTGE_TO_EV[];\n')
    output_test_table(ofc)
    ofc.write(';\n#endif\n')
    output_shutter_speeds(ofc)
    output_apertures(ofc)

    ofh.write("extern uint8_t SHUTTER_SPEEDS[];\n")
    ofh.write("extern uint8_t SHUTTER_SPEEDS_BITMAP[];\n")
    ofh.write("extern uint8_t APERTURES_EIGHTH[];\n")
    ofh.write("extern uint8_t APERTURES_TENTH[];\n")
    ofh.write("extern uint8_t APERTURES_THIRD[];\n")
    ofh.write("extern uint8_t TEMP_EV_ADJUST_CHANGE_TEMPS[];\n")

    ofh.write("\n#endif\n")

    ofh.close()
    ofc.close()

def try_resistor_values():
    working = [ ]
    for i in xrange(1, 2000):
        r = i/10.0
#        sys.stderr.write("Trying %.3f\n" % r)
        e, pr = output_ev_table('STAGE', r)
        if e:
            working.append(r)
    sys.stderr.write('\n'.join(("%.2f" % x for x in working)))

if __name__ == '__main__':
    assert len(sys.argv) >= 2

    if sys.argv[1] == 'try':
        try_resistor_values()
    elif sys.argv[1] == 'graph':
        output_sanity_graph()
    elif sys.argv[1] == 'output':
        output()
    elif sys.argv[1] == 'testtenth':
        test_get_tenth_bit()
    else:
        assert False
