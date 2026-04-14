(Begin preamble)
G90
G21
(Begin operation: TC: Default Tool)
(Machine units: mm/min)
(Begin toolchange)
M5
M6 T1
G43 H1
M3 S1000
(Finish operation: TC: Default Tool)
(Begin operation: Fixture)
(Machine units: mm/min)
G54
(Finish operation: Fixture)
(Begin operation: Profile)
(Machine units: mm/min)
G0 X10.00 Y20.00 Z30.00
(Finish operation: Profile)
(Begin postamble)
