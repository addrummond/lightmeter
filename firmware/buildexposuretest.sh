#!/bin/sh

gcc -I./ -c bcd.c -o bcd.testout
gcc -I./ -DTEST -c exposure.c -o exposure.testout
gcc -I./ -c tables.c -o tables.testout
gcc bcd.testout exposure.testout tables.testout -o testexposure
