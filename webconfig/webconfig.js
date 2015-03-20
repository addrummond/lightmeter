var SIGNAL_FREQ = 1000;
var I_MODE_F1 = 18500;
var I_MODE_F2 = 19000;

function goetzel(freq, input, output) {
    var inter = new Float32Array(input.length);
    for (var i = 0; i < input.length; ++i) {
        var prev1 = i >= 1 ? input[i-1] : 0;
        var prev2 = i >= 2 ? input[i-2] : 0;
        inter[i] = input[i] + 2*Math.cos(2*Math.PI*freq)*prev1 - prev2;
    }

    for (var i = 0; i < input.length; ++i) {
        var prev = i >= 1 ? inter[i-1] : inter[inter.length-1];
        output[i] = inter[i] - Math.cos(-2*Math.PI*freq)*prev;
    }
}

function fir(coefficients, input, output) {
    var ops = 0;
    for (var i = 0; i < input.length; ++i) {
        var o = 0;
        for (var j = i, ci = 0; ci < coefficients.length; --j, ++ci) {
            var c = coefficients[ci];
            var v;
            if (j >= 0)
                v = input[j];
            else
                v = input[input.length+j];
            o += c*v;
            ++ops;
        }
        output[i] = o;
    }
    console.log('OPS: ' + ops);
}

// Filters out everything except 19000Hz+/-100.
var CS = [
      0.002557915382766357,
      -0.020906425544830624,
      -0.000960939427215661,
      -0.0012470051092847469,
      0.0003097908913539428,
      0.0003097908913539428,
      -0.0012470051092847469,
      -0.000960939427215661,
      -0.020906425544830624,
      0.002557915382766357
];

function ssb(s, sh, f, t) {
    return s(t)*Math.cos(2*Math.PI*f*t) - sh(t)*Math.sin(2*Math.PI*f*t);
}

function approximate_square_wave(freq, mag, dutyCycle, phaseShift, hilbert) {
    // TODO: Figure out relation of this value to longest/shortest usable
    // duty cycle.
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

function approximate_triangle_wave(freq, mag, dutyCycle, phaseShift, hilbert) {
    // See http://mathworld.wolfram.com/FourierSeriesTriangleWave.html

    // It seems that the longest/shortest usable duty cycle is on the order of
    // 1/SERIES_LENGTH. Conservatively, we can use duty cycles as low as 0.1
    // and as high as 0.9 with a SERIES_LENGTH of 20.
    var SERIES_LENGTH = 5;

    if (dutyCycle == null)
        dutyCycle = 0.5 ;
    if (phaseShift == null)
        phaseShift = 0;

    var m = 1/dutyCycle;

    var f = (hilbert ? Math.cos : Math.sin);

    return function (t) {
        t *= freq*2;

        // Convenient to have it start at beginning of positive.
        t -= dutyCycle;

        function b(n) {
            var k = (2*m*m)/(n*n*(m-1)*Math.PI*Math.PI);
            var s = Math.sin((n*(m-1)*Math.PI)/(m));
            return k*s;
        }

        var v = 0;
        var s = (hilbert ? -1 : 1);
        for (var i = 1; i <= SERIES_LENGTH; ++i, s *= -1) {
            v += s*b(i)*f(i*t*Math.PI);
        }

        return mag*v + mag;
    };
}

function hilbert_of_approximate_triangle_wave(freq, mag, dutyCycle, phaseShift) {
    return approximate_triangle_wave(freq, mag, dutyCycle, phaseShift, true);
}

function encode_signal(out, sampleRate, signal, signalFreq, carrierFreq, mag) {
    if (signal.length == 0)
        return;

    var rat = parseInt(carrierFreq/signalFreq);
    if (out.length < signal.length/2 * rat)
        throw new Error("Assertion error in encode_signal: output buffer too short");

    var oi = 0;
    for (var i = 0; i < signal.length;) {
        var start = i;

        var seq1Count = 1;
        var seq2Count = 1;
        var seq1V = signal[i];
        var lastOne = false;
        for (++i; signal[i] == seq1V && i < signal.length; ++i, ++seq1Count)
            ;

        if (i < signal.length) {
            var seq2V = signal[i];
            for (++i; signal[i] == seq2V && i < signal.length; ++i, ++seq2Count);
                ;
        }
        else {
            lastOne = true;
            seq2Count = seq1Count;
        }

        var phaseShift, dutyCycle;
        if (seq1V == 0) {
            phaseShift = 1;
            dutyCycle = seq2Count / (seq1Count + seq2Count);
        }
        else {
            phaseShift = 0;
            dutyCycle = seq1Count / (seq1Count + seq2Count);
        }

        var dv = (i - start)*0.5;
        if (lastOne)
            dv *= 0.5;
        var freq2 = signalFreq/dv;
        var wf = approximate_triangle_wave(freq2, mag, dutyCycle, phaseShift);
        var hwf = hilbert_of_approximate_triangle_wave(freq2, mag, dutyCycle, phaseShift);

        console.log('r', sampleRate/freq2);
        for (var j = 0; j < sampleRate/freq2; ++j) {
            var t = j/sampleRate;
            var v = ssb(wf, hwf, carrierFreq, t);
            //var v = hwf(t);
            out[oi++] = v;
        }
    }
}

var FILTER = true;

audioCtx = new AudioContext();
var buffer = audioCtx.createBuffer(1, audioCtx.sampleRate, audioCtx.sampleRate);

function test_message() {
    var samples = buffer.getChannelData(0);
    var myMessage = [1,0,1,0];//[1,0,1,1,0,0,1,1,1,0,0,0,1,1,1,1,0,0,0,0];//1,1,0,0,0];//[1,0,1,0,1,1,1,0,0,0,1,1,0,0];//1,0,1,0,1];
    myMessage = new Array(1000);
    for (var i = 0; i < myMessage.length; ++i) {
        myMessage[i] = i%2;
    }
    console.log(JSON.stringify(myMessage));
    encode_signal(samples, audioCtx.sampleRate, myMessage, 1050, 14700, 0.2);
    for (var i = 0; i < myMessage.length*(14700/1050)*0.5; ++i) {
        var t = i/audioCtx.sampleRate;
        var v = samples[i];
        v -= 0.2*Math.cos(2*Math.PI*14700*t+Math.PI/2);
        //v *= Math.cos(2*Math.PI*14700*t);
        document.write(v + '<br>\n');
    }
}

function test_f() {
    var samples = buffer.getChannelData(0);
    function myf(t) {
        //return approximate_triangle_wave(1000, 0.25, 0.5)(t);//*Math.cos(2*Math.PI*14700*t);
        return ssb(approximate_triangle_wave(1000,0.25,0.5), hilbert_of_approximate_triangle_wave(1000,0.25,0.5), 14700, t)*0.5*Math.cos(2*Math.PI*14700*t+Math.PI/2);
        //return hilbert_of_approximate_triangle_wave(1050, 0.25, 0.5)(t);
    }

    for (var i = 0; i < samples.length; ++i) {
        var t = i / audioCtx.sampleRate;
        samples[i] = myf(t);
        document.write(samples[i] + '<br>\n');
    }
}

//test_f();
test_message();

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
    filter2.gain.value = 100;

    bufS.connect(filter2);
    //filter1.connect(filter2);

    filter2.connect(audioCtx.destination);
}
else {
    bufS.connect(audioCtx.destination);
}
bufS.start();
