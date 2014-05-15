#!/bin/sh

avrdude -B 3 -c avrispmkII -p attiny1634 -U "efuse:w:0x1F:m"
avrdude -B 3 -c avrispmkII -p attiny1634 -U "hfuse:w:0xDF:m"
avrdude -B 3 -c avrispmkII -p attiny1634 -U "lfuse:w:0x62:m"
