import wave
import math
import sys
import re

def samples_to_wave(samples, of):
    w = wave.open(of, 'w')
    w.setparams((1, 2, 44100, len(samples), 'NONE', 'not compressed'))
    w.writeframes(samples)
    w.close()

def get_samples_from_webapp(s):
    lines = re.split(r"\r?\n", s)
    samples = bytearray(len(lines)*2)
    i = 0
    for l in lines:
        if re.match(r"^\s*$", l):
            continue

        m = re.match(r"\s*([^\s]+)\s*$", l)
        if not m:
            raise Exception("Bad file format [1]")
        v = None
        try:
            v = float(m.group(1))
        except ValueError:
            raise Exception("Bad file format [2]")

        if v < -1.0 or v > 1.0:
            print(v)
            raise Exception("Bad file format [3] %s = %f line %i" % (m.group(1), v, i//2))

        fv = int(round(v*65536.0*0.5))
        samples[i] = fv & 0xFF
        samples[i+1] = (fv >> 8) & 0xFF
        i += 2
    return samples

if __name__ == '__main__':
    inputfile = sys.argv[1]
    outputfile = sys.argv[2]
    times_to_repeat = int(sys.argv[3]) if len(sys.argv) >= 4 else 1

    f = open(inputfile)
    c = f.read()
    samples = None
    if inputfile.endswith(".web"):
        samples = get_samples_from_webapp(c) * times_to_repeat
    else:
        assert False
    if outputfile.endswith(".wav"):
        samples_to_wave(samples, outputfile)
    else:
        assert False
