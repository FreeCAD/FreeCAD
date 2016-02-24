# Preliminary postprocessor support for Anilam Crusader M CNC controller
# This code modified from iso.py and emc2.py distriuted with HeeksCAD as of Sep 2010
# Kurt Jensen 6 Sep 2010
# Use at your own risk.
import nc
import iso

class Creator(iso.Creator):
    def init(self): 
        iso.Creator.init(self)
        self.arc_centre_absolute = True
        
    def SPACE(self): return(' ')

    # This version of COMMENT removes comments from the resultant GCode
    # Note: The Anilam hates comments when importing code.
    
    def COMMENT(self,comment): return('')

    def program_begin(self, id, comment):
        self.write('%\n');  # Start of file token that Anilam Crusader M likes
        # No Comments for the Anilam crusaher M, please......
        #self.write( ('(' + comment + ')' + '\n') )
        
    def program_end(self):
        self.write_blocknum()
        self.write('G29E\n')  # End of code signal for Anilam Crusader M
        self.write('%\n')     # EOF signal for Anilam Crusader M

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.write_blocknum()
        self.write( self.IMPERIAL() + '\n')
        self.fmt.number_of_decimal_places = 4

    def metric(self):
        self.write_blocknum()
        self.write( self.METRIC() + '\n' )
        self.fmt.number_of_decimal_places = 3

    def absolute(self):
        self.write_blocknum()
        self.write( self.ABSOLUTE() + '\n')

    def incremental(self):
        self.write_blocknum()
        self.write( self.INCREMENTAL() + '\n' )

    def polar(self, on=True):
        if (on) :
            self.write_blocknum()
            self.write(self.POLAR_ON() + '\n' )
        else : 
            self.write_blocknum()
            self.write(self.POLAR_OFF() + '\n' )

    def set_plane(self, plane):
        if (plane == 0) : 
            self.write_blocknum()
            self.write('G17\n')
        elif (plane == 1) :
            self.write_blocknum()
            self.write('G18\n')
        elif (plane == 2) : 
            self.write_blocknum()
            self.write('G19\n')

    def comment(self, text):
       self.write_blocknum()
       
    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.write_blocknum()
        self.write(('T%i' % id) + '\n')
        self.t = id

    def tool_defn(self, id, name='', params=None):
        self.write_blocknum()
        self.write(('T10%.2d' % id) + ' ')

        if (radius != None):
            self.write(('X%.3f' % radius) + ' ')

        if (length != None):
            self.write('Z%.3f' % length)

        self.write('\n')   

    # This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
    # These are selected by values from 1 to 9 inclusive.
    def workplane(self, id):
        if ((id >= 1) and (id <= 6)):
            self.write_blocknum()
            self.write( (self.WORKPLANE() % (id + self.WORKPLANE_BASE())) + '\n')
        if ((id >= 7) and (id <= 9)):
            self.write_blocknum()
            self.write( ((self.WORKPLANE() % (6 + self.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\n')
    
    # inhibit N codes being generated for line numbers:
    def write_blocknum(self): 
        pass
        
    def drill(self, x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None, rapid_to_clearance = None):
        self.write('(Canned drill cycle ops are not yet supported here on this Anilam Crusader M postprocessor)')

nc.creator = Creator()

