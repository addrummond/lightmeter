#!/bin/sh

gcc -c -I ./ divmulutils.c -o divmulutilstest.out
gcc -c -DTEST -I ./ bcd.c -o bcdtest.out
gcc divmulutilstest.out bcdtest.out -o bcdtest
