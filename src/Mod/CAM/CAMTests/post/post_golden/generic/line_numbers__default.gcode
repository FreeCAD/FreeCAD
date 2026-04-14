N100 (Begin preamble)
N110 G90
N120 G21
N130 (Begin operation: TC: Default Tool)
N140 (Machine units: mm/min)
N150 (Begin toolchange)
N160 M5
N170 M6 T1
N180 G43 H1
N190 M3 S1000
N200 (Finish operation: TC: Default Tool)
N210 (Begin operation: Fixture)
N220 (Machine units: mm/min)
N230 G54
N240 (Finish operation: Fixture)
N250 (Begin operation: Profile)
N260 (Machine units: mm/min)
N270 G0 X10.000 Y20.000 Z30.000
N280 (Finish operation: Profile)
N290 (Begin postamble)
