import wave
import math
import sys
import re

def samples_to_wave(samples, of):
    w = wave.open(of, 'w')
    w.setparams((1, 1, 44000, len(samples), 'NONE', 'not compressed'))
    w.writeframes(samples)
    w.close()

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
        v = samples[i]
        if v > 127:
            v = -(v-127)
        samples[i] = int(m.group(1))
        i += 1
    return samples

if __name__ == '__main__':
    inputfile = sys.argv[1]
    outputfile = sys.argv[2]
    times_to_repeat = int(sys.argv[3]) if len(sys.argv) >= 4 else 1

    f = open(inputfile)
    c = f.read()
    samples = get_samples_from_text(c) * times_to_repeat
    samples_to_wave(samples, outputfile)

    # Test output of sine wave.
    #samples = bytearray(100000)
    #f = 0.0
    #for i in range(0, len(samples)):
    #    samples[i] = int(round(math.sin(f)*30 + 127))
    #    f += math.pi*2/50
    #samples_to_wave(samples, outputfile)
