#!/bin/sh

llvm-gcc -g -I./ -c goetzel.c -o goetzel.o
llvm-gcc -g -I./ -DTEST -c hfsdp.c -o hfsdp.o
llvm-gcc -g hfsdp.o goetzel.o -o ./testhfsdp
