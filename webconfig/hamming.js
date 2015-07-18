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

             var M1 = parseInt("0b1".substr(2), 2);
             var M2 = parseInt("0b1110".substr(2), 2);
             var M3 = parseInt("0b11111110000".substr(2), 2);
             var M4 = parseInt("0b11111111111111100000000000".substr(2), 2);
function hammingify_uint32( n)
{
    do { if (! (n < (1 << (31-5)))) { throw new Error("ASSERTION ERROR"); } } while (0);


    n = ((n & M1) << 2) |
        ((n & M2) << 3) |
        ((n & M3) << 4) |
        ((n & M4) << 5);

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

             var IM1 = parseInt("0b100".substr(2), 2);
             var IM2 = parseInt("0b1110000".substr(2), 2);
             var IM3 = parseInt("0b111111100000000".substr(2), 2);
             var IM4 = parseInt("0b1111111111111110000000000000000".substr(2), 2);

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


        n ^= 1;
    }
    else if (error_bit_index) {

        n ^= (1 << (error_bit_index-1));
    }



    if (bits_set_in_uint32(n) % 2 == 0)
        return -1;



    return ((n & IM1) >> 2) |
           ((n & IM2) >> 3) |
           ((n & IM3) >> 4) |
           ((n & IM4) >> 5);
}

function hamming_get_init_sequence_byte_length()
{
    return 5*4;
}

function hamming_get_encoded_message_byte_length( len) {
    var x = len*4;
    x += x % 3;
    return x / 3;
}

function hamming_get_encoded_message_byte_length_with_init_sequence( len) {
    return hamming_get_init_sequence_byte_length() + hamming_get_encoded_message_byte_length(len);
}

function hamming_get_max_output_length_given_input_length( len)
{
    var x = len*3;
    x += x % 4;
    return (x / 4);
}


       var MAGIC_NUMBER_HAMMING = 0;

function hamming_encode_message( input,



               out,
          withInit)
{






    if (MAGIC_NUMBER_HAMMING == 0)
        MAGIC_NUMBER_HAMMING = hammingify_uint32(24826601);

    var i = 0;
    if (withInit) {
        var ilen = hamming_get_init_sequence_byte_length();
        for (; i < ilen; i += 4) {
            out[i+0] = (MAGIC_NUMBER_HAMMING & 0xFF);
            out[i+1] = (MAGIC_NUMBER_HAMMING & 0xFF00) >> 8;
            out[i+2] = (MAGIC_NUMBER_HAMMING & 0xFF0000) >> 16;
            out[i+3] = (MAGIC_NUMBER_HAMMING & 0xFF000000) >> 24;
        }
    }

    var j;
    for (j = 0; j < input.length; j += 3, i += 4) {
        var v = input[j] |
                     (j + 1 >= input.length ? 0 : (input[j+1] << 8)) |
                     (j + 2 >= input.length ? 0 : (input[j+2] << 16));
        var h = hammingify_uint32(v);
        out[i+0] = (h & 0xFF);
        out[i+1] = (h & 0xFF00) >> 8;
        out[i+2] = (h & 0xFF0000) >> 16;
        out[i+3] = (h & 0xFF000000) >> 24;
    }


}


function hamming_bitshift_buffer_forward( buffer, length, nbits)
{
    do { if (! (nbits < 8)) { throw new Error("ASSERTION ERROR"); } } while (0);

    if (length == 0)
        return;

    var backup = 0;
    var i;
    for (i = 0; i < length; ++i) {
        var r = (buffer[i] << nbits);
        r |= (backup >> (8 - nbits));

        r &= 0xFF;

        backup = buffer[i];
        buffer[i] = r;
    }
    if (nbits > 0) {
        buffer[i] = (backup >> (8 - nbits));

        buffer[i] &= 0xFF;

    }
}



function hamming_scan_for_init_sequence( input



)
{


    var result = { };





    var magic_start_bit_index = -1;
    var magic_count = 0;
    var bit_index;
    for (bit_index = 0; bit_index < (input.length*8) - 32;) {
        var byte = parseInt(bit_index / 8);
        var bit = bit_index % 8;
        var b1 = ((input[byte+0] >> bit) | (input[byte+1] << (8-bit)));
        var b2 = ((input[byte+1] >> bit) | (input[byte+2] << (8-bit)));
        var b3 = ((input[byte+2] >> bit) | (input[byte+3] << (8-bit)));
        var b4 = ((input[byte+3] >> bit) | (input[byte+4] << (8-bit)));

        b1 &= 0xFF;
        b2 &= 0xFF;
        b3 &= 0xFF;
        b4 &= 0xFF;


        var v = dehammingify_uint32(b1 | (b2 << 8) | (b3 << 16) | (b4 << 24));
        if (v == 24826601) {
            if (magic_start_bit_index == -1)
                magic_start_bit_index = bit_index;
            ++magic_count;
            if (magic_count >= 5) {
                result.bit_index = magic_start_bit_index;
                result.count = magic_count;
                return result;
            }
            bit_index += 32;
        }
        else {
            if (magic_start_bit_index != -1) {
                result.bit_index = magic_start_bit_index;
                result.count = magic_count;
                return result;
            }
            ++bit_index;
        }
    }

    result.bit_index = magic_start_bit_index;
    result.count = magic_count;
    return result;


}



function hamming_decode_message( input,



               out)
{






    var i, oi = 0;
    for (i = 0; i < input.length; i += 4) {
        var v = dehammingify_uint32(input[i] | (input[i+1] << 8) | (input[i+2] << 16) | (input[i+3] << 24));

        if (v == -1)
            return -i;

        out[oi++] = v & 0xFF;
        out[oi++] = (v & 0xFF00) >> 8;
        out[oi++] = (v & 0xFF0000) >> 16;
    }

    return oi;


}



function




hmming_test()

{


    var n;
    for (n = 0; n < (1 << (31-5)); ++n) {
        var h = hammingify_uint32(n);

        var i;
        for (i = 0; i <= 32; ++i) {
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
