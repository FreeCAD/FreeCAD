N110 (begin preamble)
N120 G17 G54 G40 G49 G80 G90
N130 G21
N140 (begin operation: TC: Default Tool)
N150 (machine: mach3_4, mm/min)
N160 M5
N170  M6 T1 
G43 H1
N180  M3 S1000
N190 (finish operation: TC: Default Tool)
N200 (begin operation: Fixture)
N210 (machine: mach3_4, mm/min)
N220  G54
N230 (finish operation: Fixture)
N240 (begin operation: Profile)
N250 (machine: mach3_4, mm/min)
N260  G0 X10.000 Y20.000 Z30.000
N270 (finish operation: Profile)
(begin postamble)
N280 M05
N290 G17 G54 G90 G80 G40
N300 M2
