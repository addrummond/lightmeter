#!/bin/sh

gcc -I./ -c bcd.c -o bcd.testout
gcc -I./ -DTEST -c exposure.c -o exposure.testout
gcc -I./ -DTEST -c tables.c -o tables.testout
gcc -I./ -DTEST -c mymemset.c -o mymemset.o
gcc bcd.testout exposure.testout tables.testout mymemset.o -o testexposure
