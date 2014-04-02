To run at 16.5MHz (required to use vusb without an external crystal), fuses on the attiny85 need to be set to:

   0xe1 (low) 0x5d (high)

See http://www.simpleavr.com/avr/vusbtiny

It's not clear to me from the datasheet how this setting actually derives a 16.5MHz clock...