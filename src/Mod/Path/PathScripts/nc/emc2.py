import nc
import iso
import math

class Creator(iso.Creator):
	def init(self): 
		iso.Creator.init(self) 

	def SPACE(self): return('')
        def TAP(self): return('G33.1')
        def TAP_DEPTH(self, format, depth): return(self.SPACE() + 'K' + (format.string(depth)))
	def BORE_FEED_OUT(self): return('G85')
	def BORE_SPINDLE_STOP_RAPID_OUT(self): return('G86')
	def BORE_DWELL_FEED_OUT(self, format, dwell): return('G89') + self.SPACE() + (format.string(dwell))
	def FEEDRATE(self): return((self.SPACE() + ' F'))

	def program_begin(self, id, comment):
		self.write( ('(' + comment + ')' + '\n') )

 ############################################################################
    ##  Settings
    
	def imperial(self):
            self.write( self.IMPERIAL() + '\t (Imperial Values)\n')
            self.fmt.number_of_decimal_places = 4

	def metric(self):
            self.fmt.number_of_decimal_places = 3
            self.write( self.METRIC() + '\t (Metric Values)\n' )

	def absolute(self):
		self.write( self.ABSOLUTE() + '\t (Absolute Coordinates)\n')

	def incremental(self):
		self.write( self.INCREMENTAL() + '\t (Incremental Coordinates)\n' )

	def polar(self, on=True):
		if (on) :
			self.write(self.POLAR_ON() + '\t (Polar ON)\n' )
		else : 
			self.write(self.POLAR_OFF() + '\t (Polar OFF)\n' )

	def set_plane(self, plane):
		if (plane == 0) : 
			self.write(self.PLANE_XY() + '\t (Select XY Plane)\n')
		elif (plane == 1) :
			self.write(self.PLANE_XZ() + '\t (Select XZ Plane)\n')
		elif (plane == 2) : 
			self.write(self.PLANE_YZ() + '\t (Select YZ Plane)\n')

	def comment(self, text):
		self.write((self.COMMENT(text) + '\n'))

	# This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
	# These are selected by values from 1 to 9 inclusive.
	def workplane(self, id):
		if ((id >= 1) and (id <= 6)):
			self.write( (self.WORKPLANE() % (id + self.WORKPLANE_BASE())) + '\t (Select Relative Coordinate System)\n')
		if ((id >= 7) and (id <= 9)):
			self.write( ((self.WORKPLANE() % (6 + self.WORKPLANE_BASE())) + ('.%i' % (id - 6))) + '\t (Select Relative Coordinate System)\n')

	def report_probe_results(self, x1=None, y1=None, z1=None, x2=None, y2=None, z2=None, x3=None, y3=None, z3=None, x4=None, y4=None, z4=None, x5=None, y5=None, z5=None, x6=None, y6=None, z6=None, xml_file_name=None ):
		if (xml_file_name != None):
			self.comment('Generate an XML document describing the probed coordinates found');
			self.write('(LOGOPEN,')
			self.write(xml_file_name)
			self.write(')\n')

		self.write('(LOG,<POINTS>)\n')

		if ((x1 != None) or (y1 != None) or (z1 != None)):
			self.write('(LOG,<POINT>)\n')

		if (x1 != None):
			self.write('#<_value>=[' + x1 + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y1 != None):
			self.write('#<_value>=[' + y1 + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z1 != None):
			self.write('#<_value>=[' + z1 + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x1 != None) or (y1 != None) or (z1 != None)):
			self.write('(LOG,</POINT>)\n')

		if ((x2 != None) or (y2 != None) or (z2 != None)):
			self.write('(LOG,<POINT>)\n')

		if (x2 != None):
			self.write('#<_value>=[' + x2 + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y2 != None):
			self.write('#<_value>=[' + y2 + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z2 != None):
			self.write('#<_value>=[' + z2 + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x2 != None) or (y2 != None) or (z2 != None)):
			self.write('(LOG,</POINT>)\n')

		if ((x3 != None) or (y3 != None) or (z3 != None)):
			self.write('(LOG,<POINT>)\n')

		if (x3 != None):
			self.write('#<_value>=[' + x3 + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y3 != None):
			self.write('#<_value>=[' + y3 + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z3 != None):
			self.write('#<_value>=[' + z3 + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x3 != None) or (y3 != None) or (z3 != None)):
			self.write('(LOG,</POINT>)\n')

		if ((x4 != None) or (y4 != None) or (z4 != None)):
			self.write('(LOG,<POINT>)\n')

		if (x4 != None):
			self.write('#<_value>=[' + x4 + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y4 != None):
			self.write('#<_value>=[' + y4 + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z4 != None):
			self.write('#<_value>=[' + z4 + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x4 != None) or (y4 != None) or (z4 != None)):
			self.write('(LOG,</POINT>)\n')

		if ((x5 != None) or (y5 != None) or (z5 != None)):
			self.write('(LOG,<POINT>)\n')

		if (x5 != None):
			self.write('#<_value>=[' + x5 + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y5 != None):
			self.write('#<_value>=[' + y5 + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z5 != None):
			self.write('#<_value>=[' + z5 + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x5 != None) or (y5 != None) or (z5 != None)):
			self.write('(LOG,</POINT>)\n')

		if ((x6 != None) or (y6 != None) or (z6 != None)):
			self.write('(LOG,<POINT>)\n')

		if (x6 != None):
			self.write('#<_value>=[' + x6 + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y6 != None):
			self.write('#<_value>=[' + y6 + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z6 != None):
			self.write('#<_value>=[' + z6 + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x6 != None) or (y6 != None) or (z6 != None)):
			self.write('(LOG,</POINT>)\n')

		self.write('(LOG,</POINTS>)\n')

		if (xml_file_name != None):
			self.write('(LOGCLOSE)\n')
			
	def open_log_file(self, xml_file_name=None ):
		self.write('(LOGOPEN,')
		self.write(xml_file_name)
		self.write(')\n')
			
	def close_log_file(self):
		self.write('(LOGCLOSE)\n')
			
	def log_coordinate(self, x=None, y=None, z=None):
		if ((x != None) or (y != None) or (z != None)):
			self.write('(LOG,<POINT>)\n')

		if (x != None):
			self.write('#<_value>=[' + x + ']\n')
			self.write('(LOG,<X>#<_value></X>)\n')

		if (y != None):
			self.write('#<_value>=[' + y + ']\n')
			self.write('(LOG,<Y>#<_value></Y>)\n')

		if (z != None):
			self.write('#<_value>=[' + z + ']\n')
			self.write('(LOG,<Z>#<_value></Z>)\n')

		if ((x != None) or (y != None) or (z != None)):
			self.write('(LOG,</POINT>)\n')

	def log_message(self, message=None ):
		self.write('(LOG,' + message + ')\n')
		
	def start_CRC(self, left = True, radius = 0.0):
		if self.t == None:
			raise "No tool specified for start_CRC()"
		if left:
			self.write(('G41' + self.SPACE() + 'D%i') % self.t  + '\t (start left cutter radius compensation)\n' )
		else:
			self.write(('G42' + self.SPACE() + 'D%i') % self.t  + '\t (start right cutter radius compensation)\n' )

	def end_CRC(self):
		self.g = 'G40'
		self.write_preps()
		self.write_misc()
		self.write('\t (end cutter radius compensation)\n')
		
	
	

	def tool_defn(self, id, name='', params=None):
		pass
			
nc.creator = Creator()

