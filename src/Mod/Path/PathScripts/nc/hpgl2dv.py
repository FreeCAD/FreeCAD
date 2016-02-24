# hpgl2dv.py
#
# Copyright (c) 2009, Dan Heeks
# This program is released under the BSD license. See the file COPYING for details.
#

# This is the same as the hpgl2d machine, but uses units of 0.25mm instead of 0.01mm

import nc
import hpgl2d

class Creator(hpgl2d.Creator):
    def init(self): 
        hpgl2d.Creator.init(self) 

    def imperial(self):
        self.units_to_mc_units = 101.6 # multiplier from inches to machine units

    def metric(self):
        self.units_to_mc_units = 4 # multiplier from mm to machine units

nc.creator = Creator()
