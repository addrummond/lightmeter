Attiny84
--------

To run at 8MHz:

   0xc2 (low) 0xdf (high)


Attiny 85
---------

To run at 16.5MHz (required to use vusb without an external crystal), fuses on the attiny85 need to be set to:

   0xe1 (low) 0xdf (high)

See http://www.simpleavr.com/avr/vusbtiny (but ignoring value given for high byte).

It's not clear to me from the datasheet how this setting actually derives a 16.5MHz clock...

To run at 8MHz:

   0xc2 (low) 0xdf (high)