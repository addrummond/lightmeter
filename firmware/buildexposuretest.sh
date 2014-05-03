#!/bin/sh

gcc -I./ -c bcd.c -o bcd.testout
gcc -I./ -c divmulutils.c -o divmulutils.out
gcc -I./ -DTEST -c exposure.c -o exposure.testout
gcc -I./ -DTEST -c tables.c -o tables.testout
gcc divmulutils.out bcd.testout exposure.testout tables.testout -o testexposure
