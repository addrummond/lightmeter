import math
import sys

# http://www.vishay.com/docs/81521/bpw34.pdf, Fig 1, p. 2.
# Temp in C, RDC in log10 pA.
# Slope is 1/16 with y in log10.
# Constant is 1.
def temp_to_rdc(temp):
    return (temp / 16.0) + 1.0

# http://www.vishay.com/docs/80085/measurem.pdf, Fig 20, p. 7
# Temp in C, OCV in mV.
# Slope is -2.857142857142857.
# Constant is 442.85714285714298
#def temp_to_ocv(temp):
#    return (temp * -2.857142857142857) + 442.85714285714298

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
def irrad_to_illum(irrad):
    return irrad + 3.0

# Convert lux to EV at ISO 100.
# See http://stackoverflow.com/questions/5401738/how-to-convert-between-lux-and-exposure-value
def lux_to_ev_at_100(lux):
    return lux * 2.5

def temp_and_voltage_to_ev(temp, v):
    rdc = temp_to_rdc(temp)
    irrad = ocv_and_rdc_to_irrad(v, rdc)
    lux = irrad_to_illum(irrad)
    return lux_to_ev_at_100(lux)

# Temperature, EV and voltage are all represented as unsigned 8-bit values
# on the microcontroller.
#
# Temperature: from -51C to 51C in 0.4C intervals (0 = -51C)
# Voltage: from 0mV to 510mV in 2mV intervals.
# EV: from -5 to 26EV in 1/8 EV intervals.
#
# The table is a 2D array of bytes mapping (temperature, voltage) to EV*8.
# Temperature goes up in steps of 16 (in terms of the 8-bit value).
# Voltage goes up in steps of 2 (in terms of the 8-bit value).
#
# The resulting table would take up (256/2)*(256/16) = 2048 bytes = 2K,
# which would just about fit in the attiny85's 8K of flash. However,
# to make the table more compact, each row (series of voltages) is stored
# with the first 8-bit value followed by a series of (compacted) 1-bit values,
# where each 1-bit value indicates the (always positive) difference with the
# previous value. Each row is of the form:
#
#     8-bit 15*1-bit,  8-bit 15*1-bit,  8-bit 15*1-bit, 8-bit 15*1-bit, 8-bit 15*1-bit,  8-bit 15*1-bit,  8-bit 15*1-bit, 8-bit 15*1-bit
#
# The value of any given cell can therefore be determined by doing at most
# 15 additions. Each addition takes 1 clock cycle. If we conservatively assume
# 10 cycles for each iteration of the loop, that's 150 cycles, which at 8MHz
# is about 1/150th of a second.
#
# Each row is therefore 16 bytes, meaning that the table as a whole takes
# up a more managable 256 bytes.
#
#             voltage
#
#               (0) (2) (4) (8)
#  temp  (0)    ev  ev  ev  ev ...
#        (16)   ev  ev  ev  ev ...
#        (32)   ev  ev  ev  ev ...
#             ...
def output_table():
    sys.stdout.write('    { ')

    for t in xrange(0, 256, 16):
        temperature = (t * 0.4) - 51.0

        for sv in xrange(0, 256, 32):
            # Write the absolute 8-bit EV value.
            voltage = sv * 2.0
            ev = temp_and_voltage_to_ev(temperature, voltage)
            eight = int(round(ev * 8))
            if sv == 0 and t != 0:
                sys.stdout.write("      ")
            sys.stdout.write("%i," % eight)

            # Write the 1-bit differences (two bytes).
            prev = eight
            for j in xrange(0, 2):
                eight2 = None
                o = ""
                for v in xrange(sv + 2, sv + 16, 2):
                    ev2 = temp_and_voltage_to_ev(temperature, v)
                    eight2 = int(round(ev2 * 8))
                    assert eight2 - prev <= 1
                    if eight2 - prev == 0:
                        o += '0'
                    else:
                        o += '1'
                    prev = eight2
                sys.stdout.write("%i," % int(o,2))
                prev = eight2

        sys.stdout.write('\n')

    sys.stdout.write('    }')

if __name__ == '__main__':
    sys.stdout.write("""
#include <stdint.h>

const uint8_t TEMP_AND_VOLTAGE_TO_EV[] =
""")
   
    output_table()
    
    sys.stdout.write(";\n")
