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
from shipUtils import Paths, Math


class Ship:
    def __init__(self, obj, solids):
        """ Transform a generic object to a ship instance.

        Keyword arguments:
        obj -- Part::FeaturePython created object which should be transformed
        in a ship instance.
        solids -- Set of solids which will compound the ship hull.
        """
        # Add an unique property to identify the Ship instances
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "True if it is a valid ship instance, False otherwise",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyBool",
                        "IsShip",
                        "Ship",
                        tooltip).IsShip = True
        # Add the main dimensions
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Ship length [m]",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyLength",
                        "Length",
                        "Ship",
                        tooltip).Length = 0.0
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Ship breadth [m]",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyLength",
                        "Breadth",
                        "Ship",
                        tooltip).Breadth = 0.0
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Ship draft [m]",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyLength",
                        "Draft",
                        "Ship",
                        tooltip).Draft = 0.0
        # Add the subshapes
        obj.Shape = Part.makeCompound(solids)
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Set of external faces of the ship hull",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("Part::PropertyPartShape",
                        "ExternalFaces",
                        "Ship",
                        tooltip)
        obj.Proxy = self

    def onChanged(self, fp, prop):
        """Detects the ship data changes.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        prop -- Modified property name.
        """
        if prop == "Length" or prop == "Breadth" or prop == "Draft":
            pass

    def execute(self, fp):
        """Detects the entity recomputations.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        """
        fp.Shape = Part.makeCompound(fp.Shape.Solids)


class ViewProviderShip:
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
        return ":/icons/Ship_Instance.svg"


def weights(obj):
    """Returns the ship weights list. If weights has not been set this tool
    will generate the default ones.

    Keyword arguments:
    obj -- Ship inmstance object.
    """
    # Test if is a ship instance
    props = obj.PropertiesList
    try:
        props.index("IsShip")
    except ValueError:
        return None
    if not obj.IsShip:
        return None
    # Test if properties already exist
    try:
        props.index("WeightNames")
    except ValueError:
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Ship Weights names",
            None,
            QtGui.QApplication.UnicodeUTF8))
        lighweight = str(QtGui.QApplication.translate(
            "Ship",
            "Lightweight",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyStringList",
                        "WeightNames",
                        "Ship",
                        tooltip).WeightNames = [lighweight]
    try:
        props.index("WeightMass")
    except ValueError:
        # Compute a mass aproximation
        from shipHydrostatics import Tools
        disp = Tools.displacement(obj, obj.Draft)
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Ship Weights masses [tons]",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyFloatList",
                        "WeightMass",
                        "Ship",
                        tooltip).WeightMass = [1000.0 * disp[0]]
    try:
        props.index("WeightPos")
    except ValueError:
        # Compute a CoG aproximation
        from shipHydrostatics import Tools
        disp = Tools.displacement(obj, obj.Draft)
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Ship Weights centers of gravity",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyVectorList",
                        "WeightPos",
                        "Ship",
                        tooltip).WeightPos = [Vector(disp[1].x,
                                              0.0,
                                              obj.Draft)]
    # Setup the weights list
    weights = []
    for i in range(len(obj.WeightNames)):
        weights.append([obj.WeightNames[i],
                        obj.WeightMass[i],
                        obj.WeightPos[i]])
    return weights
