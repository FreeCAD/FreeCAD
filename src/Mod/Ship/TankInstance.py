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

import time
from math import *
from PySide import QtGui, QtCore
import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Base, Vector, Matrix, Placement, Rotation
import Part
import Units
from shipUtils import Paths, Math
import shipUtils.Units as USys


COMMON_BOOLEAN_ITERATIONS = 10


class Tank:
    def __init__(self, obj, shapes, ship):
        """ Transform a generic object to a ship instance.

        Keyword arguments:
        obj -- Part::FeaturePython created object which should be transformed
        in a weight instance.
        shapes -- Set of solid shapes which will compound the tank.
        ship -- Ship where the tank is allocated.
        """
        # Add an unique property to identify the Weight instances
        tooltip = unicode(QtGui.QApplication.translate(
            "ship_tank",
            "True if it is a valid tank instance, False otherwise",
            None))
        obj.addProperty("App::PropertyBool",
                        "IsTank",
                        "Tank",
                        tooltip).IsTank = True
        # Set the subshapes
        obj.Shape = Part.makeCompound(shapes)

        obj.Proxy = self

    def onChanged(self, fp, prop):
        """Detects the ship data changes.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        prop -- Modified property name.
        """
        if prop == "Vol":
            pass

    def execute(self, fp):
        """Detects the entity recomputations.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        """
        pass

    def getVolume(self, fp, level, return_shape=False):
        """Return the fluid volume inside the tank, provided the filling level.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        level -- Percentage of filling level (interval [0, 1]).
        return_shape -- False if the tool should return the fluid volume value,
        True if the tool should return the volume shape.
        """
        if level <= 0.0:
            if return_shape:
                return Part.Vertex()
            return Units.Quantity(0.0, Units.Volume)
        if level >= 1.0:
            if return_shape:
                return fp.Shape.copy()
            return Units.Quantity(fp.Shape.Volume, Units.Volume)

        # Build up the cutting box
        bbox = fp.Shape.BoundBox
        dx = bbox.XMax - bbox.XMin
        dy = bbox.YMax - bbox.YMin
        dz = bbox.ZMax - bbox.ZMin

        box = App.ActiveDocument.addObject("Part::Box","Box")
        length_format = USys.getLengthFormat()
        box.Placement = Placement(Vector(bbox.XMin - dx,
                                         bbox.YMin - dy,
                                         bbox.ZMin - dz),
                                  Rotation(App.Vector(0,0,1),0))
        box.Length = length_format.format(3.0 * dx)
        box.Width = length_format.format(3.0 * dy)
        box.Height = length_format.format((1.0 + level) * dz)

        # Create a new object on top of a copy of the tank shape
        Part.show(fp.Shape.copy())
        tank = App.ActiveDocument.Objects[-1]

        # Compute the common boolean operation
        App.ActiveDocument.recompute()
        common = App.activeDocument().addObject("Part::MultiCommon",
                                                "TankVolHelper")
        common.Shapes = [tank, box]
        App.ActiveDocument.recompute()
        if len(common.Shape.Solids) == 0:
            # The common operation is failing, let's try moving a bit the free
            # surface
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Tank volume operation failed. The tool is retrying that"
                " slightly moving the free surface position",
                None)
            App.Console.PrintWarning(msg + '\n')
            rand_bounds = 0.01 * dz
            i = 0
            while len(common.Shape.Solids) == 0 and i < COMMON_BOOLEAN_ITERATIONS:
                i += 1
                box.Height = length_format.format(
                    (1.0 + level) * dz + random.uniform(-random_bounds,
                                                        random_bounds))
                App.ActiveDocument.recompute()

        if return_shape:
            ret_value = common.Shape.copy()
        else:
            ret_value = Units.Quantity(common.Shape.Volume, Units.Volume)

        App.ActiveDocument.removeObject(common.Name)
        App.ActiveDocument.removeObject(tank.Name)
        App.ActiveDocument.removeObject(box.Name)
        App.ActiveDocument.recompute()

        return ret_value

    def getCoG(self, fp, vol, roll=Units.parseQuantity("0 deg"),
                              trim=Units.parseQuantity("0 deg")):
        """Return the fluid volume center of gravity, provided the volume of
        fluid inside the tank.

        The returned center of gravity is referred to the untransformed ship.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        vol -- Volume of fluid.
        roll -- Ship roll angle.
        trim -- Ship trim angle.

        If the fluid volume is bigger than the total tank one, it will be
        conveniently clamped.
        """
        # Change the units of the volume, and clamp the value
        if vol <= 0.0:
            return Vector()
        if vol >= fp.Shape.Volume:
            vol = 0.0
            for solid in fp.Shape.Solids:
                vol += solid.Volume
                sCoG = solid.CenterOfMass
                cog.x = cog.x + sCoG.x * solid.Volume
                cog.y = cog.y + sCoG.y * solid.Volume
                cog.z = cog.z + sCoG.z * solid.Volume
            cog.x = cog.x / vol
            cog.y = cog.y / vol
            cog.z = cog.z / vol
            return cog

        # Get a first estimation of the level
        level = vol.Value / fp.Shape.Volume

        # Transform the tank shape
        current_placement = fp.Placement
        m = current_placement.toMatrix()
        m.rotateX(roll.getValueAs("rad"))
        m.rotateY(-trim.getValueAs("rad"))
        fp.Placement = Placement(m)

        # Iterate to find the fluid shape
        for i in range(COMMON_BOOLEAN_ITERATIONS):
            shape = self.getVolume(fp, level, return_shape=True)
            error = (vol.Value - shape.Volume) / fp.Shape.Volume
            if abs(error) < 0.01:
                break
            level += error

        # Get the center of gravity
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

        # Untransform the object to retrieve the original position
        fp.Placement = current_placement
        p = Part.Point(cog)
        m = Matrix()
        m.rotateY(trim.getValueAs("rad"))
        m.rotateX(-roll.getValueAs("rad"))
        p.rotate(Placement(m))

        return Vector(p.X, p.Y, p.Z)


class ViewProviderTank:
    def __init__(self, obj):
        """Add this view provider to the selected object.

        Keyword arguments:
        obj -- Object which must be modified.
        """
        obj.Proxy = self

    def attach(self, obj):
        """Setup the scene sub-graph of the view provider, this method is
        mandatory.
        """
        return

    def updateData(self, fp, prop):
        """If a property of the handled feature has changed we have the chance
        to handle this here.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        prop -- Modified property name.
        """
        return

    def getDisplayModes(self, obj):
        """Return a list of display modes.

        Keyword arguments:
        obj -- Object associated with the view provider.
        """
        modes = []
        return modes

    def getDefaultDisplayMode(self):
        """Return the name of the default display mode. It must be defined in
        getDisplayModes."""
        return "Flat Lines"

    def setDisplayMode(self, mode):
        """Map the display mode defined in attach with those defined in
        getDisplayModes. Since they have the same names nothing needs to be
        done. This method is optional.

        Keyword arguments:
        mode -- Mode to be activated.
        """
        return mode

    def onChanged(self, vp, prop):
        """Detects the ship view provider data changes.

        Keyword arguments:
        vp -- View provider object affected.
        prop -- Modified property name.
        """
        pass

    def __getstate__(self):
        """When saving the document this object gets stored using Python's
        cPickle module. Since we have some un-pickable here (the Coin stuff)
        we must define this method to return a tuple of all pickable objects
        or None.
        """
        return None

    def __setstate__(self, state):
        """When restoring the pickled object from document we have the chance
        to set some internals here. Since no data were pickled nothing needs
        to be done here.
        """
        return None

    def getIcon(self):
        """Returns the icon for this kind of objects."""
        return ":/icons/Ship_Tank.svg"
