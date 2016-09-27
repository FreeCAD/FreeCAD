################################################################################
# transform.py
#
# NC code creator for attaching Z coordinates to a surface
#

import nc
import area
import recreator

transformed = False

class FeedXY:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, 0)
        original.feed(x, y)

class FeedZ:
    def __init__(self, z):
        self.z = z
        
    def Do(self, original, matrix):
        original.feed(z = self.z)
        
class FeedXYZ:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        original.feed(x,y,z)

class RapidXY:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, 0)
        original.rapid(x, y)

class RapidZ:
    def __init__(self, z):
        self.z = z
        
    def Do(self, original, matrix):
        original.rapid(z = self.z)
        
class RapidXYZ:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        original.rapid(x,y,z)
        
class Feedrate:
    def __init__(self, h, v):
        self.h = h
        self.v = v
        
    def Do(self, original, matrix):
        original.feedrate_hv(self.h, self.v)
        
class Arc:
    def __init__(self, x, y, z, i, j, ccw):
        self.x = x
        self.y = y
        self.z = z
        self.i = i
        self.j = j
        self.ccw = ccw
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, self.z)
        cx,cy,cz = matrix.TransformedPoint(original.x + self.i, original.y + self.j, self.z)
        i = cx - original.x
        j = cy - original.y
        if self.ccw:
            original.arc_ccw(x, y, z, i, j)
        else:
            original.arc_cw(x, y, z, i, j)
            
class Drill:
    def __init__(self, x, y, dwell, depthparams, retract_mode, spindle_mode, internal_coolant_on, rapid_to_clearance):
        self.x = x
        self.y = y
        self.dwell = dwell
        self.depthparams = depthparams
        self.retract_mode = retract_mode
        self.spindle_mode = spindle_mode
        self.internal_coolant_on = internal_coolant_on
        self.rapid_to_clearance = rapid_to_clearance
        
    def Do(self, original, matrix):
        x,y,z = matrix.TransformedPoint(self.x, self.y, 0.0)
        original.drill(x, y, self.dwell, self.depthparams, self.retract_mode, self.spindle_mode, self.internal_coolant_on, self.rapid_to_clearance)
    
class Absolute:
    def __init__(self):
        pass
    
    def Do(self, original, matrix):
        original.absolute()
        
class Incremental:
    def __init__(self):
        pass
    
    def Do(self, original, matrix):
        original.incremental()
        
class EndCannedCycle:
    def __init__(self):
        pass
    
    def Do(self, original, matrix):
        original.end_canned_cycle()
        
class Comment:
    def __init__(self, text):
        self.text = text
    
    def Do(self, original, matrix):
        original.comment(self.text)

################################################################################
matrix_fixtures = {}

class Creator(recreator.Redirector):
    def __init__(self, original, matrix_list):
        recreator.Redirector.__init__(self, original)
        self.matrix_list = matrix_list
        self.commands = []
        
        # allocate fixtures to pattern positions
        if self.pattern_uses_subroutine() == True:
            save_fixture = self.get_fixture()
            for matrix in self.matrix_list:
                global matrix_fixtures
                if (matrix in matrix_fixtures) == False:
                    matrix_fixtures[matrix] = self.get_fixture()
                    self.increment_fixture()
            self.set_fixture(save_fixture)

    def DoAllCommands(self):
        subroutine_written = False
            
        for matrix in self.matrix_list:
            if len(self.commands) > 0:
                if self.pattern_uses_subroutine() == True:
                    # set fixture
                    global matrix_fixtures
                    self.set_fixture(matrix_fixtures[matrix])
                    # rapid to the pattern point in x and y
                    x,y,z = matrix.TransformedPoint(0.0, 0.0, 0.0)
                    self.original.rapid(x, y)
                                    
            subroutine_started = False
            output_disabled = False
            
            for command in self.commands:
                if (output_disabled == False) and (self.pattern_uses_subroutine() == True):
                    # in main program do commands up to the first z move
                    cname = command.__class__.__name__
                    if cname == 'FeedZ' or cname == 'Drill':
                        if subroutine_written:
                            self.sub_call(None)
                            self.disable_output() # ignore all the other commands
                            output_disabled = True
                        else:
                            if subroutine_started == False:
                                self.sub_begin(None)
                                subroutine_started = True
                                self.incremental()
                command.Do(self.original, matrix)
            
            self.enable_output()
            output_disabled = False
            
            if subroutine_started:
                self.absolute()
                self.flush_nc()
                self.sub_end()
                subroutine_written = True
                self.sub_call(None)
                    
    def tool_change(self, id):
        if id != self.original.current_tool():
            self.DoAllCommands()
            self.commands = []
        self.original.tool_change(id)

    def feedrate(self, f):
        self.commands.append(Feedrate(f, f))

    def feedrate_hv(self, fh, fv):
        self.commands.append(Feedrate(fh, fv))
        
    def rapid(self, x=None, y=None, z=None, a=None, b=None, c=None):
        if x != None: self.x = x
        if y != None: self.y = y
        if z != None: self.z = z
        if self.x == None and self.y == None and self.z == None:
            return
        if z == None:
            self.commands.append(RapidXY(self.x, self.y))
        elif x == None and y == None:
            self.commands.append(RapidZ(self.z))
        else:
            self.commands.append(RapidXYZ(self.x, self.y, self.z))
        
    def feed(self, x=None, y=None, z=None, a = None, b = None, c = None):
        if x != None: self.x = x
        if y != None: self.y = y
        if z != None: self.z = z
        if self.x == None and self.y == None and self.z == None:
            return
        if z == None:
            self.commands.append(FeedXY(self.x, self.y))
        elif x == None and y == None:
            self.commands.append(FeedZ(self.z))
        else:
            self.commands.append(FeedXYZ(self.x, self.y, self.z))
        
    def arc(self, x=None, y=None, z=None, i=None, j=None, k=None, r=None, ccw = True):
        if x != None: self.x = x
        if y != None: self.y = y
        if z != None: self.z = z
        if self.x == None and self.y == None and self.z == None:
            return
        self.commands.append(Arc(self.x, self.y, self.z, i, j, ccw))
        
    def drill(self, x=None, y=None, dwell=None, depthparams = None, retract_mode=None, spindle_mode=None, internal_coolant_on=None, rapid_to_clearance=None):
        self.commands.append(Drill(x, y, dwell, depthparams, retract_mode, spindle_mode, internal_coolant_on, rapid_to_clearance))

    def end_canned_cycle(self):
        self.commands.append(EndCannedCycle())
        
#    def absolute(self):
#        self.commands.append(Absolute())

#    def incremental(self):
#        self.commands.append(Incremental())

    def comment(self, text):
        self.commands.append(Comment(text))
        
################################################################################

def transform_begin(matrix_list):
    global transformed
    if transformed == True:
        transform_end()
    nc.creator = Creator(nc.creator, matrix_list)
    transformed = True

def transform_end():
    global transformed
    nc.creator.DoAllCommands()
    nc.creator = nc.creator.original
    transformed = False
