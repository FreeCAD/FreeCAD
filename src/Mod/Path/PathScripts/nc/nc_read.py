################################################################################
# nc_read.py
#
# Base class for NC code parsing

################################################################################
import area
import math
count = 0

class Program:   # stores start and end lines of programs and subroutines
    def __init__(self):
        self.start_line = None
        self.end_line = None

class Parser:

    def __init__(self, writer):
        self.writer = writer
        self.currentx = None
        self.currenty = None
        self.currentz = None
        self.absolute_flag = True
        self.drillz = None
        self.need_m6_for_t_change = True
        
    def __del__(self):
        self.file_in.close()

    ############################################################################
    ##  Internals

    def readline(self):
        self.line = self.file_in.readline().rstrip()
        if (len(self.line)) : return True
        else : return False

    def set_current_pos(self, x, y, z):
        if (x != None) :
            if self.absolute_flag or self.currentx == None: self.currentx = x
            else: self.currentx = self.currentx + x
        if (y != None) :
            if self.absolute_flag or self.currenty == None: self.currenty = y
            else: self.currenty = self.currenty + y
        if (z != None) :
            if self.absolute_flag or self.currentz == None: self.currentz = z
            else: self.currentz = self.currentz + z

    def incremental(self):
        self.absolute_flag = False

    def absolute(self):
        self.absolute_flag = True
        
    def Parse(self, name):
        self.file_in = open(name, 'r')
        
        self.path_col = None
        self.f = None
        self.arc = 0
        self.q = None
        self.r = None
        self.drilling = None
        self.drilling_uses_clearance = False
        self.drilling_clearance_height = None

        while (self.readline()):
            self.a = None
            self.b = None
            self.c = None
            self.h = None
            self.i = None
            self.j = None
            self.k = None
            self.p = None
            self.s = None
            self.x = None
            self.y = None
            self.z = None
            self.t = None
            self.m6 = False

            self.writer.begin_ncblock()

            self.move = False
            self.height_offset = False
            self.drill = False
            self.drill_off = False
            self.no_move = False
            
            words = self.pattern_main.findall(self.line)
            for word in words:
                self.col = None
                self.cdata = False
                self.ParseWord(word)
                self.writer.add_text(word, self.col, self.cdata)

            if self.t != None:
                if (self.m6 == True) or (self.need_m6_for_t_change == False):
                    self.writer.tool_change( self.t )

            if self.height_offset and (self.z != None):
                self.drilling_clearance_height = self.z
                    
            if self.drill:
                self.drilling = True
            
            if self.drill_off:
                self.drilling = False

            if self.drilling:
                rapid_z = self.r
                if self.drilling_uses_clearance and (self.drilling_clearance_height != None):
                    rapid_z = self.drilling_clearance_height
                if self.z != None: self.drillz = self.z
                self.writer.rapid(self.x, self.y, rapid_z)
                self.writer.feed(self.x, self.y, self.drillz)
                self.writer.feed(self.x, self.y, rapid_z)

            else:
                if (self.move and not self.no_move):
                    if (self.arc==0):
                        if self.path_col == "feed":
                            self.writer.feed(self.x, self.y, self.z)
                        else:
                            self.writer.rapid(self.x, self.y, self.z, self.a, self.b, self.c)
                    else:
                        i = self.i
                        j = self.j
                        k = self.k
                        if self.arc_centre_absolute == True:
                            pass
                        else:
                            if (self.arc_centre_positive == True) and (self.oldx != None) and (self.oldy != None):
                                x = self.oldx
                                if self.x != None: x = self.x
                                if (self.x > self.oldx) != (self.arc > 0):
                                    j = -j
                                y = self.oldy
                                if self.y != None: y = self.y
                                if (self.y > self.oldy) != (self.arc < 0):
                                    i = -i

                                #fix centre point
                                r = math.sqrt(i*i + j*j)
                                p0 = area.Point(self.oldx, self.oldy)
                                p1 = area.Point(x, y)
                                v = p1 - p0
                                l = v.length()
                                h = l/2
                                d = math.sqrt(r*r - h*h)
                                n = area.Point(-v.y, v.x)
                                n.normalize()
                                if self.arc == -1: d = -d
                                c = p0 + (v * 0.5) + (n * d)
                                i = c.x
                                j = c.y

                            else:
                                i = i + self.oldx
                                j = j + self.oldy
                        if self.arc == -1:
                            self.writer.arc_cw(self.x, self.y, self.z, i, j, k)
                        else:
                            self.writer.arc_ccw(self.x, self.y, self.z, i, j, k)
                    if self.x != None: self.oldx = self.x
                    if self.y != None: self.oldy = self.y
                    if self.z != None: self.oldz = self.z
            self.writer.end_ncblock()

        
