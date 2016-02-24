################################################################################
# iso.py
#
# Simple ISO NC code creator
#
# Hirutso Enni, 2009-01-13

import nc
import math
from format import Format
from format import *

################################################################################
class Creator(nc.Creator):

    def __init__(self):
        nc.Creator.__init__(self)

        # internal variables
        self.a = 0
        self.b = 0
        self.c = 0
        self.f = Address('F', fmt = Format(number_of_decimal_places = 2))
        self.fh = None
        self.fv = None
        self.fhv = False
        self.g_plane = Address('G', fmt = Format(number_of_decimal_places = 0))
        self.g_list = []
        self.i = 0
        self.j = 0
        self.k = 0
        self.m = []
        self.r = 0
        self.s = AddressPlusMinus('S', fmt = Format(number_of_decimal_places = 2), modal = False)
        self.t = None
        self.x = None
        self.y = None
        self.z = None
        self.g0123_modal = False
        self.drill_modal = False
        self.prev_f = ''
        self.prev_g0123 = ''
        self.prev_drill = ''
        self.prev_retract = ''
        self.prev_z = ''
        self.useCrc = False
        self.useCrcCenterline = False
        self.gCRC = ''
        self.fmt = Format()
        self.absolute_flag = True
        self.ffmt = Format(number_of_decimal_places = 2)
        self.sfmt = Format(number_of_decimal_places = 1)
        self.in_quadrant_splitting = False
        self.in_canned_cycle = False
        self.first_drill_pos = True
        self.shift_x = 0.0
        self.shift_y = 0.0
        self.shift_z = 0.0        
        self.start_of_line = False
        self.internal_coolant_on = None
        self.g98_not_g99 = None # True for G98 ouput, False for G99 output
        self.current_fixture = None
        self.fixture_wanted = '54'
        self.move_done_since_tool_change = False
        self.tool_defn_params = {}
        self.program_id = None
        self.current_sub_id = None
        self.subroutine_files = []
        self.program_name = None
        self.temp_file_to_append_on_close = None
        self.fixture_order = ['54', '55', '56', '57', '58', '59']
        for i in range(1, 50):
            self.fixture_order.append('54.' + str(i))
        self.output_disabled = False
        self.z_for_g43 = None

        # optional settings
        self.arc_centre_absolute = False
        self.arc_centre_positive = False
        self.drillExpanded = False
        self.dwell_allowed_in_G83 = False
        self.can_do_helical_arcs = True
        self.z_for_g53 = None # set this to a value to output G53 Zvalue in tool change and at program end
        self.output_h_and_d_at_tool_change = False
        self.output_block_numbers = True
        self.start_block_number = 10
        self.block_number_increment = 10
        self.block_number_restart_after = None
        self.output_tool_definitions = True
        self.output_g43_on_tool_change_line = False
        self.output_internal_coolant_commands = False
        self.output_g98_and_g99 = True
        self.output_g43_z_before_drilling_if_g98 = False
        self.output_cutviewer_comments = False
        self.output_fixtures = False
        self.use_this_program_id = None
        self.subroutines_in_own_files = False
        self.pattern_done_with_subroutine = False
        self.output_comment_before_tool_change = True
        self.output_arcs_as_lines = False
        self.m_codes_on_their_own_line = False
        
    ############################################################################
    ##  Codes

    def SPACE_STR(self): return ''
    def SPACE(self):
        if self.start_of_line == True:
            self.start_of_line = False            
            return ''
        else:
            return self.SPACE_STR()
            
    def FORMAT_FEEDRATE(self): return('%.2f') 
    def FORMAT_ANG(self): return('%.1f')
    def FORMAT_TIME(self): return self.fmt

    def BLOCK(self): return('N%i')
    def COMMENT(self,comment): return( ('(%s)' % comment ) )
    def VARIABLE(self): return( '#%i')
    def VARIABLE_SET(self): return( '=%.3f')

    def PROGRAM(self): return( 'O%i')
    def PROGRAM_END(self): return( 'M02')

    def SUBPROG_CALL(self): return( 'M98' + self.SPACE() + 'P%i')
    def SUBPROG_END(self): return( 'M99')

    def STOP_OPTIONAL(self): return('M01')
    def STOP(self): return('M00')

    def IMPERIAL(self): return('G20')
    def METRIC(self): return('G21')
    def ABSOLUTE(self): return('G90')
    def INCREMENTAL(self): return('G91')
    def SET_TEMPORARY_COORDINATE_SYSTEM(self): return('G92')
    def REMOVE_TEMPORARY_COORDINATE_SYSTEM(self): return('G92.1')
    def POLAR_ON(self): return('G16')
    def POLAR_OFF(self): return('G15')
    def PLANE_XY(self): return('17')
    def PLANE_XZ(self): return('18')
    def PLANE_YZ(self): return('19')

    def TOOL(self): return('T%i' + self.SPACE() + 'M06')
    def TOOL_DEFINITION(self): return('G10' + self.SPACE() + 'L1')

    def WORKPLANE(self): return('G%i')
    def WORKPLANE_BASE(self): return(53)

    def SPINDLE_CW(self): return('M03')
    def SPINDLE_CCW(self): return('M04')
    def COOLANT_OFF(self): return('M09')
    def COOLANT_MIST(self): return('M07')
    def COOLANT_FLOOD(self): return('M08')
    def GEAR_OFF(self): return('?')
    def GEAR(self): return('M%i')
    def GEAR_BASE(self): return(37)

    def RAPID(self): return('G00')
    def FEED(self): return('G01')
    def ARC_CW(self): return('G02')
    def ARC_CCW(self): return('G03')
    def DWELL(self, dwell): return('G04' + self.SPACE() + self.TIME() + (self.FORMAT_TIME().string(dwell)))
    def DRILL(self): return('G81')
    def DRILL_WITH_DWELL(self, dwell): return('G82' + self.SPACE() + self.TIME() + (self.FORMAT_TIME().string(dwell)))
    def PECK_DRILL(self): return('G83')
    def PECK_DEPTH(self, depth): return('Q' + (self.fmt.string(depth)))
    def RETRACT(self, height): return('R' + (self.fmt.string(height)))
    def END_CANNED_CYCLE(self): return('G80')
    def TAP(self): return('G84')
    def TAP_DEPTH(self, depth): return('K' + (self.fmt.string(depth)))
    def INTERNAL_COOLANT_ON(self): return('M18')
    def INTERNAL_COOLANT_OFF(self): return('M9')

    def X(self): return('X')
    def Y(self): return('Y')
    def Z(self): return('Z')
    def A(self): return('A')
    def B(self): return('B')
    def C(self): return('C')
    def CENTRE_X(self): return('I')
    def CENTRE_Y(self): return('J')
    def CENTRE_Z(self): return('K')
    def RADIUS(self): return('R')
    def TIME(self): return('P')

    def PROBE_TOWARDS_WITH_SIGNAL(self): return('G38.2')
    def PROBE_TOWARDS_WITHOUT_SIGNAL(self): return('G38.3')
    def PROBE_AWAY_WITH_SIGNAL(self): return('G38.4')
    def PROBE_AWAY_WITHOUT_SIGNAL(self): return('G38.5')

    def MACHINE_COORDINATES(self): return('G53')

    def EXACT_PATH_MODE(self): return('G61')
    def EXACT_STOP_MODE(self): return('G61.1')

    def RETRACT_TO_CLEARANCE(self): return('G98')
    def RETRACT_TO_STANDOFF(self): return('G99')
        
    ############################################################################
    ##  Internals
    def write(self, s):
        if self.output_disabled == False:
            nc.Creator.write(self, s)
        if '\n' in s:
            self.start_of_line = s[-1] == '\n'

    def write_feedrate(self):
        self.write(self.SPACE())
        self.f.write(self)

    def write_preps(self):
        i = 0
        if self.g_plane.str:
            i += 1
            self.write(self.SPACE())
        self.g_plane.write(self)
        for g in self.g_list:
            if i > 0:
                self.write('\n' if self.m_codes_on_their_own_line else self.SPACE())
            self.write(g)
            i += 1
        self.g_list = []

    def write_misc(self):
        if (len(self.m)):
            self.write('\n' if self.m_codes_on_their_own_line else self.SPACE())
            self.write(self.m.pop())

    def write_spindle(self):
        if self.s.str:
            self.write('\n' if self.m_codes_on_their_own_line else self.SPACE())
            self.s.write(self)

    def output_fixture(self):
        if self.current_fixture != self.fixture_wanted:
            self.current_fixture = self.fixture_wanted            
            self.g_list.append('G' + str(self.current_fixture))

    def increment_fixture(self):
        for i in range(0, len(self.fixture_order) - 1):
            if self.fixture_order[i] == self.fixture_wanted:
                self.fixture_wanted = self.fixture_order[i+1]
                return
        raise 'too many fixtures wanted!'
    
    def get_fixture(self):
        return self.fixture_wanted
    
    def set_fixture(self, fixture):
        self.fixture_wanted = fixture        
          
    ############################################################################
    ##  Programs

    def program_begin(self, id, name=''):
        if self.use_this_program_id:
            id = self.use_this_program_id
        if self.PROGRAM() != None:
            self.write((self.PROGRAM() % id) + self.SPACE() + (self.COMMENT(name)))
            self.write('\n')
        self.program_id = id
        self.program_name = name

    def add_stock(self, type_name, params):
        if self.output_cutviewer_comments:
            self.write("(STOCK/" + type_name)
            for param in params:
                self.write(",")
                self.write(str(param))
            self.write(")\n")

    def program_stop(self, optional=False):
        if (optional) : 
            self.write(self.SPACE() + self.STOP_OPTIONAL() + '\n')
            self.prev_g0123 = ''
        else : 
            self.write(self.STOP() + '\n')
            self.prev_g0123 = ''
            
    def number_file(self, filename):
        import tempfile
        temp_filename = tempfile.gettempdir()+'/renumbering.txt'
        
        # make a copy of file
        f_in = open(filename, 'r')
        f_out = open(temp_filename, 'w')
        while (True):
            line = f_in.readline()
            if (len(line) == 0) : break
            f_out.write(line)
        f_in.close()
        f_out.close()
                
        # read copy
        f_in = open(temp_filename, 'r')
        f_out = open(filename, 'w')
        n = self.start_block_number
        while (True):
            line = f_in.readline()
           
            if (len(line) == 0) : break
            f_out.write(self.BLOCK() % n + self.SPACE_STR() + line)
            n += self.block_number_increment
            if self.block_number_restart_after != None:
                if n >= self.block_number_restart_after:
                    n = self.start_block_number
        f_in.close()
        f_out.close()

    def program_end(self):
        if self.z_for_g53 != None:
            self.write(self.SPACE() + self.MACHINE_COORDINATES() + self.SPACE() + 'Z' + self.fmt.string(self.z_for_g53) + '\n')
        self.write(self.SPACE() + self.PROGRAM_END() + '\n')
        
        if self.temp_file_to_append_on_close != None:
            f_in = open(self.temp_file_to_append_on_close, 'r')
            while (True):
                line = f_in.readline()
                if (len(line) == 0) : break
                self.write(line)
            f_in.close()
            
        self.file_close()
            
        if self.output_block_numbers:
            # number every line of the file afterwards
            self.number_file(self.filename)
            
            for f in self.subroutine_files:
                self.number_file(f)

    def flush_nc(self):
        if len(self.g_list) == 0 and len(self.m) == 0: return
        self.write_preps()
        self.write_misc()
        self.write('\n')

    ############################################################################
    ##  Subprograms
    
    def make_subroutine_name(self, id):
        s = self.filename
        for i in reversed(range(0, len(s))):
            if s[i] == '.':
                return s[0:i] + 'sub' + str(id) + s[i:]
        
        # '.' not found
        return s + 'sub' + str(id)
    
    def sub_begin(self, id, name=None):
        if id == None:
            if self.current_sub_id == None:
                self.current_sub_id = self.program_id
            self.current_sub_id += 1
            id = self.current_sub_id
            
        if name == None:
            name = self.program_name + ' subroutine ' + str(id)
            
        self.save_file = self.file
        if self.subroutines_in_own_files:
            new_name = self.make_subroutine_name(id)
            self.file = open(new_name, 'w')
            self.subroutine_files.append(new_name)
        else:
            ## use temporary file
            import tempfile
            temp_filename = tempfile.gettempdir()+'/subroutines.txt'
            if self.temp_file_to_append_on_close == None:
                self.temp_file_to_append_on_close = temp_filename
                self.file = open(temp_filename, 'w')
            else:
                self.file = open(temp_filename, 'a')            
        
        if self.PROGRAM() != None:
            self.write((self.PROGRAM() % id) + self.SPACE() + (self.COMMENT(name)))
            self.write('\n')

    def sub_call(self, id):
        if id == None:
            id = self.current_sub_id
        self.write(self.SPACE() + (self.SUBPROG_CALL() % id) + '\n')

    def sub_end(self):
        self.write(self.SPACE() + self.SUBPROG_END() + '\n')

        self.file.close()
        self.file = self.save_file
        
    def disable_output(self):
        self.output_disabled = True
        
    def enable_output(self):
        self.output_disabled = False
        
    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.g_list.append(self.IMPERIAL())
        self.fmt.number_of_decimal_places = 4

    def metric(self):
        self.g_list.append(self.METRIC())
        self.fmt.number_of_decimal_places = 3

    def absolute(self):
        self.g_list.append(self.ABSOLUTE())
        self.absolute_flag = True

    def incremental(self):
        self.g_list.append(self.INCREMENTAL())
        self.absolute_flag = False

    def polar(self, on=True):
        if (on) : self.g_list.append(self.POLAR_ON())
        else : self.g_list.append(self.POLAR_OFF())

    def set_plane(self, plane):
        if (plane == 0) : self.g_plane.set(self.PLANE_XY())
        elif (plane == 1) : self.g_plane.set(self.PLANE_XZ())
        elif (plane == 2) : self.g_plane.set(self.PLANE_YZ())

    def set_temporary_origin(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.write(self.SPACE() + (self.SET_TEMPORARY_COORDINATE_SYSTEM()))
        if (x != None): self.write( self.SPACE() + 'X ' + (self.fmt.string(x + self.shift_x)) )
        if (y != None): self.write( self.SPACE() + 'Y ' + (self.fmt.string(y + self.shift_y)) )
        if (z != None): self.write( self.SPACE() + 'Z ' + (self.fmt.string(z + self.shift_z)) )
        if (a != None): self.write( self.SPACE() + 'A ' + (self.fmt.string(a)) )
        if (b != None): self.write( self.SPACE() + 'B ' + (self.fmt.string(b)) )
        if (c != None): self.write( self.SPACE() + 'C ' + (self.fmt.string(c)) )
        self.write('\n')

    def remove_temporary_origin(self):
        self.write(self.SPACE() + (self.REMOVE_TEMPORARY_COORDINATE_SYSTEM()))
        self.write('\n')
    ############################################################################
    ##  new graphics origin- make a new coordinate system and snap it onto the geometry
    ##  the toolpath generated should be translated 
    def translate(self,x=None, y=None, z=None):
        self.shift_x = -x
        self.shift_y = -y
        self.shift_z = -z

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        if self.output_comment_before_tool_change:
            self.comment('tool change to ' + self.tool_defn_params[id]['name']);
            
        if self.output_cutviewer_comments:
            import cutviewer
            if id in self.tool_defn_params:
                cutviewer.tool_defn(self, id, self.tool_defn_params[id])
        if (self.t != None) and (self.z_for_g53 != None):
            self.write('G53 Z' + str(self.z_for_g53) + '\n')
        self.write(self.SPACE() + (self.TOOL() % id))
        if self.output_g43_on_tool_change_line == True:
            self.write(self.SPACE() + 'G43')
        self.write('\n')
        if self.output_h_and_d_at_tool_change == True:
            if self.output_g43_on_tool_change_line == False:
                self.write(self.SPACE() + 'G43')
            self.write(self.SPACE() + 'D' + str(id) + self.SPACE() + 'H' + str(id) + '\n')
        self.t = id
        self.move_done_since_tool_change = False

    def tool_defn(self, id, name='',params=None):
        self.tool_defn_params[id] = params
        if self.output_tool_definitions:
            self.write(self.SPACE() + self.TOOL_DEFINITION())
            self.write(self.SPACE() + ('P%i' % id) + ' ')

            if (params['diameter'] != None):
                self.write(self.SPACE() + ('R%.3f' % (float(params['diameter'])/2)))

            if (params['cutting edge height'] != None):
                self.write(self.SPACE() + 'Z%.3f' % float(params['cutting edge height']))

            self.write('\n')

    def offset_radius(self, id, radius=None):
        pass

    def offset_length(self, id, length=None):
        pass
    
    def current_tool(self):
        return self.t


    ############################################################################
    ##  Datums
    
    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    # This is the coordinate system we're using.  G54->G59, G59.1, G59.2, G59.3
    # These are selected by values from 1 to 9 inclusive.
    def workplane(self, id):
        if ((id >= 1) and (id <= 6)):
            self.g_list.append(self.WORKPLANE() % (id + self.WORKPLANE_BASE()))
        if ((id >= 7) and (id <= 9)):
            self.g_list.append(((self.WORKPLANE() % (6 + self.WORKPLANE_BASE())) + ('.%i' % (id - 6))))
        

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.f.set(f)
        self.fhv = False

    def feedrate_hv(self, fh, fv):
        self.fh = fh
        self.fv = fv
        self.fhv = True

    def calc_feedrate_hv(self, h, v):
        if math.fabs(v) > math.fabs(h * 2):
            # some horizontal, so it should be fine to use the horizontal feed rate
            self.f.set(self.fv)
        else:
            # not much, if any horizontal component, so use the vertical feed rate
            self.f.set(self.fh)

    def spindle(self, s, clockwise):
        if clockwise == True:
            self.s.set(s, self.SPINDLE_CW(), self.SPINDLE_CCW())
        else:
            self.s.set(s, self.SPINDLE_CCW(), self.SPINDLE_CW())

    def coolant(self, mode=0):
        if (mode <= 0) : self.m.append(self.COOLANT_OFF())
        elif (mode == 1) : self.m.append(self.COOLANT_MIST())
        elif (mode == 2) : self.m.append(self.COOLANT_FLOOD())

    def gearrange(self, gear=0):
        if (gear <= 0) : self.m.append(self.GEAR_OFF())
        elif (gear <= 4) : self.m.append(self.GEAR() % (gear + GEAR_BASE()))

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None ):
        if self.same_xyz(x, y, z, a, b, c): return
        self.on_move()

        if self.g0123_modal:
            if self.prev_g0123 != self.RAPID():
                self.write(self.SPACE() + self.RAPID())
                self.prev_g0123 = self.RAPID()
        else:
            self.write(self.SPACE() + self.RAPID())
        self.write_preps()
        if (x != None):
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.X() + (self.fmt.string(x + self.shift_x)))
            else:
                dx = x - self.x
                self.write(self.SPACE() + self.X() + (self.fmt.string(dx)))
            self.x = x
        if (y != None):
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.Y() + (self.fmt.string(y + self.shift_y)))
            else:
                dy = y - self.y
                self.write(self.SPACE() + self.Y() + (self.fmt.string(dy)))

            self.y = y
        if (z != None):
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.Z() + (self.fmt.string(z + self.shift_z)))
            else:
                dz = z - self.z
                self.write(self.SPACE() + self.Z() + (self.fmt.string(dz)))

            self.z = z

        if (a != None):
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.A() + (self.fmt.string(a)))
            else:
                da = a - self.a
                self.write(self.SPACE() + self.A() + (self.fmt.string(da)))
            self.a = a

        if (b != None):
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.B() + (self.fmt.string(b)))
            else:
                db = b - self.b
                self.write(self.SPACE() + self.B() + (self.fmt.string(db)))
            self.b = b

        if (c != None):
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.C() + (self.fmt.string(c)))
            else:
                dc = c - self.c
                self.write(self.SPACE() + self.C() + (self.fmt.string(dc)))
            self.c = c
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def feed(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if self.same_xyz(x, y, z, a, b, c): return
        self.on_move()
        if self.g0123_modal:
            if self.prev_g0123 != self.FEED():
                self.write(self.SPACE() + self.FEED())
                self.prev_g0123 = self.FEED()
        else:
            self.write(self.SPACE() + self.FEED())
        self.write_preps()
        dx = dy = dz = 0
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.X() + (self.fmt.string(x + self.shift_x)))
            else:
                self.write(self.SPACE() + self.X() + (self.fmt.string(dx)))
            self.x = x
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.Y() + (self.fmt.string(y + self.shift_y)))
            else:
                self.write(self.SPACE() + self.Y() + (self.fmt.string(dy)))

            self.y = y
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.Z() + (self.fmt.string(z + self.shift_z)))
            else:
                self.write(self.SPACE() + self.Z() + (self.fmt.string(dz)))

            self.z = z

        if (a != None):
            da = a - self.a
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.A() + (self.fmt.string(a)))
            else:
                self.write(self.SPACE() + self.A() + (self.fmt.string(da)))
            self.a = a

        if (b != None):
            db = b - self.b
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.B() + (self.fmt.string(b)))
            else:
                self.write(self.SPACE() + self.B() + (self.fmt.string(db)))
            self.b = b

        if (c != None):
            dc = c - self.c
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.C() + (self.fmt.string(c)))
            else:
                self.write(self.SPACE() + self.C() + (self.fmt.string(dc)))
            self.c = c

        if (self.fhv) : self.calc_feedrate_hv(math.sqrt(dx*dx+dy*dy), math.fabs(dz))
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')

    def same_xyz(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if (x != None):
            if (self.fmt.string(x + self.shift_x)) != (self.fmt.string(self.x)):
                return False
        if (y != None):
            if (self.fmt.string(y + self.shift_y)) != (self.fmt.string(self.y)):
                return False
        if (z != None):
            if (self.fmt.string(z + self.shift_z)) != (self.fmt.string(self.z)):
                return False
        if (a != None):
            if (self.fmt.string(a)) != (self.fmt.string(self.a)):
                return False
        if (b != None):
            if (self.fmt.string(b)) != (self.fmt.string(self.b)):
                return False
        if (c != None):
            if (self.fmt.string(c)) != (self.fmt.string(self.c)):
                return False
        return True

    
    def get_quadrant(self, dx, dy):
        if dx < 0:
            if dy < 0:
                return 2
            else:
                return 1

        else:
            if dy < 0:
                return 3
            else:
                return 0
    
    def quadrant_start(self, q, i, j, rad):
        while q > 3: q = q - 4
        if q == 0:
            return i + rad, j
        if q == 1:
            return i, j + rad
        if q == 2:
            return i - rad, j
        return i, j - rad

    def quadrant_end(self, q, i, j, rad):
        return self.quadrant_start(q + 1, i, j, rad)
    
    def get_arc_angle(self, sdx, sdy, edx, edy, cw):
        angle_s = math.atan2(sdy, sdx);        
        angle_e = math.atan2(edy, edx);
        if cw:
            if angle_s < angle_e: angle_s = angle_s + 2 * math.pi
        else:
            if angle_e < angle_s: angle_e = angle_e + 2 * math.pi
        return angle_e - angle_s

    def arc(self, cw, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        if self.same_xyz(x, y, z): return
        
        if self.output_arcs_as_lines or (self.can_do_helical_arcs == False and self.in_quadrant_splitting == False and (z != None) and (math.fabs(z - self.z) > 0.000001) and (self.fmt.string(z) != self.fmt.string(self.z))):
            # split the helical arc into little line feed moves
            
            if x == None: x = self.x
            if y == None: y = self.y
            sdx = self.x - i
            sdy = self.y - j
            edx = x - i
            edy = y - j
            radius = math.sqrt(sdx*sdx + sdy*sdy)
            arc_angle = self.get_arc_angle(sdx, sdy, edx, edy, cw)
            angle_start = math.atan2(sdy, sdx);
            tolerance = 0.02
            angle_step = 2.0 * math.atan( math.sqrt ( tolerance /(radius - tolerance) ))
            segments = int(math.fabs(arc_angle / angle_step) + 1)
            angle_step = arc_angle / segments
            angle = angle_start
            if z != None:
                z_step = float(z - self.z)/segments
                next_z = self.z
                
            for p in range(0, segments):
                angle = angle + angle_step
                next_x = i + radius * math.cos(angle)
                next_y = j + radius * math.sin(angle)
                if z == None:
                    next_z = None
                else:
                    next_z = next_z + z_step
                self.feed(next_x, next_y, next_z)
            return

        if self.arc_centre_positive == True and self.in_quadrant_splitting == False:
            # split in to quadrant arcs
            self.in_quadrant_splitting = True
            
            if x == None: x = self.x
            if y == None: y = self.y
            sdx = self.x - i
            sdy = self.y - j
            edx = x - i
            edy = y - j
            
            qs = self.get_quadrant(sdx, sdy)
            qe = self.get_quadrant(edx, edy)
            
            if qs == qe:
                arc_angle = math.fabs(self.get_arc_angle(sdx, sdy, edx, edy, cw))
                # arc_angle will be either less than pi/2 or greater than 3pi/2
                if arc_angle > 3.14:
                    if cw:
                        qs = qs + 4
                    else:
                        qe = qe + 4
                        
            if qs == qe:
                self.arc(cw, x, y, z, i, j, k, r)
            else:
                rad = math.sqrt(sdx * sdx + sdy * sdy)
                if cw:
                    if qs < qe: qs = qs + 4
                else:
                    if qe < qs: qe = qe + 4
                    
                q = qs
                while 1:
                    x1 = x
                    y1 = y
                    if q != qe:
                        if cw:
                            x1, y1 = self.quadrant_start(q, i, j, rad)
                        else:
                            x1, y1 = self.quadrant_end(q, i, j, rad)
                            
                    if (self.fmt.string(x1) != self.fmt.string(self.x)) or (self.fmt.string(y1) != self.fmt.string(self.y)):
                        if (math.fabs(x1 - self.x) > 0.01) or (math.fabs(y1 - self.y) > 0.01):
                            self.arc(cw, x1, y1, z, i, j, k, r)
                        else:
                            self.feed(x1, y1, z)
                    if q == qe:
                        break
                    if cw:
                        q = q - 1
                    else:
                        q = q + 1
                    
            self.in_quadrant_splitting = False
            return
            
        self.on_move()
        arc_g_code = ''
        if cw: arc_g_code = self.ARC_CW()
        else: arc_g_code = self.ARC_CCW()
        if self.g0123_modal:
            if self.prev_g0123 != arc_g_code:
                self.write(self.SPACE() + arc_g_code)
                self.prev_g0123 = arc_g_code
        else:
            self.write(self.SPACE() + arc_g_code)
        self.write_preps()
        if (x != None):
            dx = x - self.x
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.X() + (self.fmt.string(x + self.shift_x)))
            else:
                self.write(self.SPACE() + self.X() + (self.fmt.string(dx)))
        if (y != None):
            dy = y - self.y
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.Y() + (self.fmt.string(y + self.shift_y)))
            else:
                self.write(self.SPACE() + self.Y() + (self.fmt.string(dy)))
        if (z != None):
            dz = z - self.z
            if (self.absolute_flag ):
                self.write(self.SPACE() + self.Z() + (self.fmt.string(z + self.shift_z)))
            else:
                self.write(self.SPACE() + self.Z() + (self.fmt.string(dz)))
        if (i != None):
            if self.arc_centre_absolute == False:
                i = i - self.x
            s = self.fmt.string(i)
            if self.arc_centre_positive == True:
                if s[0] == '-':
                    s = s[1:]
            self.write(self.SPACE() + self.CENTRE_X() + s)
        if (j != None):
            if self.arc_centre_absolute == False:
                j = j - self.y
            s = self.fmt.string(j)
            if self.arc_centre_positive == True:
                if s[0] == '-':
                    s = s[1:]
            self.write(self.SPACE() + self.CENTRE_Y() + s)
        if (k != None):
            if self.arc_centre_absolute == False:
                k = k - self.z
            s = self.fmt.string(k)
            if self.arc_centre_positive == True:
                if s[0] == '-':
                    s = s[1:]
            self.write(self.SPACE() + self.CENTRE_Z() + s)
        if (r != None):
            s = self.fmt.string(r)
            if self.arc_centre_positive == True:
                if s[0] == '-':
                    s = s[1:]
            self.write(self.SPACE() + self.RADIUS() + s)
#       use horizontal feed rate
        if (self.fhv) : self.calc_feedrate_hv(1, 0)
        self.write_feedrate()
        self.write_spindle()
        self.write_misc()
        self.write('\n')
        if (x != None):
            self.x = x
        if (y != None):
            self.y = y
        if (z != None):
            self.z = z

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(True, x, y, z, i, j, k, r)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(False, x, y, z, i, j, k, r)

    def dwell(self, t):
        self.write_preps()
        self.write(self.SPACE() + self.DWELL(t))
        self.write_misc()
        self.write('\n')

    def on_move(self):
        if self.output_fixtures:
            self.output_fixture()
        self.move_done_since_tool_change = True

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        pass

    def rapid_unhome(self):
        pass

    def set_machine_coordinates(self):
        self.write(self.SPACE() + self.MACHINE_COORDINATES())
        self.prev_g0123 = ''

    ############################################################################
    ##  CRC
    
    def use_CRC(self):
        return self.useCrc

    def CRC_nominal_path(self):
        return self.useCrcCenterline

    def start_CRC(self, left = True, radius = 0.0):
        # set up prep code, to be output on next line
        if self.t == None:
            raise "No tool specified for start_CRC()"
        if left:
            self.write(self.SPACE() + 'G41')
        else:
            self.write(self.SPACE() + 'G42')
        self.write((self.SPACE() + 'D%i\n') % self.t)

    def end_CRC(self):
        self.write(self.SPACE() + 'G40\n')

    ############################################################################
    ##  Cycles

    def pattern(self):
        pass
    
    def pattern_uses_subroutine(self):
        return self.pattern_done_with_subroutine

    def pocket(self):
        pass

    def profile(self):
        pass
    
    def write_internal_coolant_commands(self, internal_coolant_on):
        if (internal_coolant_on != None) and (self.output_internal_coolant_commands == True):
            if internal_coolant_on == True:
                if self.internal_coolant_on != True:
                    self.write(self.SPACE())
                    self.write(self.INTERNAL_COOLANT_ON() + '\n')
                    self.internal_coolant_on = True
            else:
                if self.internal_coolant_on != False:
                    self.write(self.SPACE())
                    self.write(self.INTERNAL_COOLANT_OFF() + '\n')
                    self.internal_coolant_on = False

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
        if (depthparams.clearance_height == None):        
            self.first_drill_pos = False
            return
        
        self.write_internal_coolant_commands(internal_coolant_on)
            
        drillExpanded = self.drillExpanded
        if (depthparams.step_down != 0) and (dwell != 0):
            # pecking and dwell together
            if self.dwell_allowed_in_G83 != True:     
                drillExpanded = True
          
        if drillExpanded:
            # for machines which don't understand G81, G82 etc.
            peck_depth = depthparams.step_down
            if peck_depth == None:
                peck_depth = depthparams.final_depth
            current_z = depthparams.start_depth
            self.rapid(x, y)
            
            first = True
            last_cut = False
            
            while True:
                next_z = current_z - peck_depth
                if next_z < (depthparams.final_depth + 0.001):
                    next_z = depthparams.final_depth
                    last_cut = True
                if next_z >= current_z:
                    break;
                if first:
                    self.rapid(z = depthparams.start_depth + depthparams.rapid_safety_space)
                else:
                    self.rapid(z = current_z)
                self.feed(z = next_z)
                if dwell != 0 and last_cut:
                    self.dwell(dwell)        
                if last_cut:self.rapid(z = depthparams.clearance_height)
                else:
                    if rapid_to_clearance:
                        self.rapid(z = depthparams.clearance_height)
                    else:
                        self.rapid(z = depthparams.start_depth + depthparams.rapid_safety_space)
                current_z = next_z
                first = False
            
            self.first_drill_pos = False
            return

        if self.output_g98_and_g99 == True:
            if rapid_to_clearance == True:
                if self.output_g43_z_before_drilling_if_g98:
                    if self.fmt.string(depthparams.clearance_height) != self.z_for_g43:
                        self.z_for_g43 = self.fmt.string(depthparams.clearance_height)
                        self.write(self.SPACE() + 'G43' + self.SPACE() + 'Z' + self.z_for_g43 + '\n')

            if self.first_drill_pos ==True and rapid_to_clearance == True:
                self.rapid(x, y)            
                self.rapid(z = depthparams.clearance_height)            

        self.in_canned_cycle = True
        self.write_preps()
        
        if (depthparams.step_down != 0):        
            # G83 peck drilling
            if self.drill_modal:
                if self.PECK_DRILL() + self.PECK_DEPTH(depthparams.step_down) != self.prev_drill:
                    self.write(self.SPACE() + self.PECK_DRILL() + self.SPACE() + self.PECK_DEPTH(depthparams.step_down))  
                    self.prev_drill = self.PECK_DRILL() + self.PECK_DEPTH(depthparams.step_down)
            else:       
                self.write(self.PECK_DRILL() + self.PECK_DEPTH(depthparams.step_down)) 
            
            if (self.dwell != 0) and self.dwell_allowed_in_G83:
                self.write(self.SPACE() + self.TIME() + (self.FORMAT_TIME().string(dwell)))
                          
        else:        
            # We're either just drilling or drilling with dwell.        
            if (dwell == 0):        
                # We're just drilling. 
                if self.drill_modal:       
                    if  self.DRILL() != self.prev_drill:
                        self.write(self.SPACE() + self.DRILL())  
                        self.prev_drill = self.DRILL()
                else:
                    self.write(self.SPACE() + self.DRILL())
      
            else:        
                # We're drilling with dwell.

                if self.drill_modal:       
                    if  self.DRILL_WITH_DWELL(dwell) != self.prev_drill:
                        self.write(self.SPACE() + self.DRILL_WITH_DWELL(dwell))  
                        self.prev_drill = self.DRILL_WITH_DWELL(dwell)
                else:
                    self.write(self.SPACE() + self.DRILL_WITH_DWELL(dwell))

        if self.output_g98_and_g99 == True:
            if rapid_to_clearance == True:
                if self.g98_not_g99 != True:
                    self.write(self.SPACE() + self.RETRACT_TO_CLEARANCE())
                    self.g98_not_g99 = True
            else:
                if self.g98_not_g99 != False:
                    self.write(self.SPACE() + self.RETRACT_TO_STANDOFF())
                    self.g98_not_g99 = False                    
    
    # Set the retraction point to the 'standoff' distance above the starting z height.        
        retract_height = depthparams.start_depth + depthparams.rapid_safety_space        
        if (x != None):        
            self.write(self.SPACE() + self.X() + (self.fmt.string(x + self.shift_x)))        
            self.x = x 
       
        if (y != None):        
            self.write(self.SPACE() + self.Y() + (self.fmt.string(y + self.shift_y)))        
            self.y = y
                      
        if self.drill_modal:
            if depthparams.start_depth != self.prev_z:
                self.write(self.SPACE() + self.Z() + (self.fmt.string(depthparams.final_depth)))
                self.prev_z=depthparams.start_depth
        else:             
            self.write(self.SPACE() + self.Z() + (self.fmt.string(depthparams.final_depth)))    # This is the 'z' value for the bottom of the hole.
            self.z = (depthparams.start_depth + depthparams.rapid_safety_space)            # We want to remember where z is at the end (at the top of the hole)

        if self.drill_modal:
            if self.prev_retract  != self.RETRACT(retract_height) :
                self.write(self.SPACE() + self.RETRACT(retract_height))               
                self.prev_retract = self.RETRACT(retract_height)
        else:              
            self.write(self.SPACE() + self.RETRACT(retract_height))
           
        if (self.fv) :
            self.f.set(self.fv) 

        self.write_feedrate()
        self.write_spindle()            
        self.write_misc()    
        self.write('\n')
        self.first_drill_pos = False

    def end_canned_cycle(self):
        if self.in_canned_cycle == False:
            return
        self.write(self.SPACE() + self.END_CANNED_CYCLE() + '\n')
        self.write_internal_coolant_commands(0)
        self.prev_drill = ''
        self.prev_g0123 = ''
        self.prev_z = ''   
        self.prev_f = '' 
        self.prev_retract = ''
        self.in_canned_cycle = False
        self.first_drill_pos = True
          
    ############################################################################
    ##  Misc

    def comment(self, text):
        self.write((self.COMMENT(text) + '\n'))

    def insert(self, text):
        pass

    def block_delete(self, on=False):        
        pass

    def variable(self, id):
        return (self.VARIABLE() % id)

    def variable_set(self, id, value):
        self.write(self.SPACE() + (self.VARIABLE() % id) + self.SPACE() + (self.VARIABLE_SET() % value) + '\n')

    # This routine uses the G92 coordinate system offsets to establish a temporary coordinate
    # system at the machine's current position.  It can then use absolute coordinates relative
    # to this position which makes coding easy.  It then moves to the 'point along edge' which
    # should be above the workpiece but still on one edge.  It then backs off from the edge
    # to the 'retracted point'.  It then plunges down by the depth value specified.  It then
    # probes back towards the 'destination point'.  The probed X,Y location are stored
    # into the 'intersection variable' variables.  Finally the machine moves back to the
    # original location.  This is important so that the results of multiple calls to this
    # routine may be compared meaningfully.
    def probe_single_point(self, point_along_edge_x=None, point_along_edge_y=None, depth=None, retracted_point_x=None, retracted_point_y=None, destination_point_x=None, destination_point_y=None, intersection_variable_x=None, intersection_variable_y=None, probe_offset_x_component=None, probe_offset_y_component=None ):
        self.write(self.SPACE() + (self.SET_TEMPORARY_COORDINATE_SYSTEM() + (' X 0 Y 0 Z 0') + ('\t(Temporarily make this the origin)\n')))

        if (self.fhv) : self.calc_feedrate_hv(1, 0)
        self.write_feedrate()
        self.write('\t(Set the feed rate for probing)\n')

        self.rapid(point_along_edge_x,point_along_edge_y)
        self.rapid(retracted_point_x,retracted_point_y)
        self.feed(z=depth)

        self.write((self.PROBE_TOWARDS_WITH_SIGNAL() + (' X ' + (self.fmt.string(destination_point_x)) + ' Y ' + (self.fmt.string(destination_point_y)) ) + ('\t(Probe towards our destination point)\n')))

        self.comment('Back off the workpiece and re-probe more slowly')
        self.write(self.SPACE() + ('#' + intersection_variable_x + '= [#5061 - [ 0.5 * ' + probe_offset_x_component + ']]\n'))
        self.write(self.SPACE() + ('#' + intersection_variable_y + '= [#5062 - [ 0.5 * ' + probe_offset_y_component + ']]\n'))
        self.write(self.RAPID())
        self.write(self.SPACE() + ' X #' + intersection_variable_x + ' Y #' + intersection_variable_y + '\n')

        self.write(self.SPACE() + self.FEEDRATE() + self.ffmt.string(self.fh / 2.0) + '\n')

        self.write((self.SPACE() + self.PROBE_TOWARDS_WITH_SIGNAL() + (' X ' + (self.fmt.string(destination_point_x)) + ' Y ' + (self.fmt.string(destination_point_y)) ) + ('\t(Probe towards our destination point)\n')))

        self.comment('Store the probed location somewhere we can get it again later')
        self.write(('#' + intersection_variable_x + '=' + probe_offset_x_component + ' (Portion of probe radius that contributes to the X coordinate)\n'))
        self.write(('#' + intersection_variable_x + '=[#' + intersection_variable_x + ' + #5061]\n'))
        self.write(('#' + intersection_variable_y + '=' + probe_offset_y_component + ' (Portion of probe radius that contributes to the Y coordinate)\n'))
        self.write(('#' + intersection_variable_y + '=[#' + intersection_variable_y + ' + #5062]\n'))

        self.comment('Now move back to the original location')
        self.rapid(retracted_point_x,retracted_point_y)
        self.rapid(z=0)
        self.rapid(point_along_edge_x,point_along_edge_y)
        self.rapid(x=0, y=0)

        self.write((self.REMOVE_TEMPORARY_COORDINATE_SYSTEM() + ('\t(Restore the previous coordinate system)\n')))

    def probe_downward_point(self, x=None, y=None, depth=None, intersection_variable_z=None):
        self.write((self.SET_TEMPORARY_COORDINATE_SYSTEM() + (' X 0 Y 0 Z 0') + ('\t(Temporarily make this the origin)\n')))
        if (self.fhv) : self.calc_feedrate_hv(1, 0)
        self.write(self.FEEDRATE() + ' [' + self.ffmt.string(self.fh) + ' / 5.0 ]')
        self.write('\t(Set the feed rate for probing)\n')

        if x != None and y != None:
       	   self.write(self.RAPID())
       	   self.write(' X ' + x + ' Y ' + y + '\n')

        self.write((self.PROBE_TOWARDS_WITH_SIGNAL() + ' Z ' + (self.fmt.string(depth)) + ('\t(Probe towards our destination point)\n')))

        self.comment('Store the probed location somewhere we can get it again later')
        self.write(('#' + intersection_variable_z + '= #5063\n'))

        self.comment('Now move back to the original location')
        self.rapid(z=0)
        self.rapid(x=0, y=0)

        self.write((self.REMOVE_TEMPORARY_COORDINATE_SYSTEM() + ('\t(Restore the previous coordinate system)\n')))


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

    # Rapid movement to the midpoint between the two points specified.
    # NOTE: The points are specified either as strings representing numbers or as strings
    # representing variable names.  This allows the HeeksCNC module to determine which
    # variable names are used in these various routines.
    def rapid_to_midpoint(self, x1=None, y1=None, z1=None, x2=None, y2=None, z2=None):
        self.write(self.RAPID())
        if ((x1 != None) and (x2 != None)):
            self.write((' X ' + '[[[' + x1 + ' - ' + x2 + '] / 2.0] + ' + x2 + ']'))

        if ((y1 != None) and (y2 != None)):
            self.write((' Y ' + '[[[' + y1 + ' - ' + y2 + '] / 2.0] + ' + y2 + ']'))

        if ((z1 != None) and (z2 != None)):
            self.write((' Z ' + '[[[' + z1 + ' - ' + z2 + '] / 2.0] + ' + z2 + ']'))

        self.write('\n')

    # Rapid movement to the intersection of two lines (in the XY plane only). This routine
    # is based on information found in http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
    # written by Paul Bourke.  The ua_numerator, ua_denominator, ua and ub parameters
    # represent variable names (with the preceding '#' included in them) for use as temporary
    # variables.  They're specified here simply so that HeeksCNC can manage which variables
    # are used in which GCode calculations.
    #
    # As per the notes on the web page, the ua_denominator and ub_denominator formulae are
    # the same so we don't repeat this.  If the two lines are coincident or parallel then
    # no movement occurs.
    #
    # NOTE: The points are specified either as strings representing numbers or as strings
    # representing variable names.  This allows the HeeksCNC module to determine which
    # variable names are used in these various routines.
    def rapid_to_intersection(self, x1, y1, x2, y2, x3, y3, x4, y4, intersection_x, intersection_y, ua_numerator, ua_denominator, ua, ub_numerator, ub):
        self.comment('Find the intersection of the two lines made up by the four probed points')
        self.write(ua_numerator + '=[[[' + x4 + ' - ' + x3 + '] * [' + y1 + ' - ' + y3 + ']] - [[' + y4 + ' - ' + y3 + '] * [' + x1 + ' - ' + x3 + ']]]\n')
        self.write(ua_denominator + '=[[[' + y4 + ' - ' + y3 + '] * [' + x2 + ' - ' + x1 + ']] - [[' + x4 + ' - ' + x3 + '] * [' + y2 + ' - ' + y1 + ']]]\n')
        self.write(ub_numerator + '=[[[' + x2 + ' - ' + x1 + '] * [' + y1 + ' - ' + y3 + ']] - [[' + y2 + ' - ' + y1 + '] * [' + x1 + ' - ' + x3 + ']]]\n')

        self.comment('If they are not parallel')
        self.write('O900 IF [' + ua_denominator + ' NE 0]\n')
        self.comment('And if they are not coincident')
        self.write('O901    IF [' + ua_numerator + ' NE 0 ]\n')

        self.write('       ' + ua + '=[' + ua_numerator + ' / ' + ua_denominator + ']\n')
        self.write('       ' + ub + '=[' + ub_numerator + ' / ' + ua_denominator + ']\n') # NOTE: ub denominator is the same as ua denominator
        self.write('       ' + intersection_x + '=[' + x1 + ' + [[' + ua + ' * [' + x2 + ' - ' + x1 + ']]]]\n')
        self.write('       ' + intersection_y + '=[' + y1 + ' + [[' + ua + ' * [' + y2 + ' - ' + y1 + ']]]]\n')
        self.write('       ' + self.RAPID())
        self.write(' X ' + intersection_x + ' Y ' + intersection_y + '\n')

        self.write('O901    ENDIF\n')
        self.write('O900 ENDIF\n')

    # We need to calculate the rotation angle based on the line formed by the
    # x1,y1 and x2,y2 coordinate pair.  With that angle, we need to move
    # x_offset and y_offset distance from the current (0,0,0) position.
    #
    # The x1,y1,x2 and y2 parameters are all variable names that contain the actual
    # values.
    # The x_offset and y_offset are both numeric (floating point) values
    def rapid_to_rotated_coordinate(self, x1, y1, x2, y2, ref_x, ref_y, x_current, y_current, x_final, y_final):
        self.comment('Rapid to rotated coordinate')
        self.write( '#1 = [atan[' + y2 + ' - ' + y1 + ']/[' + x2 +' - ' + x1 + ']] (nominal_angle)\n')
        self.write( '#2 = [atan[' + ref_y + ']/[' + ref_x + ']] (reference angle)\n')
        self.write( '#3 = [#1 - #2] (angle)\n' )
        self.write( '#4 = [[[' + (self.fmt.string(0)) + ' - ' + (self.fmt.string(x_current)) + '] * COS[ #3 ]] - [[' + (self.fmt.string(0)) + ' - ' + (self.fmt.string(y_current)) + '] * SIN[ #3 ]]]\n' )
        self.write( '#5 = [[[' + (self.fmt.string(0)) + ' - ' + (self.fmt.string(x_current)) + '] * SIN[ #3 ]] + [[' + (self.fmt.string(0)) + ' - ' + (self.fmt.string(y_current)) + '] * COS[ #3 ]]]\n' )

        self.write( '#6 = [[' + (self.fmt.string(x_final)) + ' * COS[ #3 ]] - [' + (self.fmt.string(y_final)) + ' * SIN[ #3 ]]]\n' )
        self.write( '#7 = [[' + (self.fmt.string(y_final)) + ' * SIN[ #3 ]] + [' + (self.fmt.string(y_final)) + ' * COS[ #3 ]]]\n' )

        self.write( self.RAPID() + ' X [ #4 + #6 ] Y [ #5 + #7 ]\n' )

    def BEST_POSSIBLE_SPEED(self, motion_blending_tolerance, naive_cam_tolerance): 
        statement = 'G64'

        if (motion_blending_tolerance > 0):
            statement += ' P ' + str(motion_blending_tolerance)

        if (naive_cam_tolerance > 0):
            statement += ' Q ' + str(naive_cam_tolerance)

        return(statement)
            
    def set_path_control_mode(self, mode, motion_blending_tolerance, naive_cam_tolerance ):
        if (mode == 0):
            self.write( self.EXACT_PATH_MODE() + '\n' )
        if (mode == 1):
            self.write( self.EXACT_STOP_MODE() + '\n' )
        if (mode == 2):
            self.write( self.BEST_POSSIBLE_SPEED( motion_blending_tolerance, naive_cam_tolerance ) + '\n' )
        

################################################################################

nc.creator = Creator()
