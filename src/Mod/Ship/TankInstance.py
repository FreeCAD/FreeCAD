#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
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
from pivy.coin import *
from pivy import coin
import FreeCAD
import FreeCADGui
from FreeCAD import Base, Vector
import Part
import Units
from shipUtils import Paths, Math


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
        tooltip = str(QtGui.QApplication.translate(
            "ship_tank",
            "True if it is a valid tank instance, False otherwise",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyBool",
                        "IsTank",
                        "Tank",
                        tooltip).IsTank = True
        # Add the volume property (The volume of fluid will be set by each
        # loading condition)
        tooltip = str(QtGui.QApplication.translate(
            "ship_tank",
            "Volume of fluid [m^3]",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyFloat",
                        "Vol",
                        "Tank",
                        tooltip).Vol = 0.0
        # Add the density property (The volume of fluid will be set by each
        # loading condition)
        tooltip = str(QtGui.QApplication.translate(
            "ship_tank",
            "Density [kg / m^3]",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyFloat",
                        "Dens",
                        "Tank",
                        tooltip).Dens = 0.0
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

    def setFillingLevel(self, fp, level):
        """Compute the mass of the object, already taking into account the
        type of subentities.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        level -- Percentage of filling level (from 0 to 100).
        """
        shape = fp.Shape
        solids = shape.Solids

        # Get the cutting box
        bbox = shape.BoundBox
        z_min = bbox.ZMin
        z_max = bbox.ZMax
        dx = bbox.XMax - bbox.XMin
        dy = bbox.YMax - bbox.YMin
        dz = level / 100.0 * (z_max - z_min)
        z = z_min + dz
        try:
            box = Part.makeBox(3.0 * dx,
                               3.0 * dy,
                               (z_max - z_min) + dz,
                               Vector(bbox.XMin - dx,
                                      bbox.YMin - dy,
                                      bbox.ZMin - (z_max - z_min)))
        except:
            fp.Vol = 0.0
            return Units.parseQuantity('0 m^3')

        # Start computing the common part of each solid component with the
        # cutting box, adding the volume
        vol = 0.0
        for s in solids:
            try:
                fluid = s.common(box)
                v = fluid.Volume        
            except:
                v = 0.0
            vol += v

        # Get the volume quantity and store it with the right units
        vol = Units.Quantity(vol, Units.Volume)
        fp.Vol = vol.getValueAs("m^3").Value

        return vol


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
        return "Shaded"

    def setDisplayMode(self, mode):
        """Map the display mode defined in attach with those defined in
        getDisplayModes. Since they have the same names nothing needs to be
        done. This method is optinal.

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
