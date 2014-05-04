#!/bin/sh

gcc -c -DTEST -I ./ bcd.c -o bcdtest.out
gcc bcdtest.out -o bcdtest
