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
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Set of weight instances",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyStringList",
                        "Weights",
                        "Ship",
                        tooltip).Weights = []
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Set of tank instances",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyStringList",
                        "Tanks",
                        "Ship",
                        tooltip).Tanks = []
        tooltip = str(QtGui.QApplication.translate(
            "Ship",
            "Set of load conditions",
            None,
            QtGui.QApplication.UnicodeUTF8))
        obj.addProperty("App::PropertyStringList",
                        "LoadConditions",
                        "Ship",
                        tooltip).LoadConditions = []

        obj.Proxy = self

    def onChanged(self, fp, prop):
        """Detects the ship data changes.

        Keyword arguments:
        fp -- Part::FeaturePython object affected.
        prop -- Modified property name.
        """
        if prop == "Length" or prop == "Breadth" or prop == "Draft":
            pass

    def cleanWeights(self, fp):
        """Reanalyse the weights list looking for duplicated opbjects, or
        removed ones.
        """
        if not len(fp.Weights):
            return
        # Filter out the duplicated elements
        filtered_list = []
        [filtered_list.append(x) for x in fp.Weights if x not in filtered_list]
        if cmp(fp.Weights, filtered_list):
            fp.Weights = filtered_list
        # Filter out the removed/non-valid objects
        object_names = []
        for obj in fp.Document.Objects:
            object_names.append(obj.Name)
        filtered_list = []
        for obj_name in fp.Weights:
            if obj_name in object_names:
                for obj in fp.Document.Objects:
                    if obj.Name == obj_name:
                        try:
                            if obj.IsWeight: filtered_list.append(obj_name)
                        except:
                            pass
                        break
        if cmp(fp.Weights, filtered_list):
            fp.Weights = filtered_list

    def cleanTanks(self, fp):
        """Reanalyse the weights list looking for duplicated opbjects, or
        removed ones.
        """
        if not len(fp.Tanks):
            return
        # Filter out the duplicated elements
        filtered_list = []
        [filtered_list.append(x) for x in fp.Tanks if x not in filtered_list]
        if cmp(fp.Tanks, filtered_list):
            fp.Tanks = filtered_list
        # Filter out the removed/non-valid objects
        object_names = []
        for obj in fp.Document.Objects:
            object_names.append(obj.Name)
        filtered_list = []
        for obj_name in fp.Tanks:
            if obj_name in object_names:
                for obj in fp.Document.Objects:
                    if obj.Name == obj_name:
                        try:
                            if obj.IsTank: filtered_list.append(obj_name)
                        except:
                            pass
                        break
        if cmp(fp.Tanks, filtered_list):
            fp.Tanks = filtered_list

    def cleanLoadConditions(self, fp):
        """Reanalyse the weights list looking for duplicated opbjects, or
        removed ones.
        """
        if not len(fp.LoadConditions):
            return
        # Filter out the duplicated elements
        filtered_list = []
        [filtered_list.append(x) for x in fp.LoadConditions if x not in filtered_list]
        if cmp(fp.LoadConditions, filtered_list):
            fp.LoadConditions = filtered_list
        # Filter out the removed/non-valid objects
        object_names = []
        for obj in fp.Document.Objects:
            object_names.append(obj.Name)
        filtered_list = []
        for obj_name in fp.LoadConditions:
            if obj_name in object_names:
                for obj in fp.Document.Objects:
                    if obj.Name == obj_name:
                        try:
                            if obj.TypeId == 'Spreadsheet::Sheet':
                                filtered_list.append(obj_name)
                        except:
                            pass
                        break
        if cmp(fp.LoadConditions, filtered_list):
            fp.LoadConditions = filtered_list

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

    def claimChildren(self):
        objs = []
        # Locate the owner ship object
        doc_objs = FreeCAD.ActiveDocument.Objects
        obj = None
        for doc_obj in doc_objs:
            try:
                v_provider = doc_obj.ViewObject.Proxy
                if v_provider == self:
                    obj = doc_obj
            except:
                continue
        if obj is None:
            FreeCAD.Console.PrintError("Orphan view provider found...\n")
            FreeCAD.Console.PrintError(self)
            FreeCAD.Console.PrintError('\n')
            return objs

        # Claim the weights
        bad_linked = 0
        for i, w in enumerate(obj.Weights):
            try:
                w_obj = FreeCAD.ActiveDocument.getObject(w)
                objs.append(w_obj)
            except:
                del obj.Weights[i - bad_linked]
                bad_linked += 1

        # Claim the tanks
        bad_linked = 0
        for i, t in enumerate(obj.Tanks):
            try:
                t_obj = FreeCAD.ActiveDocument.getObject(t)
                objs.append(t_obj)
            except:
                del obj.Tanks[i - bad_linked]
                bad_linked += 1

        # Claim the loading conditions
        bad_linked = 0
        for i, t in enumerate(obj.LoadConditions):
            try:
                t_obj = FreeCAD.ActiveDocument.getObject(t)
                objs.append(t_obj)
            except:
                del obj.LoadConditions[i - bad_linked]
                bad_linked += 1

        return objs

    def getIcon(self):
        """Returns the icon for this kind of objects."""
        return ":/icons/Ship_Instance.svg"
