import math
import sys
import re

##########
# Configuration values
##########

reference_voltage = 2000 # mV

# Table cells not calculated for voltages lower than this.
voltage_offset = 80 # mV

sensor_cap_value = 3300 # pF

amp_timings = [ # In microseconds
    0.5,
    5,
    10,
    15,
    20
]

amp_normal_timing = 50.0

clock_hz = 48000000

##########

b_voltage_offset = int(round((voltage_offset/reference_voltage)*256))


# For PNJ4K01F
# See http://www.semicon.panasonic.co.jp/en/products/detail/?cat=CED7000&part=PNJ4K01F
# http://www.mathportal.org/calculators/analytic-geometry/two-point-form-calculator.php
def sensor_ua_to_lux(ua):
    incand_ratio = 1.1
    return ((100.0/43.0)*ua)/incand_ratio

def sensor_cap_time_and_mv_to_ua(us, mv):
    return ((sensor_cap_value*mv)/(us*1000.0))

def sensor_cap_time_and_mv_to_lux(us, mv):
    return sensor_ua_to_lux(sensor_cap_time_and_mv_to_ua(us, mv))

def us_to_ticks(us):
    return int(round(us*(clock_hz/1000000.0)))

#
# EV table.
#

bv_to_voltage = ((1/256.0) * reference_voltage)

# Convert luminance to EV at ISO 100.
# See http://stackoverflow.com/questions/5401738/how-to-convert-between-lux-and-exposure-value
# http://en.wikipedia.org/wiki/Exposure_value#EV_as_a_measure_of_luminance_and_illuminance
# This is (implicitly) using C=12.5.
def luminance_to_ev_at_100(lux):
    if lux == 0:
        lux = 0.00001
    return math.log(lux, 2) + 3.0

def illuminance_to_ev_at_100(lux):
    if lux == 0:
        lux = 0.00001
    return math.log(lux/2.5, 2)

def illuminance_to_luminance(lux):
    return lux/math.pi

# Difference between luminance and illuminance will be a constant in log space.
# We store illuminance values in the table then add extra if we're measuring
# illuminance. This calculates how much extra.
LUMINANCE_COMPENSATION = luminance_to_ev_at_100(illuminance_to_luminance(10)) - illuminance_to_ev_at_100(10)
assert LUMINANCE_COMPENSATION >= 2.0 and LUMINANCE_COMPENSATION <= 3.0

# Voltage in mV, timing in us.
def voltage_and_timing_to_ev(v, timing):
    lux = sensor_cap_time_and_mv_to_lux(timing, v)
    return illuminance_to_ev_at_100(lux)


#
# EV and voltage are all represented as unsigned 8-bit
# values on the microcontroller.
#
# Voltage: from 0 up in 1/256ths of the reference voltage.
# EV: from -5 to 26EV in 1/8 EV intervals.
#
# The table is an array mapping voltage to EV*8. The resulting table
# would take up 256 bytes.  To make
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
# value bytes and one for the diff bit pattern index bytes). This was
# previously necessary because the monolithic array was too large to
# index with a single byte. It may not be necessary now but does no harm.
#
# EV values for very low voltages are not stored. The
# VOLTAGE_TO_EV_ABS_OFFSET constant indicates the voltage for the
# first element of the table.
#

def get_xth_bit(ev, x):
    """Given a representation of a voltage, return either 0 or 1,
       indicating respectively whether the voltage is nearest to the
       xth immediately below/equal to the eighth or to the xth
       immediately above it. If the differences are identical, returns 1.
       Storing these additional "tenth/third bits" makes it possible to report
       exposures to a 1/10EV and 1/3EV resolution with full accuracy."""
    eighths = int(round(ev*8.0))
    evv = ev % 1.0
    first_third_above = 0.0
    while first_third_above <= evv:
        first_third_above += 1.0/x
    diffdown = evv - (first_third_above - (1.0/x))
    diffup = first_third_above - evv
    if diffdown < diffup:
        return 0
    else:
        return 1

def get_third_bit(ev):
    return get_xth_bit(ev, 3.0)
def get_tenth_bit(ev):
    return get_xth_bit(ev, 10.0)

def output_ev_table(of, name_prefix, timing):
    bitpatterns = [ ]
    vallist_abs = [ ]
    vallist_diffs = [ ]
    tenth_bits = [ ]
    third_bits = [ ]
    for sv in range(b_voltage_offset, 256, 16):
        # Write the absolute 8-bit EV value.
        voltage = (sv * bv_to_voltage)
        assert voltage >= 0
        if voltage > reference_voltage:
            break
        ev = voltage_and_timing_to_ev(voltage, timing)
        eight = int(round((ev+5.0) * 8.0))
        if eight < 0:
            eight = 0

        # Write the 1-bit differences (two bytes).
        prev = eight
        num = ""
        bpis = [ None, None ]
        for j in range(0, 2):
            eight2 = None
            o = ""
            for k in range(0, 8):
                v = ((sv + (j * 8.0) + k) * bv_to_voltage)
                ev2 = voltage_and_timing_to_ev(v, timing)
                eight2 = int(round((ev2+5.0) * 8.0))
                if eight2 < 0:
                    eight2 = 0
#               sys.stderr.write(">>> %i %i\n" % (eight2, prev))
                if not (eight2 - prev == 0 or not (j == 0 and k == 0)) or \
                   not (eight2 - prev <= 1) or \
                   not (eight2 - prev >= 0):
                    return False, sv, (prev, eight2)
                if eight2 - prev == 0:
                    o += '0'
                elif eight2 - prev == 1:
                    o += '1'
                prev = eight2

                tenth_bits.append(get_tenth_bit(ev2))
                third_bits.append(get_third_bit(ev2))

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

#    for v in range(0, 16):
#        for t in range(0, 256, 16):
#            sys.stderr.write(str(vallist_abs[t + v]) + " ")
#        sys.stderr.write("\n")

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[] = {\n')
    for p in bitpatterns:
        of.write('0b%s,' % p)
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_ABS[] = {')
    for i in range(len(vallist_abs)):
        if i % 32 == 0:
            of.write('\n    ');
        of.write('%i,' % vallist_abs[i])
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_DIFFS[] = {')
    for i in range(len(vallist_diffs)):
        if i % 32 == 0:
            of.write('\n    ');
        of.write('%i,' % vallist_diffs[i])
    of.write('\n};\n')

    def write_x_bits(x):
        name = "TENTHS" if x == 10 else "THIRDS"
        xth_bits = tenth_bits if x == 10 else third_bits
        of.write('const uint8_t ' + name_prefix + ('_LIGHT_VOLTAGE_TO_EV_%s[] = { ' % name))
        bits = ['1' if x == 1 else '0' for x in xth_bits]
        bytes = [ ]
        for i in range(0, len(bits), 8):
            bytes.append('0b' + ''.join(bits[i:i+8]))
        of.write(', '.join(bytes))
        of.write(' };\n')
    write_x_bits(3)
    write_x_bits(10)

    return True, None, ()

# This is useful for santiy checking calculations. It outputs a graph of
# amplified voltage against EV which can be compared with the voltage at the
# input pin.
def output_sanity_graph():
    f = open("sanitygraph.csv", "w")
    f.write("v," + ','.join(["s" + str(n+1) for n in range(len(amp_timings))]) + ',')
    f.write(','.join(["b" + str(n+1) for n in range(len(amp_timings))]) + '\n')
    for v in range(b_voltage_offset, 256):
        f.write("%i" % v)
        voltage = (v * bv_to_voltage)
        bins = [ ]
        for stage in range(len(amp_timings)):
            ev = voltage_and_timing_to_ev(voltage, amp_timings[stage])
            ev8 = int(round((ev + 5.0) * 8.0))
            bins.append(ev8)
            f.write(",%f" % ev)
        f.write("," + ",".join(map(str,bins)))
        f.write("\n")
    f.close()

    f = open("luxtoev.csv", "w")
    f.write("lux,loglux,logillum,loglum\n")
    lux = 5
    while lux <= 656000:
        f.write("%i,%f,%f,%f\n" % (lux, math.log(lux,10), illuminance_to_ev_at_100(lux), luminance_to_ev_at_100(lux)))
        lux += lux
    f.close()

# Straight up array that we use to test that the bitshifting logic is
# working correctly. (Test will currently only be performed for normal light table.)
def output_test_table(of):
    of.write('    { ')
    for v in range(b_voltage_offset, 256):
        voltage = (v * bv_to_voltage)
#            sys.stderr.write('TAVY ' + str(temperature) + ',' + str(voltage) + '\n')
        ev = voltage_and_timing_to_ev(voltage, amp_normal_timing)
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
# Shutter speeds are represented using 13 characters: 0-9, ''.', 'S' and 'X'.
#
# E.g.:
#
#     8      One eighth of a second
#     5s     Five seconds
#     1.25   One and one quarter seconds
#     16X0   1/16000 of a second
#
# We want a way to represent each character using 4 bits. There are max 5
# characters per speed, so we have a total of (168*20)/8 = 420 bytes.
#
# Special case chars:
#
#     X -> '00'
#
shutter_speeds_bitmap = [
    None, # String terminator
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'S', # 12
    '.', # 13
    'X', # 14: = "00"
]
assert len(shutter_speeds_bitmap) <= 16

shutter_speeds_thirds = [
    '60', '48', '38', # 'S' postfix is implicit.
    '30', '24', '18',
    '15', '12', '9',
    '8', '6', '5',
    '4', '3', '2.5',
    '2', '1.6', '1.3',
    '1', '1.3', '1.6', # Initial '1/' starts with 1.3.
    '2', '2.5', '3',
    '4', '5', '6',
    '8', '10', '13',
    '15', '20', '25',
    '30', '40', '50',
    '60', '75', '95',
    '125', '160', '180',
    '250', '310', '400',
    '500', '630', '800',
    '10X', '13X', '16X',
    '20X', '25X', '32X',
    '40X', '50X', '63X',
    '80X', '10X0', '13X0',
    '16X0'
]
assert(max(map(len, shutter_speeds_thirds)) == 4)

shutter_speeds_eighths = [
    '60', '55', '50', '46', '42', '39', '36', '32', # 'S' postfix is implicit.
    '30', '27', '25', '23', '21', '19', '18', '16',
    '15', '14', '13', '12', '11', '10', '9', '8.2',  # TODO: Bit awkward around '9'
    '8', '7.5', '6.5', '6', '5.5', '5', '4.8', '4.4',
    '4', '3.7', '3.4', '3.1', '2.8', '2.6', '2.4', '2.2',
    '2', '1.8', '1.7', '1.5', '1.4', '1.3', '1.2', '1.1',
    '1', '1.1', '1.2', '1.3', '1.4', '1.5', '1.7', '1.8', # Initial '1/' starts with '1.1'
    '2', '2.2', '2.4', '2.6', '2.8', '3', '3.4', '3.7',
    '4', '4.4', '4.8', '5.2', '5.7', '6.2', '6.7', '7.3',
    '8', '8.7', '9.5', '10', '11', '12', '13', '14',  # TODO: Bit awkward around '14'.
    '15', '16', '18', '19', '21', '23', '25', '27',
    '30', '33', '36', '39', '42', '46', '50', '55',
    '60', '65', '70', '80', '85', '90', '100', '110',
    '125', '135', '150', '160', '180', '190', '210', '230',
    '250', '270', '300', '320', '350', '390', '420', '460',
    '500', '550', '590', '650', '700', '770', '840', '920',
    '10X', '11X', '12X', '13X', '14X', '15X', '17X', '18X',
    '20X', '22X', '24X', '26X', '28X', '30X', '34X', '37X',
    '40X', '44X', '48X', '52X', '57X', '62X', '68X', '73X',
    '80X', '87X', '95X', '10X0', '11X0', '12X0', '13X0', '15X0',
    '16X0'
]
assert(max(map(len, shutter_speeds_eighths)) == 4)

shutter_speeds_tenths = [
    '60', '56', '52', '48', '45', '42', '40', '37', '35', '32', # 'S' postfix is implicit.
    '30', '28', '26', '24', '23', '21', '20', '19', '17', '16', # TODO: Check '16'
    '15', '14', '13', '12', '11', '10.5', '10', '9', '8.5', '8', # TODO: Difficult to give an appropriate but unique value around '8'.
    '8', '7.5', '7', '6.5', '6', '5.5', '5.3', '5', '4.5', '4.3',
    '4', '3.7', '3.5', '3.2', '3', '2.8', '2.6', '2.5', '2.3', '2.1',
    '2', '1.9', '1.7', '1.6', '1.5', '1.4', '1.3', '1.2', '1.1', '1.05',
    '1', '1.1', '1.05', '1.2', '1.3', '1.4', '1.5', '1.6', '1.7', '1.9', # Initial slash starts at '1.1'.
    '2', '2.1', '2.3', '2.5', '2.6', '2.8', '3.0', '3.2', '3.5', '3.7',
    '4', '4.3', '4.6', '4.9', '5.3', '5.7', '6.1', '6.5', '7.0', '7.5',
    '8', '8.5', '9', '9.5', '10.5', '11', '12', '13', '14', '14.5',
    '15', '16', '17', '19', '20', '21', '23', '24', '26', '28',
    '30', '32', '35', '37', '40', '42', '46', '49', '52', '56',
    '60', '65', '70', '75', '80', '85', '90', '100', '105', '110',
    '125', '135', '145', '155', '165', '175', '190', '205', '220', '230',
    '250', '270', '290', '310', '330', '350', '380', '405', '435', '465',
    '500', '535', '575',  '615', '660', '710', '760', '810', '870','930',
    '10X', '11X', '115X', '12X', '13X', '14X', '15X', '16X', '17X', '19X',
    '20X', '21X', '23X', '25X', '26X', '28X', '30X', '32X', '35X', '37X',
    '40X', '43X', '46X', '50X', '53X', '57X', '61X', '65X', '70X', '75X',
    '80X', '86X', '92X', '98X', '10X0', '11X0', '12X0', '13X0', '14X0', '15X0',
    '16X0'
]
assert(max(map(len, shutter_speeds_tenths)) == 4)


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
# There is a small amount of redundancy in the tables, since the full-stop apertures
# are duplicated three times and the half-stop apertures two times. However, it is
# unlikely that much flash space could be saved by eliminating this redundancy since
# it would complicate the code.
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
    for x in [('THIRD', shutter_speeds_thirds), ('EIGHTH', shutter_speeds_eighths), ('TENTH', shutter_speeds_tenths)]:
        name = x[0]
        ss = x[1]
        of.write('const uint8_t SHUTTER_SPEEDS_%s[] = {\n' % name)
        x = 0
        for spd in ss:
            indexes = [shutter_speeds_bitmap.index(c) for c in spd]
            for i in range(0, 4, 2):
                v1 = None
                v2 = None
                if i >= len(indexes):
                    v1 = 0
                else:
                    v1 = indexes[i]
                if i+1 >= len(indexes):
                    v2 = 0
                else:
                    v2 = indexes[i+1]
                of.write('0b{:08b},'.format(v1 | (v2 << 4)))
            if x % 4 == 3:
                of.write("\n")
            x += 1
        of.write('\n};\n')
    of.write('const uint8_t SHUTTER_SPEEDS_BITMAP[] = { ')
    for c in shutter_speeds_bitmap:
        if c is None:
            of.write("'\\0',")
        else:
            of.write("'%s'," % c) # None of the chars need escaping.
    of.write('};\n')

def output_apertures(of):
    of.write('const uint8_t APERTURES_THIRD[] = {\n')
    for a in apertures_third:
        of.write("0b{:04b}{:04b},".format(ord(a[1]) - ord('0'), ord(a[0]) - ord('0')))
    of.write('};\n')

    of.write('const uint8_t APERTURES_TENTH[] = {\n')
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
        if i % 8 == 7:
            of.write("\n")
        i += 1
    of.write('};\n')

    of.write('const uint8_t APERTURES_EIGHTH[] = {\n')
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

    ofh.write("#define NUM_AMP_STAGES %i\n" % len(amp_timings))
    ofh.write("#define FOR_EACH_AMP_STAGE(x) ")
    for i in range(1, len(amp_timings)+1):
        ofh.write("x(%i) " % i)
    for i in range(0, len(amp_timings)):
        ofh.write("#define STAGE%i_TICKS %i\n" % (i, us_to_ticks(amp_timings[i])))
    ofh.write("\n")

    ofc.write("#include <stdint.h>\n")

    e, pr = None, None
    for i in range(len(amp_timings)):
        timing = amp_timings[i]

        e, sv, pr = output_ev_table(ofc, 'STAGE' + str(i+1), amp_timings[i])
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_ABS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_DIFFS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_TENTHS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_THIRDS[];\n" % (i+1))
        if not e:
            sys.stderr.write("R ERROR %.3f: %i (%.3f, %.3f) at stage %i\n" % (amp_timings[i], sv, pr[0], pr[1], i))
            break

    ofh.write("#define VOLTAGE_TO_EV_ABS_OFFSET " + str(b_voltage_offset) + '\n')
    ofh.write("#define LUMINANCE_COMPENSATION " + str(int(round(LUMINANCE_COMPENSATION*8.0))) + '\n')

    ofc.write('\n#ifdef TEST\n')
    ofc.write('const uint8_t TEST_VOLTAGE_TO_EV[] =\n')
    ofh.write('extern const uint8_t TEST_VOLTGE_TO_EV[];\n')
    output_test_table(ofc)
    ofc.write(';\n#endif\n')
    output_shutter_speeds(ofc)
    output_apertures(ofc)

    ofh.write("extern uint8_t SHUTTER_SPEEDS_EIGHTH[];")
    ofh.write("extern uint8_t SHUTTER_SPEEDS_TENTH[];")
    ofh.write("extern uint8_t SHUTTER_SPEEDS_THIRD[];")
    ofh.write("extern uint8_t SHUTTER_SPEEDS_BITMAP[];\n")
    ofh.write("extern uint8_t APERTURES_EIGHTH[];\n")
    ofh.write("extern uint8_t APERTURES_TENTH[];\n")
    ofh.write("extern uint8_t APERTURES_THIRD[];\n")

    ofh.write("\n#endif\n")

    ofh.close()
    ofc.close()

def try_timing_values():
    working = [ ]
    for i in range(1, 2000):
        r = i/10.0
#        sys.stderr.write("Trying %.3f\n" % r)
        e, pr = output_ev_table(sys.stdout, 'STAGE', r)
        if e:
            working.append(r)
    sys.stderr.write('\n'.join(("%.2f" % x for x in working)))

if __name__ == '__main__':
    assert len(sys.argv) >= 2

    if sys.argv[1] == 'try':
        try_timing_values()
    elif sys.argv[1] == 'graph':
        output_sanity_graph()
    elif sys.argv[1] == 'output':
        output()
    elif sys.argv[1] == 'testtenth':
        test_get_tenth_bit()
    else:
        assert False
