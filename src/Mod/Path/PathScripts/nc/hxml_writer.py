import tempfile

class HxmlWriter:
    def __init__(self):
        self.file_out = open(tempfile.gettempdir()+'/backplot.xml', 'w')
        self.file_out.write('<?xml version="1.0" ?>\n')
        self.file_out.write('<nccode>\n')
        self.t = None
        self.oldx = None
        self.oldy = None
        self.oldz = None

    def __del__(self):
        self.file_out.write('</nccode>\n')
        self.file_out.close()

    def write(self, s):
        self.file_out.write(s)

############################################

    def begin_ncblock(self):
        self.file_out.write('\t<ncblock>\n')

    def end_ncblock(self):
        self.file_out.write('\t</ncblock>\n')

    def add_text(self, s, col, cdata):
        s.replace('&', '&amp;')
        s.replace('"', '&quot;')
        s.replace('<', '&lt;')
        s.replace('>', '&gt;')
        if (cdata) : (cd1, cd2) = ('<![CDATA[', ']]>')
        else : (cd1, cd2) = ('', '')
        if (col != None) : self.file_out.write('\t\t<text col="'+col+'">'+cd1+s+cd2+'</text>\n')
        else : self.file_out.write('\t\t<text>'+cd1+s+cd2+'</text>\n')

    def set_mode(self, units):
        self.file_out.write('\t\t<mode')
        if (units != None) : self.file_out.write(' units="'+str(units)+'"')
        self.file_out.write(' />\n')
        
    def metric(self):
        self.set_mode(units = 1.0)
        
    def imperial(self):
        self.set_mode(units = 25.4)

    def begin_path(self, col):
        if (col != None) : self.file_out.write('\t\t<path col="'+col+'">\n')
        else : self.file_out.write('\t\t<path>\n')

    def end_path(self):
        self.file_out.write('\t\t</path>\n')
        
    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.begin_path("rapid")
        self.add_line(x, y, z, a, b, c)
        self.end_path()
        
    def feed(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.begin_path("feed")
        self.add_line(x, y, z, a, b, c)
        self.end_path()

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.begin_path("feed")
        self.add_arc(x, y, z, i, j, k, r, -1)
        self.end_path()

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.begin_path("feed")
        self.add_arc(x, y, z, i, j, k, r, 1)
        self.end_path()
        
    def tool_change(self, id):
        self.file_out.write('\t\t<tool')
        if (id != None) : 
            self.file_out.write(' number="'+str(id)+'"')
            self.file_out.write(' />\n')
        self.t = id
            
    def current_tool(self):
        return self.t
            
    def spindle(self, s, clockwise):
        pass
    
    def feedrate(self, f):
        pass

    def add_line(self, x, y, z, a = None, b = None, c = None):
        self.file_out.write('\t\t\t<line')
        if (x != None) :
            self.file_out.write(' x="%.6f"' % x)
        if (y != None) :
            self.file_out.write(' y="%.6f"' % y)
        if (z != None) :
            self.file_out.write(' z="%.6f"' % z)
        if (a != None) : self.file_out.write(' a="%.6f"' % a)
        if (b != None) : self.file_out.write(' b="%.6f"' % b)
        if (c != None) : self.file_out.write(' c="%.6f"' % c)
        self.file_out.write(' />\n')
        if x != None: self.oldx = x
        if y != None: self.oldy = y
        if z != None: self.oldz = z
        
    def add_arc(self, x, y, z, i, j, k, r = None, d = None):
        self.file_out.write('\t\t\t<arc')
        if (x != None) :
            self.file_out.write(' x="%.6f"' % x)
        if (y != None) :
            self.file_out.write(' y="%.6f"' % y)
        if (z != None) :
            self.file_out.write(' z="%.6f"' % z)
        if (i != None):
            if self.oldx == None: print 'arc move "i" without x set!'
            else: self.file_out.write(' i="%.6f"' % (i - self.oldx))
        if (j != None):
            if self.oldy == None: print 'arc move "j" without y set!'
            else: self.file_out.write(' j="%.6f"' % (j - self.oldy))
        if (k != None):
            if self.oldz == None: print 'arc move "k" without z set!'
            else: self.file_out.write(' k="%.6f"' % (k - self.oldz))
        if (r != None) : self.file_out.write(' r="%.6f"' % r)
        if (d != None) : self.file_out.write(' d="%i"' % d)
        self.file_out.write(' />\n')
        if x != None: self.oldx = x
        if y != None: self.oldy = y
        if z != None: self.oldz = z
