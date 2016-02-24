import nc
import iso_modal
import math

################################################################################
class Creator(iso_modal.Creator):

    def __init__(self):
        iso_modal.Creator.__init__(self)
        self.arc_centre_positive = True
        self.drillExpanded = True
        self.can_do_helical_arcs = False
        self.fmt.number_of_decimal_places = 2

    def tool_defn(self, id, name='', params=None):
        pass
    
    def dwell(self, t):
        # to do, find out what dwell is on this machine
        pass
    
    def metric(self):
        iso_modal.Creator.metric(self)
        self.fmt.number_of_decimal_places = 2
        
    def SPACE(self):
         return('')
            
################################################################################

nc.creator = Creator()
