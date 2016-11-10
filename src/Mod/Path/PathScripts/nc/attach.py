################################################################################
# attach.py
#
# NC code creator for attaching Z coordinates to a surface
#

import recreator
import ocl
import ocl_funcs
import nc

attached = False
units = 1.0

################################################################################
class Creator(recreator.Redirector):

    def __init__(self, original):
        recreator.Redirector.__init__(self, original)

        self.stl = None
        self.cutter = None
        self.minz = None
        self.path = None
        self.pdcf = None
        self.material_allowance = 0.0

    ############################################################################
    ##  Shift in Z
    
    def setPdcfIfNotSet(self):
        if self.pdcf == None:
            self.pdcf = ocl.PathDropCutter()
            self.pdcf.setSTL(self.stl)
            self.pdcf.setCutter(self.cutter)
            self.pdcf.setSampling(0.1)
            self.pdcf.setZ(self.minz/units)
                    
    def z2(self, z):
        path = ocl.Path()
        # use a line with no length
        path.append(ocl.Line(ocl.Point(self.x, self.y, self.z), ocl.Point(self.x, self.y, self.z)))
        self.setPdcfIfNotSet()
        if (self.z>self.minz):
            self.pdcf.setZ(self.z)  # Adjust Z if we have gotten a higher limit (Fix pocketing loosing steps when using attach?)
        else:
            self.pdcf.setZ(self.minz/units) # Else use minz
        self.pdcf.setPath(path)
        self.pdcf.run()
        plist = self.pdcf.getCLPoints()
        p = plist[0]
        return p.z + self.material_allowance/units
        
    def cut_path(self):
        if self.path == None: return
        self.setPdcfIfNotSet()
        
        if (self.z>self.minz):
            self.pdcf.setZ(self.z)  # Adjust Z if we have gotten a higher limit (Fix pocketing loosing steps when using attach?)
        else:
            self.pdcf.setZ(self.minz/units) # Else use minz
            
       # get the points on the surface
        self.pdcf.setPath(self.path)
        
        self.pdcf.run()
        plist = self.pdcf.getCLPoints()
        
        #refine the points
        f = ocl.LineCLFilter()
        f.setTolerance(0.005)
        for p in plist:
            f.addCLPoint(p)
        f.run()
        plist = f.getCLPoints()
        
        i = 0
        for p in plist:
            if i > 0:
                self.original.feed(p.x/units, p.y/units, p.z/units + self.material_allowance/units)
            i = i + 1
            
        self.path = ocl.Path()
        
    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None ):
        if z != None:
            if z < self.z:
                return
        recreator.Redirector.rapid(self, x, y, z, a, b, c)

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
        if self.path == None: self.path = ocl.Path()
        self.path.append(ocl.Line(ocl.Point(px, py, pz), ocl.Point(self.x, self.y, self.z)))
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        px = self.x
        py = self.y
        pz = self.z
        recreator.Redirector.arc(self, x, y, z, i, j, k, r, ccw)
        
        # add an arc to the path
        if self.path == None: self.path = ocl.Path()
        self.path.append(ocl.Arc(ocl.Point(px, py, pz), ocl.Point(self.x, self.y, self.z), ocl.Point(i, j, pz), ccw))
        
    def set_ocl_cutter(self, cutter):
        self.cutter = cutter

################################################################################

def attach_begin():
    global attached
    if attached == True:
        attach_end()
    nc.creator = Creator(nc.creator)
    recreator.units = units
    attached = True
    nc.creator.pdcf = None
    nc.creator.path = None

def attach_end():
    global attached
    nc.creator.cut_path()
    nc.creator = nc.creator.original
    attached = False
