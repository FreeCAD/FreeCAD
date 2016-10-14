################################################################################
# cad_nc_read.py
#
# Base class for NC code parsing and backplotting
#
# Dan Falck 2011/01/06

################################################################################
class Parser:

    def __init__(self):
        self.currentx = -1.0
        self.currenty = 0.0
        self.currentz = 0.0        
        x,y,z = 0.0,0.0,0.0
        self.absolute_flag = True

    ############################################################################
    ##  Internals

    def files_open(self, name, oname=None):
        if (oname == None ):
            oname = (name+'.scr')
        self.file_in = open(name, 'r')
        self.file_out = open(oname, 'w')

    def files_close(self):
        self.file_in.close()
        self.file_out.write('-linetype set continuous\n')
        self.file_out.write('\n')
        self.file_out.close()

    def readline(self):
        self.line = self.file_in.readline().rstrip()
        if (len(self.line)): 
            return True
        else: 
            return False

    def write(self, s):
        self.file_out.write(s)

    ############################################################################
    def end_ncblock(self):
        self.file_out.write('Delay 0\n')

    def add_text(self, s, col=None, cdata=False):
        return

    def set_mode(self, units=None):
        #self.file_out.write(' units="'+str(units)+'"')
        return

    def set_tool(self, number=None):        
        if (number != None): 
            self.file_out.write('-LAYER New T'+str(number)+'\n')
            self.file_out.write('-LAYER Set T'+str(number)+'\n')

    def begin_path(self, col=None):
        if (col != None): 
            if col == 'rapid':
                self.file_out.write('-color Red\n')
                #self.file_out.write('')
                self.file_out.write('-linetype set dashed\n')
                self.file_out.write('\n')
            else:
                self.file_out.write('-color Green\n')
                #self.file_out.write('')
                self.file_out.write('-linetype set continuous\n')
                self.file_out.write('\n')
        else : self.file_out.write('\n')

    def end_path(self):
        self.file_out.write('\n')

    def add_line(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if (x == None and y == None and z == None and a == None and b == None and c == None) : return
        #self.file_out.write('line %s,%s %s,%s' %(self.currentx,self.currenty,x,y))
        if (x == None) : x = self.currentx 
        if (y == None) : y = self.currenty 
        if (z == None) : z = self.currentz        
        self.file_out.write('line %s,%s,%s %s,%s,%s\n' %(self.currentx,self.currenty,self.currentz,x,y,z))        
        self.currentx = x 
        self.currenty = y
        self.currentz = z  

    def add_arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, d=None):
        if (x == None and y == None and z == None and i == None and j == None and k == None and r == None and d == None) : return
        
        z = self.currentz
        if (x == None) : x = self.currentx 
        if (y == None) : y = self.currenty 
        if (z == None) : z = self.currentz    
        if (d == 1):
            self.file_out.write('arc %s,%s,%s\n' %(self.currentx,self.currenty,self.currentz))
            self.file_out.write('c\n')
            self.file_out.write('%s,%s,%s\n' %(self.currentx+i,self.currenty+j,self.currentz))
            self.file_out.write('%s,%s,%s' %(x,y,z))
        else:
            self.file_out.write('arc %s,%s,%s\n' %(x,y,z))
            self.file_out.write('c\n')
            self.file_out.write('%s,%s,%s\n' %(self.currentx+i,self.currenty+j,self.currentz))
            self.file_out.write('%s,%s,%s' %(self.currentx,self.currenty,self.currentz))
        self.currentx = x 
        self.currenty = y
        self.currentz = z  

        
    def incremental(self):
        self.absolute_flag = False
        
    def absolute(self):
        self.absolute_flag = True
