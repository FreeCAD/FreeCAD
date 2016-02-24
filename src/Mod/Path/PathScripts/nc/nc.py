################################################################################
# nc.py
#
# Base class for NC code creation
# And global functions for calling current creator
#
# Hirutso Enni, 2009-01-13
# altered by Dan Falck 2010-08-04
# added tap() arguments Michael Haberler 2010-10-07
################################################################################

ncOFF = 0

ncLEFT = -1
ncRIGHT = +1

ncCW = -1
ncCCW = +1

ncMIST = 1
ncFLOOD = 2

################################################################################
class Creator:

    def __init__(self):
        pass

    ############################################################################
    ##  Internals

    def file_open(self, name):
        if name == "mem":
            self.gcode = ""
        else:
            self.file = open(name, 'w')
        self.filename = name

    def file_close(self):
        if self.filename != "mem":
            self.file.close()

    def write(self, s):
        if self.filename == "mem":
            self.gcode += s
        else:
            self.file.write(s)
    def retrieve_gcode(self):
        return self.gcode

    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        """Begin a program"""
        pass
    
    def add_stock(self, type_name, params):
        pass

    def program_stop(self, optional=False):
        """Stop the machine"""
        pass

    def program_end(self):
        """End the program"""
        pass

    def flush_nc(self):
        """Flush all pending codes"""
        pass

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=''):
        """Begin a subprogram"""
        pass

    def sub_call(self, id):
        """Call a subprogram"""
        pass

    def sub_end(self):
        """Return from a subprogram"""
        pass

    ############################################################################
    ##  Settings
    
    def imperial(self):
        """Set imperial units"""
        pass

    def metric(self):
        """Set metric units"""
        pass

    def absolute(self):
        """Set absolute coordinates"""
        pass

    def incremental(self):
        """Set incremental coordinates"""
        pass

    def polar(self, on=True):
        """Set polar coordinates"""
        pass

    def set_plane(self, plane):
        """Set plane"""
        pass

    def set_temporary_origin(self, x=None, y=None, z=None, a=None, b=None, c=None):
        """Set temporary origin G92"""
        pass

    def remove_temporary_origin(self):
        """Remote temporary origin G92.1"""
        pass
 
    ############################################################################
    ##  Tools
    
    def tool_change(self, id):
        """Change the tool"""
        pass

    def tool_defn(self, id, name='', params=None):
        """Define a tool"""
        pass

    def offset_radius(self, id, radius=None):
        """Set tool radius offsetting"""
        pass

    def offset_length(self, id, length=None):
        """Set tool length offsetting"""
        pass
    
    def current_tool(self):
        return None

    ############################################################################
    ##  Datums
    
    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        """Shift the datum"""
        pass

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        """Set the datum"""
        pass

    def workplane(self, id):
        """Set the workplane"""
        pass

    def clearanceplane(self,z=None):
        """set clearance plane"""
        pass

    ############################################################################
    ##  APT360 like Transformation Definitions
    ##  These definitions were created while looking at Irvin Kraal's book on APT
    ##  - Numerical Control Progamming in APT - page 211

    def matrix(self,a1=None,b1=None,c1=None,a2=None,b2=None,c2=None,a3=None,b3=None,c3=None):
        """Create a matrix for transformations"""
        pass
    def translate(self,x=None,y=None,z=None):
        """Translate in x,y,z direction"""
        pass
    def rotate(self,xyrot=None,yzrot=None,zxrot=None,angle=None):
        """Rotate about a coordinate axis"""
        pass
    def scale(self,k=None):
        """Scale by factor k"""
        pass
    def matrix_product(self,matrix1=None,matrix2=None):
        """Create matrix that is the product of two other matrices"""
        pass
    def mirror_plane(self,plane1=None,plane2=None,plane3=None):
        """Mirror image about one or more coordinate planes"""
        pass
    def mirror_line(self,line=None):
        """Mirror about a line"""
        pass


    ############################################################################
    ##  Rates + Modes
    
    def feedrate(self, f):
        """Set the feedrate"""
        pass

    def feedrate_hv(self, fh, fv):
        """Set the horizontal and vertical feedrates"""
        pass

    def spindle(self, s, clockwise=True):
        """Set the spindle speed"""
        pass

    def coolant(self, mode=0):
        """Set the coolant mode"""
        pass

    def gearrange(self, gear=0):
        """Set the gear range"""
        pass

    ############################################################################
    ##  Moves
    
    def rapid(self, x=None, y=None, z=None):
        """Rapid move"""
        pass

    def feed(self, x=None, y=None, z=None):
        """Feed move"""
        pass

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        """Clockwise arc move"""
        pass

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        """Counterclockwise arc move"""
        pass

    def dwell(self, t):
        """Dwell"""
        pass

    def rapid_home(self, x=None, y=None, z=None):
        """Rapid relative to home position"""
        pass

    def rapid_unhome(self):
        """Return from rapid home"""
        pass

    def set_machine_coordinates(self):
        """Set machine coordinates"""
        pass

    ############################################################################
    ##  Cutter radius compensation
    
    def use_CRC(self):
        """CRC"""
        return False
        
    ############################################################################
    ##  Cycles

    def pattern(self):
        """Simple pattern eg. circle, rect"""
        pass

    def pocket(self):
        """Pocket routine"""
        pass

    def profile(self):
        """Profile routine"""
        pass

    def drill(self, x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None, rapid_to_clearance=None):
        """Drilling routines"""
        pass

    # original prototype was:
    # def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):
    #
    # current call is like so:
    # tap(x=10, y=10, z=0, tap_mode=0, depth=12.7, standoff=6.35, direction=0, pitch=1.25)
    # just add tap_mode & direction parameters

    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        """Tapping routines"""
        pass

    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        """Boring routines"""
        pass

    def end_canned_cycle(self):
        pass

    ############################################################################
    ##  Misc

    def comment(self, text):
        """Insert a comment"""
        pass

    def insert(self, text):
        """APT style INSERT statement"""
        pass

    def block_delete(self, on=False):
        """block to ignore if block delete switch is on"""
        pass

    def variable(self, id):
        """Insert a variable"""
        pass

    def variable_set(self, id, value):
        """Set a variable"""
        pass

    def probe_linear_centre_outside(self, x1=None, y1=None, depth=None, x2=None, y2=None ):
        pass

    def probe_single_point(self, point_along_edge_x=None, point_along_edge_y=None, depth=None, retracted_point_x=None, retracted_point_y=None, destination_point_x=None, destination_point_y=None, intersection_variable_x=None, intersection_variable_y=None, probe_offset_x_component=None, probe_offset_y_component=None ):
        pass

    def probe_downward_point(self, x=None, y=None, depth=None, intersection_variable_z=None):
        pass

    def report_probe_results(self, x1=None, y1=None, z1=None, x2=None, y2=None, z2=None, x3=None, y3=None, z3=None, x4=None, y4=None, z4=None, x5=None, y5=None, z5=None, x6=None, y6=None, z6=None, xml_file_name=None ):
        pass

    def open_log_file(self, xml_file_name=None ):
        pass

    def log_coordinate(self, x=None, y=None, z=None):
        pass

    def log_message(self, message=None):
        pass

    def close_log_file(self):
        pass

    def rapid_to_midpoint(self, x1=None, y1=None, z1=None, x2=None, y2=None, z2=None):
        pass

    def rapid_to_intersection(self, x1, y1, x2, y2, x3, y3, x4, y4, intersection_x, intersection_y, ua_numerator, ua_denominator, ua, ub_numerator, ub):
        pass

    def rapid_to_rotated_coordinate(self, x1, y1, x2, y2, ref_x, ref_y, x_current, y_current, x_final, y_final):
        pass

    def set_path_control_mode(self, mode, motion_blending_tolerance, naive_cam_tolerance ):
        pass


################################################################################

creator = Creator()

############################################################################
##  Internals

def write(s):
    creator.write(s)
    
def output(filename):
    creator.file_open(filename)

def retrieve_gcode():
    return creator.retrieve_gcode()


############################################################################
##  Programs

def program_begin(id, name=''):
    creator.program_begin(id, name)

def add_stock(type_name, params):
    creator.add_stock(type_name, params)

def program_stop(optional=False):
    creator.program_stop(optional)

def program_end():
    creator.program_end()

def flush_nc():
    creator.flush_nc()

############################################################################
##  Subprograms

def sub_begin(id, name=''):
    creator.sub_begin(id, name)

def sub_call(id):
    creator.sub_call(id)

def sub_end():
    creator.sub_end()

############################################################################
##  Settings

def imperial():
    creator.imperial()

def metric():
    creator.metric()

def absolute():
    creator.absolute()

def incremental():
    creator.incremental()

def polar(on=True):
    creator.polar(on)

def set_plane(plane):
    creator.set_plane(plane)

def set_temporary_origin(x=None, y=None, z=None, a=None, b=None, c=None):
    creator.set_temporary_origin(x,y,z,a,b,c)

def remove_temporary_origin():
    creator.remove_temporary_origin()
 
############################################################################
##  Tools

def tool_change(id):
    creator.tool_change(id)

def tool_defn(id, name='', params=None):
    creator.tool_defn(id, name, params)

def offset_radius(id, radius=None):
    creator.offset_radius(id, radius)

def offset_length(id, length=None):
    creator.offset_length(id, length)

def current_tool(self):
    return creator.current_tool()

############################################################################
##  Datums

def datum_shift(x=None, y=None, z=None, a=None, b=None, c=None):
    creator.datum_shift(x, y, z, a, b, c)

def datum_set(x=None, y=None, z=None, a=None, b=None, c=None):
    creator.datum_set(x, y, z, a, b, c)

def workplane(id):
    creator.workplane(id)

def clearanceplane(z=None):
    creator.clearanceplane(z)

############################################################################
##  APT360 like Transformation Definitions
##  These definitions were created while looking at Irvin Kraal's book on APT
##  - Numerical Control Progamming in APT - page 211

def matrix(a1=None,b1=None,c1=None,a2=None,b2=None,c2=None,a3=None,b3=None,c3=None):
    creator.matrix(a1,b1,c1,a2,b2,c2,a3,b3,c3)
    
def translate(x=None,y=None,z=None):
    creator.translate(x,y,z)
    
def rotate(xyrot=None,yzrot=None,zxrot=None,angle=None):
    creator.rotate(xyrot,yzrot,zxrot,angle)
    
def scale(k=None):
    creator.scale(k)
    
def matrix_product(matrix1=None,matrix2=None):
    creator.matrix_product(matrix1,matrix2)
    
def mirror_plane(plane1=None,plane2=None,plane3=None):
    creator.mirror_plane(plane1,plane2,plane3)
   
def mirror_line(line=None):
    creator.mirror_line(line)
    


############################################################################
##  Rates + Modes

def feedrate(f):
    creator.feedrate(f)

def feedrate_hv(fh, fv):
    creator.feedrate_hv(fh, fv)

def spindle(s, clockwise=True):
    creator.spindle(s, clockwise)

def coolant(mode=0):
    creator.coolant(mode)

def gearrange(gear=0):
    creator.gearrange(gear)

############################################################################
##  Moves

def rapid(x=None, y=None, z=None):
    creator.rapid(x, y, z)

def feed(x=None, y=None, z=None):
    creator.feed(x, y, z)

def arc_cw(x=None, y=None, z=None, i=None, j=None, k=None, r=None):
    creator.arc_cw(x, y, z, i, j, k, r)

def arc_ccw(x=None, y=None, z=None, i=None, j=None, k=None, r=None):
    creator.arc_ccw(x, y, z, i, j, k, r)

def dwell(t):
    creator.dwell(t)

def rapid_home(x=None, y=None, z=None):
    creator.rapid_home(x, y, z)

def rapid_unhome():
    creator.rapid_unhome()
    
def set_machine_coordinates():
    creator.set_machine_coordinates()

############################################################################
##  Cutter radius compensation

def use_CRC():
    return creator.use_CRC()

def CRC_nominal_path():
    return creator.CRC_nominal_path()

def start_CRC(left = True, radius = 0.0):
    creator.start_CRC(left, radius)

def end_CRC():
    creator.end_CRC()

############################################################################
##  Cycles

def pattern():
    creator.pattern()

def pocket():
    creator.pocket()

def profile():
    creator.profile()

def drill(x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None, rapid_to_clearance=None):
    creator.drill(x, y, dwell, depthparams, retract_mode, spindle_mode, internal_coolant_on, rapid_to_clearance)


def tap(x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
    creator.tap(x, y, z, zretract, depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out, tap_mode, direction)

def bore(x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
    creator.bore(x, y, z, zretract, depth, standoff, dwell_Bottom, feed_in, feed_out, stoppos, shift_back, shift_right, backbore, stop)

def end_canned_cycle():
    creator.end_canned_cycle()

def peck(count, first, last=None, step=0.0):
    pecks = []
    peck = first
    if (last == None) : last = first
    for i in range(0,count):
        pecks.append(peck)
        if (peck - step > last) : peck -= step
    return pecks

############################################################################
##  Misc

def comment(text):
    creator.comment(text)

def insert(text):
    creator.insert(text)

def block_delete(on=False):
    creator.block_delete(on)   

def variable(id):
    creator.variable(id)

def variable_set(id, value):
    creator.variable_set(id, value)

def probe_single_point(point_along_edge_x=None, point_along_edge_y=None, depth=None, retracted_point_x=None, retracted_point_y=None, destination_point_x=None, destination_point_y=None, intersection_variable_x=None, intersection_variable_y=None, probe_offset_x_component=None, probe_offset_y_component=None ):
    creator.probe_single_point(point_along_edge_x, point_along_edge_y, depth, retracted_point_x, retracted_point_y, destination_point_x, destination_point_y, intersection_variable_x, intersection_variable_y, probe_offset_x_component, probe_offset_y_component )

def probe_downward_point(x=None, y=None, depth=None, intersection_variable_z=None):
    creator.probe_downward_point(x, y, depth, intersection_variable_z)

def report_probe_results(x1=None, y1=None, z1=None, x2=None, y2=None, z2=None, x3=None, y3=None, z3=None, x4=None, y4=None, z4=None, x5=None, y5=None, z5=None, x6=None, y6=None, z6=None, xml_file_name=None ):
    creator.report_probe_results(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, x5, y5, z5, x6, y6, z6, xml_file_name)

def open_log_file(xml_file_name=None ):
    creator.open_log_file(xml_file_name)

def log_coordinate(x=None, y=None, z=None):
    creator.log_coordinate(x, y, z)

def log_message(message=None):
    creator.log_message(message)

def close_log_file():
    creator.close_log_file()

def rapid_to_midpoint(x1=None, y1=None, z1=None, x2=None, y2=None, z2=None):
    creator.rapid_to_midpoint(x1, y1, z1, x2, y2, z2)

def rapid_to_intersection(x1, y1, x2, y2, x3, y3, x4, y4, intersection_x, intersection_y, ua_numerator, ua_denominator, ua, ub_numerator, ub):
    creator.rapid_to_intersection(x1, y1, x2, y2, x3, y3, x4, y4, intersection_x, intersection_y, ua_numerator, ua_denominator, ua, ub_numerator, ub)

def rapid_to_rotated_coordinate(x1, y1, x2, y2, ref_x, ref_y, x_current, y_current, x_final, y_final):
    creator.rapid_to_rotated_coordinate(x1, y1, x2, y2, ref_x, ref_y, x_current, y_current, x_final, y_final)

def set_path_control_mode(mode, motion_blending_tolerance, naive_cam_tolerance ):
    creator.set_path_control_mode(mode, motion_blending_tolerance, naive_cam_tolerance )


    

