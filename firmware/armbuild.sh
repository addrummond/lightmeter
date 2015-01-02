#!/bin/sh

CFLAGS=`cat CFLAGS`
LINKFLAGS=`cat LINKFLAGS`
ST=-save-temps

CC=arm-none-eabi-gcc
CFLAGS="$ST $CFLAGS"

$CC $CFLAGS -c exposure.c -o exposure.o
$CC $CFLAGS -c bcd.c -o bcd.o
