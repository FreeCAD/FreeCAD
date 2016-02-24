import nc
import iso_codes
import emc2



class CodesEMC2(iso_codes.Codes):
    def SPACE(self): return(' ')
    def TAP(self): return('G33.1')
    def TAP_DEPTH(self, format, depth): return(self.SPACE() + 'K' + (format % depth))


    # This version of COMMENT removes comments from the resultant GCode
    #def COMMENT(self,comment): return('')

iso_codes.codes = CodesEMC2()


class CreatorEMC2tap(emc2.CreatorEMC2):
    def init(self): 
        iso.CreatorEMC2.init(self) 


    # G33.1 tapping with EMC for now
    # unsynchronized (chuck) taps NIY (tap_mode = 1)
    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        # mystery parameters: 
        # zretract=None, dwell_bottom=None,pitch=None, stoppos=None, spin_in=None, spin_out=None):
        # I dont see how to map these to EMC Gcode

        if (standoff == None):        
            # This is a bad thing.  All the drilling cycles need a retraction (and starting) height.        
            return
        if (z == None): 
            return	# We need a Z value as well.  This input parameter represents the top of the hole 
        if (pitch == None): 
            return	# We need a pitch value.
        if (direction == None): 
            return	# We need a direction value.

        if (tap_mode != 0):
            self.comment('only rigid tapping currently supported')
            return

        self.write_preps()
        self.write_blocknum()                
        self.write_spindle()
        self.write('\n')

        # rapid to starting point; z first, then x,y iff given

        # Set the retraction point to the 'standoff' distance above the starting z height.        
        retract_height = z + standoff        

        # unsure if this is needed:
        if self.z != retract_height:
                self.rapid(z = retract_height)

        # then continue to x,y if given
        if (x != None) or (y != None):
                self.write_blocknum()                
                self.write(iso_codes.codes.RAPID() )           

                if (x != None):        
                        self.write(iso_codes.codes.X() + (self.fmt % x))        
                        self.x = x 

                if (y != None):        
                        self.write(iso_codes.codes.Y() + (self.fmt % y))        
                        self.y = y
                self.write('\n')

        self.write_blocknum()                
        self.write( iso_codes.codes.TAP() )
        self.write( iso_codes.codes.TAP_DEPTH(self.ffmt,pitch) + iso_codes.codes.SPACE() )            
        self.write(iso_codes.codes.Z() + (self.fmt % (z - depth)))	# This is the 'z' value for the bottom of the tap.
        self.write_misc()    
        self.write('\n')

        self.z = retract_height    # this cycle returns to the start position, so remember that as z value



nc.creator = CreatorEMC2tap()

