* Output current we need to detect varies between 0.001 amps and 0.00000001
  amps.

* Assuming ideal capacitor charging scenario, and 0.03 microfarad cap
  (= 0.00000003 farad), voltage after 1/1000 second will be 33V with a 0.001
  amp charging current, and 0.00033V with a 0.00000001 amp charging current.

* Change to 0.003 microfarad (= 3000 picofarad, 0.000000003 farad) cap. Then we get 0.0033V at
  0.00000001 amp charging current and 1/1000 second and 3.3V at 0.001 amp
  charging current and 1/100000 second. (That's eighty clock cycles at 8MHz.)

* Verified that we get the same result if we assume that cap is in parallel
  with a very large resistor and then do source transform. (I.e., consider
  the circuit as a voltage source charging a cap through a resistor.)
