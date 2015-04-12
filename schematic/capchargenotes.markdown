* Capacitor is 3300pF = 0.0000000033F.

* Output current we need to detect varies between 0.001 amps and 0.00000001
  amps.

* To get to 3V from 0.00000001 amps takes about a second.

* To get 0.003V from 0.00000001 amps takes about 1/1000 second = 48000 clock
  cycles at 48MHz.

* To get 3V from 0.001 amps takes about 0.00001 seconds = 10microseconds =
  480 clock cycles at 48MHz.
