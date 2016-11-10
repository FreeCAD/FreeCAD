import nc

units = 1.0

class Redirector(nc.Creator):

    def __init__(self, original):
        nc.Creator.__init__(self)

        self.original = original
        self.x = None
        self.y = None
        self.z = None
        if original.x != None: self.x = original.x * units
        if original.y != None: self.y = original.y * units
        if original.z != None: self.z = original.z * units
        self.imperial = False
        
    def cut_path(self):
        pass

    ############################################################################
    ##  Programs
    def write(self, s):
        self.original.write(s)
        
    def output_fixture(self):
        self.original.output_fixture()
        
    def increment_fixture(self):
        self.original.increment_fixture()

    def get_fixture(self):
        return self.original.get_fixture()
    
    def set_fixture(self, fixture):
        self.original.set_fixture(fixture)
        
    def program_begin(self, id, name=''):
        self.cut_path()
        self.original.program_begin(id, name)

    def program_stop(self, optional=False):
        self.cut_path()
        self.original.program_stop(optional)

    def program_end(self):
        self.cut_path()
        self.original.program_end()

    def flush_nc(self):
        self.cut_path()
        self.original.flush_nc()

    ############################################################################
    ##  Subprograms
    
    def sub_begin(self, id, name=None):
        self.cut_path()
        self.original.sub_begin(id, name)

    def sub_call(self, id):
        self.cut_path()
        self.original.sub_call(id)

    def sub_end(self):
        self.cut_path()
        self.original.sub_end()
        
    def disable_output(self):
        self.original.disable_output()
        
    def enable_output(self):
        self.original.enable_output()

    ############################################################################
    ##  Settings
    
    def imperial(self):
        self.cut_path()
        self.imperial = True
        self.original.imperial()

    def metric(self):
        self.cut_path()
        self.original.metric()

    def absolute(self):
        self.cut_path()
        self.original.absolute()

    def incremental(self):
        self.cut_path()
        self.original.incremental()

    def polar(self, on=True):
        self.cut_path()
        self.original.polar(on)

    def set_plane(self, plane):
        self.cut_path()
        self.original.set_plane(plane)

    def set_temporary_origin(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.set_temporary_origin(x,y,z,a,b,c)

    def remove_temporary_origin(self):
        self.cut_path()
        self.original.remove_temporary_origin()

    ############################################################################
    ##  Tools

    def tool_change(self, id):
        self.cut_path()
        self.original.tool_change(id)

    def tool_defn(self, id, name='', params=None):
        self.cut_path()
        self.original.tool_defn(id, name, params)

    def offset_radius(self, id, radius=None):
        self.cut_path()
        self.original.offset_radius(id, radius)

    def offset_length(self, id, length=None):
        self.cut_path()
        self.original.offset_length(id, length)

    ############################################################################
    ##  Datums

    def datum_shift(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.datum_shift(x, y, z, a, b, c)

    def datum_set(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.datum_set(x, y, z, a, b, c)

    def workplane(self, id):
        self.cut_path()
        self.original.workplane(id)

    ############################################################################
    ##  Rates + Modes

    def feedrate(self, f):
        self.cut_path()
        self.original.feedrate(f)

    def feedrate_hv(self, fh, fv):
        self.cut_path()
        self.original.feedrate_hv(fh, fv)

    def spindle(self, s, clockwise=True):
        self.cut_path()
        self.original.spindle(s, clockwise)

    def coolant(self, mode=0):
        self.cut_path()
        self.original.coolant(mode)

    def gearrange(self, gear=0):
        self.cut_path()
        self.original.gearrange(gear)

    ############################################################################
    ##  Moves

    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.rapid(x, y, z, a, b, c)
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        
    def cut_path(self):
        pass
    
    def z2(self, z):
        return z
    
    def feed(self, x=None, y=None, z=None, a = None, b = None, c = None):
        px = self.x
        py = self.y
        pz = self.z
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units
        if self.x == None or self.y == None or self.z == None:
            self.cut_path()
            self.original.feed(x, y, z)
            return
        if px == self.x and py == self.y:
            # z move only
            self.cut_path()
            self.original.feed(self.x/units, self.y/units, self.z2(self.z)/units)
            return
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        if self.x == None or self.y == None or self.z == None:
            raise "first attached move can't be an arc"
        px = self.x
        py = self.y
        pz = self.z
        if x != None: self.x = x * units
        if y != None: self.y = y * units
        if z != None: self.z = z * units

    def arc_cw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(x, y, z, i, j, k, r, False)

    def arc_ccw(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None):
        self.arc(x, y, z, i, j, k, r, True)

    def dwell(self, t):
        self.cut_path()
        self.original.dwell(t)

    def rapid_home(self, x=None, y=None, z=None, a=None, b=None, c=None):
        self.cut_path()
        self.original.rapid_home(x, y, z, a, b, c)

    def rapid_unhome(self):
        self.cut_path()
        self.original.rapid_unhome()

    ############################################################################
    ##  Cutter radius compensation

    def use_CRC(self):
        return self.original.use_CRC()

    def start_CRC(self, left = True, radius = 0.0):
        self.cut_path()
        self.original.start_CRC(left, radius)

    def end_CRC(self):
        self.cut_path()
        self.original.end_CRC()

    ############################################################################
    ##  Cycles

    def pattern(self):
        self.cut_path()
        self.original.pattern()
        
    def pattern_uses_subroutine(self):
        return self.original.pattern_uses_subroutine()
        
    def pocket(self):
        self.cut_path()
        self.original.pocket()

    def profile(self):
        self.cut_path()
        self.original.profile()

    def circular_pocket(self, x=None, y=None, ToolDiameter=None, HoleDiameter=None, ClearanceHeight=None, StartHeight=None, MaterialTop=None, FeedRate=None, SpindleRPM=None, HoleDepth=None, DepthOfCut=None, StepOver=None ):
        self.cut_path()
        self.circular_pocket(x, y, ToolDiameter, HoleDiameter, ClearanceHeight, StartHeight, MaterialTop, FeedRate, SpindleRPM, HoleDepth, DepthOfCut, StepOver)

    def drill(self, x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None, rapid_to_clearance=None):
        self.cut_path()
        self.original.drill(x, y, dwell, depthparams, spindle_mode, internal_coolant_on, rapid_to_clearance)

    # argument list adapted for compatibility with Tapping module
    # wild guess - I'm unsure about the purpose of this file and wether this works -haberlerm
    def tap(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None, tap_mode=None, direction=None):
        self.cut_path()
        self.original.tap( x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_bottom, pitch, stoppos, spin_in, spin_out, tap_mode, direction)


    def bore(self, x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, feed_in=None, feed_out=None, stoppos=None, shift_back=None, shift_right=None, backbore=False, stop=False):
        self.cut_path()
        self.original.bore(x, y, self.z2(z), self.z2(zretract), depth, standoff, dwell_Bottom, feed_in, feed_out, stoppos, shift_back, shift_right, backbore, stop)

    def end_canned_cycle(self):
        self.original.end_canned_cycle()
        
    ############################################################################
    ##  Misc

    def comment(self, text):
        self.cut_path()
        self.original.comment(text)

    def variable(self, id):
        self.cut_path()
        self.original.variable(id)

    def variable_set(self, id, value):
        self.cut_path()
        self.original.variable_set(id, value)
        
    def set_ocl_cutter(self, cutter):
        self.original.set_ocl_cutter(cutter)
