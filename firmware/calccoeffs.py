import math
import sys

def calc(baseband, carrier, sample, nseries, maxflush):
    """Assuming a constant baseband frequency, there will be nseries sidebands
       displaced at intervals of baseband. However, the baseband frequency will
       vary over a range determined by the maximum number of 0s or 1s transmitted
       in sequence (maxflush). E.g., if maxflush == 1, the frequency range will
       be limited to the baseband frequency:

         <----------------------->                       (baseband period)
           __________
                     |
                     |
                     |
                     |__________

       However, if maxflush is e.g. 2, then in addition to waves with the
       baseband frequency, we will also have waves such as the following:

          <------------------------------->              (1.5 * baseband period)
           ____________________
                               |
                               |
                               |
                               |__________


          <----------------------------------------->    (2 * baseband period)
           ____________________
                               |
                               |
                               |
                               |____________________

       Since the additional waves all have lower frequencies than the baseband,
       this pushes the center of the sideband further up the frequency range.
       For the following calculation, we assume that on average the same number
       of zeroes and ones are transmitted in any given signal."""

    rad = 0.0
    baseperiod = 1/baseband
    n = int(maxflush)*2-1
    for fl in range(n):
       newperiod = baseperiod + (baseperiod * 0.5 * fl)
       basefreq = 1/newperiod
       rad += 2 * math.pi * ((carrier + (basefreq/2))/sample)
    rad /= n

    return dict(coscoeff=math.cos(rad), sincoeff = math.sin(rad))

if __name__ == '__main__':
    assert len(sys.argv) == 6
    args = { }
    for arg in sys.argv[1:]:
        name, val = arg.split('=')
        args[name] = float(val)
    r = calc(**args)
    print("cos = %.10f, sin = %.10f" % (r['coscoeff'], r['sincoeff']))
