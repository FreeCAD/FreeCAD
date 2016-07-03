################################################################################
# makerbot_codes.py
#
# a lot like iso_codes.py but with reprap/makerbot specific M codes.
#
# Brad Collette, 12th Sept 2010
#
# Many of these codes have nothing to do with reprap/additive machining but are left here in anticipation of future hybrid machines.

class Codes():
	def SPACE(self): return(' ')
	def FORMAT_FEEDRATE(self): return('%.2f') 
	def FORMAT_IN(self): return('%.5f')
	def FORMAT_MM(self): return('%.3f')
	def FORMAT_ANG(self): return('%.1f')
	def FORMAT_TIME(self): return('%.2f')
	def FORMAT_DWELL(self): return('P%f')

	def BLOCK(self): return('N%i' + self.SPACE())
	def COMMENT(self,comment): return( (' (%s)\n' % comment ) )
	def VARIABLE(self): return( '#%i')
	def VARIABLE_SET(self): return( '=%.3f')

	def PROGRAM(self): return( 'O%i')
	def PROGRAM_END(self): return( 'M02')

	def SUBPROG_CALL(self): return( 'M98' + self.SPACE() + 'P%i')
	def SUBPROG_END(self): return( 'M99')

	def STOP_OPTIONAL(self): return('M01')
	def STOP(self): return('M00')

	def IMPERIAL(self): return(self.SPACE() + 'G20')
	def METRIC(self): return(self.SPACE() + 'G21' + self.SPACE())
	def ABSOLUTE(self): return(self.SPACE() + 'G90' + self.SPACE())
	def INCREMENTAL(self): return(self.SPACE() + 'G91')
	def SET_TEMPORARY_COORDINATE_SYSTEM(self): return('G92' + self.SPACE())
	def REMOVE_TEMPORARY_COORDINATE_SYSTEM(self): return('G92.1' + self.SPACE())
	def POLAR_ON(self): return(self.SPACE() + 'G16')
	def POLAR_OFF(self): return(self.SPACE() + 'G15')
	def PLANE_XY(self): return(self.SPACE() + 'G17')
	def PLANE_XZ(self): return(self.SPACE() + 'G18')
	def PLANE_YZ(self): return(self.SPACE() + 'G19')

	def TOOL(self): return(self.SPACE() +'T%i')
	def TOOL_DEFINITION(self): return('G10' + self.SPACE() + 'L1' + self.SPACE())

	def WORKPLANE(self): return('G%i')
	def WORKPLANE_BASE(self): return(53)

	def FEEDRATE(self): return((self.SPACE() + ' F'))
	def SPINDLE(self, format, speed): return(self.SPACE() + 'S' + (format % speed))
	def SPINDLE_CW(self): return(self.SPACE() + 'M03')
	def SPINDLE_CCW(self): return(self.SPACE() + 'M04')
	def COOLANT_OFF(self): return(self.SPACE() + 'M09')
	def COOLANT_MIST(self): return(self.SPACE() + 'M07')
	def COOLANT_FLOOD(self): return(self.SPACE() + 'M08')
	def GEAR_OFF(self): return(self.SPACE() + '?')
	def GEAR(self): return('M%i')
	def GEAR_BASE(self): return(37)

	def RAPID(self): return('G0')
	def FEED(self): return('G1')
	def ARC_CW(self): return('G2')
	def ARC_CCW(self): return('G3')
	def DWELL(self): return('G04')
	def DRILL(self): return(self.SPACE() + 'G81')
	def DRILL_WITH_DWELL(self, format, dwell): return(self.SPACE() + 'G82' + (format % dwell))
	def PECK_DRILL(self): return(self.SPACE() + 'G83')
	def PECK_DEPTH(self, format, depth): return(self.SPACE() + 'Q' + (format % depth))
	def RETRACT(self, format, height): return(self.SPACE() + 'R' + (format % height))
	def END_CANNED_CYCLE(self): return(self.SPACE() + 'G80')

	def X(self): return(self.SPACE() + 'X')
	def Y(self): return(self.SPACE() + 'Y')
	def Z(self): return(self.SPACE() + 'Z')
	def A(self): return(self.SPACE() + 'A')
	def B(self): return(self.SPACE() + 'B')
	def C(self): return(self.SPACE() + 'C')
	def CENTRE_X(self): return(self.SPACE() + 'I')
	def CENTRE_Y(self): return(self.SPACE() + 'J')
	def CENTRE_Z(self): return(self.SPACE() + 'K')
	def RADIUS(self): return(self.SPACE() + 'R')
	def TIME(self): return(self.SPACE() + 'P')

	def PROBE_TOWARDS_WITH_SIGNAL(self): return('G38.2' + self.SPACE())
	def PROBE_TOWARDS_WITHOUT_SIGNAL(self): return('G38.3' + self.SPACE())
	def PROBE_AWAY_WITH_SIGNAL(self): return('G38.4' + self.SPACE())
	def PROBE_AWAY_WITHOUT_SIGNAL(self): return('G38.5' + self.SPACE())

	def MACHINE_COORDINATES(self): return('G53' + self.SPACE())

	def EXTRUDER_ON (self): return('M101') #deprecated
	def EXTRUDER_OFF (self): return('M103') 
	def EXTRUDER_TEMP (self, degree_celsius): return('M104 S' + '%s' % degree_celsius) 
	def EXTRUDER_TEMP_WAIT (self, degree_celsius): return('M109 S' + '%s' % degree_celsius) 
	def READ_EXTRUDER_TEMP (self): return('M105')
	def EXTRUDER_SPEED_PWM (self, speed_in_PWM): return('M108 S'  + '%s' % speed_in_PWM) #deprecated
 	def EXTRUDER_SPEED_RPM (self, speed_in_RPM): return('M108 P'  + '%s' % speed_in_RPM) #deprecated
 	
 	def STEPPERS_OFF(self): return(self.SPACE() + 'M118') 

	def ALL_WAIT (self): return(self.SPACE() + 'M116')  # Wait for all temperature and slow-changing variables to reach set values

	def FAN_ON (self): return(self.SPACE() + 'M106') 
	def FAN_OFF (self): return(self.SPACE() + 'M107')

	def VALVE_OPEN (self, delay): return(self.SPACE() + ('M126 P' + '%' % delay) ) 
	def VALVE_CLOSE (self, delay): return(self.SPACE() + ('M127 P' + '%' % delay) )  

	def BUILD_BED_TEMP (self, degree_celsius): return('M140 S' + '%s' % degree_celsius) 
	def BED_HOLDING_PRESSURE (self, pressure): return('M142 S' + '%s' % pressure) 

	def CHAMBER_TEMP (self, degree_celsius): return('M141 S' + '%s' % degree_celsius) 

#The following codes are listed on the reprap wiki page at http://reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes but require more study.
#
#G28 	G 	Y 	Xnnn Ynnn Znnn 	Move to origin (on specified axes only, if X/Y/Z parameters are present)
#M105 	M 	N 	none 	Request current extruder and base temperatures (in Celsius)
#M110 	M 	N 	none 	Set current line number to Nxxx value preceeding command
#M111 	M 	N 	Snnn 	Set debug level bitfield to value of parameter (default 6)
#M112 	M 	N 	none 	Emergency stop (stop immediately, discarding any buffered commands)
#M113 	M 	N 	Snnn 	Set Extruder PWM (to value defined by pot, or to parameter value if present)
#M114 	M 	N 	none 	Get Current Position (return current X, Y, Z and E values)
#M117 	M 	N 	none 	Get Zero Position (return X, Y, Z and E values of endstop hits)


codes = Codes()
