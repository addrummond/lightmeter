TLV61220 (boost converter)
--------

    SW  1--- VBAT
    GND ---- VOUT
    EN  ---- FB

Pin layout with one-row DIP adapter with chip the right way up:

      |
    X | - SW-----|
    X |          |
    X | - GND--- | ---- gnd
    X |          |
    X | - EN-----|
    X |        _ | ____ gnd
    X | - FB---| |
    X |       R1 |
    X | - VOUT-| |
    X |          L
    X | - VBAT---|----------|
      |                     |
                            |
                           cap 10uf

Pin layout with chip upside down:

      |
    X | - FB
    X |
    X | - VOUT
    X |
    X | - VBAT
    X |
    X | - SW
    X |  
    X | - GND
    X |
    X | - EN
      |
