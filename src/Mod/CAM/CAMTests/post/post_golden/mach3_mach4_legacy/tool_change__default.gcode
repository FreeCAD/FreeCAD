(begin preamble)
G17 G54 G40 G49 G80 G90
G21
(begin operation: TC: Default Tool)
(machine: mach3_4, mm/min)
M5
M6 T1 
G43 H1
M3 S1000
(finish operation: TC: Default Tool)
(begin operation: Fixture)
(machine: mach3_4, mm/min)
G54
(finish operation: Fixture)
(begin operation: Profile)
(machine: mach3_4, mm/min)
M5
M6 T1 
G43 H1
M3 S3000
(finish operation: Profile)
(begin postamble)
M05
G17 G54 G90 G80 G40
M2
