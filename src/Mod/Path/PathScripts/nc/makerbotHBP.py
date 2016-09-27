import nc
import makerbot_codes as maker
import datetime
import iso_modal
import math


now = datetime.datetime.now()

################################################################################
class CreatorMakerbotHBP(iso_modal.CreatorIsoModal):
    def __init__(self):
        iso_modal.CreatorIsoModal.__init__(self)

        self.absolute_flag = True
        self.prev_g91 = ''


################################################################################
# program begin and end

    def program_begin(self, id, name=''):
        self.write((maker.codes.COMMENT(now)))
        self.write((maker.codes.EXTRUDER_TEMP('220')) + (maker.codes.COMMENT('Extruder Temp')) )
        self.write((maker.codes.BUILD_BED_TEMP('110'))+ (maker.codes.COMMENT('Build Bed Temp')) )
        self.write((maker.codes.FAN_OFF()) + (maker.codes.COMMENT('Fan Off')) )
        self.write((maker.codes.METRIC()) + (maker.codes.COMMENT('Metric units')) )
        self.write((maker.codes.ABSOLUTE()) + (maker.codes.COMMENT('Absolute units')) )
        self.write('G92 X0 Y0 Z0 (You are now at 0,0,0)\n')
        self.write('G0 Z15 (Move up for warmup)\n')
        self.write((maker.codes.EXTRUDER_SPEED_PWM('255')) + (maker.codes.COMMENT('Extruder Speed')) )
        self.write('M6 T0 (Wait for tool to heat up)\n')
        self.write('G04 P5000 (Wait 5 seconds)\n')
        self.write((maker.codes.EXTRUDER_ON_FWD()) + (maker.codes.COMMENT('Extruder On')) )
        self.write('G04 P5000 (Wait 5 seconds)\n')
        self.write((maker.codes.EXTRUDER_OFF()) + (maker.codes.COMMENT('Extruder Off')) )
        self.write('M01 (The heated build platform is heating up. Wait until after the lights have turned off for the first time, clear the test extrusion, and click yes.)\n')
        self.write('G0 Z0    (Go back to zero.)\n')

    def program_end(self):
        self.write((maker.codes.COMMENT('End of the file. Begin cool-down')))
        self.write((maker.codes.EXTRUDER_TEMP('0')) + (maker.codes.COMMENT('Extruder Temp')) )
        self.write((maker.codes.BUILD_BED_TEMP('0')) + (maker.codes.COMMENT('Build Bed Temp')) )
        self.write((maker.codes.FAN_ON()) + (maker.codes.COMMENT('Fan On')) )
        self.write('G92 Z0 (zero our z axis - hack b/c skeinforge mangles gcodes in end.txt)\n')
        self.write('G1 Z10 (go up 10 b/c it was zeroed earlier.)\n')
        self.write('G1 X0 Y0 Z10 (go to 0,0,z)\n')
        self.write((maker.codes.STEPPERS_OFF()) + (maker.codes.COMMENT('Steppers Off')) )

    def program_stop(self):
        self.write((maker.codes.EXTRUDER_TEMP('0')))
        self.write((maker.codes.BUILD_BED_TEMP('0')))
        self.write((maker.codes.STEPPERS_OFF()))
        
################################################################################
# general
    def write_blocknum(self):
        pass

    def set_plane(self, plane):
         pass
          
    def workplane(self, id):
        pass
        
    def spindle(self, s, clockwise):
        pass
################################################################################
# Extruder Control
        
    def extruder_on(self):
         self.write((maker.codes.EXTRUDER_ON()) + ('\n'))
        
    def extruder_off(self):
         self.write((maker.codes.EXTRUDER_OFF()) + ('\n'))
        
    def set_extruder_flowrate(self, flowrate):
         self.write((maker.codes.EXTRUDER_SPEED_PWM(flowrate)) + ('\n'))

    def extruder_temp(self, temp):
         self.write((maker.codes.EXTRUDER_TEMP(temp)) + ('\n'))
   
################################################################################
# Build Environment Control
    def build_bed_temp(self, temp):
         self.write((maker.codes.BUILD_BED_TEMP(temp)) + ('\n'))
        
    def chamber_temp(self, temp):
         self.write((maker.codes.CHAMBER_TEMP(temp)) + ('\n'))   
         
################################################################################
# Fan Control
    def fan_on(self):
         self.write((maker.codes.FAN_ON()) + ('\n'))
        
    def fan_off(self):
         self.write((maker.codes.FAN_OFF()) + ('\n'))
        
################################################################################
# Custom routines

    def wipe(self):
        self.write(('(This would be a good place for a custom wipe routine)\n'))

################################################################################
# APT style INSERT- insert anything into program

    def insert(self, text):
        self.write((text + '\n'))

################################################################################
# tool info
    def tool_change(self, id):
    	pass
     #	self.write_blocknum()
     #  self.write((maker.codes.TOOL() % id) + '\n')
     #  self.t = id
    
    def tool_defn(self, id, name='', params=None):
	pass
############################################################################
##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None ):
        self.write_blocknum()
        if self.g0123_modal:
            if self.prev_g0123 != maker.codes.RAPID():
                self.write(maker.codes.RAPID())
                self.prev_g0123 = maker.codes.RAPID()
        else:
            self.write(maker.codes.RAPID())
        self.write_preps()
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(maker.codes.X() + (self.fmt % x))
            else:
                self.write(maker.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(maker.codes.Y() + (self.fmt % y))
            else:
                self.write(maker.codes.Y() + (self.fmt % dy))

            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(maker.codes.Z() + (self.fmt % z))
            else:
                self.write(maker.codes.Z() + (self.fmt % dz))

            self.z = z

        if (a != None):
            da = a - self.a
            if (self.absolute_flag ):
                self.write(maker.codes.A() + (self.fmt % a))
            else:
                self.write(maker.codes.A() + (self.fmt % da))
            self.a = a

        if (b != None):
            db = b - self.b
            if (self.absolute_flag ):
                self.write(maker.codes.B() + (self.fmt % b))
            else:
                self.write(maker.codes.B() + (self.fmt % db))
            self.b = b

        if (c != None):
            dc = c - self.c
            if (self.absolute_flag ):
                self.write(maker.codes.C() + (self.fmt % c))
            else:
                self.write(maker.codes.C() + (self.fmt % dc))
            self.c = c
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None, a = None, b = None, c = None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        if self.g0123_modal:
            if self.prev_g0123 != maker.codes.FEED():
                self.write(maker.codes.FEED())
                self.prev_g0123 = maker.codes.FEED()
        else:
            self.write(maker.codes.FEED())
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(maker.codes.X() + (self.fmt % x))
            else:
                self.write(maker.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(maker.codes.Y() + (self.fmt % y))
            else:
                self.write(maker.codes.Y() + (self.fmt % dy))

            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(maker.codes.Z() + (self.fmt % z))
            else:
                self.write(maker.codes.Z() + (self.fmt % dz))

            self.z = z
        if (self.fhv) : self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def same_xyz(self, x=None, y=None, z=None):
        if (x != None):
            if (self.fmt % x) != (self.fmt % self.x):
                return False
        if (y != None):
            if (self.fmt % y) != (self.fmt % self.y):
                return False
        if (z != None):
            if (self.fmt % z) != (self.fmt % self.z):
                return False
            
        return True

    def arc(self, cw, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.same_xyz(x, y, z): return
        self.write_blocknum()
        arc_g_code = ''
        if cw: arc_g_code = maker.codes.ARC_CW()
        else: arc_g_code = maker.codes.ARC_CCW()
        if self.g0123_modal:
            if self.prev_g0123 != arc_g_code:
                self.write(arc_g_code)
                self.prev_g0123 = arc_g_code
        else:
            self.write(arc_g_code)
        self.write_preps()
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(maker.codes.X() + (self.fmt % x))
            else:
                self.write(maker.codes.X() + (self.fmt % dx))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(maker.codes.Y() + (self.fmt % y))
            else:
                self.write(maker.codes.Y() + (self.fmt % dy))
            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(maker.codes.Z() + (self.fmt % z))
            else:
                self.write(maker.codes.Z() + (self.fmt % dz))
            self.z = z
        if (i != None) : self.write(maker.codes.CENTRE_X() + (self.fmt % i))
        if (j != None) : self.write(maker.codes.CENTRE_Y() + (self.fmt % j))
        if (k != None) : self.write(maker.codes.CENTRE_Z() + (self.fmt % k))
        if (r != None) : self.write(maker.codes.RADIUS() + (self.fmt % r))
#       use horizontal feed rate
        if (self.fhv) : self.calc_feedrate_hv(1, 0)
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(True, x, y, z, i, j, k, r)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(False, x, y, z, i, j, k, r)

    def dwell(self, t):
        self.write_blocknum()
        self.write_preps()
        self.write(maker.codes.DWELL() + (maker.codes.TIME() % t))
        self.write_misc()
        self.write('\n')

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def rapid_unhome(self):
        pass

    def set_machine_coordinates(self):
        self.write(maker.codes.MACHINE_COORDINATES())
        self.prev_g0123 = ''

nc.creator = CreatorMakerbotHBP()

