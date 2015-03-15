var A_MODE_FIRST_FP500_FREQ_HZ = 5000;
var A_MODE_SECOND_FP500_FREQ_HZ = 9000;

var BASE_FREQ = 1000;

// Function that generates square waves with the same period as sin/cos.
function squareW(x, k) {
    var y = parseInt(x / Math.PI);
    if (y % 2 == k)
        return 1;
    else
        return 0;
}

function generateInitPips(sampleRate, n, mag) {
    if (typeof(mag) != 'number')
        mag = 1.0;

    var k1 = 2 * Math.PI * BASE_FREQ * (BASE_FREQ/sampleRate);
    function mag1AtT(t) {
        return 0.5 * squareW(k1*t, 0);
    }
    function mag2AtT(t) {
        return 0.5 * squareW(k1*t, 1);
    }

    var k2 = 2 * Math.PI * A_MODE_FIRST_FP500_FREQ_HZ;
    var k3 = 2 * Math.PI * A_MODE_SECOND_FP500_FREQ_HZ;
    function val1AtT(t) {
        return Math.sin(k2*t);
    }
    function val2AtT(t) {
        return Math.sin(k3*t);
    }

    var buffer = audioCtx.createBuffer(1, (n/BASE_FREQ)*sampleRate, sampleRate);
    var samples = buffer.getChannelData(0);
    for (var i = 0; i < samples.length; ++i) {
        var t = i/sampleRate;
        samples[i] = mag1AtT(t)*val1AtT(t) + mag2AtT(t)*val2AtT(t);
        samples[i] *= mag;
    }

    return buffer;
}

audioCtx = new AudioContext();
var pips = generateInitPips(audioCtx.sampleRate, 64);
var bufS = audioCtx.createBufferSource();
bufS.buffer = pips;
bufS.connect(audioCtx.destination);
bufS.start();
