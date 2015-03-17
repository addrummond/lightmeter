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

function ssb(s, sh, f, t) {
    return s(t)*Math.cos(2*Math.PI*f*t) - sh(t)*Math.sin(2*Math.PI*f*t);
}

function square(freq, mag) {
    return function (t) {
        t = Math.abs(((t*freq*2)%2));
        if (t <= 0.5)
            return mag;
        if (t <= 1.5)
            return 0;
        else
            return mag;
    };
}

function hilbert_of_square(freq, mag) {
    return function (t) {
        t *= freq*Math.PI*2;

        var v = 0;
        for (var n = 1, s = 1; n < 25; n += 2, s *= -1) {
            v += s*(Math.sin(n*t)/n);
        }
        return 0.5*mag*v*4/Math.PI;// - mag/2;
    };
}

function sinfm(freq, mag) {
    return function (t) {
        return mag*Math.sin(Math.PI*2*freq*t);
    }
}

function minuscosfm(freq, mag) {
    return function (t) {
        return -(mag*Math.cos(Math.PI*2*freq*t));
    }
}

function myf(t) {
    return Math.cos(2*Math.PI*5000*t)*ssb(square(1000, 0.2), hilbert_of_square(1000, 0.2), 5000, t);
    //return square(1000, 0.2, 0)(t);
    //return hilbert_of_square(1000,0.1)(t);
}

audioCtx = new AudioContext();
var buffer = audioCtx.createBuffer(1, audioCtx.sampleRate, audioCtx.sampleRate);
var samples = buffer.getChannelData(0);
for (var i = 0; i < samples.length; ++i) {
    var t = i / audioCtx.sampleRate;
    samples[i] = myf(t);
    document.write(samples[i] + '<br>\n');
}
var bufS = audioCtx.createBufferSource();
bufS.buffer = buffer;
bufS.connect(audioCtx.destination);
bufS.start();

/*function generateInitPips(sampleRate, n, mag) {
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
bufS.start();*/
