################################################################################
# attach.py
#
# NC code creator for attaching Z coordinates to a surface
#

import recreator
import nc

swap = False

PUT_Y_VALUE_IN_A = 1

################################################################################
class Creator(recreator.Redirector):

    def __init__(self, original, type, factor):
        recreator.Redirector.__init__(self, original)
        self.factor = factor
        self.type = type

    def feed(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if self.type == PUT_Y_VALUE_IN_A:
            a = None
            if y != None: a = y * self.factor
            self.original.feed(x, None, z, a, b, c)

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if self.type == PUT_Y_VALUE_IN_A:
            a = None
            if y != None: a = y * self.factor
            self.original.rapid(x, None, z, a, b, c)
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        # to do
        pass

################################################################################

def use_a_for_y(radius):
    cancel_swap()
    if radius < 0.001:
       return
    global swap
    radians_factor = 1 / radius
    factor = radians_factor * 180 / 3.1415926535897932384
    nc.creator = Creator(nc.creator, PUT_Y_VALUE_IN_A, factor)
    swap = True

def cancel_swap():
    global swap
    if swap:
        nc.creator = nc.creator.original
        swap = False
