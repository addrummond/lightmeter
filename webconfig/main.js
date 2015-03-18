var SIGNAL_FREQ = 1000;
var I_MODE_F1 = 18500;
var I_MODE_F2 = 19000;

function ssb(s, sh, f, t) {
    return s(t)*Math.cos(2*Math.PI*f*t) - sh(t)*Math.sin(2*Math.PI*f*t);
}

function approximate_square_wave(freq, mag, dutyCycle, phaseShift, hilbert) {
    var SERIES_LENGTH = 6;

    if (dutyCycle == null)
        dutyCycle = 0.5;
    if (phaseShift == null)
        phaseShift = 0;

    // See http://lpsa.swarthmore.edu/Fourier/Series/ExFS.html#CosSeriesDC=0.5
    return function (t) {
        t *= freq;
        // Convenient to have it start at beginning of positive.
        t -= dutyCycle/2;
        t += phaseShift;

        function a(n) {
            return 2*(mag/(n*Math.PI))*Math.sin(n*Math.PI*dutyCycle);
        }
        function fnh(i) {
            return a(i) * Math.cos(i*Math.PI*2*t);
        }
        function fh(i) {
            return a(i) * Math.sin(i*Math.PI*2*t);
        }

        var v = mag*dutyCycle;
        var f = (hilbert ? fh : fnh);
        for (var i = 1; i < SERIES_LENGTH; ++i) {
            v += f(i);
        }

        return v;
    };
}

function hilbert_of_approximate_square_wave(freq, mag, dutyCycle, phaseShift) {
    return approximate_square_wave(freq, mag, dutyCycle, phaseShift, true);
}

function encode_signal(out, sampleRate, signal, signalFreq, carrierFreq, mag) {
    if (signal.length == 0)
        return;

    var rat = parseInt(carrierFreq/signalFreq);
    if (out.length < signal.length * rat * 2)
        throw new Error("Assertion error in encode_signal: output buffer too short");

    for (var i = 0; i < signal.length;) {
        var start = i;

        var seq1Count = 0;
        var seq2Count = 0;
        var seq1V = signal[i];
        for (++i; signal[i] == seq1V && i < signal.length; ++i, ++seq1Count)
            ;

        if (i < signal.length) {
            var seq2V = signal[i];
            for (++i; signal[i] == seq2V && i < signal.length; ++i, ++seq2Count);
                ;
        }
        else {
            seq2Count = seq1Count;
        }

        var phaseShift, dutyCycle;
        if (seq1V == 0) {
            phaseShift = 1;
            dutyCycle = seq2Count / seq1Count;
        }
        else {
            phaseShift = 0;
            dutyCycle = seq1Count / seq2Count;
        }
        dutyCycle /= 2;

        var dv = i - start;
        var wf = approximate_square_wave((signalFreq/dv)*2, mag, dutyCycle, phaseShift);
        var hwf = hilbert_of_approximate_square_wave((signalFreq/dv)*2, mag, dutyCycle, phaseShift);

        for (var j = start; j < i; ++j) {
            for (var k = 0; k < rat; ++k) {
                var oi = j*rat + k;
                if (oi >= out.length)
                    break;

                var t = oi / sampleRate;
                var v = ssb(wf, hwf, carrierFreq, t);
                out[oi] = v;
            }
        }
    }
}

function approximate_triangle_wave(freq, mag, m, hilbert) {
    // See http://mathworld.wolfram.com/FourierSeriesTriangleWave.html

    var SERIES_LENGTH = 4;

    var f = (hilbert ? Math.cos : Math.sin);

    return function (t) {
        // Not quite sure where the PI factor comes from, but it must be missing
        // somewhere below.
        t *= freq * Math.PI;

        function b(n) {
            var k = (2*m*m)/(n*n*(m-1)*Math.PI*Math.PI);
            var s = Math.sin((n*(m-1)*Math.PI)/(m));
            return k*s;
        }

        var v = 0;
        var s = (hilbert ? -1 : 1);
        for (var i = 1; i <= SERIES_LENGTH; ++i, s *= -1) {
            v += s*b(i)*f(i*t);
        }

        return mag*v + mag;
    };
}

function hilbert_of_approximate_triangle_wave(freq, mag, m) {
    return approximate_triangle_wave(freq, mag, m, true);
}

function myf(t) {
    //return hilbert_of_approximate_triangle_wave(1000, 0.5, 2)(t);
    //return approximate_triangle_wave(1000,0.25,2)(t)*Math.sin(Math.PI*2*19000*t);
    return ssb(approximate_triangle_wave(1000,0.25,2), hilbert_of_approximate_triangle_wave(1000, 0.25,2), 19000, t);

    // QAC.
    //var s1 = approximate_square_wave(SIGNAL_FREQ, 0.2, 0.5, 0);
    //var s2 = approximate_square_wave(SIGNAL_FREQ, 0.2, 0.5, 1);
    //return s1(t)*Math.cos(2*Math.PI*I_MODE_F1*t) - s2(t)*Math.sin(2*Math.PI*I_MODE_F1*t);

    return (//0.2*Math.cos(2*Math.PI*I_MODE_F1*t) +
            ssb(approximate_square_wave(SIGNAL_FREQ, 0.2, 0.5),
                hilbert_of_approximate_square_wave(SIGNAL_FREQ, 0.2, 0.5),
                I_MODE_F1, t)) +
           //0
           (//0.2*Math.cos(2*Math.PI*I_MODE_F2*t) +
           ssb(approximate_square_wave(SIGNAL_FREQ, 0.2, 0.5),
               hilbert_of_approximate_square_wave(SIGNAL_FREQ, 0.2, 0.5),
               I_MODE_F2, t))
           ;
}

var FILTER = false;

audioCtx = new AudioContext();
var buffer = audioCtx.createBuffer(1, audioCtx.sampleRate, audioCtx.sampleRate);
var samples = buffer.getChannelData(0);

for (var i = 0; i < samples.length; ++i) {
    var t = i / audioCtx.sampleRate;
    samples[i] = myf(t);
    document.write(samples[i] + '<br>\n');
}
/*var message = [1,0,1,0];//[1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0];
encode_signal(samples, audioCtx.sampleRate, [1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0], 1000, 19000, 0.2);
for (var i = 0; i < message.length*(19000/1000); ++i) {
    document.write(samples[i] + '<br>\n');
}*/

var bufS = audioCtx.createBufferSource();
bufS.buffer = buffer;
if (FILTER) {
    var filter1;
    filter1 = audioCtx.createBiquadFilter();
    filter1.type = 'bandpass';
    filter1.frequency.value = (I_MODE_F1+(SIGNAL_FREQ/2) + I_MODE_F2+(SIGNAL_FREQ/2))/2;
    filter1.Q.value = filter1.frequency.value / (I_MODE_F2-I_MODE_F1+50);
    filter1.gain.value = 1;

    var filter2 = audioCtx.createBiquadFilter();
    filter2.type = 'highpass';
    filter2.frequency.value = I_MODE_F1-50;
    filter2.gain.value = 1;

    bufS.connect(filter2);
    filter1.connect(filter2);

    filter2.connect(audioCtx.destination);
}
else {
    bufS.connect(audioCtx.destination);
}
bufS.start();
