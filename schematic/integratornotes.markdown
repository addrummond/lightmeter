Newest calculations
-----

* We're using a 0.15uF cap.
* Output equation: http://www.wolframalpha.com/input/?i=v+%3D+-%281%2FC%29+*+%28integrate+I+dt%29
* Got this by moving R inside integral http://en.wikipedia.org/wiki/Op_amp_integrator
* With photocurrent of 0.1 microamps and time of 1/2000 second, we get
  output voltage of 0.00033V.
* With 1/500 second this goes up to 0.00133V.
* Still too low.
* Switch to the 0.015uF cap and this goes up to 0.0033V at 1/2000 and 0.0133V
  at 1/500. This seems ok.
* Cap here: http://www2.mouser.com/ProductDetail/Murata-Electronics/GCM188L81H153KA37D/?qs=sGAEpiMZZMsh%252b1woXyUXj2tpDktHmw%252biMN4CL%2fjiqKI%3d

Old new calculations
-----

* Assume that target is to integrate for no more than 1/500 second.
* Assume that max output voltage should be 2.5V.
* Max current output of op amp is ~15mA. Assume max is 5mA.
* With V=2.8, this gives min resistor size of 560Ohm.
* 0.002 = 560 * C
* C = 3.57 uF.
* Choose C as 2.2uF as we're already using some of those.
* Min photocurrent we're interested in is 0.1uA = 10e-7.
* This gives effective voltage of 10e-7*560 = 0.00056V.
* Max photocurrent we're interested in is 100uA.
* This gives effective voltage of 10e-3*560 = 5.6V
* ----
* Very cheap and precise cap at 0.15uF http://www2.mouser.com/ProductDetail/Murata-Electronics/GCM188L81C154KA37D/?qs=sGAEpiMZZMsh%252b1woXyUXj2tpDktHmw%252bi4hIsQAtPuOw%3d

Old calculations
-----
* Max photocurrent in linear region is 100uA.
* Assuming we swing as far as 2.6V, that gives us a 26k resistor for the initial
  amplification stage (say 26.1k as it's a standard value).
* We don't want to have to integrate for more than 1/2000 of a second, which
  is 500 microseconds.
* We want to charge the cap for around 1/2000 of a second (fastest typical
  flash sync speed would be 1/500).
* Choose a 100Ohm resistor to charge the cap.
* 0.0005 = 100 * C
* C = 5uA.
* Change that to 4.7uF since we're using a ton of those already.
* But the current is possibly a bit on the high side with a 100Ohm resistor,
  so switch to a 2.2uF cap and 220Ohm resistor.
* Voltage after 1/2000 second if photocurrent is 0.1uA: 0.00177V.
* Voltage after 1/2000 second if photocurrent is 1uA: 0.0177V.
* Voltage after 1/2000 second if photocurrent is 100uA: 1.77V.
* Voltage after 1/1000 second if photocurrent is 0.1uA: 0.0023410896029647955V
* This is pushing things a bit. The difficult part is going to be charging it
  long enough to deal with low light levels accurately.
* To be on the safe side, we can't expect the op amp to provide more than
  10mA of output current to charge the cap. That gives us a minimum resistor
  size of 280Ohms.
* And we don't really want to make the charging window any larger than 1/2000
  if we want to be able to do accurate flash sync in all circumstances.
* We need to figure out the photocurrent for 1EV, which is our target for the
  bottom of the scale.
* Hmm, we do indeed need to measure down to 0.1 microamps.
* Let's ease up a bit and say that we can allow 1/1000 second
