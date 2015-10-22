#!/usr/bin/env python
# -*- coding: utf-8 -*-

# tester for Part.makeWireString

import FreeCAD
import Part
import PartDesign

print "testWire started"

# test strings 
# if string contains funky characters, it has to be declared as Unicode or it 
# turns into the default encoding (usually utf8).  FT2 doesn't do utf8.
#String = 'Wide WMA_'                                 # wide glyphs for tracking
#String = 'Big'
#String = u'ecAnO'                                    # UCS-2 w/ only ASCII   
#String = u'ucs2uéçÄñØ'                               # UCS-2   
#String = 'utf8!uéçÄñØ'                               # UTF-8   
#String = 'abcdefghijklmnopqrstuvwxyz0123456789'
#String = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
#String = 'Big Daddy'                                 # white space 
#String = 'AVWAIXA.V'                                 # kerning 
String = 'FreeCAD'                                    # ASCII  

#FontPath = '/usr/share/fonts/truetype/msttcorefonts/' 
#FontName = 'Times_New_Roman_Italic.ttf'                  
FontPath = '/usr/share/fonts/truetype/msttcorefonts/'
FontName = 'Arial.ttf'                                   
#FontName = 'NOTArial.ttf'                             # font file not found error
#FontPath = '/usr/share/fonts/truetype/msttcorefonts/' 
#FontName = 'ariali.ttf'                              #symlink to ttf
#FontPath = '/usr/share/fonts/truetype/' 
#FontName = 'Peterbuilt.ttf'                          # overlapping script font
#FontPath = '/usr/share/fonts/truetype/' 
#FontName = 'dyspepsia.ttf'                           # overlapping script font  # :)

Height  = 2000                                        # out string height FCunits
Track = 0                                             # intercharacter spacing
 
print "testWire.py input String contains ", len(String), " characters."
  
s = Part.makeWireString(String,FontPath,FontName,Height,Track)                

print "returned from makeWireString"
print "testWire.py output contains ", len(s), " WireChars."

for char in s:
    for contour in char:
        Part.show(contour)
        
print "testWire ended."
