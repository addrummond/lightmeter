var A_MODE_FIRST_FP500_FREQ_HZ = 5000;
var A_MODE_SECOND_FP500_FREQ_HZ = 5500;

var BASE_FREQ = 1000;

// Function that generates square waves with the same period as sin.
function squareW(x, k) {
    var y = parseInt(x / Math.PI);
    if (y % 2 == k)
        return 1;
    else
        return 0;
}
function roundW(x, k) {
    var v;
    if (k == 0)
        v = Math.sin(x);
    else
        v = Math.cos(x);
    return Math.abs(v);
}

// Using phase modulation.
/*function generateInitPips(sampleRate, n, mag) {
    if (typeof(mag) != 'number')
        mag = 1.0;

    var tPerSwitch = 1/BASE_FREQ;
    var k1 = Math.PI / tPerSwitch;
    function phaseAtT(t) {
        return roundW(k1*t, 0) * Math.PI/2;
    }

    var buffer = audioCtx.createBuffer(1, (n/BASE_FREQ)*sampleRate, sampleRate);
    var samples = buffer.getChannelData(0);
    for (var i = 0; i < samples.length; ++i) {
        var t = i/sampleRate;
        samples[i] = Math.sin(2*Math.PI*A_MODE_FIRST_FP500_FREQ_HZ*t + phaseAtT(t));
        samples[i] *= mag * 0.2;
        document.write(samples[i] + '<br>\n');
    }

    return buffer;
}*/

function generateInitPips(sampleRate, n, mag) {
    if (typeof(mag) != 'number')
        mag = 1.0;

    var tPerSwitch = 1/BASE_FREQ;
    var k1 = Math.PI / tPerSwitch;
    function mag1AtT(t) {
        return 0.5 * roundW(k1*t, 0);
    }
    function mag2AtT(t) {
        return 0.5 * roundW(k1*t, 1);
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
        samples[i] *= mag * 0.2;
        document.write(samples[i] + '<br>\n');// ', ' +  t + ', ' + mag1AtT(t) + '<br>\n');
    }

    return buffer;
}

audioCtx = new AudioContext();
var pips = generateInitPips(audioCtx.sampleRate, 640);
var bufS = audioCtx.createBufferSource();
bufS.buffer = pips;
bufS.connect(audioCtx.destination);
bufS.start();
