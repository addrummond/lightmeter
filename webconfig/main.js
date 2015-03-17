var A_MODE_FIRST_FP500_FREQ_HZ = 5000;
var A_MODE_SECOND_FP500_FREQ_HZ = 5500;

var BASE_FREQ = 1000;

function ssb(s, sh, f, t) {
    return s(t)*Math.cos(2*Math.PI*f*t) - sh(t)*Math.sin(2*Math.PI*f*t);
}

var SERIES_LENGTH = 3;

function approximate_square_wave(freq, mag, dutycycle, hilbert) {
    return function (t) {
        // See http://lpsa.swarthmore.edu/Fourier/Series/ExFS.html#CosSeriesDC=0.5

        t *= freq;

        // Convenient to have it start at beginning of positive.
        t += dutycycle*2 + (1-dutycycle);

        function a(n) {
            return 2*(mag/(n*Math.PI))*Math.sin(n*Math.PI*dutycycle);
        }

        function fnh(i) {
            return a(i) * Math.cos(i*Math.PI*2*t);
        }
        function fh(i) {
            return a(i) * Math.sin(i*Math.PI*2*t);
        }

        var v = mag*dutycycle;
        var f = (hilbert ? fh : fnh);
        for (var i = 1; i < SERIES_LENGTH; ++i) {
            v += f(i);
        }

        return v;

        // Old code that just does 50% duty cycle waves.
        //
        /*var v = 0;
        for (var i = 1; i < SERIES_LENGTH*2; i += 2) {
            v += Math.sin(i*2*Math.PI*freq*t)/i;
        }
        return mag*((4*v)/Math.PI);*/
    };
}

function hilbert_of_approximate_square_wave(freq, mag, dutycycle) {
    return approximate_square_wave(freq, mag, dutycycle, true);
}

function myf(t) {
    //return approximate_square_wave(1000, 0.75, 0.75)(t);
    //return /*Math.sin(2*Math.PI*9000*t)*0.2 + */ssb(approximate_square_wave(1000, 0.2, 0.75), hilbert_of_approximate_square_wave(1000, 0.2, 0.75), 9000, t);
    return ssb(approximate_square_wave(1000, 0.2, 0.5), hilbert_of_approximate_square_wave(1000, 0.2, 0.5), 19000, t);
    //return 0.5*Math.sin(2*Math.PI*10000*t + 0.2*Math.PI*Math.cos(2*Math.PI*1000*t));
    //return Math.sin(2*Math.PI*1000*t)*Math.sin(2*Math.PI*t*19000);
}

var FILTER = true;

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
if (FILTER) {
    var filter;
    filter = audioCtx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = 19500;
    filter.Q.value = 19500/2000;
    filter.gain.value = 1;
    bufS.connect(filter);
    filter.connect(audioCtx.destination);
}
else {
    bufS.connect(audioCtx.destination);
}
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
