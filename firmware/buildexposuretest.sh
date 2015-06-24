#!/bin/sh

python3 calculate_tables.py output

gcc -I./ -c bcd.c -o bcd.o
gcc -I./ -DTEST -save-temps -c exposure.c -o exposure.o
gcc -I./ -DTEST -c tables.c -o tables.o
gcc -I./ -DTEST -c mymemset.c -o mymemset.o
gcc bcd.o exposure.o tables.o mymemset.o -o testexposure
