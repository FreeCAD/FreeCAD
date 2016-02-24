################################################################################
# printbot3d.py
#
# Dan Heeks 18th October 2010

import nc
import iso_modal
import math

################################################################################
class CreatorPrintbot(iso_modal.CreatorIsoModal):

    def __init__(self):
        iso_modal.CreatorIsoModal.__init__(self)

    def tool_defn(self, id, name='', params=None):
        pass

    def write_blocknum(self):
        pass

    def set_plane(self, plane):
         pass
          
    def workplane(self, id):
        pass

# Extruder Control
        
    def extruder_on(self):
        self.write('M101\n')
        
    def extruder_off(self):
        self.write('M103\n')

    def set_extruder_flowrate(self, flowrate):
        # re-use the spindle speed function
        self.spindle(flowrate, True)        

    def extruder_temp(self, temp):
         self.write((maker.codes.EXTRUDER_TEMP(temp)) + ('\n'))
         
# General
    def rapid(x=None, y=None, z=None, a=None, b=None, c=None):
        # do a G1 even for rapid moves
        iso_modal.CreatorIsoModal.feed(self, x, y, z)

    def feed(self, x=None, y=None, z=None, a = None, b = None, c = None):
        iso_modal.CreatorIsoModal.feed(self, x, y, z)
            
################################################################################

nc.creator = CreatorPrintbot()
