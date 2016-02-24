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
        self.m_codes_on_their_own_line = True
        self.output_g98_and_g99 = False
        #self.fmt = Format(dp_wanted = False, add_trailing_zeros = True, add_plus = True)

    #def SPACE_STR(self): return ' '
    def PROGRAM(self): return None
    def RETRACT(self, height): return('R' + (self.fmt.string(height)))
    def PECK_DEPTH(self, depth): return('O' + (self.fmt.string(depth)))

    def program_begin(self, id, name=''):
        self.write('(' + name + ')\n')

    def imperial(self):
        #self.g_list.append(self.IMPERIAL())
        self.fmt.number_of_decimal_places = 4

    def metric(self):
        #self.g_list.append(self.METRIC())
        self.fmt.number_of_decimal_places = 3
        
    def comment(self, text):
        pass

nc.creator = Creator()

