(Begin preamble)
G90
G20
(Begin operation: TC: Default Tool)
(Machine units: in/min)
(Begin toolchange)
M5
M6 T1
G43 H1
M3 S1000
(Finish operation: TC: Default Tool)
(Begin operation: Fixture)
(Machine units: in/min)
G54
(Finish operation: Fixture)
(Begin operation: Profile)
(Machine units: in/min)
G0 X0.39 Y0.79 Z1.18
(Finish operation: Profile)
(Begin postamble)
