       function bits_set_in_uint32( n)
{

    var count;
    for (count = 0; n; ++count, n &= n-1);
    return count;
}

             var PMASK1 = parseInt("0b01010101010101010101010101010101".substr(2), 2);
             var PMASK2 = parseInt("0b01100110011001100110011001100110".substr(2), 2);
             var PMASK4 = parseInt("0b01111000011110000111100001111000".substr(2), 2);
             var PMASK8 = parseInt("0b01111111100000000111111110000000".substr(2), 2);
             var PMASK16 = parseInt("0b01111111111111111000000000000000".substr(2), 2);

function hammingify_uint32( n)
{
    do { if (! (n < (1 << (31-5)))) { throw new Error("ASSERTION ERROR"); } } while (0);


    n = ((n & parseInt("0b1".substr(2), 2)) << 2) |
        ((n & parseInt("0b1110".substr(2), 2)) << 3) |
        ((n & parseInt("0b11111110000".substr(2), 2)) << 4) |
        ((n & parseInt("0b11111111111111100000000000".substr(2), 2)) << 5);

    var pb1 = n & PMASK1;
    var pb2 = n & PMASK2;
    var pb4 = n & PMASK4;
    var pb8 = n & PMASK8;
    var pb16 = n & PMASK16;

    var pc1 = bits_set_in_uint32(pb1);
    var pc2 = bits_set_in_uint32(pb2);
    var pc4 = bits_set_in_uint32(pb4);
    var pc8 = bits_set_in_uint32(pb8);
    var pc16 = bits_set_in_uint32(pb16);

    if (pc1 % 2 == 0)
        n ^= (1 << (1-1));
    if (pc2 % 2 == 0)
        n ^= (1 << (2-1));
    if (pc4 % 2 == 0)
        n ^= (1 << (4-1));
    if (pc8 % 2 == 0)
        n ^= (1 << (8-1));
    if (pc16 % 2 == 0)
        n ^= (1 << (16-1));


    if (bits_set_in_uint32(n) % 2 == 0)
        n ^= (1 << (32-1));

    return n;
}


function dehammingify_uint32( n)
{
    var pb1 = n & PMASK1;
    var pb2 = n & PMASK2;
    var pb4 = n & PMASK4;
    var pb8 = n & PMASK8;
    var pb16 = n & PMASK16;

    var pc1 = bits_set_in_uint32(pb1);
    var pc2 = bits_set_in_uint32(pb2);
    var pc4 = bits_set_in_uint32(pb4);
    var pc8 = bits_set_in_uint32(pb8);
    var pc16 = bits_set_in_uint32(pb16);

    var error_bit_index = 0;
    if (pc1 % 2 == 0)
        error_bit_index += 1;
    if (pc2 % 2 == 0)
        error_bit_index += 2;
    if (pc4 % 2 == 0)
        error_bit_index += 4;
    if (pc8 % 2 == 0)
        error_bit_index += 8;
    if (pc16 % 2 == 0)
        error_bit_index += 16;

    if (error_bit_index == 1 || error_bit_index == 2 || error_bit_index == 4 ||
        error_bit_index == 8 || error_bit_index == 16) {

        return -1;
    }
    else if (error_bit_index) {

        n ^= (1 << (error_bit_index-1));
    }


    if (bits_set_in_uint32(n) % 2 == 0)
        return -1;



    return ((n & parseInt("0b100".substr(2), 2)) >> 2) |
           ((n & parseInt("0b1110000".substr(2), 2)) >> 3) |
           ((n & parseInt("0b111111100000000".substr(2), 2)) >> 4) |
           ((n & parseInt("0b1111111111111110000000000000000".substr(2), 2)) >> 5);
}



function




test()

{


    var n;
    for (n = 0; n < (1 << (31-5)); ++n) {
        var h = hammingify_uint32(n);

        var i;
        for (i = 0; i <= 32; ++i) {
console.log("HERE");
            if (i != 0) {
                var h2 = h ^ (1 << (i-1));

                var r = dehammingify_uint32(h2);
                if (! (((r == -1 && (i == 1 || i == 2 || i == 4 || i == 8 || i == 16 || i == 32)) ||
                       (r == n)))) {




                    console.log("FAILED FOR i = ", i, "n = ", n, "r = ", r);

                }
            }
        }
    }

    return 0;
}
