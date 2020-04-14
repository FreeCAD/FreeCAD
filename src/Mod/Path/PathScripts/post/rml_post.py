#***************************************************************************
#*   (c) Jon Nordby (jononor@gmail.com) 2015                               *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/


TOOLTIP='''
FreeCAD Path post-processor to output code for the Roland Modela MDX-## machines.

The machine speaks RML-1, specified in 'Roland RML-1 Programming Guidelines'
https://itp.nyu.edu/classes/cdp-spring2014/files/2014/09/RML1_Manual.pdf
http://altlab.org/d/content/m/pangelo/ideas/rml_command_guide_en_v100.pdf

The format has some overlap with HPGL:
https://en.wikipedia.org/wiki/HPGL
http://paulbourke.net/dataformats/hpgl/
'''

import FreeCAD
import Part
import PathScripts.PostUtils as PostUtils

# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__','io']:
    pythonopen = open


# Entrypoint used by FreeCAD
def export(objectslist, filename, argstring):
    "Export objects as Roland Modela code."
    # pylint: disable=unused-argument

    code = ""
    for obj in objectslist:
        code += convertobject(obj)

    gfile = pythonopen(filename,"w")
    gfile.write(code)
    gfile.close()

def convertobject(obj):
    gcode = obj.Path.toGCode()
    gcode = parse(gcode)
    return gcode

def motoron():
    return [ "!MC1;" ]
def motoroff():
    return [ "!MC0;" ]
def home():
    return [ "H;" ]

def setjog():
    # "!PZ%d,%d;",iz_down,iz_up); // set z down, jog
    return ""

def addheader():
    return [ "PA;PA;" ] # absolute positioning
def addfooter():
    return []

def mm2cord(mm):
    mm = float(mm)
    return int(40.0*mm)

def feed(x=None, y=None, z=None, state=None):
    c = []
    if state is None:
        state = {}

    if x is not None:
        x = float(x)
        state['X'] = x
    if y is not None:
        y = float(y)
        state['Y'] = y
    if z is not None:
        z = float(z)
        state['Z'] = z

    if x is not None and y is not None and z is not None:
        # 3d motion
        c.append("Z%d,%d,%d;" % (mm2cord(x), mm2cord(y), mm2cord(z)))
    elif x is not None and y is not None:
        # 2d in XY plane
        c.append("PD%d,%d;" % (mm2cord(x), mm2cord(y)))
    elif z is not None:
        pass
    return c

def jog(x=None, y=None, z=None, state=None):
    c = []
    if state is None:
        state = {}
    if x is not None and y is not None:
        x, y = float(x), float(y)
        c.append("PU%d,%d;" % (mm2cord(x), mm2cord(y)))
        state['X'] = x
        state['Y'] = y
    if z is not None:
        z = float(z)
        c.append("PU;")
        state['Z'] = z

    return c

def xyarc(args, state):
    # no native support in RML/Modela, convert to linear line segments
    c = []

    lastPoint = FreeCAD.Vector(state['X'], state['Y'])
    newPoint = FreeCAD.Vector(float(args['X']), float(args['Y']))
    centerOffset = FreeCAD.Vector(float(args['I']), float(args['J']))
    center = lastPoint + centerOffset
    radius = (center - lastPoint).Length
    xyNormal = FreeCAD.Vector(0, 0, 1)
    circle = Part.Circle(center, xyNormal, radius)
    p0 = circle.parameter(lastPoint)
    p1 = circle.parameter(newPoint)
    arc = Part.ArcOfCircle(circle, p0, p1)
    steps = 64 # specify max error instead?
    points = arc.discretize(steps)
    # consider direction?
    #print('p = Part.ArcOfCircle(Part.Circle(FreeCAD.Vector(%f, %f), FreeCAD.Vector(0, 0, 1), %f), %f, %f)' % (center.x, center.y, radius, p0, p1))
    for p in points:
        c += feed(p.x, p.y, state['Z'], state)
    return c

def speed(xy=None, z=None, state=None):
    c = []
    if state is None:
        state = {}
    print(xy, z, state)
    if xy is not None:
        xy = float(xy)
        if xy > 0.0 and xy != state['XYspeed']:
            c.append("VS%.1f;" % xy)
            state['XYspeed'] = xy
    if z is not None:
        z = float(z)
        if z > 0.0 and z != state['Zspeed']:
            c.append("!VZ%.1f;" % z)
            state['Zspeed'] = z
    return c

def convertgcode(cmd, args, state):
    """Convert a single gcode command to equivalent Roland code"""
    if cmd == 'G0':
        # jog
        return jog(args['X'], args['Y'], args['Z'], state)
    elif cmd == 'G1':
        # linear feed
        c = []
        # feedrate
        c += speed(xy=args['F'], z=args['F'], state=state)
        # motion
        c += feed(args['X'], args['Y'], args['Z'], state)
        return c
    elif cmd == 'G2' or cmd == 'G3':
        # arc feed
        c = []
        # feedrate
        c += speed(xy=args['F'], state=state)
        # motion
        if args['X'] and args['Y'] and args['Z']:
            # helical motion
            pass
        elif args['X'] and args['Y']:
            # arc in plane
            c += xyarc(args, state)
        return c
    elif cmd == 'G20':
        # inches mode
        raise ValueError("rml_post: Inches mode not supported")
    elif cmd == 'G21':
        # millimeter mode
        return ""
    elif cmd == 'G40':
        # tool compensation off
        return ""
    elif cmd == 'G80':
        # cancel all cycles (drill normally)
        return "PU;"
    elif cmd == 'G81':
        c = []
        # feedrate
        c += speed(z=args['F'], state=state)
        # motion
        c += jog(args['X'], args['Y'], state=state)
        c += feed(args['X'], args['Y'], args['Z'], state)
        return c
    elif cmd == 'G90':
        # absolute mode?
        return ""
    elif cmd == 'G98':
        # feedrate
        return ""
    else:
        raise NotImplementedError("rml_post: GCode command %s not understood" % (cmd,))


def parse(inputstring):
    "parse(inputstring): returns a parsed output string"

    state = { 'X': 0.0, 'Y': 0.0, 'Z': 0.0, 'XYspeed': -1.0, 'Zspeed': -1.0 }
    output = []

    # header
    output += addheader()
    output += motoron()

    output += speed(2.0, 1.0, state) # defaults

    # respect clearance height?

    # treat the input line by line
    lines = inputstring.split("\n")
    for line in lines:
        if not line:
            continue
        parsed = PostUtils.stringsplit(line)
        command = parsed['command']
        print('cmd', line)
        try:
            if command:
                code = convertgcode(command, parsed, state)
                if not isinstance(code, list):
                    code = [ code ]
                if len(code) and code[0]:
                    output += code
        except NotImplementedError as e:
            print(e)

    # footer
    output += motoroff()
    output += home()
    output += addfooter()

    return '\n'.join(output)

print (__name__ + " gcode postprocessor loaded.")

