################################################################################
# tnc151.py
#
# Post Processor for the Heidenhain TNC151 machine
#

import nc
import iso_modal
import math

################################################################################
class Creator(iso_modal.Creator):

    def __init__(self):
        iso_modal.Creator.__init__(self)
        self.fmt.add_plus = True
        self.fmt.add_trailing_zeros = True
        self.f.fmt.add_plus = True
        self.s.fmt.add_plus = True
        self.n = 1
        self.waiting_t = None
        self.waiting_for_program_begin = False
        
    ######## Codes
    
    def SPACE(self): return(' ')
    def TOOL(self): return('T%i')
    
    ######## Overridden functions

    def write_blocknum(self):
        self.write(self.BLOCK() % self.n)
        self.n += 1
        
    def program_begin(self, id, name=''):
        self.waiting_for_program_begin = True
        
    def write_waiting_program_begin(self):
        if self.waiting_for_program_begin == True:
            self.write('% 123')
            self.waiting_for_program_begin = False
            
    def imperial(self):
        self.write_waiting_program_begin()
        self.write(' G70\n')
        self.fmt.number_of_decimal_places = 4

    def metric(self):
        self.write_waiting_program_begin()
        self.write(' G71\n')
        self.fmt.number_of_decimal_places = 3


    # no tool definition lines wanted
    def tool_defn(self, id, name='', params=None):
        pass

    # no comments wanted
    def comment(self, text):
        pass 
    
    def spindle(self, s, clockwise):
        iso_modal.Creator.spindle(self, s, clockwise)
        self.write_waiting_tool_change()
         
    def tool_change(self, id):
        self.waiting_t = id
        
    def write_waiting_tool_change(self):
        if self.waiting_t:
            if len(self.g_list) > 0:
                self.write_blocknum()            
                for g in self.g_list:
                    self.write(self.SPACE() + g)
                self.g_list = []
                self.write('\n')
            self.write_blocknum()
            self.write(self.SPACE() + (self.TOOL() % self.waiting_t))
            self.write_preps()
            self.write_spindle()
            self.write_misc()
            self.write('\n')
            self.t = self.waiting_t
            self.waiting_t = None
        
    def workplane(self, id):
        pass
################################################################################

nc.creator = Creator()
