import math
import sys

if sys.version_info <= (3, 0):
    sys.stderr.write("Run this script using Python 3")
    sys.exit(1)

def calc(baseband, carrier, sample):
    return dict(coscoeff=math.cos(2*math.pi*(carrier/sample)),
                sincoeff=math.sin(2*math.pi*(carrier/sample)))

if __name__ == '__main__':
    assert len(sys.argv) == 4
    args = { }
    for arg in sys.argv[1:]:
        name, val = arg.split('=')
        args[name] = float(val)
    r = calc(**args)
    print("cos = %.10f, sin = %.10f" % (r['coscoeff'], r['sincoeff']))
