################################################################################
# drag knife.py
#
# NC code creator for attaching Z coordinates to a surface
#
# Dan Heeks 26th April 2012

import recreator
dragging = False
from kurve_funcs import cut_curve as cut_curve
import nc
import area

################################################################################
class Creator(recreator.Redirector):

    def __init__(self, original, drag_distance):
        recreator.Redirector.__init__(self, original)

        self.drag_distance = drag_distance
        self.path = None
        
    def cut_path(self):
        if self.path == None: return

        print self.drag_distance
        self.path.OffsetForward(self.drag_distance, False)
        
        nc.creator = nc.creator.original

        if self.path.getNumVertices() > 0:
            v = self.path.FirstVertex()
            nc.creator.feed(v.p.x, v.p.y)
            
        cut_curve(self.path)
        nc.creator = self
            
        self.path = area.Curve()
        
    def feed(self, x=None, y=None, z=None, a=None, b=None, c=None):
        px = self.x
        py = self.y
        pz = self.z
        recreator.Redirector.feed(self, x, y, z, a, b, c)
        if self.x == None or self.y == None or self.z == None:
            return
        if px == self.x and py == self.y:
            return
            
        # add a line to the path
        if self.path == None: self.path = area.Curve()
        self.path.append(area.Point(self.x, self.y))
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        recreator.Redirector.arc(self, x, y, z, i, j, k, r, ccw)
        
        # add an arc to the path
        if self.path == None:  self.path = area.Curve()
        self.path.append(area.Vertex(1 if ccw else -1, area.Point(self.x, self.y), area.Point(i, j)))

def drag_begin(drag_distance):
    global dragging
    if dragging == True:
        drag_end()
    nc.creator = Creator(nc.creator, drag_distance)
    dragging = True

def drag_end():
    global dragging
    nc.creator.cut_path()
    nc.creator = nc.creator.original
    attached = False
