################################################################################
# nc_read.py
#
# Base class for NC code parsing
#
# Hirutso Enni, 2009-01-13

################################################################################
class Parser:

    def __init__(self):
        self.currentx = 0.0
        self.currenty = 0.0
        self.currentz = 0.0
        self.absolute_flag = True

    ############################################################################
    ##  Internals

    def files_open(self, name, oname=None):
        if (oname == None ):
            oname = (name+'.nc.xml')
        self.file_in = open(name, 'r')
        self.file_out = open(oname, 'w')

        self.file_out.write('<?xml version="1.0" ?>\n')
        self.file_out.write('<nccode>\n')

    def files_close(self):
        self.file_out.write('</nccode>\n')

        self.file_in.close()
        self.file_out.close()

    def readline(self):
        self.line = self.file_in.readline().rstrip()
        if (len(self.line)) : return True
        else : return False

    def write(self, s):
        self.file_out.write(s)

    ############################################################################
    ##  Xml

    def begin_ncblock(self):
        self.file_out.write('\t<ncblock>\n')

    def end_ncblock(self):
        self.file_out.write('\t</ncblock>\n')

    def add_text(self, s, col=None, cdata=False):
        s.replace('&', '&amp;')
        s.replace('"', '&quot;')
        s.replace('<', '&lt;')
        s.replace('>', '&gt;')
        if (cdata) : (cd1, cd2) = ('<![CDATA[', ']]>')
        else : (cd1, cd2) = ('', '')
        if (col != None) : self.file_out.write('\t\t<text col="'+col+'">'+cd1+s+cd2+'</text>\n')
        else : self.file_out.write('\t\t<text>'+cd1+s+cd2+'</text>\n')

    def set_mode(self, units=None):
        self.file_out.write('\t\t<mode')
        if (units != None) : self.file_out.write(' units="'+str(units)+'"')
        self.file_out.write(' />\n')

    def set_tool(self, number=None):
        self.file_out.write('\t\t<tool')
        if (number != None) : 
		self.file_out.write(' number="'+str(number)+'"')
        	self.file_out.write(' />\n')

    def begin_path(self, col=None):
        if (col != None) : self.file_out.write('\t\t<path col="'+col+'">\n')
        else : self.file_out.write('\t\t<path>\n')

    def end_path(self):
        self.file_out.write('\t\t</path>\n')

    def add_line(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if (x == None and y == None and z == None and a == None and b == None and c == None) : return
        self.file_out.write('\t\t\t<line')
        if (x != None) :
            if self.absolute_flag: self.currentx = x
            else: self.currentx = self.currentx + x
            self.file_out.write(' y="%.6f"' % (self.currentx/2))
        if (y != None) :
            if self.absolute_flag: self.currenty = y
            else: self.currenty = self.currenty + y
            #self.file_out.write(' y="%.6f"' % self.currenty)
        if (z != None) :
            if self.absolute_flag: self.currentz = z
            else: self.currentz = self.currentz + z
            self.file_out.write(' x="%.6f"' % self.currentz)
        if (a != None) : self.file_out.write(' a="%.6f"' % a)
        if (b != None) : self.file_out.write(' b="%.6f"' % b)
        if (c != None) : self.file_out.write(' c="%.6f"' % c)
        self.file_out.write(' />\n')


    def add_lathe_increment_line(self, u=None,  w=None):
# needed for representing U and W moves in lathe code- these are non modal incremental moves
# U == X and W == Z
        if (u == None  and w == None ) : return
        self.file_out.write('\t\t\t<line')
        if (u != None) :
            self.currentx = self.currentx + u
            self.file_out.write(' y="%.6f"' % (self.currentx/2))
        if (w != None) :
            self.currentz = self.currentz + w
            self.file_out.write(' x="%.6f"' % self.currentz)
        self.file_out.write(' />\n')






    def add_arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, d=None):
        if (x == None and y == None and z == None and i == None and j == None and k == None and r == None and d == None) : return
        self.file_out.write('\t\t\t<arc')
        if (x != None) :
            if self.absolute_flag: self.currentx = x
            else: self.currentx = self.currentx + x
            self.file_out.write(' y="%.6f"' % (self.currentx/2))
        if (y != None) :
            if self.absolute_flag: self.currenty = y
            else: self.currenty = self.currenty + y
            #self.file_out.write(' y="%.6f"' % self.currenty)
        if (z != None) :
            if self.absolute_flag: self.currentz = z
            else: self.currentz = self.currentz + z
            self.file_out.write(' x="%.6f"' % self.currentz)
        
        #if (j != None) : self.file_out.write(' i="%.6f"' % j)
        #if (i != None) : self.file_out.write(' j="%.6f"' % i)
        #if (k != None) : self.file_out.write(' k="%.6f"' % k)

        if (k != None) : self.file_out.write(' i="%.6f"' % k)
        if (i != None) : self.file_out.write(' j="%.6f"' % i)
        if (j != None) : self.file_out.write(' k="%.6f"' % j)

        if (r != None) : self.file_out.write(' r="%.6f"' % r)
        if (d != None) : self.file_out.write(' d="%i"' % d)
        self.file_out.write(' />\n')
        
    def incremental(self):
        self.absolute_flag = False
        
    def absolute(self):
        self.absolute_flag = True
