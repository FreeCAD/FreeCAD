################################################################################
# siegkx1.py
#
# Post Processor for the Sieg KX1 machine
# It is just an ISO machine, but I don't want the tool definition lines
#
# Dan Heeks, 5th March 2009

import nc
import iso_modal
import math

################################################################################
class Creator(iso_modal.Creator):

    def __init__(self):
        iso_modal.Creator.__init__(self)
        self.output_tool_definitions = False
            
################################################################################

nc.creator = Creator()
