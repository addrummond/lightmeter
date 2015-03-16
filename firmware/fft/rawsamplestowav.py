import wave
import math
import sys
import re

def complement_convert(x):
    if x < 128:
        return x + 128
    else:
        return -((256-x)-128)

def samples_to_wave(samples, of):
    w = wave.open(of, 'w')
    w.setparams((1, 1, 44100, len(samples), 'NONE', 'not compressed'))
    w.writeframes(samples)
    w.close()

def samples_to_text(samples, of):
    f = of
    if isinstance(f, str):
        f = open(f, "w")
    for s in samples:
        f.write("%i\n" % (s if s < 127 else -(256-s)))
    f.close()

def get_samples_from_text(s):
    lines = re.split(r"\r?\n", s)
    samples = bytearray(len(lines))
    i = 0
    for l in lines:
        if re.match(r"^\s*$", l):
            continue

        m = re.match(r"^\s*(\d{1,3})\s*$", l)
        if not m:
            raise Exception("Bad file format")
        v = int(m.group(1))
        samples[i] = v
        i += 1
    return samples

def get_samples_from_webapp(s):
    lines = re.split(r"\r?\n", s)
    samples = bytearray(len(lines))
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
            raise Exception("Bad file format [3]")

        samples[i] = int(round((v+1.0)*128.0))
        i += 1
    return samples

if __name__ == '__main__':
    inputfile = sys.argv[1]
    outputfile = sys.argv[2]
    times_to_repeat = int(sys.argv[3]) if len(sys.argv) >= 4 else 1

    f = open(inputfile)
    c = f.read()
    samples = None
    if inputfile.endswith(".web"):
        samples = get_samples_from_webapp(c)
    else:
        samples = get_samples_from_text(c) * times_to_repeat
        samples = bytearray((complement_convert(x) for x in samples))
    if outputfile.endswith(".wav"):
        samples_to_wave(samples, outputfile)
    else:
        samples_to_text(samples, outputfile)

    # Test output of sine wave.
    #samples = bytearray(100000)
    #f = 0.0
    #for i in range(0, len(samples)):
    #    samples[i] = int(round(math.sin(f)*30 + 127))
    #    f += math.pi*2/50
    #samples_to_wave(samples, outputfile)
