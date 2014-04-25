avr-gcc  -Wall -Wpadded -Os -fshort-enums -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -DF_CPU=16500000L -mmcu=attiny85 clock.c -o clock.out
avr-objcopy -O ihex -R .eeprom clock.out clock.hex
cp clock.hex /tmp
