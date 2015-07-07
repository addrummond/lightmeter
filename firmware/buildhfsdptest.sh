#!/bin/sh

gcc -I./ -c goetzel.c -o goetzel.o
gcc -I./ -DTEST -c hfsdp.c -o hfsdp.o
gcc hfsdp.o goetzel.o -o ./testhfsdp
