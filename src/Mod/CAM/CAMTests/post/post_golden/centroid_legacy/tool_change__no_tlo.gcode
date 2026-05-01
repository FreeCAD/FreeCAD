G90 G80 G40 G49
;begin preamble
G53 G00 G17
G21
;begin operation
M6 T1
M3 S1000
;end operation: TC: Default Tool
;begin operation
G54
;end operation: Fixture
;begin operation
M6 T1
M3 S3000
;end operation: Profile
;begin postamble
M5
M25
G49 H0
G90 G80 G40 G49
M99
