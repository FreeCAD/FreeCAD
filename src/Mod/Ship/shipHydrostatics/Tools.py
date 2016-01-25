#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2016                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import math
import random
from FreeCAD import Vector, Rotation, Matrix, Placement
import Part
import Units
import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtGui, QtCore
import Instance
from shipUtils import Math
import shipUtils.Units as USys


DENS = Units.parseQuantity("1025 kg/m^3")  # Salt water
COMMON_BOOLEAN_ITERATIONS = 10


def placeShipShape(shape, draft, roll, trim):
    """Move the ship shape such that the free surface matches with the plane
    z=0. The transformation will be applied on the input shape, so copy it
    before calling this method if it should be preserved.

    Position arguments:
    shape -- Ship shape
    draft -- Ship draft
    roll -- Roll angle
    trim -- Trim angle

    Returned values:
    shape -- The same transformed input shape. Just for debugging purposes, you
    can discard it.
    base_z -- The new base z coordinate (after applying the roll angle). Useful
    if you want to revert back the transformation
    """
    # Roll the ship. In order to can deal with large roll angles, we are
    # proceeding as follows:
    # 1.- Applying the roll with respect the base line
    # 2.- Recentering the ship in the y direction
    # 3.- Readjusting the base line
    shape.rotate(Vector(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), roll)
    base_z = shape.BoundBox.ZMin
    shape.translate(Vector(0.0, draft * math.sin(math.radians(roll)), -base_z))
    # Trim the ship. In this case we only need to correct the x direction
    shape.rotate(Vector(0.0, 0.0, 0.0), Vector(0.0, -1.0, 0.0), trim)
    shape.translate(Vector(draft * math.sin(math.radians(trim)), 0.0, 0.0))
    shape.translate(Vector(0.0, 0.0, -draft))

    return shape, base_z


def getUnderwaterSide(shape, force=True):
    """Get the underwater shape, simply cropping the provided shape by the z=0
    free surface plane.

    Position arguments:
    shape -- Solid shape to be cropped

    Keyword arguments:
    force -- True if in case the common boolean operation fails, i.e. returns
    no solids, the tool should retry it slightly moving the free surface. False
    otherwise. (True by default)

    Returned value:
    Cropped shape. It is not modifying the input shape
    """
    # Convert the shape into an active object
    Part.show(shape)
    orig = App.ActiveDocument.Objects[-1]

    bbox = shape.BoundBox
    xmin = bbox.XMin
    xmax = bbox.XMax
    ymin = bbox.YMin
    ymax = bbox.YMax
    zmin = bbox.ZMin
    zmax = bbox.ZMax

    # Create the "sea" box to intersect the ship
    L = xmax - xmin
    B = ymax - ymin
    H = zmax - zmin

    box = App.ActiveDocument.addObject("Part::Box","Box")
    length_format = USys.getLengthFormat()
    box.Placement = Placement(Vector(xmin - L, ymin - B, zmin - H),
                              Rotation(App.Vector(0,0,1),0))
    box.Length = length_format.format(3.0 * L)
    box.Width = length_format.format(3.0 * B)
    box.Height = length_format.format(- zmin + H)

    App.ActiveDocument.recompute()
    common = App.activeDocument().addObject("Part::MultiCommon",
                                            "UnderwaterSideHelper")
    common.Shapes = [orig, box]
    App.ActiveDocument.recompute()
    if force and len(common.Shape.Solids) == 0:
        # The common operation is failing, let's try moving a bit the free
        # surface
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Boolean operation failed when trying to get the underwater side."
            " The tool is retrying such operation slightly moving the free"
            " surface position",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintWarning(msg + '\n')
        random_bounds = 0.01 * H
        i = 0
        while len(common.Shape.Solids) == 0 and i < COMMON_BOOLEAN_ITERATIONS:
            i += 1
            box.Height = length_format.format(
                - zmin + H + random.uniform(-random_bounds, random_bounds))
            App.ActiveDocument.recompute() 

    out = common.Shape
    App.ActiveDocument.removeObject(common.Name)
    App.ActiveDocument.removeObject(orig.Name)
    App.ActiveDocument.removeObject(box.Name)
    App.ActiveDocument.recompute()
    return out


def areas(ship, n, draft=None,
                   roll=Units.parseQuantity("0 deg"), 
                   trim=Units.parseQuantity("0 deg")):
    """Compute the ship transversal areas

    Position arguments:
    ship -- Ship object (see createShip)
    n -- Number of points to compute

    Keyword arguments:
    draft -- Ship draft (Design ship draft by default)
    roll -- Roll angle (0 degrees by default)
    trim -- Trim angle (0 degrees by default)

    Returned value:
    List of sections, each section contains 2 values, the x longitudinal
    coordinate, and the transversal area. If n < 2, an empty list will be
    returned.
    """
    if n < 2:
        return []

    if draft is None:
        draft = ship.Draft

    shape, _ = placeShipShape(ship.Shape.copy(), draft, roll, trim)
    shape = getUnderwaterSide(shape)

    # Sections distance computation
    bbox = shape.BoundBox
    xmin = bbox.XMin
    xmax = bbox.XMax
    dx = (xmax - xmin) / (n - 1.0)

    # Since we are computing the sections in the total length (not in the
    # length between perpendiculars), we can grant that the starting and
    # ending sections have null area
    areas = [(Units.Quantity(xmin, Units.Length),
              Units.Quantity(0.0, Units.Area))]
    # And since we just need to compute areas we will create boxes with its
    # front face at the desired transversal area position, computing the
    # common solid part, dividing it by faces, and getting only the desired
    # ones.
    App.Console.PrintMessage("Computing transversal areas...\n")
    App.Console.PrintMessage("Some Inventor representation errors can be"
                             " shown, please ignore them.\n")
    for i in range(1, n - 1):
        App.Console.PrintMessage("{0} / {1}\n".format(i, n - 2))
        x = xmin + i * dx
        try:
            f = Part.Face(shape.slice(Vector(1,0,0), x))
        except Part.OCCError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Part.OCCError: Transversal area computation failed",
                None,
                QtGui.QApplication.UnicodeUTF8)
            App.Console.PrintError(msg + '\n')
            areas.append((Units.Quantity(x, Units.Length),
                          Units.Quantity(0.0, Units.Area)))
            continue
        # It is a valid face, so we can add this area
        areas.append((Units.Quantity(x, Units.Length),
                      Units.Quantity(f.Area, Units.Area)))
    # Last area is equal to zero (due to the total length usage)
    areas.append((Units.Quantity(xmax, Units.Length),
                  Units.Quantity(0.0, Units.Area)))
    App.Console.PrintMessage("Done!\n")
    return areas


def displacement(ship, draft=None,
                       roll=Units.parseQuantity("0 deg"), 
                       trim=Units.parseQuantity("0 deg")):
    """Compute the ship displacement

    Position arguments:
    ship -- Ship object (see createShip)

    Keyword arguments:
    draft -- Ship draft (Design ship draft by default)
    roll -- Roll angle (0 degrees by default)
    trim -- Trim angle (0 degrees by default)

    Returned values:
    disp -- The ship displacement (a density of the water of 1025 kg/m^3 is
    assumed)
    B -- Bouyance application point, i.e. Center of mass of the underwater side
    Cb -- Block coefficient

    The Bouyance center is refered to the original ship position.
    """
    if draft is None:
        draft = ship.Draft

    shape, base_z = placeShipShape(ship.Shape.copy(), draft, roll, trim)
    shape = getUnderwaterSide(shape)

    vol = 0.0
    cog = Vector()
    if len(shape.Solids) > 0:
        for solid in shape.Solids:
            vol += solid.Volume
            sCoG = solid.CenterOfMass
            cog.x = cog.x + sCoG.x * solid.Volume
            cog.y = cog.y + sCoG.y * solid.Volume
            cog.z = cog.z + sCoG.z * solid.Volume
        cog.x = cog.x / vol
        cog.y = cog.y / vol
        cog.z = cog.z / vol

    bbox = shape.BoundBox
    Vol = (bbox.XMax - bbox.XMin) * (bbox.YMax - bbox.YMin) * abs(bbox.ZMin)

    # Undo the transformations on the bouyance point
    B = Part.Point(Vector(cog.x, cog.y, cog.z))
    m = Matrix()
    m.move(Vector(0.0, 0.0, draft))
    m.move(Vector(-draft * math.sin(trim.getValueAs("rad")), 0.0, 0.0))
    m.rotateY(trim.getValueAs("rad"))
    m.move(Vector(0.0,
                  -draft * math.sin(roll.getValueAs("rad")),
                  base_z))
    m.rotateX(-roll.getValueAs("rad"))
    B.transform(m)

    try:
        cb = vol / Vol
    except ZeroDivisionError:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "ZeroDivisionError: Null volume found during the displacement"
            " computation!",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        cb = 0.0


    # Return the computed data
    return (DENS * Units.Quantity(vol, Units.Volume),
            Vector(B.X, B.Y, B.Z),
            cb)


def wettedArea(shape, draft, roll=Units.parseQuantity("0 deg"),
                             trim=Units.parseQuantity("0 deg")):
    """Compute the ship wetted area

    Position arguments:
    shape -- External faces of the ship hull
    draft -- Ship draft

    Keyword arguments:
    roll -- Roll angle (0 degrees by default)
    trim -- Trim angle (0 degrees by default)

    Returned value:
    The wetted area, i.e. The underwater side area
    """
    shape, _ = placeShipShape(shape.copy(), draft, roll, trim)
    shape = getUnderwaterSide(shape, force=False)

    area = 0.0
    for f in shape.Faces:
        area = area + f.Area
    return Units.Quantity(area, Units.Area)


def moment(ship, draft=None,
                 roll=Units.parseQuantity("0 deg"), 
                 trim=Units.parseQuantity("0 deg")):
    """Compute the moment required to trim the ship 1cm

    Position arguments:
    ship -- Ship object (see createShip)

    Keyword arguments:
    draft -- Ship draft (Design ship draft by default)
    roll -- Roll angle (0 degrees by default)
    trim -- Trim angle (0 degrees by default)

    Returned value:
    Moment required to trim the ship 1cm. Such moment is positive if it cause a
    positive trim angle. The moment is expressed as a mass by a distance, not as
    a force by a distance
    """
    disp_orig, B_orig, _ = displacement(ship, draft, roll, trim)
    xcb_orig = Units.Quantity(B_orig.x, Units.Length)

    factor = 10.0
    x = 0.5 * ship.Length.getValueAs('cm').Value
    y = 1.0
    angle = math.atan2(y, x) * Units.Radian
    trim_new = trim + factor * angle
    disp_new, B_new, _ = displacement(ship, draft, roll, trim_new)
    xcb_new = Units.Quantity(B_new.x, Units.Length)

    mom0 = -disp_orig * xcb_orig
    mom1 = -disp_new * xcb_new
    return (mom1 - mom0) / factor


def floatingArea(ship, draft=None,
                       roll=Units.parseQuantity("0 deg"), 
                       trim=Units.parseQuantity("0 deg")):
    """Compute the ship floating area

    Position arguments:
    ship -- Ship object (see createShip)

    Keyword arguments:
    draft -- Ship draft (Design ship draft by default)
    roll -- Roll angle (0 degrees by default)
    trim -- Trim angle (0 degrees by default)

    Returned values:
    area -- Ship floating area
    cf -- Floating area coefficient
    """
    if draft is None:
        draft = ship.Draft

    # We wanna intersect the whole ship with the free surface, so in this case
    # we must not use the underwater side (or the tool will fail)
    shape, _ = placeShipShape(ship.Shape.copy(), draft, roll, trim)

    try:
        f = Part.Face(shape.slice(Vector(0,0,1), 0.0))
        area = Units.Quantity(f.Area, Units.Area)
    except Part.OCCError:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Part.OCCError: Floating area cannot be computed",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        area = Units.Quantity(0.0, Units.Area)

    bbox = shape.BoundBox
    Area = (bbox.XMax - bbox.XMin) * (bbox.YMax - bbox.YMin)
    try:
        cf = area.Value / Area
    except ZeroDivisionError:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "ZeroDivisionError: Null area found during the floating area"
            " computation!",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        cf = 0.0

    return area, cf


def BMT(ship, draft=None, trim=Units.parseQuantity("0 deg")):
    """Calculate "ship Bouyance center" - "transversal metacenter" radius

    Position arguments:
    ship -- Ship object (see createShip)

    Keyword arguments:
    draft -- Ship draft (Design ship draft by default)
    trim -- Trim angle (0 degrees by default)

    Returned value:
    BMT radius
    """
    if draft is None:
        draft = ship.Draft

    roll = Units.parseQuantity("0 deg")
    _, B0, _ = displacement(ship, draft, roll, trim)


    nRoll = 2
    maxRoll = Units.parseQuantity("7 deg")

    BM = 0.0
    for i in range(nRoll):
        roll = (maxRoll / nRoll) * (i + 1)
        _, B1, _ = displacement(ship, draft, roll, trim)
        #     * M
        #    / \
        #   /   \  BM     ==|>   BM = (BB/2) / sin(alpha/2)
        #  /     \
        # *-------*
        #     BB
        BB = B1 - B0
        BB.x = 0.0
        # nRoll is actually representing the weight function
        BM += 0.5 * BB.Length / math.sin(math.radians(0.5 * roll)) / nRoll
    return Units.Quantity(BM, Units.Length)


def mainFrameCoeff(ship, draft=None):
    """Compute the main frame coefficient

    Position arguments:
    ship -- Ship object (see createShip)

    Keyword arguments:
    draft -- Ship draft (Design ship draft by default)

    Returned value:
    Ship main frame area coefficient
    """
    if draft is None:
        draft = ship.Draft

    shape, _ = placeShipShape(ship.Shape.copy(), draft,
                              Units.parseQuantity("0 deg"),
                              Units.parseQuantity("0 deg"))
    shape = getUnderwaterSide(shape)

    try:
        f = Part.Face(shape.slice(Vector(1,0,0), 0.0))
        area = f.Area
    except Part.OCCError:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "Part.OCCError: Main frame area cannot be computed",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        area = 0.0

    bbox = shape.BoundBox
    Area = (bbox.YMax - bbox.YMin) * (bbox.ZMax - bbox.ZMin)

    try:
        cm = area / Area
    except ZeroDivisionError:
        msg = QtGui.QApplication.translate(
            "ship_console",
            "ZeroDivisionError: Null area found during the main frame area"
            " coefficient computation!",
            None,
            QtGui.QApplication.UnicodeUTF8)
        App.Console.PrintError(msg + '\n')
        cm = 0.0

    return cm


class Point:
    """Hydrostatics point, that contains the following members:

    draft -- Ship draft
    trim -- Ship trim
    disp -- Ship displacement
    xcb -- Bouyance center X coordinate
    wet -- Wetted ship area
    mom -- Triming 1cm ship moment
    farea -- Floating area
    KBt -- Transversal KB height
    BMt -- Transversal BM height
    Cb -- Block coefficient.
    Cf -- Floating coefficient.
    Cm -- Main frame coefficient.

    The moment to trim the ship 1 cm is positive when is resulting in a positive
    trim angle.
    """
    def __init__(self, ship, faces, draft, trim):
        """Compute all the hydrostatics.

        Position argument:
        ship -- Ship instance
        faces -- Ship external faces
        draft -- Ship draft
        trim -- Trim angle
        """
        disp, B, cb = displacement(ship, draft=draft, trim=trim)
        if not faces:
            wet = 0.0
        else:
            wet = wettedArea(faces, draft=draft, trim=trim)
        mom = moment(ship, draft=draft, trim=trim)
        farea, cf = floatingArea(ship, draft=draft, trim=trim)
        bm = BMT(ship, draft=draft, trim=trim)
        cm = mainFrameCoeff(ship, draft=draft)
        # Store final data
        self.draft = draft
        self.trim = trim
        self.disp = disp
        self.xcb = Units.Quantity(B.x, Units.Length)
        self.wet = wet
        self.farea = farea
        self.mom = mom
        self.KBt = Units.Quantity(B.z, Units.Length)
        self.BMt = bm
        self.Cb = cb
        self.Cf = cf
        self.Cm = cm