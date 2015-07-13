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

function fir(coefficients, input, output) {
    for (var i = 0; i < input.length; ++i) {
        var o = 0, ci;
        for (var j = i, ci = 0; ci < coefficients.length; --j, ++ci) {
            var c = coefficients[ci];
            var v;
            if (j >= 0)
                v = input[j];
            else
                v = input[input.length+j];
            o += c * v;
        }
        output[i] = o;
    }
}

function filter_below_17500_at_44100(buffer_in, buffer_out)
{
    var CS = [
          -0.008773638033184997,
          0.009357892710825723,
          -0.010554635817558757,
          0.008340088013772331,
          -0.002152007417465064,
          -0.007187478912713112,
          0.017467430185060372,
          -0.025680440692784242,
          0.029070873432189042,
          -0.026260306102679768,
          0.017961316619322994,
          -0.006929583363486038,
          -0.002936373680287187,
          0.008074565276900758,
          -0.006735257115986192,
          -0.00017436576561198606,
          0.009319367779764108,
          -0.016231131157432868,
          0.017221409674721674,
          -0.011071265324582759,
          -0.00014492320392081414,
          0.01176763381993594,
          -0.01836630323638912,
          0.01622320048911814,
          -0.005266516590764755,
          -0.01042156610025096,
          0.023881690174243228,
          -0.028057738144694568,
          0.01903684970409776,
          0.0016283074277931738,
          -0.026566669446709053,
          0.044596203179861287,
          -0.04457503889471697,
          0.019745719379580782,
          0.029078063988884347,
          -0.0928195179499722,
          0.15637928156717756,
          -0.20315976523480075,
          0.22035083004992345,
          -0.20315976523480075,
          0.15637928156717756,
          -0.0928195179499722,
          0.029078063988884347,
          0.019745719379580782,
          -0.04457503889471697,
          0.044596203179861287,
          -0.026566669446709053,
          0.0016283074277931738,
          0.01903684970409776,
          -0.028057738144694568,
          0.023881690174243228,
          -0.01042156610025096,
          -0.005266516590764755,
          0.016223200489118142,
          -0.01836630323638912,
          0.01176763381993594,
          -0.00014492320392081414,
          -0.011071265324582753,
          0.017221409674721674,
          -0.016231131157432868,
          0.009319367779764108,
          -0.00017436576561198606,
          -0.006735257115986192,
          0.008074565276900758,
          -0.0029363736802871846,
          -0.006929583363486038,
          0.017961316619322994,
          -0.026260306102679775,
          0.029070873432189042,
          -0.025680440692784242,
          0.017467430185060372,
          -0.007187478912713112,
          -0.002152007417465064,
          0.008340088013772331,
          -0.010554635817558757,
          0.009357892710825726,
          -0.008773638033184997
    ];

    return fir(CS, buffer_in, buffer_out);
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

var SIGNAL_SERIES_LENGTH = 11;
function encode_signal(out, sampleRate, signal, repeat, signalFreq, carrierFreq, mag) {
    if (signal.length == 0)
        return;

    if (out.length < (signal.length*repeat)/2 * parseInt(carrierFreq/signalFreq))
        throw new Error("Assertion error in encode_signal: output buffer too short");

    var oi = 0;
    var elapsedTime = 0;
    var loopCountPlusOne = 1;
    for (var i = 0; i < signal.length*repeat; ++loopCountPlusOne) {
        var seq1Count = 1;
        var seq2Count = 1;
        var seq1V = signal[i % signal.length];
        var lastOne = false;
        for (++i; signal[i % signal.length] == seq1V && i < signal.length*repeat; ++i)
            ++seq1Count;

        if (i < signal.length*repeat) {
            var seq2V = signal[i % signal.length];
            for (++i; signal[i % signal.length] == seq2V && i < signal.length*repeat; ++i)
                ++seq2Count;
        }
        else {
            lastOne = true;
            seq2Count = seq1Count;
        }

        var phaseShift, dutyCycle;
        var countSum = seq1Count+seq2Count;
        if (seq1V == 0) {
            var countRatio = seq1Count/seq2Count;
            phaseShift = 2*seq2Count / countRatio;
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
        for (var j = 0; j < dd && oi < out.length; j += 1) {
            t = j/sampleRate;
            //var v = wf(elapsedTime+t);
            var v = ssb(wf, hwf, carrierFreq, elapsedTime+t);
            //var v = 0.5*wf(elapsedTime+t);
            out[oi++] += v;
        }
        elapsedTime = loopCountPlusOne * (dd/sampleRate);
    }

    return oi;
}

audioCtx = new AudioContext();
var buffer = audioCtx.createBuffer(1, audioCtx.sampleRate*6, audioCtx.sampleRate);

function test_message() {
    var samples = buffer.getChannelData(0);
    var myMessage;
    myMessage = new Array(1000);
    for (var i = 0; i < myMessage.length; ++i) {
        myMessage[i] = !!(i % 4)+0;
    }
    //myMessage = [0,0,1];//[1,0,1,1,0,0,1,1,1,0,0,0,1,1,1,1,0,0,0,0];//1,1,0,0,0];//[1,0,1,0,1,1,1,0,0,0,1,1,0,0];//1,0,1,0,1];
    console.log(audioCtx.sampleRate, JSON.stringify(myMessage));
    var MAG = 0.3;
    var siglen  = encode_signal(samples, audioCtx.sampleRate, myMessage, 1,                  SIGNAL_FREQ, MASTER_DATA_HZ,  MAG);
    var siglen2 = encode_signal(samples, audioCtx.sampleRate, [1,0],     myMessage.length/2, SIGNAL_FREQ, MASTER_CLOCK_HZ, MAG);
    if (siglen != siglen2)
        throw new Error("LENGTH MISMATCH!!");

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
        return 0.5*Math.cos(2*Math.PI*t*(126/8))*Math.cos(2*Math.PI*t*18000 /*+ 10*Math.cos(2*Math.PI*t*126)*/);// + 0.5*Math.cos(2*Math.PI*t*18000);
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
