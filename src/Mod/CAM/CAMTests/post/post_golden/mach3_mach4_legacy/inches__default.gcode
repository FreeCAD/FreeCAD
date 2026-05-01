(begin preamble)
G17 G54 G40 G49 G80 G90
G20
(begin operation: TC: Default Tool)
(machine: mach3_4, in/min)
M5
M6 T1 
G43 H1
M3 S1000
(finish operation: TC: Default Tool)
(begin operation: Fixture)
(machine: mach3_4, in/min)
G54
(finish operation: Fixture)
(begin operation: Profile)
(machine: mach3_4, in/min)
G0 X0.3937 Y0.7874 Z1.1811
(finish operation: Profile)
(begin postamble)
M05
G17 G54 G90 G80 G40
M2
