import math
import sys
import re

##########
# Configuration values.
##########

reference_voltage = 2800 # mV

op_amp_gain       = 1.0#reference_voltage/1100.0

# The gain from the second stage of amplification.
second_stage_gain = 1.0+(97.6/10.0)

# The value of the resistor in the second stage of the op amp the gain is
# proportional to the reciprocal of which.
second_stage_recip_resistor = 10.0 # kOhm

amp_stages = [
    # For VEMD2503X01.
    # Uses 4 distinct resistor values.
    # Resistor value (kOhm)  Stops subtracted by ND filter (should be >= 0)
    ( 750.0,                 0.0  ),
    ( 97.6,                  0.0  ),
    ( 15.0,                  0.0  ),
    ( 1.6,                   0.0  ),
    ( 97.5,                  8.0  ),
    ( 15.0,                  8.0  ),
    ( 1.6,                   8.0  )
]

op_amp_normal_resistor = amp_stages[1][0]

op_amp_resistor_value_to_resistor_number = { }
rvalcount = 0
for s in amp_stages:
    if not op_amp_resistor_value_to_resistor_number.has_key(s[0]):
        op_amp_resistor_value_to_resistor_number[s[0]] = rvalcount
        rvalcount += 1

# Table cells not calculated for voltages lower than this.
# This is because in the region just above v = 0, the changes
# between different voltage values lead to large discontinuities
# in EV values which the table compression mechanism can't handle.
voltage_offset                   = 250.0   # mV

# Set to highest temperature in which device is likely to be used,
# so that we can compensate for temperature by amplifying signal
# slightly.
reference_temperature            = 50.0     # VEMD2503X01, BPW34, C

##########


b_voltage_offset = int(round((voltage_offset/reference_voltage)*256))
# So that we don't introduce any rounding error into calculations.
voltage_offset = (b_voltage_offset/256.0)*reference_voltage

def get_function_from_function_table(filename, inverse=False):
    f = open(filename)
    lines = f.read().split("\n")
    # Skip column headers if any
    try:
        float(lines[0][0])
        float(lines[0][1])
    except:
        lines = lines[1:]
    min = 10000
    max = 0
    function = { }
    for l in lines:
        if re.match(r"^\s*$", l):
            continue
        fields = l.split(",")
        v = float(fields[0])
        frac = float(fields[1])
        if v < min:
            min = v
        elif v > max:
            max = v
        if inverse:
            function[frac] = v
        else:
            function[v] = frac
    f.close()
    def f(x):
        if x < min:
            return function[min]
        elif x > max:
            return function[max]
        else:
            iup = x
            while not function.has_key(iup):
                iup += 1
            idown = x
            while not function.has_key(idown):
                idown -= 1
            diff = iup-idown
            r = None
            if diff == 0:
                r = function[iup]
            else:
                downweight = ((iup-x)*1.0)/diff
                upweight = ((x-idown)*1.0)/diff
                r = function[iup]*upweight + function[idown]*downweight
            return r
    return f

luminosity_func = get_function_from_function_table("tables/luminosity_function.csv")
vemd2503x01_spectral_sensitivity_func = get_function_from_function_table("tables/vemd2503x01_spectral_sensitivity.csv")
qb11_spectral_sensitivity_func = get_function_from_function_table("tables/qb11_spectral_sensitivity.csv")
qb12_spectral_sensitivity_func = get_function_from_function_table("tables/qb12_spectral_sensitivity.csv")


#
# Luminosity calculations.
#

# VEMD2503X01
def output_sensitivity_curves():
    f = open("sensitivity_curves.csv", "w")
    f.write("wavelength,diode relative spectral sensitivity,qb12 relative spectral sensitivity,qb11 relative spectral sensitivity,luminosity func\n")
    for i in xrange(0,900):
        lum = luminosity_func(i)
        diode = vemd2503x01_spectral_sensitivity_func(i)
        qb12 = qb12_spectral_sensitivity_func(i)
        qb11 = qb11_spectral_sensitivity_func(i)
        product12 = lum * qb12
        product11 = lum * qb11
        f.write("%i,%f,%f,%f,%f\n" % (i, diode, product12, product11, lum))
    f.close()
output_sensitivity_curves()

irrad_to_illum_constant = None
# VEMD2503X01 with QB12 filter.
def irrad_to_illum(irrad):
    global irrad_to_illum_constant
    # Assuming white light.
    if irrad_to_illum_constant is None:
        area = 0.0
        for i in xrange(0, 900):
            lum = luminosity_func(i)
            qb12 = qb12_spectral_sensitivity_func(i)
            area += lum * qb12
        irrad_to_illum_constant = (683.002 * area) / 900.0
    return irrad * irrad_to_illum_constant

illum_to_irrad_constant = None
def illum_to_irrad(illum):
    if illum_to_irrad_constant is None:
        irrad_to_illum(5.0) # Dummy value.
        return illum / irrad_to_illum_constant

#
# EV table.
#

# Useful link for calculating equations from lines (I *always* mess this up doing it by hand):
# http://www.mathportal.org/calculators/analytic-geometry/two-point-form-calculator.php

bv_to_voltage = ((1/256.0) * reference_voltage)

# http:www.vishay.comdocs81521bpw34.pdf, p. 2 Fig 2
# Temp in C.
# For BPW34
#def temp_to_rrlc(temp): # Temperature to relative reverse light current
#    slope = 0.002167
#    k = 0.97
#    return (temp*slope) + k
#
# For BPW21
#def temp_to_rrlc(temp):
#    return ((19/022500.0)*temp) + (97.0/100.0)
#
# For VEMD2503X01. See p. 2 Fig 2 of datasheet.
def temp_to_rrlc(temp):
    return (317.0/300000.0)*temp + (9683.0/10000.0)


# Log10 reverse light current microamps to log10 lux.
# http://www.vishay.com/docs/81521/bpw34.pdf, p. 3 Fig 4
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
#def log_rlc_to_log_lux(rlc):
#    return (rlc + 1.756) / 0.806

#
# For VEMD2503X01 with QB12 filter.
# See p. 2 Fig 3 of datasheet.
# Note that irradience is given in the odd unit of mW/cm^2, which is W/m^2
# multiplied by 10.
def log_rlc_to_log_irrad(rlc):
    # Log10 reverse light current microamps to log10 irradience (W/m^2).
    return (12.0/13.0)*rlc + (8.0/65.0)
def log_rlc_to_log_lux(rlc):
    irrad = log_rlc_to_log_irrad(rlc)
    # TODO: Bit ugly to go out of log space.
    return math.log(irrad_to_illum(math.pow(10,irrad)),10)

# Convert log10 lux to EV at ISO 100.
# See http://stackoverflow.com/questions/5401738/how-to-convert-between-lux-and-exposure-value
# http://en.wikipedia.org/wiki/Exposure_value#EV_as_a_measure_of_luminance_and_illuminance
# This is (implicitly) using C=250.
LOG10_2_5 = math.log(2.5, 10)
LOG2_10 = math.log(2,10)
def log_illuminance_to_ev_at_100(log_lux):
    ev = log_lux - LOG10_2_5
    # Convert from log10 to log2.
    ev /= LOG2_10
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
    #v = ir => i = v/r
    v /= 1000.0 # v, mV -> V
    r *= 1000.0 # kOhm -> ohm

    if v < 1e-06:
        v = 1e-06

    v = math.log(v, 10) - math.log(second_stage_gain, 10)
    r = math.log(r, 10)
    i = v - r + TADJ

    # Get i in microamps.
    i += math.log(1000000, 10)

    log_lux = log_rlc_to_log_lux(i) * temp_to_rrlc(reference_temperature)
    ev = log_illuminance_to_ev_at_100(log_lux)
    return ev


#
# EV and voltage are all represented as unsigned 8-bit
# values on the microcontroller.
#
# Voltage: from 0 up in 1/256ths of the reference voltage.
# EV: from -5 to 26EV in 1/8 EV intervals.
# EV: from -5 to 26EV in 18 EV intervals.
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
# index with a single byte; may not be necessary now but does no harm.)
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

def output_ev_table(of, name_prefix, op_amp_resistor, filter_offset):
    bitpatterns = [ ]
    vallist_abs = [ ]
    vallist_diffs = [ ]
    oar = op_amp_resistor
    tenth_bits = [ ]
    third_bits = [ ]
    for sv in xrange(b_voltage_offset, 256, 16):
        # Write the absolute 8-bit EV value.
        voltage = (sv * bv_to_voltage)
        assert voltage >= 0
        if voltage > reference_voltage:
            break
        ev = voltage_and_oa_resistor_to_ev(voltage, oar)
        ev += filter_offset
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
                v = ((sv + (j * 8.0) + k) * bv_to_voltage)
                ev2 = voltage_and_oa_resistor_to_ev(v, oar)
                ev2 += filter_offset
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

#    for v in xrange(0, 16):
#        for t in xrange(0, 256, 16):
#            sys.stderr.write(str(vallist_abs[t + v]) + " ")
#        sys.stderr.write("\n")

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[] = {\n')
    for p in bitpatterns:
        of.write('0b%s,' % p)
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_ABS[] = {')
    for i in xrange(len(vallist_abs)):
        if i % 32 == 0:
            of.write('\n    ');
        of.write('%i,' % vallist_abs[i])
    of.write('\n};\n')

    of.write('const uint8_t ' + name_prefix + '_LIGHT_VOLTAGE_TO_EV_DIFFS[] = {')
    for i in xrange(len(vallist_diffs)):
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
        for i in xrange(0, len(bits), 8):
            bytes.append('0b' + ''.join(bits[i:i+8]))
        of.write(', '.join(bytes))
        of.write(' };\n')
    write_x_bits(3)
    write_x_bits(10)

    return True, ()

# This is useful for santiy checking calculations. It outputs a graph of
# amplified voltage against EV which can be compared with voltage readings
# over the two input pins (or between the input pin and ground if we're
# using single input mode).
def output_sanity_graph():
    f = open("sanitygraph.csv", "w")
    f.write("v," + ','.join(["s" + str(n+1) for n in xrange(len(amp_stages))]) + ',')
    f.write(','.join(["b" + str(n+1) for n in xrange(len(amp_stages))]) + '\n')
    for v in xrange(b_voltage_offset, 256):
        f.write("%i" % v)
        voltage = (v * bv_to_voltage)
        bins = [ ]
        for stage in xrange(len(amp_stages)):
            ev = voltage_and_oa_resistor_to_ev(voltage, amp_stages[stage][0])
            ev +=  amp_stages[stage][1]
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
    # 'S' postfix is implicit.
    '60',
    '48',
    '38',

    '30',
    '24',
    '18',

    '15',
    '12',
    '9',

    '8',
    '6',
    '5',

    '4',
    '3',
    '2.5',

    '2',
    '1.6',
    '1.3',

    '1',
    # Initial '1/' starts here.
    '1.3',
    '1.6',

    '2',
    '2.5',
    '3',

    '4',
    '5',
    '6',

    '8',
    '10',
    '13',

    '15',
    '20',
    '25',

    '30',
    '40',
    '50',

    '60',
    '75',
    '95',

    '125',
    '160',
    '180',

    '250',
    '310',
    '400',

    '500',
    '630',
    '800',

    '10X',
    '13X',
    '16X',

    '20X',
    '25X',
    '32X',

    '40X',
    '50X',
    '63X',

    '80X',
    '10X0',
    '13X0',

    '16X0'
]
assert(max(map(len, shutter_speeds_thirds)) == 4)

shutter_speeds_eighths = [
    # 'S' postfix is implicit.
    '60',
    '55',
    '50',
    '46',
    '42',
    '39',
    '36',
    '32',

    '30',
    '27',
    '25',
    '23',
    '21',
    '19',
    '18',
    '16',

    '15',
    '14',
    '13',
    '12',
    '11',
    '10',
    '9', # TODO: Bit awkward here
    '8.2',

    '8',
    '7.5',
    '6.5',
    '6',
    '5.5',
    '5',
    '4.8',
    '4.4',

    '4',
    '3.7',
    '3.4',
    '3.1',
    '2.8',
    '2.6',
    '2.4',
    '2.2',

    '2',
    '1.8',
    '1.7',
    '1.5',
    '1.4',
    '1.3',
    '1.2',
    '1.1',

    '1',
    # Initial '1/' starts here.
    '1.1',
    '1.2',
    '1.3',
    '1.4',
    '1.5',
    '1.7',
    '1.8',

    '2',
    '2.2',
    '2.4',
    '2.6',
    '2.8',
    '3',
    '3.4',
    '3.7',

    '4',
    '4.4',
    '4.8',
    '5.2',
    '5.7',
    '6.2',
    '6.7',
    '7.3',

    '8',
    '8.7',
    '9.5',
    '10',
    '11',
    '12',
    '13',
    '14', # TODO: Bit awkward here.

    '15',
    '16',
    '18',
    '19',
    '21',
    '23',
    '25',
    '27',

    '30',
    '33',
    '36',
    '39',
    '42',
    '46',
    '50',
    '55',

    '60',
    '65',
    '70',
    '80',
    '85',
    '90',
    '100',
    '110',

    '125',
    '135',
    '150',
    '160',
    '180',
    '190',
    '210',
    '230',

    '250',
    '270',
    '300',
    '320',
    '350',
    '390',
    '420',
    '460',

    '500',
    '550',
    '590',
    '650',
    '700',
    '770',
    '840',
    '920',

    '10X',
    '11X',
    '12X',
    '13X',
    '14X',
    '15X',
    '17X',
    '18X',

    '20X',
    '22X',
    '24X',
    '26X',
    '28X',
    '30X',
    '34X',
    '37X',

    '40X',
    '44X',
    '48X',
    '52X',
    '57X',
    '62X',
    '68X',
    '73X',

    '80X',
    '87X',
    '95X',
    '10X0',
    '11X0',
    '12X0',
    '13X0',
    '15X0',
    '16X0'
]
assert(max(map(len, shutter_speeds_eighths)) == 4)

shutter_speeds_tenths = [
    # 'S' postfix is implicit.
    '60',
    '56',
    '52',
    '48',
    '45',
    '42',
    '40',
    '37',
    '35',
    '32',

    '30',
    '28',
    '26',
    '24',
    '23',
    '21',
    '20',
    '19',
    '17',
    '16', # TODO: Check this

    '15',
    '14',
    '13',
    '12',
    '11',
    '10.5',
    '10',
    '9',
    '8.5',
    '8', # TODO: Difficult to give an appropriate but unique value here.

    '8',
    '7.5',
    '7',
    '6.5',
    '6',
    '5.5',
    '5.3',
    '5',
    '4.5',
    '4.3',

    '4',
    '3.7',
    '3.5',
    '3.2',
    '3',
    '2.8',
    '2.6',
    '2.5',
    '2.3',
    '2.1',

    '2',
    '1.9',
    '1.7',
    '1.6',
    '1.5',
    '1.4',
    '1.3',
    '1.2',
    '1.1',
    '1.05',

    '1',
    # Initial slash starts here.
    '1.1',
    '1.05',
    '1.2',
    '1.3',
    '1.4',
    '1.5',
    '1.6',
    '1.7',
    '1.9',

    '2',
    '2.1',
    '2.3',
    '2.5',
    '2.6',
    '2.8',
    '3.0',
    '3.2',
    '3.5',
    '3.7',

    '4',
    '4.3',
    '4.6',
    '4.9',
    '5.3',
    '5.7',
    '6.1',
    '6.5',
    '7.0',
    '7.5',

    '8',
    '8.5',
    '9',
    '9.5',
    '10.5',
    '11',
    '12',
    '13',
    '14',
    '14.5',

    '15',
    '16',
    '17',
    '19',
    '20',
    '21',
    '23',
    '24',
    '26',
    '28',

    '30',
    '32',
    '35',
    '37',
    '40',
    '42',
    '46',
    '49',
    '52',
    '56',

    '60',
    '65',
    '70',
    '75',
    '80',
    '85',
    '90',
    '100',
    '105',
    '110',

    '125',
    '135',
    '145',
    '155',
    '165',
    '175',
    '190',
    '205',
    '220',
    '230',

    '250',
    '270',
    '290',
    '310',
    '330',
    '350',
    '380',
    '405',
    '435',
    '465',

    '500',
    '535',
    '575',
    '615',
    '660',
    '710',
    '760',
    '810',
    '870',
    '930',

    '10X',
    '11X',
    '115X',
    '12X',
    '13X',
    '14X',
    '15X',
    '16X',
    '17X',
    '19X',

    '20X',
    '21X',
    '23X',
    '25X',
    '26X',
    '28X',
    '30X',
    '32X',
    '35X',
    '37X',

    '40X',
    '43X',
    '46X',
    '50X',
    '53X',
    '57X',
    '61X',
    '65X',
    '70X',
    '75X',

    '80X',
    '86X',
    '92X',
    '98X',
    '10X0',
    '11X0',
    '12X0',
    '13X0',
    '14X0',
    '15X0',

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
            for i in xrange(0, 4, 2):
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

    ofh.write("#define NUM_AMP_STAGES %i\n" % len(amp_stages))
    ofh.write("#define FOR_EACH_AMP_STAGE(x) ")
    for i in xrange(1, len(amp_stages)+1):
        ofh.write("x(%i) " % i)
    ofh.write("\n")

    ofc.write("#include <stdint.h>\n")

    e, pr = None, None
    for i in xrange(len(amp_stages)):
        resistor_value = amp_stages[i][0]
        stops_subtracted = amp_stages[i][1]

        ofh.write("#define STAGE%i_RESISTOR_NUMBER %i\n" % (i+1, op_amp_resistor_value_to_resistor_number[resistor_value]))
        ofh.write("#define STAGE%i_STOPS_SUBTRACTED %i\n" % (i+1, int(stops_subtracted)))

        e, pr = output_ev_table(ofc, 'STAGE' + str(i+1), amp_stages[i][0], amp_stages[i][1])
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_BITPATTERNS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_ABS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_DIFFS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_TENTHS[];\n" % (i+1))
        ofh.write("extern const uint8_t STAGE%i_LIGHT_VOLTAGE_TO_EV_THIRDS[];\n" % (i+1))
        if not e:
            sys.stderr.write("R ERROR %.3f: (%.3f, %.3f) at stage %i\n" % (amp_stages[i][0], pr[0], pr[1], i))
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
