################################################################################
# iso_crc.py
#
# a class derived from iso machine, with Cutter Radius Compensation turned on.
#
# Dan Heeks, 4th May 2010

import nc
import iso
import math

################################################################################
class Creator(iso.Creator):

    def __init__(self):
        iso.Creator.__init__(self)
        self.useCrc = True

################################################################################

nc.creator = Creator()
