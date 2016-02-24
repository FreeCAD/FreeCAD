# -*- coding: utf-8 -*-
################################################################################
# iso.py
#
# Simple ISO NC code creator
#
# Hirutso Enni, 2009-01-13

import nc
import math
import circular_pocket as circular

################################################################################
class Creator(nc.Creator):

    def __init__(self):
        nc.Creator.__init__(self)

        self.a = 0
        self.b = 0
        self.c = 0
        self.f = ''
        self.fh = None
        self.fv = None
        self.fhv = False
        self.g = ''
        self.i = 0
        self.j = 0
        self.k = 0
        self.m = []
        self.n = 10
        self.r = 0
        self.s = ''
        self.t = None
        self.x = 0
        self.y = 0
        self.z = 500
	
	self.x1=0
	self.x2=0
	self.y1=0
	self.y2=0
	self.SPACE=' '

        self.fmt = self.FORMAT_MM()
	pass
    ############################################################################
    ##  Internals
    def FORMAT_IN(self): return('%.5f')
    def FORMAT_MM(self): return('%.3f')
    def COMMENT(self,comment): return( ('(%s)' % comment ) )

    def write_feedrate(self):
        #self.write(self.f)
        self.f = ''

    def write_preps(self):
        #self.write(self.g)
        self.g = ''

    def write_misc(self):
        #if (len(self.m)) : self.write(self.m.pop())
	pass

    def write_blocknum(self):
        #self.write(iso.BLOCK % self.n)
        #self.write(iso.SPACE)
        #self.n += 10
	pass

    def write_spindle(self):
        #self.write(self.s)
        #self.s = ''
	pass

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        #self.write((iso.PROGRAM % id) + iso.SPACE + (iso.COMMENT % name))
        #self.write("// program v jazyku"+self.SPACE+"REZ\nMA 0.0000 , 0.0000\n")
	self.write("// program v jazyku"+self.SPACE+"REZ\nGO_LEFT\nMA 0.0000 , 0.0000\n")
	#self.write('\n')

    def program_stop(self, optional=False):
        #self.write_blocknum()
        #if (optional) : self.write(iso.STOP_OPTIONAL + '\n')
        #else : self.write(iso.STOP + '\n')
	self.write("// stop programu v jazyku REZ\n")
  
    def program_end(self):
        #self.write_blocknum()
        #self.write(iso.PROGRAM_END + '\n')
	self.write('PL_OFF \n')
	self.write('PARK \n')
	self.write('FS \n')
	self.write('MA  0.0000 ,  0.0000\n')
	self.write("// koniec programu v jazyku REZ\n")
	#self.write(iso.PROGRAM_END + '\n')
    def flush_nc(self):
        #self.write_blocknum()
        self.write_preps()
        self.write_misc()
        #self.write('\n')

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        #self.write((iso.PROGRAM % id) + iso.SPACE + (iso.COMMENT % name))
        #self.write('\n')
	pass
    def sub_call(self, id):
        #self.write_blocknum()
        #self.write((iso.SUBPROG_CALL % id) + '\n')
	#self.write('\n')
	pass

    def sub_end(self):
        #self.write_blocknum()
        #self.write(iso.SUBPROG_END + '\n')
	#self.write('\n')
	pass
    ############################################################################
    ##  Settings
    
    def imperial(self):
        #self.g += iso.IMPERIAL
        #self.fmt = iso.FORMAT_IN
	#self.write('\n')
	pass

    def metric(self):
        #self.g += iso.METRIC
        #self.fmt = iso.FORMAT_MM
	#self.write('\n')
	pass

    def absolute(self):
        #self.g += iso.ABSOLUTE
	#self.write('\n')
	pass

    def incremental(self):
        #self.g += iso.INCREMENTAL
	#self.write('\n')
	pass
    def polar(self, on=True):
        #if (on) : self.g += iso.POLAR_ON
        #else : self.g += iso.POLAR_OFF
	#self.write('\n')
	pass
    def set_plane(self, plane):
        #if (plane == 0) : self.g += iso.PLANE_XY
        #elif (plane == 1) : self.g += iso.PLANE_XZ
        #elif (plane == 2) : self.g += iso.PLANE_YZ
	#self.write('\n')
	pass
    ############################################################################
    ##  Tools

    def tool_change(self, id):
        #self.write_blocknum()
        #self.write((iso.TOOL % id) + '\n')
	#self.write('\n')
	pass
    def tool_defn(self, id, name='', params=None):
        pass

    def offset_radius(self, id, radius=None):
	pass

    def offset_length(self, id, length=None):
	pass

    ############################################################################
    ##  Datums
    
    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    # This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
    # These are selected by values from 1 to 9 inclusive.
    def workplane(self, id):
	#if ((id >= 1) and (id <= 6)):
	#	self.g += iso.WORKPLANE % (id + iso.WORKPLANE_BASE)
	#if ((id >= 7) and (id <= 9)):
	#	self.g += ((iso.WORKPLANE % (6 + iso.WORKPLANE_BASE)) + ('.%i' % (id - 6)))
	pass
    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        #self.f = iso.FEEDRATE + (self.fmt % f)
        #self.fhv = False
	pass

    def feedrate_hv(self, fh, fv):
        #self.fh = fh
        #self.fv = fv
        #self.fhv = True
	pass
    def calc_feedrate_hv(self, h, v):
        #l = math.sqrt(h*h+v*v)
        #if (h == 0) : self.f = iso.FEEDRATE + (self.fmt % self.fv)
        #elif (v == 0) : self.f = iso.FEEDRATE + (self.fmt % self.fh)
        #else:
         #   self.f = iso.FEEDRATE + (self.fmt % (self.fh * l * min([1/h, 1/v])))
	pass

    def spindle(self, s, clockwise):
	#if s < 0: clockwise = not clockwise
	#s = abs(s)
        #self.s = iso.SPINDLE % s
	#if clockwise:
	#	self.s = self.s + iso.SPINDLE_CW
	#else:
	#	self.s = self.s + iso.SPINDLE_CCW
	pass

    def coolant(self, mode=0):
        #if (mode <= 0) : self.m.append(iso.COOLANT_OFF)
        #elif (mode == 1) : self.m.append(iso.COOLANT_MIST)
        #elif (mode == 2) : self.m.append(iso.COOLANT_FLOOD)
	pass

    def gearrange(self, gear=0):
        #if (gear <= 0) : self.m.append(iso.GEAR_OFF)
        #elif (gear <= 4) : self.m.append(iso.GEAR % (gear + GEAR_BASE))
	pass

    ############################################################################
    ##  Moves

    #def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
    def rapid(self, x=0.0000, y=0.0000, z=0.0000, a=0.0000, b=0.0000, c=0.0000,how=False):
	#self.write_blocknum()
        if (x == None):
	   if (y == None):
	      return
	print 'rychlopsuv'
	print x
	print y
	print z
	print a
	print b
	print c
	self.write('PL_OFF\n')
	self.write('PARK\n')
	self.write('FS\n')
	self.write('MA ')
        #self.write_preps()
        #if (x != None):
        #    dx = x - self.x
        #    self.write(iso.X + (self.fmt % x))
        #    self.x = x
        #if (y != None):
        #    dy = y - self.y
        #    self.write(iso.Y + (self.fmt % y))
        #    self.y = y
        #if (z != None):
        #    dz = z - self.z
        #    self.write(iso.Z + (self.fmt % z))
        #    self.z = z
        #if (a != None) : self.write(iso.A + (iso.FORMAT_ANG % a))
        #if (b != None) : self.write(iso.B + (iso.FORMAT_ANG % b))
        #if (c != None) : self.write(iso.C + (iso.FORMAT_ANG % c))
        #self.write_spindle()
        #self.write_misc()
	self.write((' %.4f' %x))
	#self.write(('%f' %x) )
	self.write(' , ')
	self.write((' %.4f' %y))
	self.write('\n')
	self.write('SS\n')
	self.write('AD_W\n')
	self.write('PL_ON')
	self.write('\n')
	self.x=x
	self.y=y

    def feed(self, x=0.0000, y=0.0000, z=0.0000,how=False):
        if self.same_xyz(x, y, z): 
	  return
        if (x == None):
	  if (y == None):
	      return

	print 'MA rez'
	print x
	print y
	print z
	#self.write_blocknum()
        #self.write(iso.FEED)
        #self.write_preps()
        #dx = dy = dz = 0
        #if (x != None):
        #    dx = x - self.x
        #    self.write(iso.X + (self.fmt % x))
        #    self.x = x
        #if (y != None):
        #    dy = y - self.y
        #    self.write(iso.Y + (self.fmt % y))
        #    self.y = y
        #if (z != None):
        #    dz = z - self.z
        #    self.write(iso.Z + (self.fmt % z))
        #    self.z = z
        #if (self.fhv) : self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))
        #self.write_feedrate()
        #self.write_spindle()
        #self.write_misc()
        self.write('MA ')
        self.write((' %.4f' %x))
	#self.write(('%f' %x) )
	self.write(' , ')
	self.write((' %.4f' %y))
	self.write('\n')
	self.x=x
	self.y=y

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

    def arc(self, cw, x=0.0000, y=0.0000, z=0.0000, i=0.0000, j=0.0000, k=0.0000, r=0.0000):
	if self.same_xyz(x, y, z): return
	if (x == None):
	  if (y == None):
	      return
      #self.write_blocknum()
	print 'ARC rez'
	print x
	print y
	print z
	print i
	print j
	print k
	print r
	self.write('C  ')
      #  if cw: self.write(iso.ARC_CW)
      #  else: self.write(iso.ARC_CCW)
      #  self.write_preps()
       #if (x != None):
        #    self.write(iso.X + (self.fmt % x))
        #    self.x = x
        #if (y != None):
        #    self.write(iso.Y + (self.fmt % y))
        #    self.y = y
        #if (z != None):
         #   self.write(iso.Z + (self.fmt % z))
         #   self.z = z
        #if (i != None) : self.write(iso.CENTRE_X + (self.fmt % i))
        #if (j != None) : self.write(iso.CENTRE_Y + (self.fmt % j))
        #if (k != None) : self.write(iso.CENTRE_Z + (self.fmt % k))
        #if (r != None) : self.write(iso.RADIUS + (self.fmt % r))
#       use horizontal feed rate
        #if (self.fhv) : self.calc_feedrate_hv(1, 0)
        #self.write_feedrate()
        #self.write_spindle()
        #self.write_misc()
        self.write((' %.4f' %(self.x + i)))
	#self.write(('%f' %x) )
	self.write(' , ')
	self.write((' %.4f' %(self.y + j)))
	
	self.write(' , ')
	angle=0.0000
	
	self.x1=-i
	self.y1=-j
	self.x2=x-(self.x+i)
	self.y2=y-(self.y+j)

	dx=self.x1*self.x2
	dy=self.y1*self.y2
	ssucin=dx+dy
	r=math.sqrt((i*i)+(j*j))
	
	#dx=i*(x-(self.x + i))
	#dy=j*(y-(self.y + j))
	#ssucin=dx+dy
	#r=math.sqrt((i*i)+(j*j))
	ratio=ssucin/(r*r)
	angle= math.acos(ratio) * 180 / math.pi
	print 'angle' 
	print angle
#	while(angle>=90.0001):
#	  if(cw): self.write((' -90.0000\n' ))
#	  else: self.write((' 90.0000\n'))
	  #self.write(('90.0000\n' ))
#	  self.write('C  ')
#	  self.write((' %.4f' %( self.x + i) ))
#	  self.write(' , ')
#	  self.write((' %.4f' %(  self.y + j)))
#	  self.write(' , ')
#	  angle-=90
	if(cw): self.write((' %.4f' %(-angle))) 
	else: self.write((' %.4f' %(angle)))
	

	#self.write((', %.4f' % ))
	self.write('\n')
	self.x=x
	self.y=y
	self.write('// stred')
	self.write((' %f' %i))
	self.write(' ')
	self.write( (' %f' %j))
	self.write('\n')

	self.write('// koniec')
	self.write((' %f' %self.x))
	self.write(' ')
	self.write( (' %f' %self.y))
	self.write('\n')
	
    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(True, x, y, z, i, j, k, r)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(False, x, y, z, i, j, k, r)

    def dwell(self, t):
        self.write_blocknum()
        self.write_preps()
        self.write(iso.DWELL + (iso.TIME % t))
        self.write_misc()
        self.write('\n')

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def rapid_unhome(self):
        pass

    ############################################################################
    ##  Cycles

    def pattern(self):
        pass

    def pocket(self):
        pass

    def profile(self):
        pass

    def circular_pocket(self, x=None, y=None, ToolDiameter=None, HoleDiameter=None, ClearanceHeight=None, StartHeight=None, MaterialTop=None, FeedRate=None, SpindleRPM=None, HoleDepth=None, DepthOfCut=None, StepOver=None ):
	self.write_preps()
	circular.pocket.block_number = self.n
	(self.g_code, self.n) = circular.pocket.GeneratePath( x,y, ToolDiameter, HoleDiameter, ClearanceHeight, StartHeight, MaterialTop, FeedRate, SpindleRPM, HoleDepth, DepthOfCut, StepOver )
	self.write(self.g_code)

	# The drill routine supports drilling (G81), drilling with dwell (G82) and peck drilling (G83).
	# The x,y,z values are INITIAL locations (above the hole to be made.  This is in contrast to
	# the Z value used in the G8[1-3] cycles where the Z value is that of the BOTTOM of the hole.
	# Instead, this routine combines the Z value and the depth value to determine the bottom of
	# the hole.
	#
	# The standoff value is the distance up from the 'z' value (normally just above the surface) where the bit retracts
	# to in order to clear the swarf.  This combines with 'z' to form the 'R' value in the G8[1-3] cycles.
	#
	# The peck_depth value is the incremental depth (Q value) that tells the peck drilling
	# cycle how deep to go on each peck until the full depth is achieved.
	#
	# NOTE: This routine forces the mode to absolute mode so that the values  passed into
	# the G8[1-3] cycles make sense.  I don't know how to find the mode to revert it so I won't
	# revert it.  I must set the mode so that I can be sure the values I'm passing in make
	# sense to the end-machine.
	#
    def drill(self, x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None, rapid_to_clearance=None):
	if (standoff == None):
		# This is a bad thing.  All the drilling cycles need a retraction (and starting) height.
		return

	if (z == None): return	# We need a Z value as well.  This input parameter represents the top of the hole

        self.write_blocknum()
        self.write_preps()

	if (peck_depth != 0):
		# We're pecking.  Let's find a tree.
		self.write(iso.PECK_DRILL + iso.SPACE + iso.PECK_DEPTH + (self.fmt % peck_depth ))
	else:
		# We're either just drilling or drilling with dwell.
		if (dwell == 0):
			# We're just drilling.
			self.write(iso.DRILL + iso.SPACE)
		else:
			# We're drilling with dwell.
			self.write(iso.DRILL_WITH_DWELL + (iso.FORMAT_DWELL % dwell) + iso.SPACE)

	# Set the retraction point to the 'standoff' distance above the starting z height.
	retract_height = z + standoff
	#self.write(iso.RETRACT + (self.fmt % retract_height))
        if (x != None):
            dx = x - self.x
            self.write(iso.X + (self.fmt % x) + iso.SPACE)
            self.x = x
        if (y != None):
            dy = y - self.y
            self.write(iso.Y + (self.fmt % y) + iso.SPACE)
            self.y = y

	dz = (z + standoff) - self.z
			# In the end, we will be standoff distance above the z value passed in.
	self.write(iso.Z + (self.fmt % (z - depth)) + iso.SPACE)	# This is the 'z' value for the bottom of the hole.
	self.z = (z + standoff)				# We want to remember where z is at the end (at the top of the hole)
	self.write(iso.RETRACT + (self.fmt % retract_height))
        self.write_spindle()
        self.write_misc()
        self.write('\n')


#    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):
#        pass

    # argument list adapted for compatibility with Tapping module
    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        pass

    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        pass

    ############################################################################
    ##  Misc

    def comment(self, text):
        self.write('// ' + (self.COMMENT( text)) + '\n')

    def variable(self, id):
        return (iso.VARIABLE % id)

    def variable_set(self, id, value):
        self.write_blocknum()
        self.write((iso.VARIABLE % id) + (iso.VARIABLE_SET % value) + '\n')

################################################################################

nc.creator = Creator()
