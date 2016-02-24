################################################################################
# iso_modal.py
#
# a class derived from iso machine, but with XYZF G1, G2 etc modal to reduce the size of the file.
#
# Dan Heeks, 4th May 2010

import nc
import iso
import math

################################################################################
class Creator(iso.Creator):

    def __init__(self):
        iso.Creator.__init__(self)
        self.f_modal = True
        self.g0123_modal = True
        self.drill_modal = True
################################################################################

nc.creator = Creator()
