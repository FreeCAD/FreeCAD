# hpgl2d.py
#
# Copyright (c) 2009, Dan Heeks
# This program is released under the BSD license. See the file COPYING for details.
#

import nc
import math

class Creator(nc.Creator):
    def __init__(self):
        nc.Creator.__init__(self) 
        self.x = int(0)
        self.y = int(0) # these are in machine units, like 0.01mm or maybe 0.25mm
        self.metric() # set self.units_to_mc_units

    def imperial(self):
        self.units_to_mc_units = 2540 # multiplier from inches to machine units

    def metric(self):
        self.units_to_mc_units = 100 # multiplier from mm to machine units

    def program_begin(self, id, name=''):
        self.write('IN;\n')
        self.write('VS32,1;\n')
        self.write('VS32,2;\n')
        self.write('VS32,3;\n')
        self.write('VS32,4;\n')
        self.write('VS32,5;\n')
        self.write('VS32,6;\n')
        self.write('VS32,7;\n')
        self.write('VS32,8;\n')
        self.write('WU0;\n')
        self.write('PW0.349,1;\n')
        self.write('PW0.349,2;\n')
        self.write('PW0.349,3;\n')
        self.write('PW0.349,4;\n')
        self.write('PW0.349,5;\n')
        self.write('PW0.349,6;\n')
        self.write('PW0.349,7;\n')
        self.write('PW0.349,8;\n')
        self.write('SP1;\n')
        
    def program_end(self):
        self.write('SP0;\n')

    def closest_int(self, f):
        if math.fabs(f) < 0.3:
            return 0
        elif f > 0:
            return int(f + 0.5)
        else:
            return int(f - 0.5)

    def get_machine_x_y(self, x=None, y=None):
        machine_x = self.x
        machine_y = self.y
        if x != None:
            machine_x = self.closest_int(x * self.units_to_mc_units)
        if y != None:
            machine_y = self.closest_int(y * self.units_to_mc_units)
        return machine_x, machine_y
        
    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        # ignore the z, any rapid will be assumed to be done with the pen up
        mx, my = self.get_machine_x_y(x, y)
        if mx != self.x or my != self.y:
            self.write(('PU%i' % mx) + (' %i;\n' % my))
            self.x = mx
            self.y = my
            
    def feed(self, x=None, y=None, z=None, a=None, b=None, c=None):
        # ignore the z, any feed will be assumed to be done with the pen down
        mx, my = self.get_machine_x_y(x, y)
        if mx != self.x or my != self.y:
            self.write(('PD%i' % mx) + (' %i;\n' % my))
            self.x = mx
            self.y = my
            
    def arc(self, cw, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        mx, my = self.get_machine_x_y(x, y)
        if mx != self.x or my != self.y:
            cx = float(self.x) / self.units_to_mc_units + i
            cy = float(self.y) / self.units_to_mc_units + j
            sdx = -i
            sdy = -j
            edx = x - cx
            edy = y - cy
            start_angle = math.atan2(sdy, sdx)
            end_angle = math.atan2(edy, edx)
            if cw:
                if start_angle < end_angle: start_angle += 2 * math.pi
            else:
                if end_angle < start_angle: end_angle += 2 * math.pi

            a = math.fabs(end_angle - start_angle)
            if cw: a = -a

            mcx, mcy = self.get_machine_x_y(cx, cy)

            self.write(('AA%i' % mcx) + (',%i' % mcy) + (',%d;\n' % (a * 180 / math.pi)))
            
    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(True, x, y, z, i, j, k, r)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(False, x, y, z, i, j, k, r)

nc.creator = Creator()
