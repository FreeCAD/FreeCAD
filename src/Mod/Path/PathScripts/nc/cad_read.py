# Preliminary backplot support for autocad clone applications
# This code modified from iso_read.py and emc2_read.py distriuted with HeeksCAD as of Sep 2010
# Dan Falck 2011/01/06
# 

""" use this script to backplot nc files to *.scr file for autocad,bricscad,
    draftsight,progecad,ares commander, etc....
    usage: python cad_read.py temp.nc temp.scr
"""

import cad_iso_read as iso
import sys



# Override some iso parser methods to interpret arc centers as relative to origin, not relative to start of arc.

#def write_layer(name,number):
    #FILE.write('-LAYER New %s%s \n' %(name,number))      
    #FILE.write('-LAYER Set %s%s \n' %(name,number))
 

class CAD_backplot(iso.Parser):

    def __init__(self):
        iso.Parser.__init__(self)

    def Parse(self, name, oname=None):
        self.files_open(name,oname)
        
        #self.begin_ncblock()
        #self.begin_path(None)
        #self.add_line(z=500)
        #self.end_path()
        #self.end_ncblock()
        
        path_col = None
        f = None
        arc = 0

        # Storage for tool position history of last block processed to properly convert absolute arc centers
        
        oldx = -1.0
        oldy = 0.0
        oldz = 0.0
        movelist = []
        while (self.readline()):
        # self.readline returns false if the line is empty - the parsing stops if the line is empty.   
            a = None
            b = None
            c = None
            #f = None
            i = None
            j = None
            k = None
            p = None
            q = None
            r = None
            s = None
            x = None
            y = None
            z = None
            iout = None
            jout = None
            kout = None
            tool = 0
            #self.begin_ncblock()

            move = False
            #arc = 0
            #path_col = None
            drill = False
            no_move = False

            words = self.pattern_main.findall(self.line)
            for word in words:
                col = None
                cdata = False
                if (word[0] == 'A' or word[0] == 'a'):
                    col = "axis"
                    a = eval(word[1:])
                    move = True
                elif (word[0] == 'B' or word[0] == 'b'):
                    col = "axis"
                    b = eval(word[1:])
                    move = True
                elif (word[0] == 'C' or word[0] == 'c'):
                    col = "axis"
                    c = eval(word[1:])
                    move = True
                elif (word[0] == 'F' or word[0] == 'f'):
                    col = "axis"
                    f = eval(word[1:])
                    move = True
                elif (word == 'G0' or word == 'G00' or word == 'g0' or word == 'g00'):
                    ##FILE.write('-color Magenta\n')
                    path_col = "rapid"
                    col = "rapid"
                    arc = 0
                elif (word == 'G1' or word == 'G01' or word == 'g1' or word == 'g01'):
                    ##FILE.write('-color Green\n')
                    path_col = "feed"
                    col = "feed"
                    arc = 0
                elif (word == 'G2' or word == 'G02' or word == 'g2' or word == 'g02' or word == 'G12' or word == 'g12'):
                    ##FILE.write('-color Green\n')
                    path_col = "feed"
                    col = "feed"
                    arc = -1
                elif (word == 'G3' or word == 'G03' or word == 'g3' or word == 'g03' or word == 'G13' or word == 'g13'):
                    ##FILE.write('-color Green\n')
                    path_col = "feed"
                    col = "feed"
                    arc = +1
                elif (word == 'G10' or word == 'g10'):
                    no_move = True                  
                elif (word == 'L1' or word == 'l1'):
                    no_move = True
                elif (word == 'G20' or word == 'G70'):
                    col = "prep"
                    self.set_mode(units=25.4)
                elif (word == 'G21' or word == 'G71'):
                    col = "prep"
                    self.set_mode(units=1.0)
                # Note: Anilam has very non standard params for drill cycles.  Not Yet implemented!    
                elif (word == 'G81' or word == 'g81'):
                    drill = True
                    no_move = True
                    path_col = "feed"
                    col = "feed"
                elif (word == 'G82' or word == 'g82'):
                    drill = True;
                    no_move = True
                    path_col = "feed"
                    col = "feed"
                elif (word == 'G83' or word == 'g83'):
                    drill = True
                    no_move = True
                    path_col = "feed"
                    col = "feed"
                elif (word[0] == 'G') : col = "prep"
                elif (word[0] == 'I' or word[0] == 'i'):
                    col = "axis"
                    i = eval(word[1:])
                    move = True
                elif (word[0] == 'J' or word[0] == 'j'):
                    col = "axis"
                    j = eval(word[1:])
                    move = True
                elif (word[0] == 'K' or word[0] == 'k'):
                    col = "axis"
                    k = eval(word[1:])
                    move = True
                elif (word[0] == 'M') : col = "misc"
                elif (word[0] == 'N') : col = "blocknum"
                elif (word[0] == 'O') : col = "program"
                elif (word[0] == 'P' or word[0] == 'p'):
                    col = "axis"
                    p = eval(word[1:])
                    move = True
                elif (word[0] == 'Q' or word[0] == 'q'):
                    col = "axis"
                    q = eval(word[1:])
                    move = True
                elif (word[0] == 'R' or word[0] == 'r'):
                    col = "axis"
                    r = eval(word[1:])
                    move = True
                elif (word[0] == 'S' or word[0] == 's'):
                    col = "axis"
                    s = eval(word[1:])
                    move = True
                elif (word[0] == 'T') :
                    col = "tool"
                    self.set_tool( eval(word[1:]) ) 
                    tool =  eval(word[1:]) 
                 
                elif (word[0] == 'X' or word[0] == 'x'):
                    col = "axis"
                    x = eval(word[1:])
                    move = True
                elif (word[0] == 'Y' or word[0] == 'y'):
                    col = "axis"
                    y = eval(word[1:])
                    move = True
                elif (word[0] == 'Z' or word[0] == 'z'):
                    col = "axis"
                    z = eval(word[1:])
                    move = True
                elif (word[0] == '(') : (col, cdata) = ("comment", True)
                elif (word[0] == '!') : (col, cdata) = ("comment", True)
                elif (word[0] == ';') : (col, cdata) = ("comment", True)
                elif (word[0] == '#') : col = "variable"
                elif (word[0] == ':') : col = "blocknum"
                elif (ord(word[0]) <= 32) : cdata = True
                #self.add_text(word, col, cdata)
                
            if (drill):
                self.begin_path("rapid")
                self.add_line(x, y, r)
                self.end_path()

                self.begin_path("feed")
                self.add_line(x, y, z)
                self.end_path()

                self.begin_path("feed")
                self.add_line(x, y, r)
                self.end_path()
            #elif (tool):
                #write_layer('T',tool)
                

            else:
                if (move and not no_move):
                    self.begin_path(path_col)
                    #use absolute arc centers for IJK params.
                    # Subtract old XYZ off to get relative centers as expected:

                    #if path_col == 'rapid':
                        #FILE.write('-color Red\n')
                    #else:
                        #FILE.write('-color Green\n')

                    if (arc) :
                        
                        z = oldz
                        if (x != None) and (oldx != None) and (i != None): iout = i
                        if (y != None) and (oldy != None) and (j != None): jout = j
                        if (z != None) and (oldz != None) and (k != None): kout = k
                        self.add_arc(x, y, z, iout, jout, kout, r, arc)
                        #if (arc == -1):
                            ##FILE.write('arc %s,%s,%s\n' %(x,y,z))
                            ##FILE.write('c\n')
                            ##FILE.write('%s,%s,%s\n' %(oldx+i,oldy+j,oldz))
                            ##FILE.write('%s,%s,%s\n' %(oldx,oldy,z))
                           
                        #else:
                            ##FILE.write('arc %s,%s,%s\n' %(oldx,oldy,z))
                            ##FILE.write('c\n')
                            ##FILE.write('%s,%s,%s\n' %(oldx+i,oldy+j,oldz))
                            ##FILE.write('%s,%s,%s\n' %(x,y,z))

                    else: 
                        
                        self.add_line(x, y, z, a, b, c)
                        if (x == None) : x = oldx 
                        if (y == None) : y = oldy 
                        if (z == None) : z = oldz
                        scr_line = ('line %s,%s,%s %s,%s,%s \n' %(oldx,oldy,oldz,x,y,z))
                        #print scr_line
                        
                        ##FILE.write(scr_line)
                        
                    self.end_path()
            if (x != None) : oldx = x
            if (y != None) : oldy = y
            if (z != None) : oldz = z

            #oldx = x
            #oldy = y
            #oldz = z
            self.end_ncblock()

        self.files_close()
        #FILE.write('\n')
        #FILE.close()
        
################################################################################    
if __name__ == '__main__':
    parser = CAD_backplot()
    if len(sys.argv)>2:
        parser.Parse(sys.argv[1],sys.argv[2])
    else:
        parser.Parse(sys.argv[1])
