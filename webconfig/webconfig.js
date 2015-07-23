var SIGNAL_FREQ = 126;
var CARRIER_FREQ = 19500;

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

// http://www.electronics.dit.ie/staff/amoloney/lecture-24-25.pdf     (binary CMFSK)
// http://www.ece.umd.edu/~tretter/commlab/c6713slides/FSKSlides.pdf  (n-ary CMFSK)
var FREQ_DIFF = 1000;
function fsk_encode_signal(out, sampleRate, offset, signal, repeat, signalFreq, carrierFreq, mag) {
    var fb = signalFreq/1;
    var modulationIndex = FREQ_DIFF / fb;
    var tb = 1/signalFreq;

    //var prev = { };
    function s(k) {
        var bytei = parseInt(k / 8);
        var biti = k % 8;
        var b = (signal[bytei % signal.length] >> biti) & 1;

        return b ? 1 : -1;

        /*var p;
        if (prev[k-1] != null) {
            p = prev[k-1];
        }

        var r;
        if (b == 0) {
            if (p == -1)
                r = -3;
            else
                r = -1;
        }
        else {
            if (p == 1)
                r = 3;
            else
                r = 1;
        }

        prev[k] = r;

        if (k >= 2 && prev[k-2] != null)
            delete prev[k-2];

        return r;*/
    }

    var thetaMemo = { };
    function theta(k) {
        var total = 0;
        var i = 0;
        if (k > 1 && thetaMemo[k-1] != null) {
            i = k-1;
            total = thetaMemo[k-1];
        }
        for (; i < k; ++i) {
            total += s(i);
        }
        thetaMemo[k] = total;
        total *= Math.PI * modulationIndex;

        if (k > 2 && thetaMemo[k-2] != null)
            delete thetaMemo[k-2];

        return total;
    }

    var i;
    var oi = offset;
    var prevSi = -1;
    var si;
    var thetasi;
    var upto = parseInt((sampleRate/signalFreq) * signal.length * repeat);
    for (i = 0; i < upto && oi < out.length; ++i) {
        var t = (i/sampleRate);
        si = parseInt(t*signalFreq);
        if (si != prevSi) {
            thetasi = theta(si);
            prevSi = si;
        }
        var r = mag*Math.cos((2*Math.PI*carrierFreq*t) + thetasi + (Math.PI*modulationIndex)*s(si)*((t-(si*tb))/tb));
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

    //var siglen  = fsk_encode_signal(samples, audioCtx.sampleRate, 0, TEST_MESSAGE, 1*10, SIGNAL_FREQ, CARRIER_FREQ, MAG);

    var m = [0b01010101];
    siglen = fsk_encode_signal(samples, audioCtx.sampleRate, 0, m, 1000, SIGNAL_FREQ, CARRIER_FREQ, MAG);

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

//test_f();
test_message();

var bufS = audioCtx.createBufferSource();
bufS.buffer = buffer;
bufS.connect(audioCtx.destination);

function ended () {
    console.log("ONENDED");
    bufS = audioCtx.createBufferSource();
    bufS.buffer = buffer;
    bufS.onended = ended;
    bufS.start();
};
bufS.onended = ended;
bufS.start();
