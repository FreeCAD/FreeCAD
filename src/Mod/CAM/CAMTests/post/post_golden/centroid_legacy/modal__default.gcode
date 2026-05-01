G90 G80 G40 G49
;begin preamble
G53 G00 G17
G21
;begin operation
G43 H1
M6 T1
M3 S1000
;end operation: TC: Default Tool
;begin operation
G54
;end operation: Fixture
;begin operation
G0 X10.0000 Y20.0000 Z30.0000
G0 X10.0000 Y30.0000 Z30.0000
;end operation: Profile
;begin postamble
M5
M25
G49 H0
G90 G80 G40 G49
M99
