import wave
import math
import sys
import re

def samples_to_wave(samples, of):
    w = wave.open(of, 'w')
    w.setparams((1, 1, 44000, len(samples), 'NONE', 'not compressed'))
    w.writeframes(bytearray((s+128 if s < 128 else -((256-s)-128) for s in samples)))
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

if __name__ == '__main__':
    inputfile = sys.argv[1]
    outputfile = sys.argv[2]
    times_to_repeat = int(sys.argv[3]) if len(sys.argv) >= 4 else 1

    f = open(inputfile)
    c = f.read()
    samples = get_samples_from_text(c) * times_to_repeat
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
