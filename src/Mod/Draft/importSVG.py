# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides support for importing and exporting SVG files.

It enables importing/exporting objects directly to/from the 3D document
but doesn't handle the SVG output from the TechDraw module.

Currently it only reads the following entities:
* paths, lines, circular arcs, rects, circles, ellipses, polygons, polylines.

Currently unsupported:
* use, image.
"""
## @package importSVG
#  \ingroup DRAFT
#  \brief SVG file importer and exporter

# Check code with
# flake8 --ignore=E226,E266,E401,W503

__title__ = "FreeCAD Draft Workbench - SVG importer/exporter"
__author__ = "Yorik van Havre, Sebastian Hoogen"
__url__ = "https://www.freecad.org"

# TODO:
# ignoring CDATA
# handle image element (external references and inline base64)
# debug Problem with 'Sans' font from Inkscape
# debug Problem with fill color
# implement inheriting fill style from group
# handle relative units

import math
import os
import re
import xml.sax

import FreeCAD
import Draft
import DraftVecUtils
from FreeCAD import Vector
from draftutils.translate import translate
from draftutils.messages import _msg, _wrn, _err

if FreeCAD.GuiUp:
    from PySide import QtGui
    import FreeCADGui
    gui = True
    try:
        draftui = FreeCADGui.draftToolBar
    except AttributeError:
        draftui = None
else:
    gui = False
    draftui = None

# Save the native open function to avoid collisions
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open

svgcolors = {
    'Pink': (255, 192, 203),
    'Blue': (0, 0, 255),
    'Honeydew': (240, 255, 240),
    'Purple': (128, 0, 128),
    'Fuchsia': (255, 0, 255),
    'LawnGreen': (124, 252, 0),
    'Amethyst': (153, 102, 204),
    'Crimson': (220, 20, 60),
    'White': (255, 255, 255),
    'NavajoWhite': (255, 222, 173),
    'Cornsilk': (255, 248, 220),
    'Bisque': (255, 228, 196),
    'PaleGreen': (152, 251, 152),
    'Brown': (165, 42, 42),
    'DarkTurquoise': (0, 206, 209),
    'DarkGreen': (0, 100, 0),
    'MediumOrchid': (186, 85, 211),
    'Chocolate': (210, 105, 30),
    'PapayaWhip': (255, 239, 213),
    'Olive': (128, 128, 0),
    'Silver': (192, 192, 192),
    'PeachPuff': (255, 218, 185),
    'Plum': (221, 160, 221),
    'DarkGoldenrod': (184, 134, 11),
    'SlateGrey': (112, 128, 144),
    'MintCream': (245, 255, 250),
    'CornflowerBlue': (100, 149, 237),
    'Gold': (255, 215, 0),
    'HotPink': (255, 105, 180),
    'DarkBlue': (0, 0, 139),
    'LimeGreen': (50, 205, 50),
    'DeepSkyBlue': (0, 191, 255),
    'DarkKhaki': (189, 183, 107),
    'LightGrey': (211, 211, 211),
    'Yellow': (255, 255, 0),
    'Gainsboro': (220, 220, 220),
    'MistyRose': (255, 228, 225),
    'SandyBrown': (244, 164, 96),
    'DeepPink': (255, 20, 147),
    'Magenta': (255, 0, 255),
    'AliceBlue': (240, 248, 255),
    'DarkCyan': (0, 139, 139),
    'DarkSlateGrey': (47, 79, 79),
    'GreenYellow': (173, 255, 47),
    'DarkOrchid': (153, 50, 204),
    'OliveDrab': (107, 142, 35),
    'Chartreuse': (127, 255, 0),
    'Peru': (205, 133, 63),
    'Orange': (255, 165, 0),
    'Red': (255, 0, 0),
    'Wheat': (245, 222, 179),
    'LightCyan': (224, 255, 255),
    'LightSeaGreen': (32, 178, 170),
    'BlueViolet': (138, 43, 226),
    'LightSlateGrey': (119, 136, 153),
    'Cyan': (0, 255, 255),
    'MediumPurple': (147, 112, 219),
    'MidnightBlue': (25, 25, 112),
    'FireBrick': (178, 34, 34),
    'PaleTurquoise': (175, 238, 238),
    'PaleGoldenrod': (238, 232, 170),
    'Gray': (128, 128, 128),
    'MediumSeaGreen': (60, 179, 113),
    'Moccasin': (255, 228, 181),
    'Ivory': (255, 255, 240),
    'DarkSlateBlue': (72, 61, 139),
    'Beige': (245, 245, 220),
    'Green': (0, 128, 0),
    'SlateBlue': (106, 90, 205),
    'Teal': (0, 128, 128),
    'Azure': (240, 255, 255),
    'LightSteelBlue': (176, 196, 222),
    'DimGrey': (105, 105, 105),
    'Tan': (210, 180, 140),
    'AntiqueWhite': (250, 235, 215),
    'SkyBlue': (135, 206, 235),
    'GhostWhite': (248, 248, 255),
    'MediumTurquoise': (72, 209, 204),
    'FloralWhite': (255, 250, 240),
    'LavenderBlush': (255, 240, 245),
    'SeaGreen': (46, 139, 87),
    'Lavender': (230, 230, 250),
    'BlanchedAlmond': (255, 235, 205),
    'DarkOliveGreen': (85, 107, 47),
    'DarkSeaGreen': (143, 188, 143),
    'SpringGreen': (0, 255, 127),
    'Navy': (0, 0, 128),
    'Orchid': (218, 112, 214),
    'SaddleBrown': (139, 69, 19),
    'IndianRed': (205, 92, 92),
    'Snow': (255, 250, 250),
    'SteelBlue': (70, 130, 180),
    'MediumSlateBlue': (123, 104, 238),
    'Black': (0, 0, 0),
    'LightBlue': (173, 216, 230),
    'Turquoise': (64, 224, 208),
    'MediumVioletRed': (199, 21, 133),
    'DarkViolet': (148, 0, 211),
    'DarkGray': (169, 169, 169),
    'Salmon': (250, 128, 114),
    'DarkMagenta': (139, 0, 139),
    'Tomato': (255, 99, 71),
    'WhiteSmoke': (245, 245, 245),
    'Goldenrod': (218, 165, 32),
    'MediumSpringGreen': (0, 250, 154),
    'DodgerBlue': (30, 144, 255),
    'Aqua': (0, 255, 255),
    'ForestGreen': (34, 139, 34),
    'LemonChiffon': (255, 250, 205),
    'LightSlateGray': (119, 136, 153),
    'SlateGray': (112, 128, 144),
    'LightGray': (211, 211, 211),
    'Indigo': (75, 0, 130),
    'CadetBlue': (95, 158, 160),
    'LightYellow': (255, 255, 224),
    'DarkOrange': (255, 140, 0),
    'PowderBlue': (176, 224, 230),
    'RoyalBlue': (65, 105, 225),
    'Sienna': (160, 82, 45),
    'Thistle': (216, 191, 216),
    'Lime': (0, 255, 0),
    'Seashell': (255, 245, 238),
    'DarkRed': (139, 0, 0),
    'LightSkyBlue': (135, 206, 250),
    'YellowGreen': (154, 205, 50),
    'Aquamarine': (127, 255, 212),
    'LightCoral': (240, 128, 128),
    'DarkSlateGray': (47, 79, 79),
    'Khaki': (240, 230, 140),
    'DarkGrey': (169, 169, 169),
    'BurlyWood': (222, 184, 135),
    'LightGoldenrodYellow': (250, 250, 210),
    'MediumBlue': (0, 0, 205),
    'DarkSalmon': (233, 150, 122),
    'RosyBrown': (188, 143, 143),
    'LightSalmon': (255, 160, 122),
    'PaleVioletRed': (219, 112, 147),
    'Coral': (255, 127, 80),
    'Violet': (238, 130, 238),
    'Grey': (128, 128, 128),
    'LightGreen': (144, 238, 144),
    'Linen': (250, 240, 230),
    'OrangeRed': (255, 69, 0),
    'DimGray': (105, 105, 105),
    'Maroon': (128, 0, 0),
    'LightPink': (255, 182, 193),
    'MediumAquamarine': (102, 205, 170),
    'OldLace': (253, 245, 230)
}
svgcolorslower = \
    dict((key.lower(), value) for (key, value) in list(svgcolors.items()))


def getcolor(color):
    """Check if the given string is an RGB value, or if it is a named color.

    Parameters
    ----------
    color : str
        Color in hexadecimal format, long '#12ab9f' or short '#1af'

    Returns
    -------
    tuple
    (r, g, b, a)
        RGBA float tuple, where each value is between 0.0 and 1.0.
    """
    if color == "none":
        FreeCAD.Console.PrintMessage("Color defined as 'none', defaulting to black\n")
        return (0.0, 0.0, 0.0, 0.0)
    if color[0] == "#":
        if len(color) == 7 or len(color) == 9: # Color string '#RRGGBB' or '#RRGGBBAA'
            r = float(int(color[1:3], 16) / 255.0)
            g = float(int(color[3:5], 16) / 255.0)
            b = float(int(color[5:7], 16) / 255.0)
            a = 1.0
            if len(color) == 9:
                a = float(int(color[7:9], 16) / 255.0)
                FreeCAD.Console.PrintMessage(f"Non standard color format #RRGGBBAA : {color}\n")
            return (r, g, b, 1-a)
        if len(color) == 4: # Color string '#RGB'
            # Expand the hex digits
            r = float(int(color[1], 16) * 17 / 255.0)
            g = float(int(color[2], 16) * 17 / 255.0)
            b = float(int(color[3], 16) * 17 / 255.0)
            return (r, g, b, 0.0)
    if color.lower().startswith('rgb(') or color.lower().startswith('rgba('): # Color string 'rgb[a](0.12,0.23,0.3,0.0)'
        cvalues = color.lstrip('rgba(').rstrip(')').replace('%', '').split(',')
        if len(cvalues) == 3:
            a = 1.0
            if '%' in color:
                r, g, b = [int(float(cv)) / 100.0 for cv in cvalues]
            else:
                r, g, b = [int(float(cv)) / 255.0 for cv in cvalues]
        if len(cvalues) == 4:
            if '%' in color:
                r, g, b, a = [int(float(cv)) / 100.0 for cv in cvalues]
            else:
                r, g, b, a = [int(float(cv)) / 255.0 for cv in cvalues]
        return (r, g, b, 1-a)
    # Trying named color like 'MediumAquamarine'
    v = svgcolorslower.get(color.lower())
    if v:
        r, g, b = [float(vf) / 255.0 for vf in v]
        return (r, g, b, 0.0)
    FreeCAD.Console.PrintWarning(f"Unknown color format : {color} : defaulting to black\n")
    return (0.0, 0.0, 0.0, 0.0)


def transformCopyShape(shape, m):
    """Apply transformation matrix m on given shape.

    Since OCCT 6.8.0 transformShape can be used to apply certain
    non-orthogonal transformations on shapes. This way a conversion
    to BSplines in transformGeometry can be avoided.

    @sa: Part::TopoShape::transformGeometry(), TopoShapePy::transformGeometry()
    @sa: Part::TopoShape::transformShape(), TopoShapePy::transformShape()

    Parameters
    ----------
    shape : Part::TopoShape
        A given shape
    m : Base::Matrix4D
        A transformation matrix

    Returns
    -------
    shape : Part::TopoShape
        The shape transformed by the matrix
    """
    # If there is no shear, these matrix operations will be very small
    _s1 = abs(m.A11**2 + m.A12**2 - m.A21**2 - m.A22**2)
    _s2 = abs(m.A11 * m.A21 + m.A12 * m.A22)
    if _s1 < 1e-8 and _s2 < 1e-8:
        try:
            newshape = shape.copy()
            newshape.transformShape(m)
            return newshape
        # Older versions of OCCT will refuse to work on
        # non-orthogonal matrices
        except Part.OCCError:
            pass
    return shape.transformGeometry(m)


def getsize(length, mode='discard', base=1):
    """Parse the length string containing number and unit.

    Parameters
    ----------
    length : str
        The length is a string, including sign, exponential notation,
        and unit: '+56215.14565E+6mm', '-23.156e-2px'.
    mode : str, optional
        One of 'discard', 'tuple', 'css90.0', 'css96.0', 'mm90.0', 'mm96.0'.
        'discard' (default), it discards the unit suffix, and extracts
            a number from the given string.
        'tuple', return number and unit as a tuple
        'css90.0', convert the unit to pixels assuming 90 dpi
        'css96.0', convert the unit to pixels assuming 96 dpi
        'mm90.0', convert the unit to millimeters assuming 90 dpi
        'mm96.0', convert the unit to millimeters assuming 96 dpi
    base : float, optional
        A base to scale the length.

    Returns
    -------
    float
        The numeric value of the length, as is, or transformed to
        millimeters or pixels.
    float, string
        A tuple with the numeric value, and the unit if `mode='tuple'`.
    """
    # Dictionaries to convert units to millimeters or pixels.
    #
    # The `em` and `ex` units are typographical units used in systems
    # like LaTeX. Here the conversion factors are arbitrarily chosen,
    # as they should depend on a specific font size used.
    #
    # The percentage factor is arbitrarily chosen, as it should depend
    # on the viewport size or for filling patterns on the bounding box.
    if mode == 'mm90.0':
        tomm = {
            '': 25.4/90,  # default
            'px': 25.4/90,
            'pt': 4.0/3 * 25.4/90,
            'pc': 15 * 25.4/90,
            'mm': 1.0,
            'cm': 10.0,
            'in': 25.4,
            'em': 15 * 2.54/90,
            'ex': 10 * 2.54/90,
            '%': 100
        }
    elif mode == 'mm96.0':
        tomm = {
            '': 25.4/96,  # default
            'px': 25.4/96,
            'pt': 4.0/3 * 25.4/96,
            'pc': 15 * 25.4/96,
            'mm': 1.0,
            'cm': 10.0,
            'in': 25.4,
            'em': 15 * 2.54/96,
            'ex': 10 * 2.54/96,
            '%': 100
        }
    elif mode == 'css90.0':
        topx = {
            '': 1.0,  # default
            'px': 1.0,
            'pt': 4.0/3,
            'pc': 15,
            'mm': 90.0/25.4,
            'cm': 90.0/254.0,
            'in': 90,
            'em': 15,
            'ex': 10,
            '%': 100
        }
    elif mode == 'css96.0':
        topx = {
            '': 1.0,  # default
            'px': 1.0,
            'pt': 4.0/3,
            'pc': 15,
            'mm': 96.0/25.4,
            'cm': 96.0/254.0,
            'in': 96,
            'em': 15,
            'ex': 10,
            '%': 100
        }

    # Extract a number from a string like '+56215.14565E+6mm'
    _num = '([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)'
    _unit = '(px|pt|pc|mm|cm|in|em|ex|%)?'
    _full_num = _num + _unit
    number, exponent, unit = re.findall(_full_num, length)[0]
    if mode == 'discard':
        return float(number)
    elif mode == 'tuple':
        return float(number), unit
    elif mode == 'isabsolute':
        return unit in ('mm', 'cm', 'in', 'px', 'pt')
    elif mode == 'mm96.0' or mode == 'mm90.0':
        return float(number) * tomm[unit]
    elif mode == 'css96.0' or mode == 'css90.0':
        if unit != '%':
            return float(number) * topx[unit]
        else:
            return float(number) * base


def makewire(path, checkclosed=False, donttry=False):
    '''Try to make a wire out of the list of edges.

    If the wire functions fail or the wire is not closed,
    if required the TopoShapeCompoundPy::connectEdgesToWires()
    function is used.

    Parameters
    ----------
    path : Part.Edge
        A collection of edges
    checkclosed : bool, optional
        Default is `False`.
    donttry : bool, optional
        Default is `False`. If it's `True` it won't try to check
        for a closed path.

    Returns
    -------
    Part::Wire
        A wire created from the ordered edges.
    Part::Compound
        A compound made of the edges, but unable to form a wire.
    '''
    if not donttry:
        try:
            import Part
            sh = Part.Wire(Part.__sortEdges__(path))
            # sh = Part.Wire(path)
            isok = (not checkclosed) or sh.isClosed()
            if len(sh.Edges) != len(path):
                isok = False
        # BRep_API: command not done
        except Part.OCCError:
            isok = False
    if donttry or not isok:
        # Code from wmayer forum p15549 to fix the tolerance problem
        # original tolerance = 0.00001
        comp = Part.Compound(path)
        _sh = comp.connectEdgesToWires(False,
                                       10**(-1 * (Draft.precision() - 2)))
        sh = _sh.Wires[0]
        if len(sh.Edges) != len(path):
            _wrn("Unable to form a wire")
            sh = comp
    return sh


def arccenter2end(center, rx, ry, angle1, angledelta, xrotation=0.0):
    '''Calculate start and end points, and flags of an arc.

    Calculate start and end points, and flags of an arc given in
    ``center parametrization``.
    See http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

    Parameters
    ----------
    center : Base::Vector3
        Coordinates of the center of the ellipse.
    rx : float
        Radius of the ellipse, semi-major axis in the X direction
    ry : float
        Radius of the ellipse, semi-minor axis in the Y direction
    angle1 : float
        Initial angle in radians
    angledelta : float
        Additional angle in radians
    xrotation : float, optional
        Default 0. Rotation around the Z axis

    Returns
    -------
    v1, v2, largerc, sweep
        Tuple indicating the end points of the arc, and two boolean values
        indicating whether the arc is less than 180 degrees or not,
        and whether the angledelta is negative.
    '''
    vr1 = Vector(rx * math.cos(angle1), ry * math.sin(angle1), 0)
    vr2 = Vector(rx * math.cos(angle1 + angledelta),
                 ry * math.sin(angle1 + angledelta),
                 0)
    mxrot = FreeCAD.Matrix()
    mxrot.rotateZ(xrotation)
    v1 = mxrot.multiply(vr1).add(center)
    v2 = mxrot.multiply(vr2).add(center)
    fa = ((abs(angledelta) / math.pi) % 2) > 1  # < 180 deg
    fs = angledelta < 0
    return v1, v2, fa, fs


def arcend2center(lastvec, currentvec, rx, ry,
                  xrotation=0.0, correction=False):
    '''Calculate the possible centers for an arc in endpoint parameterization.

    Calculate (positive and negative) possible centers for an arc given in
    ``endpoint parametrization``.
    See http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

    the sweepflag is interpreted as: sweepflag <==>  arc is travelled clockwise

    Parameters
    ----------
    lastvec : Base::Vector3
        First point of the arc.
    currentvec : Base::Vector3
        End point (current) of the arc.
    rx : float
        Radius of the ellipse, semi-major axis in the X direction.
    ry : float
        Radius of the ellipse, semi-minor axis in the Y direction.
    xrotation : float, optional
        Default is 0. Rotation around the Z axis, in radians (CCW).
    correction : bool, optional
        Default is `False`. If it is `True`, the radii will be scaled
        by a factor.

    Returns
    -------
    list, (float, float)
        A tuple that consists of one list, and a tuple of radii.
    [(positive), (negative)], (rx, ry)
        The first element of the list is the positive tuple,
        the second is the negative tuple.
    [(Base::Vector3, float, float),
    (Base::Vector3, float, float)], (float, float)
        Types
    [(vcenter+, angle1+, angledelta+),
    (vcenter-, angle1-, angledelta-)], (rx, ry)
        The first element of the list is the positive tuple,
        consisting of center, angle, and angle increment;
        the second element is the negative tuple.
    '''
    # scalefacsign = 1 if (largeflag != sweepflag) else -1
    rx = float(rx)
    ry = float(ry)
    v0 = lastvec.sub(currentvec)
    v0.multiply(0.5)
    m1 = FreeCAD.Matrix()
    m1.rotateZ(-xrotation)  # eq. 5.1
    v1 = m1.multiply(v0)
    if correction:
        eparam = v1.x**2 / rx**2 + v1.y**2 / ry**2
        if eparam > 1:
            eproot = math.sqrt(eparam)
            rx = eproot * rx
            ry = eproot * ry
    denom = rx**2 * v1.y**2 + ry**2 * v1.x**2
    numer = rx**2 * ry**2 - denom
    results = []

    # If the division is very small, set the scaling factor to zero,
    # otherwise try to calculate it by taking the square root
    if abs(numer/denom) < 10**(-1 * (Draft.precision())):
        scalefacpos = 0
    else:
        try:
            scalefacpos = math.sqrt(numer/denom)
        except ValueError:
            _msg("sqrt({0}/{1})".format(numer, denom))
            scalefacpos = 0

    # Calculate two values because the square root may be positive or negative
    for scalefacsign in (1, -1):
        scalefac = scalefacpos * scalefacsign
        # Step2 eq. 5.2
        vcx1 = Vector(v1.y * rx/ry, -v1.x * ry/rx, 0).multiply(scalefac)
        m2 = FreeCAD.Matrix()
        m2.rotateZ(xrotation)
        centeroff = currentvec.add(lastvec)
        centeroff.multiply(0.5)
        vcenter = m2.multiply(vcx1).add(centeroff)  # Step3 eq. 5.3
        # angle1 = Vector(1, 0, 0).getAngle(Vector((v1.x - vcx1.x)/rx,
        #                                          (v1.y - vcx1.y)/ry,
        #                                          0))  # eq. 5.5
        # angledelta = Vector((v1.x - vcx1.x)/rx,
        #                     (v1.y - vcx1.y)/ry,
        #                     0).getAngle(Vector((-v1.x - vcx1.x)/rx,
        #                                        (-v1.y - vcx1.y)/ry,
        #                                        0))  # eq. 5.6
        # we need the right sign for the angle
        angle1 = DraftVecUtils.angle(Vector(1, 0, 0),
                                     Vector((v1.x - vcx1.x)/rx,
                                            (v1.y - vcx1.y)/ry,
                                            0))  # eq. 5.5
        angledelta = DraftVecUtils.angle(Vector((v1.x - vcx1.x)/rx,
                                                (v1.y - vcx1.y)/ry,
                                                0),
                                         Vector((-v1.x - vcx1.x)/rx,
                                                (-v1.y - vcx1.y)/ry,
                                                0))  # eq. 5.6
        results.append((vcenter, angle1, angledelta))

        if rx < 0 or ry < 0:
            _wrn("Warning: 'rx' or 'ry' is negative, check the SVG file")

    return results, (rx, ry)


def getrgb(color):
    """Return an RGB hexadecimal string '#00aaff' from a FreeCAD color.

    Parameters
    ----------
    color : App::Color::Color
        FreeCAD color.

    Returns
    -------
    str
        The hexadecimal string representation of the color '#00aaff'.
    """
    r = str(hex(int(color[0] * 255)))[2:].zfill(2)
    g = str(hex(int(color[1] * 255)))[2:].zfill(2)
    b = str(hex(int(color[2] * 255)))[2:].zfill(2)
    return "#" + r + g + b


class svgHandler(xml.sax.ContentHandler):
    """Parse SVG files and create FreeCAD objects."""

    def __init__(self):
        super().__init__()
        """Retrieve Draft parameters and initialize."""
        _prefs = "User parameter:BaseApp/Preferences/Mod/Draft"
        params = FreeCAD.ParamGet(_prefs)
        self.style = params.GetInt("svgstyle")
        self.disableUnitScaling = params.GetBool("svgDisableUnitScaling",
                                                 False)
        self.count = 0
        self.transform = None
        self.grouptransform = []
        self.groupstyles = []
        self.lastdim = None
        self.viewbox = None
        self.symbols = {}
        self.currentsymbol = None
        self.svgdpi = 1.0

        global Part
        import Part

        if gui and draftui:
            r = float(draftui.color.red()/255.0)
            g = float(draftui.color.green()/255.0)
            b = float(draftui.color.blue()/255.0)
            self.lw = float(draftui.linewidth)
        else:
            self.lw = float(params.GetInt("linewidth"))
            c = params.GetUnsigned("color")
            r = float(((c >> 24) & 0xFF)/255)
            g = float(((c >> 16) & 0xFF)/255)
            b = float(((c >> 8) & 0xFF)/255)
        self.col = (r, g, b, 0.0)

    def format(self, obj):
        """Apply styles to the object if the graphical interface is up."""
        if FreeCAD.GuiUp:
            v = obj.ViewObject
            if self.color:
                v.LineColor = self.color
            if self.width:
                v.LineWidth = self.width
            if self.fill:
                v.ShapeColor = self.fill

    def startElement(self, name, attrs):
        """Re-organize data into a nice clean dictionary.

        Parameters
        ----------
        name : str
            Name of the element: 'path', 'rect', 'line', 'polyline',
            'polygon', 'ellipse', 'circle', 'text', 'tspan', 'symbol'
        attrs : iterable
            Dictionary of content of the elements
        """
        self.count += 1
        _msg('processing element {0}: {1}'.format(self.count, name))
        _msg('existing group transform: {}'.format(self.grouptransform))
        _msg('existing group style: {}'.format(self.groupstyles))

        data = {}
        for (keyword, content) in list(attrs.items()):
            # print(keyword, content)
            if keyword != "style":
                content = content.replace(',', ' ')
                content = content.split()
            # print(keyword, content)
            data[keyword] = content

        # If it's the first element, which is <svg>,
        # check if the file is created by Inkscape, and its version,
        # in order to consider some attributes of the SVG file.
        if self.count == 1 and name == 'svg':
            if 'inkscape:version' in data:
                inks_doc_name = attrs.getValue('sodipodi:docname')
                inks_full_ver = attrs.getValue('inkscape:version')
                inks_ver_pars = re.search("\d+\.\d+", inks_full_ver)
                if inks_ver_pars is not None:
                    inks_ver_f = float(inks_ver_pars.group(0))
                else:
                    inks_ver_f = 99.99
                # Inkscape before 0.92 used 90 dpi as resolution
                # Newer versions use 96 dpi
                if inks_ver_f < 0.92:
                    self.svgdpi = 90.0
                else:
                    self.svgdpi = 96.0
            if 'inkscape:version' not in data:
                # exact scaling is calculated later below. Here we just want
                # to skip the DPI dialog if a unit is specified in the viewbox
                if "width" in data and "mm" in attrs.getValue('width'):
                    self.svgdpi = 96.0
                elif "width" in data and "in" in attrs.getValue('width'):
                    self.svgdpi = 96.0
                elif "width" in data and "cm" in attrs.getValue('width'):
                    self.svgdpi = 96.0
                else:
                    _inf = ("This SVG file does not appear to have been produced "
                            "by Inkscape. If it does not contain absolute units "
                            "then a DPI setting will be used.")
                    _qst = ("Do you wish to use 96 dpi? Choosing 'No' "
                            "will use the older standard 90 dpi.")
                    if FreeCAD.GuiUp:
                        msgBox = QtGui.QMessageBox()
                        msgBox.setText(translate("ImportSVG", _inf))
                        msgBox.setInformativeText(translate("ImportSVG", _qst))
                        msgBox.setStandardButtons(QtGui.QMessageBox.Yes
                                                  | QtGui.QMessageBox.No)
                        msgBox.setDefaultButton(QtGui.QMessageBox.No)
                        ret = msgBox.exec_()
                        if ret == QtGui.QMessageBox.Yes:
                            self.svgdpi = 96.0
                        else:
                            self.svgdpi = 90.0
                        if ret:
                            _msg(translate("ImportSVG", _inf))
                            _msg(translate("ImportSVG", _qst))
                            _msg("*** User specified {} "
                                 "dpi ***".format(self.svgdpi))
                    else:
                        self.svgdpi = 96.0
                        _msg(_inf)
                        _msg("*** Assuming {} dpi ***".format(self.svgdpi))
            if self.svgdpi == 1.0:
                _wrn("This SVG file (" + inks_doc_name + ") "
                     "has an unrecognised format which means "
                     "the dpi could not be determined; "
                     "assuming 96 dpi")
                self.svgdpi = 96.0

        if 'style' in data:
            if not data['style']:
                # Empty style attribute stops inheriting from parent
                pass
            else:
                content = data['style'].replace(' ', '')
                content = content.split(';')
                for i in content:
                    pair = i.split(':')
                    if len(pair) > 1:
                        data[pair[0]] = pair[1]

        for k in ['x', 'y', 'x1', 'y1', 'x2', 'y2',
                  'r', 'rx', 'ry', 'cx', 'cy', 'width', 'height']:
            if k in data:
                data[k] = getsize(data[k][0], 'css' + str(self.svgdpi))

        for k in ['fill', 'stroke', 'stroke-width', 'font-size']:
            if k in data:
                if isinstance(data[k], list):
                    if data[k][0].lower().startswith("rgb("):
                        data[k] = ",".join(data[k])
                    else:
                        data[k] = data[k][0]

        # Extract style info
        self.fill = None
        self.color = None
        self.width = None
        self.text = None

        if name == 'svg':
            m = FreeCAD.Matrix()
            if not self.disableUnitScaling:
                if 'width' in data \
                        and 'height' in data \
                        and 'viewBox' in data:
                    if len(self.grouptransform) == 0:
                        unitmode = 'mm' + str(self.svgdpi)
                    else:
                        # nested svg element
                        unitmode = 'css' + str(self.svgdpi)
                    vbw = getsize(data['viewBox'][2], unitmode)
                    vbh = getsize(data['viewBox'][3], unitmode)
                    abw = getsize(attrs.getValue('width'), unitmode)
                    abh = getsize(attrs.getValue('height'), unitmode)
                    self.viewbox = (vbw, vbh)
                    sx = abw / vbw
                    sy = abh / vbh
                    _data = data.get('preserveAspectRatio', [])
                    preservearstr = ' '.join(_data).lower()
                    uniformscaling = round(sx/sy, 5) == 1
                    if uniformscaling:
                        m.scale(Vector(sx, sy, 1))
                    else:
                        _wrn('Scaling factors do not match!')
                        if preservearstr.startswith('none'):
                            m.scale(Vector(sx, sy, 1))
                        else:
                            # preserve the aspect ratio
                            if preservearstr.endswith('slice'):
                                sxy = max(sx, sy)
                            else:
                                sxy = min(sx, sy)
                            m.scale(Vector(sxy, sxy, 1))
                elif len(self.grouptransform) == 0:
                    # fallback to current dpi
                    m.scale(Vector(25.4/self.svgdpi, 25.4/self.svgdpi, 1))
            self.grouptransform.append(m)
        if 'fill' in data:
            if data['fill'] != 'none':
                self.fill = getcolor(data['fill'])
        if 'stroke' in data:
            if data['stroke'] != 'none':
                self.color = getcolor(data['stroke'])
        if 'stroke-width' in data:
            if data['stroke-width'] != 'none':
                self.width = getsize(data['stroke-width'],
                                     'css' + str(self.svgdpi))
        if 'transform' in data:
            m = self.getMatrix(attrs.getValue('transform'))
            if name == "g":
                self.grouptransform.append(m)
            else:
                self.transform = m
        else:
            if name == "g":
                self.grouptransform.append(FreeCAD.Matrix())

        if self.style == 1:
            self.color = self.col
            self.width = self.lw

        # apply group styles
        if name == "g":
            self.groupstyles.append([self.fill, self.color, self.width])
        if self.fill is None:
            if "fill" not in data or data['fill'] != 'none':
                # do not override fill if this item has specifically set a none fill
                for groupstyle in reversed(self.groupstyles):
                    if groupstyle[0] is not None:
                        self.fill = groupstyle[0]
                        break
        if self.color is None:
            for groupstyle in reversed(self.groupstyles):
                if groupstyle[1] is not None:
                    self.color = groupstyle[1]
                    break
        if self.width is None:
            for groupstyle in reversed(self.groupstyles):
                if groupstyle[2] is not None:
                    self.width = groupstyle[2]
                    break

        pathname = None
        if 'id' in data:
            pathname = data['id'][0]
            _msg('name: {}'.format(pathname))

        # Process paths
        if name == "path":
            _msg('data: {}'.format(data))

            if not pathname:
                pathname = 'Path'

            path = []
            point = []
            lastvec = Vector(0, 0, 0)
            lastpole = None
            # command = None
            relative = False
            firstvec = None

            if "freecad:basepoint1" in data:
                p1 = data["freecad:basepoint1"]
                p1 = Vector(float(p1[0]), -float(p1[1]), 0)
                p2 = data["freecad:basepoint2"]
                p2 = Vector(float(p2[0]), -float(p2[1]), 0)
                p3 = data["freecad:dimpoint"]
                p3 = Vector(float(p3[0]), -float(p3[1]), 0)
                obj = Draft.make_dimension(p1, p2, p3)
                self.applyTrans(obj)
                self.format(obj)
                self.lastdim = obj
                data['d'] = []

            _op = '([mMlLhHvVaAcCqQsStTzZ])'
            _op2 = '([^mMlLhHvVaAcCqQsStTzZ]*)'
            _command = '\s*?' + _op + '\s*?' + _op2 + '\s*?'
            pathcommandsre = re.compile(_command, re.DOTALL)

            _num = '[-+]?[0-9]*\.?[0-9]+'
            _exp = '([eE][-+]?[0-9]+)?'
            _point = '(' + _num + _exp + ')'
            pointsre = re.compile(_point, re.DOTALL)
            _commands = pathcommandsre.findall(' '.join(data['d']))
            for d, pointsstr in _commands:
                relative = d.islower()
                _points = pointsre.findall(pointsstr.replace(',', ' '))
                pointlist = [float(number) for number, exponent in _points]

                if (d == "M" or d == "m"):
                    x = pointlist.pop(0)
                    y = pointlist.pop(0)
                    if path:
                        # sh = Part.Wire(path)
                        sh = makewire(path)
                        if self.fill and sh.isClosed():
                            sh = Part.Face(sh)
                            if sh.isValid() is False:
                                sh.fix(1e-6, 0, 1)
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature", pathname)
                        obj.Shape = sh
                        self.format(obj)
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)
                        path = []
                        # if firstvec:
                        #    Move relative to last move command
                        #    not last draw command
                        #    lastvec = firstvec
                    if relative:
                        lastvec = lastvec.add(Vector(x, -y, 0))
                    else:
                        lastvec = Vector(x, -y, 0)
                    firstvec = lastvec
                    _msg('move {}'.format(lastvec))
                    lastpole = None

                if (d == "L" or d == "l") \
                        or ((d == 'm' or d == 'M') and pointlist):
                    for x, y in zip(pointlist[0::2], pointlist[1::2]):
                        if relative:
                            currentvec = lastvec.add(Vector(x, -y, 0))
                        else:
                            currentvec = Vector(x, -y, 0)
                        if not DraftVecUtils.equals(lastvec, currentvec):
                            _seg = Part.LineSegment(lastvec, currentvec)
                            seg = _seg.toShape()
                            _msg("line {} {}".format(lastvec, currentvec))
                            lastvec = currentvec
                            path.append(seg)
                        lastpole = None
                elif (d == "H" or d == "h"):
                    for x in pointlist:
                        if relative:
                            currentvec = lastvec.add(Vector(x, 0, 0))
                        else:
                            currentvec = Vector(x, lastvec.y, 0)
                        seg = Part.LineSegment(lastvec, currentvec).toShape()
                        lastvec = currentvec
                        lastpole = None
                        path.append(seg)
                elif (d == "V" or d == "v"):
                    for y in pointlist:
                        if relative:
                            currentvec = lastvec.add(Vector(0, -y, 0))
                        else:
                            currentvec = Vector(lastvec.x, -y, 0)
                        if lastvec != currentvec:
                            _seg = Part.LineSegment(lastvec, currentvec)
                            seg = _seg.toShape()
                            lastvec = currentvec
                            lastpole = None
                            path.append(seg)
                elif (d == "A" or d == "a"):
                    piter = zip(pointlist[0::7], pointlist[1::7],
                                pointlist[2::7], pointlist[3::7],
                                pointlist[4::7], pointlist[5::7],
                                pointlist[6::7])
                    for (rx, ry, xrotation,
                         largeflag, sweepflag,
                         x, y) in piter:
                        # support for large-arc and x-rotation is missing
                        if relative:
                            currentvec = lastvec.add(Vector(x, -y, 0))
                        else:
                            currentvec = Vector(x, -y, 0)
                        chord = currentvec.sub(lastvec)
                        # small circular arc
                        _precision = 10**(-1*Draft.precision())
                        if (not largeflag) and abs(rx - ry) < _precision:
                            # perp = chord.cross(Vector(0, 0, -1))
                            # here is a better way to find the perpendicular
                            if sweepflag == 1:
                                # clockwise
                                perp = DraftVecUtils.rotate2D(chord,
                                                              -math.pi/2)
                            else:
                                # anticlockwise
                                perp = DraftVecUtils.rotate2D(chord, math.pi/2)
                            chord.multiply(0.5)
                            if chord.Length > rx:
                                a = 0
                            else:
                                a = math.sqrt(rx**2 - chord.Length**2)
                            s = rx - a
                            perp.multiply(s/perp.Length)
                            midpoint = lastvec.add(chord.add(perp))
                            _seg = Part.Arc(lastvec, midpoint, currentvec)
                            seg = _seg.toShape()
                        # big arc or elliptical arc
                        else:
                            # Calculate the possible centers for an arc
                            # in 'endpoint parameterization'.
                            _xrot = math.radians(-xrotation)
                            (solution,
                             (rx, ry)) = arcend2center(lastvec,
                                                       currentvec,
                                                       rx, ry,
                                                       xrotation=_xrot,
                                                       correction=True)
                            # Chose one of the two solutions
                            negsol = (largeflag != sweepflag)
                            vcenter, angle1, angledelta = solution[negsol]
                            # print(angle1)
                            # print(angledelta)
                            if ry > rx:
                                rx, ry = ry, rx
                                swapaxis = True
                            else:
                                swapaxis = False
                            # print('Elliptical arc %s rx=%f ry=%f'
                            #       % (vcenter, rx, ry))
                            e1 = Part.Ellipse(vcenter, rx, ry)
                            if sweepflag:
                                # Step4
                                # angledelta = -(-angledelta % (2*math.pi))
                                # angledelta = (-angledelta % (2*math.pi))
                                angle1 = angle1 + angledelta
                                angledelta = -angledelta
                                # angle1 = math.pi - angle1

                            d90 = math.radians(90)
                            e1a = Part.Arc(e1,
                                           angle1 - swapaxis * d90,
                                           angle1 + angledelta
                                                  - swapaxis * d90)
                            # e1a = Part.Arc(e1,
                            #                angle1 - 0 * swapaxis * d90,
                            #                angle1 + angledelta
                            #                       - 0 * swapaxis * d90)
                            seg = e1a.toShape()
                            if swapaxis:
                                seg.rotate(vcenter, Vector(0, 0, 1), 90)
                            _precision = 10**(-1*Draft.precision())
                            if abs(xrotation) > _precision:
                                seg.rotate(vcenter, Vector(0, 0, 1), -xrotation)
                            if sweepflag:
                                seg.reverse()
                                # DEBUG
                                # obj = self.doc.addObject("Part::Feature",
                                #                       'DEBUG %s' % pathname)
                                # obj.Shape = seg
                                # _seg = Part.LineSegment(lastvec, currentvec)
                                # seg = _seg.toShape()
                        lastvec = currentvec
                        lastpole = None
                        path.append(seg)
                elif (d == "C" or d == "c") or (d == "S" or d == "s"):
                    smooth = (d == 'S' or d == 's')
                    if smooth:
                        piter = list(zip(pointlist[2::4],
                                         pointlist[3::4],
                                         pointlist[0::4],
                                         pointlist[1::4],
                                         pointlist[2::4],
                                         pointlist[3::4]))
                    else:
                        piter = list(zip(pointlist[0::6],
                                         pointlist[1::6],
                                         pointlist[2::6],
                                         pointlist[3::6],
                                         pointlist[4::6],
                                         pointlist[5::6]))
                    for p1x, p1y, p2x, p2y, x, y in piter:
                        if smooth:
                            if lastpole is not None and lastpole[0] == 'cubic':
                                pole1 = lastvec.sub(lastpole[1]).add(lastvec)
                            else:
                                pole1 = lastvec
                        else:
                            if relative:
                                pole1 = lastvec.add(Vector(p1x, -p1y, 0))
                            else:
                                pole1 = Vector(p1x, -p1y, 0)
                        if relative:
                            currentvec = lastvec.add(Vector(x, -y, 0))
                            pole2 = lastvec.add(Vector(p2x, -p2y, 0))
                        else:
                            currentvec = Vector(x, -y, 0)
                            pole2 = Vector(p2x, -p2y, 0)

                        if not DraftVecUtils.equals(currentvec, lastvec):
                            # mainv = currentvec.sub(lastvec)
                            # pole1v = lastvec.add(pole1)
                            # pole2v = currentvec.add(pole2)
                            # print("cubic curve data:",
                            #       mainv.normalize(),
                            #       pole1v.normalize(),
                            #       pole2v.normalize())
                            _precision = 10**(-1*(2+Draft.precision()))
                            _d1 = pole1.distanceToLine(lastvec, currentvec)
                            _d2 = pole2.distanceToLine(lastvec, currentvec)
                            if True and \
                                    _d1 < _precision and \
                                    _d2 < _precision:
                                # print("straight segment")
                                _seg = Part.LineSegment(lastvec, currentvec)
                                seg = _seg.toShape()
                            else:
                                # print("cubic bezier segment")
                                b = Part.BezierCurve()
                                b.setPoles([lastvec, pole1, pole2, currentvec])
                                seg = b.toShape()
                            # print("connect ", lastvec, currentvec)
                            lastvec = currentvec
                            lastpole = ('cubic', pole2)
                            path.append(seg)
                elif (d == "Q" or d == "q") or (d == "T" or d == "t"):
                    smooth = (d == 'T' or d == 't')
                    if smooth:
                        piter = list(zip(pointlist[1::2],
                                         pointlist[1::2],
                                         pointlist[0::2],
                                         pointlist[1::2]))
                    else:
                        piter = list(zip(pointlist[0::4],
                                         pointlist[1::4],
                                         pointlist[2::4],
                                         pointlist[3::4]))
                    for px, py, x, y in piter:
                        if smooth:
                            if (lastpole is not None
                                    and lastpole[0] == 'quadratic'):
                                pole = lastvec.sub(lastpole[1]).add(lastvec)
                            else:
                                pole = lastvec
                        else:
                            if relative:
                                pole = lastvec.add(Vector(px, -py, 0))
                            else:
                                pole = Vector(px, -py, 0)
                        if relative:
                            currentvec = lastvec.add(Vector(x, -y, 0))
                        else:
                            currentvec = Vector(x, -y, 0)

                        if not DraftVecUtils.equals(currentvec, lastvec):
                            _precision = 20**(-1*(2+Draft.precision()))
                            _distance = pole.distanceToLine(lastvec,
                                                            currentvec)
                            if True and \
                                    _distance < _precision:
                                # print("straight segment")
                                _seg = Part.LineSegment(lastvec, currentvec)
                                seg = _seg.toShape()
                            else:
                                # print("quadratic bezier segment")
                                b = Part.BezierCurve()
                                b.setPoles([lastvec, pole, currentvec])
                                seg = b.toShape()
                            # print("connect ", lastvec, currentvec)
                            lastvec = currentvec
                            lastpole = ('quadratic', pole)
                            path.append(seg)
                elif (d == "Z") or (d == "z"):
                    if not DraftVecUtils.equals(lastvec, firstvec):
                        try:
                            seg = Part.LineSegment(lastvec, firstvec).toShape()
                        except Part.OCCError:
                            pass
                        else:
                            path.append(seg)
                    if path:
                        # The path should be closed by now
                        # sh = makewire(path, True)
                        sh = makewire(path, donttry=False)
                        if self.fill \
                                and len(sh.Wires) == 1 \
                                and sh.Wires[0].isClosed():
                            sh = Part.Face(sh)
                            if sh.isValid() is False:
                                sh.fix(1e-6, 0, 1)
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature", pathname)
                        obj.Shape = sh
                        self.format(obj)
                        path = []
                        if firstvec:
                            # Move relative to recent draw command
                            lastvec = firstvec
                        point = []
                        # command = None
                        if self.currentsymbol:
                            self.symbols[self.currentsymbol].append(obj)
            if path:
                sh = makewire(path, checkclosed=False)
                # sh = Part.Wire(path)
                if self.fill and sh.isClosed():
                    sh = Part.Face(sh)
                    if sh.isValid() is False:
                        sh.fix(1e-6, 0, 1)
                sh = self.applyTrans(sh)
                obj = self.doc.addObject("Part::Feature", pathname)
                obj.Shape = sh
                self.format(obj)
                if self.currentsymbol:
                    self.symbols[self.currentsymbol].append(obj)
        # end process paths

        # Process rects
        if name == "rect":
            if not pathname:
                pathname = 'Rectangle'
            edges = []
            if "x" not in data:
                data["x"] = 0
            if "y" not in data:
                data["y"] = 0
            # Negative values are invalid
            _precision = 10**(-1*Draft.precision())
            if ('rx' not in data or data['rx'] < _precision) \
                    and ('ry' not in data or data['ry'] < _precision):
                # if True:
                p1 = Vector(data['x'],
                            -data['y'],
                            0)
                p2 = Vector(data['x'] + data['width'],
                            -data['y'],
                            0)
                p3 = Vector(data['x'] + data['width'],
                            -data['y'] - data['height'],
                            0)
                p4 = Vector(data['x'],
                            -data['y'] - data['height'],
                            0)
                edges.append(Part.LineSegment(p1, p2).toShape())
                edges.append(Part.LineSegment(p2, p3).toShape())
                edges.append(Part.LineSegment(p3, p4).toShape())
                edges.append(Part.LineSegment(p4, p1).toShape())
            else:
                # rounded edges
                rx = data.get('rx')
                ry = data.get('ry') or rx
                rx = rx or ry
                if rx > 2 * data['width']:
                    rx = data['width'] / 2.0
                if ry > 2 * data['height']:
                    ry = data['height'] / 2.0

                p1 = Vector(data['x'] + rx,
                            -data['y'] - data['height'] + ry,
                            0)
                p2 = Vector(data['x'] + data['width'] - rx,
                            -data['y'] - data['height'] + ry,
                            0)
                p3 = Vector(data['x'] + data['width'] - rx,
                            -data['y'] - ry,
                            0)
                p4 = Vector(data['x'] + rx,
                            -data['y'] - ry,
                            0)

                if rx < 0 or ry < 0:
                    _wrn("Warning: 'rx' or 'ry' is negative, "
                         "check the SVG file")

                if rx >= ry:
                    e = Part.Ellipse(Vector(), rx, ry)
                    e1a = Part.Arc(e, math.radians(180), math.radians(270))
                    e2a = Part.Arc(e, math.radians(270), math.radians(360))
                    e3a = Part.Arc(e, math.radians(0), math.radians(90))
                    e4a = Part.Arc(e, math.radians(90), math.radians(180))
                    m = FreeCAD.Matrix()
                else:
                    e = Part.Ellipse(Vector(), ry, rx)
                    e1a = Part.Arc(e, math.radians(90), math.radians(180))
                    e2a = Part.Arc(e, math.radians(180), math.radians(270))
                    e3a = Part.Arc(e, math.radians(270), math.radians(360))
                    e4a = Part.Arc(e, math.radians(0), math.radians(90))
                    # rotate +90 degrees
                    m = FreeCAD.Matrix(0, -1, 0, 0, 1, 0)
                esh = []
                for arc, point in ((e1a, p1), (e2a, p2),
                                   (e3a, p3), (e4a, p4)):
                    m1 = FreeCAD.Matrix(m)
                    m1.move(point)
                    arc.transform(m1)
                    esh.append(arc.toShape())
                for esh1, esh2 in zip(esh[-1:] + esh[:-1], esh):
                    p1 = esh1.Vertexes[-1].Point
                    p2 = esh2.Vertexes[0].Point
                    if not DraftVecUtils.equals(p1, p2):
                        # straight segments
                        _sh = Part.LineSegment(p1, p2).toShape()
                        edges.append(_sh)
                    # elliptical segments
                    edges.append(esh2)
            sh = Part.Wire(edges)
            if self.fill:
                sh = Part.Face(sh)
            sh = self.applyTrans(sh)
            obj = self.doc.addObject("Part::Feature", pathname)
            obj.Shape = sh
            self.format(obj)
            if self.currentsymbol:
                self.symbols[self.currentsymbol].append(obj)

        # Process lines
        if name == "line":
            if not pathname:
                pathname = 'Line'
            p1 = Vector(data['x1'], -data['y1'], 0)
            p2 = Vector(data['x2'], -data['y2'], 0)
            sh = Part.LineSegment(p1, p2).toShape()
            sh = self.applyTrans(sh)
            obj = self.doc.addObject("Part::Feature", pathname)
            obj.Shape = sh
            self.format(obj)
            if self.currentsymbol:
                self.symbols[self.currentsymbol].append(obj)

        # Process polylines and polygons
        if name == "polyline" or name == "polygon":
            # A simpler implementation would be
            # _p = zip(points[0::2], points[1::2])
            # sh = Part.makePolygon([Vector(svgx,
            #                               -svgy,
            #                               0) for svgx, svgy in _p])
            #
            # but it would be more difficult to search for duplicate
            # points beforehand.
            if not pathname:
                pathname = 'Polyline'
            points = [float(d) for d in data['points']]
            _msg('points {}'.format(points))
            lenpoints = len(points)
            if lenpoints >= 4 and lenpoints % 2 == 0:
                lastvec = Vector(points[0], -points[1], 0)
                path = []
                if name == 'polygon':
                    points = points + points[:2]  # emulate closepath
                for svgx, svgy in zip(points[2::2], points[3::2]):
                    currentvec = Vector(svgx, -svgy, 0)
                    if not DraftVecUtils.equals(lastvec, currentvec):
                        seg = Part.LineSegment(lastvec, currentvec).toShape()
                        # print("polyline seg ", lastvec, currentvec)
                        lastvec = currentvec
                        path.append(seg)
                if path:
                    sh = Part.Wire(path)
                    if self.fill and sh.isClosed():
                        sh = Part.Face(sh)
                    sh = self.applyTrans(sh)
                    obj = self.doc.addObject("Part::Feature", pathname)
                    obj.Shape = sh
                    self.format(obj)
                    if self.currentsymbol:
                        self.symbols[self.currentsymbol].append(obj)

        # Process ellipses
        if name == "ellipse":
            if not pathname:
                pathname = 'Ellipse'
            c = Vector(data.get('cx', 0), -data.get('cy', 0), 0)
            rx = data['rx']
            ry = data['ry']

            if rx < 0 or ry < 0:
                _wrn("Warning: 'rx' or 'ry' is negative, check the SVG file")

            if rx > ry:
                sh = Part.Ellipse(c, rx, ry).toShape()
            else:
                sh = Part.Ellipse(c, ry, rx).toShape()
                sh.rotate(c, Vector(0, 0, 1), 90)
            if self.fill:
                sh = Part.Wire([sh])
                sh = Part.Face(sh)
            sh = self.applyTrans(sh)
            obj = self.doc.addObject("Part::Feature", pathname)
            obj.Shape = sh
            self.format(obj)
            if self.currentsymbol:
                self.symbols[self.currentsymbol].append(obj)

        # Process circles
        if name == "circle" and "freecad:skip" not in data:
            if not pathname:
                pathname = 'Circle'
            c = Vector(data.get('cx', 0), -data.get('cy', 0), 0)
            r = data['r']
            sh = Part.makeCircle(r)
            if self.fill:
                sh = Part.Wire([sh])
                sh = Part.Face(sh)
            sh.translate(c)
            sh = self.applyTrans(sh)
            obj = self.doc.addObject("Part::Feature", pathname)
            obj.Shape = sh
            self.format(obj)
            if self.currentsymbol:
                self.symbols[self.currentsymbol].append(obj)

        # Process texts
        if name in ["text", "tspan"]:
            if "freecad:skip" not in data:
                _msg("processing a text")
                if 'x' in data:
                    self.x = data['x']
                else:
                    self.x = 0
                if 'y' in data:
                    self.y = data['y']
                else:
                    self.y = 0
                if 'font-size' in data:
                    if data['font-size'] != 'none':
                        self.text = getsize(data['font-size'],
                                            'css' + str(self.svgdpi))
                else:
                    self.text = 1
            else:
                if self.lastdim:
                    _font_size = int(getsize(data['font-size']))
                    self.lastdim.ViewObject.FontSize = _font_size

        # Process symbols
        if name == "symbol":
            self.symbols[pathname] = []
            self.currentsymbol = pathname

        if name == "use":
            if "xlink:href" in data:
                symbol = data["xlink:href"][0][1:]
                if symbol in self.symbols:
                    _msg("using symbol " + symbol)
                    shapes = []
                    for o in self.symbols[symbol]:
                        if o.isDerivedFrom("Part::Feature"):
                            shapes.append(o.Shape)
                    if shapes:
                        sh = Part.makeCompound(shapes)
                        v = Vector(float(data['x']), -float(data['y']), 0)
                        sh.translate(v)
                        sh = self.applyTrans(sh)
                        obj = self.doc.addObject("Part::Feature", symbol)
                        obj.Shape = sh
                        self.format(obj)
                else:
                    _msg("no symbol data")

        _msg("done processing element {}".format(self.count))
    # startElement()

    def characters(self, content):
        """Read characters from the given string."""
        if self.text:
            _msg("reading characters {}".format(content))
            obj = self.doc.addObject("App::Annotation", 'Text')
            # use ignore to not break import if char is not found in latin1
            obj.LabelText = content.encode('latin1', 'ignore')
            if self.currentsymbol:
                self.symbols[self.currentsymbol].append(obj)
            vec = Vector(self.x, -self.y, 0)
            if self.transform:
                vec = self.translateVec(vec, self.transform)
                # print("own transform: ", self.transform, vec)
            for transform in self.grouptransform[::-1]:
                # vec = self.translateVec(vec, transform)
                vec = transform.multiply(vec)
            # print("applying vector: ", vec)
            obj.Position = vec
            if FreeCAD.GuiUp:
                obj.ViewObject.FontSize = int(self.text)
                if self.fill:
                    obj.ViewObject.TextColor = self.fill
                else:
                    obj.ViewObject.TextColor = (0.0, 0.0, 0.0, 0.0)

    def endElement(self, name):
        """Finish processing the element indicated by the name.

        Parameters
        ----------
        name : str
            The name of the element
        """
        if name not in ["tspan"]:
            self.transform = None
            self.text = None
        if name == "g" or name == "svg":
            _msg("closing group")
            self.grouptransform.pop()
            if self.groupstyles:
                self.groupstyles.pop()
        if name == "symbol":
            if self.doc.getObject("svgsymbols"):
                group = self.doc.getObject("svgsymbols")
            else:
                group = self.doc.addObject("App::DocumentObjectGroup",
                                           "svgsymbols")
            for o in self.symbols[self.currentsymbol]:
                group.addObject(o)
            self.currentsymbol = None

    def applyTrans(self, sh):
        """Apply transformation to the shape and return the new shape.

        Parameters
        ----------
        sh : Part.Shape or Draft.Dimension
            Object to be transformed
        """
        if isinstance(sh, Part.Shape):
            if self.transform:
                _msg("applying object transform: {}".format(self.transform))
                # sh = transformCopyShape(sh, self.transform)
                # see issue #2062
                sh = sh.transformGeometry(self.transform)
            for transform in self.grouptransform[::-1]:
                _msg("applying group transform: {}".format(transform))
                # sh = transformCopyShape(sh, transform)
                # see issue #2062
                sh = sh.transformGeometry(transform)
            return sh
        elif Draft.getType(sh) in ["Dimension","LinearDimension"]:
            pts = []
            for p in [sh.Start, sh.End, sh.Dimline]:
                cp = Vector(p)
                if self.transform:
                    _msg("applying object transform: "
                         "{}".format(self.transform))
                    cp = self.transform.multiply(cp)
                for transform in self.grouptransform[::-1]:
                    _msg("applying group transform: {}".format(transform))
                    cp = transform.multiply(cp)
                pts.append(cp)
            sh.Start = pts[0]
            sh.End = pts[1]
            sh.Dimline = pts[2]

    def translateVec(self, vec, mat):
        """Translate (move) a point or vector by a matrix.

        Parameters
        ----------
        vec : Base::Vector3
            The original vector
        mat : Base::Matrix4D
            The translation matrix, from which only the elements 14, 24, 34
            are used.
        """
        v = Vector(mat.A14, mat.A24, mat.A34)
        return vec.add(v)

    def getMatrix(self, tr):
        """Return a FreeCAD matrix from an SVG transform attribute.

        Parameters
        ----------
        tr : str
            The type of transform: 'matrix', 'translate', 'scale',
            'rotate', 'skewX', 'skewY' and its value

        Returns
        -------
        Base::Matrix4D
            The translated matrix.
        """
        _op = '(matrix|translate|scale|rotate|skewX|skewY)'
        _val = '\((.*?)\)'
        _transf = _op + '\s*?' + _val
        transformre = re.compile(_transf, re.DOTALL)
        m = FreeCAD.Matrix()
        for transformation, arguments in transformre.findall(tr):
            _args_rep = arguments.replace(',', ' ').split()
            argsplit = [float(arg) for arg in _args_rep]
            # m.multiply(FreeCAD.Matrix(1, 0, 0, 0, 0, -1))
            # print('%s:%s %s %d' % (transformation, arguments,
            #                        argsplit, len(argsplit)))
            if transformation == 'translate':
                tx = argsplit[0]
                ty = argsplit[1] if len(argsplit) > 1 else 0.0
                m.move(Vector(tx, -ty, 0))
            elif transformation == 'scale':
                sx = argsplit[0]
                sy = argsplit[1] if len(argsplit) > 1 else sx
                m.scale(Vector(sx, sy, 1))
            elif transformation == 'rotate':
                cx = 0
                cy = 0
                angle = argsplit[0]
                if len(argsplit) >= 3:
                    # Rotate around a non-origin centerpoint (note: SVG y axis is opposite FreeCAD y axis)
                    cx = argsplit[1]
                    cy = argsplit[2]
                    m.move(Vector(-cx, cy, 0)) # Reposition for rotation
                # Mirroring one axis is equal to changing the direction
                # of rotation
                m.rotateZ(math.radians(-angle))
                if len(argsplit) >= 3:
                    m.move(Vector(cx, -cy, 0)) # Reverse repositioning
            elif transformation == 'skewX':
                _m = FreeCAD.Matrix(1,
                                    -math.tan(math.radians(argsplit[0])))
                m = m.multiply(_m)
            elif transformation == 'skewY':
                _m = FreeCAD.Matrix(1, 0, 0, 0,
                                    -math.tan(math.radians(argsplit[0])))
                m = m.multiply(_m)
            elif transformation == 'matrix':
                # transformation matrix:
                #    FreeCAD                 SVG
                # (+A -C +0 +E)           (A C 0 E)
                # (-B +D -0 -F)  = (-Y) * (B D 0 F) * (-Y)
                # (+0 -0 +1 +0)           (0 0 1 0)
                # (+0 -0 +0 +1)           (0 0 0 1)
                #
                # Put the first two rows of the matrix
                _m = FreeCAD.Matrix(argsplit[0], -argsplit[2],
                                    0, argsplit[4],
                                    -argsplit[1], argsplit[3],
                                    0, -argsplit[5])
                m = m.multiply(_m)
            # else:
            #    print('SKIPPED %s' % transformation)
            # print("m = ", m)
        # print("generating transformation: ", m)
        return m
    # getMatrix
# class svgHandler


def getContents(filename, tag, stringmode=False):
    """Get the contents of all occurrences of the given tag in the file.

    Parameters
    ----------
    filename : str
        A filename to scan for tags.
    tag : str
        An SVG tag to find inside a file, for example, `some`
        in <some id="12">information</some>
    stringmode : bool, optional
        The default is False.
        If False, `filename` is a path to a file.
        If True, `filename` is already a pointer to an open file.

    Returns
    -------
    dict
        A dictionary with tagids and the information associated with that id
        results[tagid] = information
    """
    result = {}
    if stringmode:
        contents = filename
    else:
        # Use the native Python open which was saved as `pythonopen`
        f = pythonopen(filename)
        contents = f.read()
        f.close()

    # Replace the newline character with a string
    # so that it's easiert to parse; later on the newline character
    # will be restored
    contents = contents.replace('\n', '_linebreak')
    searchpat = '<' + tag + '.*?</' + tag + '>'
    tags = re.findall(searchpat, contents)
    for t in tags:
        tagid = re.findall('id="(.*?)"', t)
        if tagid:
            tagid = tagid[0]
        else:
            tagid = 'none'
        res = t.replace('_linebreak', '\n')
        result[tagid] = res
    return result


def open(filename):
    """Open filename and parse using the svgHandler().

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.

    Returns
    -------
    App::Document
        The new FreeCAD document object created, with the parsed information.
    """
    docname = os.path.split(filename)[1]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname[:-4]

    # Set up the parser
    parser = xml.sax.make_parser()
    parser.setFeature(xml.sax.handler.feature_external_ges, False)
    parser.setContentHandler(svgHandler())
    parser._cont_handler.doc = doc

    # Use the native Python open which was saved as `pythonopen`
    f = pythonopen(filename)
    parser.parse(f)
    f.close()
    doc.recompute()
    return doc


def insert(filename, docname):
    """Get an active document and parse using the svgHandler().

    If no document exist, it is created.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.
    docname : str
        The name of the active App::Document if one exists, or
        of the new one created.

    Returns
    -------
    App::Document
        The active FreeCAD document, or the document created if none exists,
        with the parsed information.
    """
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    # Set up the parser
    parser = xml.sax.make_parser()
    parser.setFeature(xml.sax.handler.feature_external_ges, False)
    parser.setContentHandler(svgHandler())
    parser._cont_handler.doc = doc

    # Use the native Python open which was saved as `pythonopen`
    parser.parse(pythonopen(filename))
    doc.recompute()


def export(exportList, filename):
    """Export the SVG file with a given list of objects.

    The objects must be derived from Part::Feature, in order to be processed
    and exported.

    Parameters
    ----------
    exportList : list
        List of document objects to export.
    filename : str
        Path to the new file.

    Returns
    -------
    None
        If `exportList` doesn't have shapes to export.
    """
    _prefs = "User parameter:BaseApp/Preferences/Mod/Draft"
    svg_export_style = FreeCAD.ParamGet(_prefs).GetInt("svg_export_style")
    if svg_export_style != 0 and svg_export_style != 1:
        _msg(translate("ImportSVG",
                       "Unknown SVG export style, switching to Translated"))
        svg_export_style = 0

    # Determine the size of the page by adding the bounding boxes
    # of all shapes
    bb = FreeCAD.BoundBox()
    for obj in exportList:
        if (hasattr(obj, "Shape")
                and obj.Shape
                and obj.Shape.BoundBox.isValid()):
            bb.add(obj.Shape.BoundBox)
        else:
            # if Draft.get_type(obj) in ("Text", "LinearDimension", ...)
            _wrn("'{}': no Shape, "
                 "calculate manual bounding box".format(obj.Label))
            bb.add(Draft.get_bbox(obj))

    if not bb.isValid():
        _err(translate("ImportSVG",
                       "The export list contains no object "
                       "with a valid bounding box"))
        return

    minx = bb.XMin
    maxx = bb.XMax
    miny = bb.YMin
    maxy = bb.YMax

    if svg_export_style == 0:
        # translated-style exports get a bit of a margin
        margin = (maxx - minx) * 0.01
    else:
        # raw-style exports get no margin
        margin = 0

    minx -= margin
    maxx += margin
    miny -= margin
    maxy += margin
    sizex = maxx - minx
    sizey = maxy - miny
    miny += margin

    # Use the native Python open which was saved as `pythonopen`
    svg = pythonopen(filename, 'w')

    # Write header.
    # We specify the SVG width and height in FreeCAD's physical units (mm),
    # and specify the viewBox so that user units maps one-to-one to mm.
    svg.write('<?xml version="1.0"?>\n')
    svg.write('<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"')
    svg.write(' "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">\n')
    svg.write('<svg')
    svg.write(' width="' + str(sizex) + 'mm" height="' + str(sizey) + 'mm"')
    if svg_export_style == 0:
        # translated-style exports have the viewbox starting at X=0, Y=0
        svg.write(' viewBox="0 0 ' + str(sizex) + ' ' + str(sizey) + '"')
    else:
        # Raw-style exports have the viewbox starting at X=xmin, Y=-ymax
        # We need the negative Y here because SVG is upside down, and we
        # flip the sketch right-way up with a scale later
        svg.write(' viewBox="%f %f %f %f"' % (minx, -maxy, sizex, sizey))

    svg.write(' xmlns="http://www.w3.org/2000/svg" version="1.1"')
    svg.write('>\n')

    # Write paths
    for ob in exportList:
        if svg_export_style == 0:
            # translated-style exports have the entire sketch translated
            # to fit in the X>0, Y>0 quadrant
            # svg.write('<g transform="translate('
            #           + str(-minx) + ',' + str(-miny + 2*margin)
            #           + ') scale(1,-1)">\n')
            svg.write('<g id="%s" transform="translate(%f,%f) '
                      'scale(1,-1)">\n' % (ob.Name, -minx, maxy))
        else:
            # raw-style exports do not translate the sketch
            svg.write('<g id="%s" transform="scale(1,-1)">\n' % ob.Name)

        svg.write(Draft.get_svg(ob))
        _label_enc = str(ob.Label.encode('utf8'))
        _label = _label_enc.replace('<', '&lt;').replace('>', '&gt;')
        # replace('"', "&quot;")
        svg.write('<title>%s</title>\n' % _label)
        svg.write('</g>\n')

    # Close the file
    svg.write('</svg>')
    svg.close()
