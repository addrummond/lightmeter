#!/bin/sh

gcc -c -I ./ mymemset.c -o mymemset.out &&
gcc -c -DTEST -I ./ bcd.c -o bcdtest.out &&
gcc mymemset.out bcdtest.out -o bcdtest
