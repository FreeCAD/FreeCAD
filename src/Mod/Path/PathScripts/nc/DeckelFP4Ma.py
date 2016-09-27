import nc
import iso
import math
import datetime
import time
from format import Format

now = datetime.datetime.now()

class Creator(iso.Creator):
    def __init__(self):
        iso.Creator.__init__(self)
        self.output_tool_definitions = False
        self.fmt = Format(dp_wanted = False, add_trailing_zeros = True, add_plus = True)

    def SPACE_STR(self): return ' '
    def PROGRAM(self): return None
    def PROGRAM_END(self): return( 'T0' + self.SPACE() + 'M06' + self.SPACE() + 'M02')
        
############################################################################
## Begin Program 


    def program_begin(self, id, comment):
        self.write( ('(Created with Deckel FP4Ma post processor ' + str(now.strftime("%Y/%m/%d %H:%M")) + ')' + '\n') )
        iso.Creator.program_begin(self, id, comment)


nc.creator = Creator()

