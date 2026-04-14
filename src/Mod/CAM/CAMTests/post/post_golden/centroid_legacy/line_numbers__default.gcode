G90 G80 G40 G49
N110 ;begin preamble
N120 G53 G00 G17
N130 G21
N140 ;begin operation
N150 G43 H1
N160  M6 T1
N170  M3 S1000
N180 ;end operation: TC: Default Tool
N190 ;begin operation
N200  G54
N210 ;end operation: Fixture
N220 ;begin operation
N230  G0 X10.0000 Y20.0000 Z30.0000
N240 ;end operation: Profile
;begin postamble
N250 M5
N260 M25
N270 G49 H0
N280 G90 G80 G40 G49
N290 M99
