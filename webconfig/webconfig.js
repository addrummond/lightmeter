var SIGNAL_FREQ = 126;
var MASTER_CLOCK_HZ = 20000;
var MASTER_DATA_HZ = 18000;

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

function ssb(s, sh, f, t) {
    return s(t)*Math.cos(2*Math.PI*f*t);
    //return s(t)*Math.cos(2*Math.PI*f*t) - sh(t)*Math.sin(2*Math.PI*f*t);
}

function approximate_square_wave(series_length, freq, mag, dutyCycle, phaseShift, hilbert) {
    if (dutyCycle == null)
        dutyCycle = 0.5;
    if (phaseShift == null)
        phaseShift = 0;

    // See http://lpsa.swarthmore.edu/Fourier/Series/ExFS.html#CosSeriesDC=0.5
    return function (t) {
        t += (1/freq)*(phaseShift/2);
        t *= freq;
        // Convenient to have it start at beginning of positive.
        t -= dutyCycle/2;

        function fnh(n) {
            return (2 * Math.sin(Math.PI*n*dutyCycle)*Math.cos(2*Math.PI*n*t))/(n*Math.PI);
        }
        function fh(n) {
            return (2 * Math.sin(Math.PI*n*dutyCycle)*Math.sin(2*Math.PI*n*t))/(n*Math.PI);
        }

        var v = dutyCycle;
        var f = (hilbert ? fh : fnh);
        for (var i = 1; i <= series_length; ++i) {
            v += f(i);
        }
        v *= mag;

        return v;
    };
}

function hilbert_of_approximate_square_wave(series_length, freq, mag, dutyCycle, phaseShift) {
    return approximate_square_wave(series_length, freq, mag, dutyCycle, phaseShift, true);
}

// Bit index into byte array.
function bi(arr, i) {
    var bit = i % 8;
    var byte = parseInt(i/8);
    var thebyte = arr[byte];
    return (thebyte & (1 << bit)) >> bit;
}

var SIGNAL_SERIES_LENGTH = 11;
function encode_signal(out, sampleRate, offset, signal, repeat, signalFreq, carrierFreq, mag) {
    if (signal.length == 0)
        return;

    var slen = signal.length * 8;

    if (out.length < (slen*repeat)/2 * parseInt(carrierFreq/signalFreq))
        throw new Error("Assertion error in encode_signal: output buffer too short");

    var oi = offset;
    var elapsedTime = 0;
    var idealElapsedTime = 0;
    var loopCountPlusOne = 1;
    for (var i = 0; i < slen*repeat; ++loopCountPlusOne) {
        var seq1Count = 1;
        var seq2Count = 1;
        var seq1V = bi(signal, i % slen);
        var lastOne = false;
        for (++i; bi(signal, i % slen) == seq1V && i < slen*repeat; ++i)
            ++seq1Count;

        if (i < slen*repeat) {
            var seq2V = bi(signal, i % slen);
            for (++i; bi(signal, i % slen) == seq2V && i < slen*repeat; ++i)
                ++seq2Count;
        }
        else {
            lastOne = true;
            seq2Count = seq1Count;
        }

        var phaseShift, dutyCycle;
        var countSum = seq1Count+seq2Count;
        if (seq1V == 0) {
            phaseShift = 2*seq2Count / countSum;
            dutyCycle = seq2Count / countSum;
        }
        else {
            phaseShift = 0;
            dutyCycle = seq1Count / countSum;
        }

        var dv = countSum*0.5;
        var freq2 = signalFreq/dv;
        var wf = approximate_square_wave(SIGNAL_SERIES_LENGTH, freq2, mag, dutyCycle, phaseShift);
        var hwf = hilbert_of_approximate_square_wave(SIGNAL_SERIES_LENGTH, freq2, mag, dutyCycle, phaseShift);

        var dd = (sampleRate/signalFreq)*dv;
        var t;
        var diff = idealElapsedTime - elapsedTime;
        for (var j = 0; j < dd && oi < out.length; j += 1) {
            t = j/sampleRate;
            //var v = wf(t + (elapsedTime % (1/signalFreq)));
            var v = ssb(wf, hwf, carrierFreq, t + diff);
            out[oi++] += v;
        }
        elapsedTime += (dd/sampleRate);
        idealElapsedTime += dv/signalFreq;
    }

    return oi - offset;
}

// http://www.electronics.dit.ie/staff/amoloney/lecture-24-25.pdf
var FREQ_DIFF = 200;
function fsk_encode_signal(out, sampleRate, offset, signal, repeat, signalFreq, carrierFreq, mag) {
    var modulationIndex = FREQ_DIFF / signalFreq;

    function s(k) {
        var bytei = parseInt(k / 8);
        var biti = k % 8;
        var b = (signal[bytei % signal.length] >> biti) & 1;
        return b ? 1 : -1;
    }

    var thetaMemo = { };
    function theta(k) {
        var total = 0;
        var i = 0;
        if (thetaMemo[k-1] != null) {
            i = k;
            total = thetaMemo[k-1];
        }
        for (; i <= k; ++i) {
            var diff = 0;
            if (i > 0)
                diff = s(i-1) - s(i);
            total += 2*Math.PI*diff*FREQ_DIFF*k*(1/signalFreq);
        }
        var r = total % (2*Math.PI);
        thetaMemo[k] = r;
        console.log("T", r);
        return r;
    }

    var i;
    var oi = offset;
    for (i = 0; i < parseInt((sampleRate/signalFreq) * signal.length * repeat) && oi < out.length; ++i) {
        var t = (i/sampleRate);
        var si = parseInt(t*signalFreq);
        var thetasi = theta(si);
        var r = mag*Math.cos((2*Math.PI*(carrierFreq+(s(si)*FREQ_DIFF))*t) + thetasi);
        out[oi++] += r;
    }

    return i;
}

audioCtx = new AudioContext();
var buffer = audioCtx.createBuffer(1, audioCtx.sampleRate*6, audioCtx.sampleRate);

var TEST_MESSAGE_ = [ 'H'.charCodeAt(0),
                      'E'.charCodeAt(0),
                      'L'.charCodeAt(0),
                      'L'.charCodeAt(0),
                      'O'.charCodeAt(0),
                      '!'.charCodeAt(0),
                      '!'.charCodeAt(0),
                      '!'.charCodeAt(0),
                      '!'.charCodeAt(0)
                    ];
var TEST_MESSAGE = new Uint8Array(hamming_get_encoded_message_byte_length_with_init_sequences(9, 10));
hamming_encode_message(TEST_MESSAGE_, TEST_MESSAGE, 10);

function debug_print_bitarray(arr) {
    var o = "";
    for (var i = 0; i < arr.length; ++i) {
        var v = arr[i];
        var s = v.toString(2);
        var j;
        for (j = s.length-1; j >= 0; --j)
            o += s[j];
        for (j = 0; j < 8-s.length; ++j)
            o += "0";
    }
    return o;
}

// Some test logging.
/*(function () {
var olen = hamming_get_max_output_length_given_input_length(TEST_MESSAGE.length);
var to = new Uint8Array(olen);
var r = hamming_scan_for_init_sequence(TEST_MESSAGE);
hamming_decode_message(TEST_MESSAGE.slice(1*4), to);
console.log("ORIGINAL", TEST_MESSAGE_);
console.log("ENCODED", TEST_MESSAGE);
console.log("DECODED", to);

console.log("TD", debug_print_bitarray([0xF0, 0xF0, 0xF0]));
console.log("PRESHIFT", debug_print_bitarray(TEST_MESSAGE));
hamming_bitshift_buffer_forward(TEST_MESSAGE, TEST_MESSAGE.length-4, 7);
console.log("SHIFTED", debug_print_bitarray(TEST_MESSAGE));
var r = hamming_scan_for_init_sequence(TEST_MESSAGE);
console.log(r);
})();*/

function test_message() {
    //console.log(TEST_MESSAGE);
    var samples = buffer.getChannelData(0);

    var MAG = 0.35;
    var siglen  = fsk_encode_signal(samples, audioCtx.sampleRate, 0, TEST_MESSAGE, 1*5, SIGNAL_FREQ, MASTER_DATA_HZ,  MAG);

    for (var i = 0; i < siglen; ++i) {
        var t = i/audioCtx.sampleRate;
        var v = samples[i];
        //v -= MAG*Math.cos(2*Math.PI*I_MODE_F1*t);
        //v *= MAG*Math.cos(2*Math.PI*I_MODE_F1*t);
        document.write(v + '<br>\n');
    }
}

function test_f() {
    var samples = buffer.getChannelData(0);
    function myf(t) {
        var bit = (t % (1/126) > (1/(126*2)) ? 1 : 0);
        return 0.5*bit*Math.cos(2*Math.PI*t*18000) +
               0.5*(!bit + 0)*Math.cos(2*Math.PI*t*18100);

        //return 0.5*Math.cos(2*Math.PI*t*(126/8))*Math.cos(2*Math.PI*t*18000 /*+ 10*Math.cos(2*Math.PI*t*126)*/);// + 0.5*Math.cos(2*Math.PI*t*18000);

        //return 0.5*Math.cos(2*Math.PI*t*(126/8))*Math.cos(2*Math.PI*t*18000) +
        //       0.5*Math.cos(2*Math.PI*t*(126/8))*Math.cos(2*Math.PI*t*20000);

        //return ((0.5*Math.cos(2*Math.PI*t*1000)*Math.cos(2*Math.PI*10000*t) - 0.5*Math.cos(2*Math.PI*t*1000)*Math.sin(2*Math.PI*10000*t)) -
        //        (0.5*Math.cos(2*Math.PI*t*1000)*Math.cos(2*Math.PI*t*10000)));

        //return approximate_square_wave(5, SIGNAL_FREQ, 1, 0.8)(t)*Math.cos(2*Math.PI*(MASTER_DATA_HZ)*t)*0.5;
        //return approximate_square_wave(20, 30/*10hz test*/, 1.0, 0.5)(t)*Math.cos(2*Math.PI*MASTER_CLOCK_HZ*t)*0.5;


        //return ssb(approximate_square_wave(3, SIGNAL_FREQ,0.25,0.5), hilbert_of_approximate_square_wave(3, SIGNAL_FREQ,0.25,0.5), MASTER_CLOCK_HZ, t)*
        //       1;

        //return ssb(function (t) { return 0.5*Math.sin(2*Math.PI*500*t) },
        //           function (t) { return -0.5*Math.cos(2*Math.PI*500*t) },
        //           MASTER_DATA_HZ,
        //           t)*1;
    }

    for (var i = 0; i < samples.length; ++i) {
        var t = i / audioCtx.sampleRate;
        samples[i] = myf(t);
        document.write(samples[i] + '<br>\n');
    }
}

var FILTER = false;

//test_f();
test_message();

var bufS = audioCtx.createBufferSource();
bufS.buffer = buffer;
if (FILTER) {
    var filter1;
    filter1 = audioCtx.createBiquadFilter();
    filter1.type = 'bandpass';
    filter1.frequency.value = (MASTER_DATA_HZ+(SIGNAL_FREQ/2) + MASTER_CLOCK_HZ+(SIGNAL_FREQ/2))/2;
    filter1.Q.value = filter1.frequency.value / (MASTER_CLOCK_HZ-MASTER_DATA_HZ+50);
    filter1.gain.value = 1;

    var filter2 = audioCtx.createBiquadFilter();
    filter2.type = 'highpass';
    filter2.frequency.value = MASTER_DATA_HZ-50;
    filter2.gain.value = 100;

    bufS.connect(filter2);
    //filter1.connect(filter2);

    filter2.connect(audioCtx.destination);
}
else {
    bufS.connect(audioCtx.destination);
}

function ended () {
    console.log("ONENDED");
    bufS = audioCtx.createBufferSource();
    bufS.buffer = buffer;
    bufS.onended = ended;
    bufS.start();
};
bufS.onended = ended;
bufS.start();
