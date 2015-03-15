var A_MODE_FIRST_FP500_FREQ_HZ = 5000;
var A_MODE_SECOND_FP500_FREQ_HZ = 9000;

function generateInitPips(n) {
    var blen = 44100*(n*(1/1000));
    var buf = new Float32Array(blen);
    var soundbuf = {
        buffer: buf,
        samplerate: 44100
    };

    var seql = blen / n;
    var period = parseInt(44100/1000);
    var j = 0;
    for (var i = 0; i < n; ++i) {
        var span;
        if (i % 2 == 0) {
            span = period / A_MODE_FIRST_FP500_FREQ_HZ;
        }
        else {
            span = period / A_MODE_SECOND_FP500_FREQ_HZ;
        }

        var j,k;
        for (j = i*period, k = 0; j < (i+1)*period; ++j, ++k) {
            var s = ((2*Math.PI)/span)*k;
            var v = Math.sin(s);
            buf[j] = v;
        }
    }

    return soundbuf;
}

var b = generateInitPips(64);
for (var i = 0; i < b.buffer.length; ++i) {
    document.write(b.buffer[i] + '<br>');
}

T("buffer", { buffer: b, loop: false }).play();
