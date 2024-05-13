# ***************************************************************************
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
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

"""
This module recreates the whole structure of an IFC file in FreeCAD (ie.
one FreeCAD object per IFC entity). It serves mostly as a proof of concept
and test to see how doable it is with large files.

The geometry of objects is not imported, only attributes, which are mapped
to FreeCAD properties.

The IfcOpenHouse model, included with ifcopenshell, imports in 4 sec on
my ryzen9 machine, for 2700 objects. Larger files like the King Arch file
(20 Mb / 750 000 objects) would import in 18 minutes...
"""

import FreeCAD
import ifcopenshell
from PySide2 import QtCore, QtGui, QtWidgets
import time


class ViewProvider:

    """A simple view provider to gather children"""

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object

    def claimChildren(self):
        children = []
        relprops = ["Item", "ForLayerSet"]  # properties that actually store parents
        for prop in self.Object.PropertiesList:
            if prop.startswith("Relating") or (prop in relprops):
                continue
            else:
                value = getattr(self.Object, prop)
                if hasattr(value, "ViewObject"):
                    children.append(value)
                elif isinstance(value, list):
                    for item in value:
                        if hasattr(item, "ViewObject"):
                            children.append(item)
        for parent in self.Object.InList:
            for prop in parent.PropertiesList:
                if prop.startswith("Relating") or (prop in relprops):
                    value = getattr(parent, prop)
                    if value == self.Object:
                        children.append(parent)
        return children


def create(ifcentity):
    """The main function that creates objects and fills properties"""

    name = "Entity" + str(ifcentity.id())
    obj = FreeCAD.ActiveDocument.getObject(name)
    if obj:
        return obj
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython", name)
    if getattr(ifcentity, "Name", None):
        obj.Label = ifcentity.Name
    else:
        obj.Label = ifcentity.is_a()
    for attr, value in ifcentity.get_info().items():
        if attr not in obj.PropertiesList:
            if attr == "id":
                attr = "StepId"
            elif attr == "type":
                attr = "Type"
            elif attr == "Name":
                continue
            if hasattr(obj, attr):
                continue
            elif isinstance(value, int):
                obj.addProperty("App::PropertyInteger", attr, "IFC")
                setattr(obj, attr, value)
            elif isinstance(value, float):
                obj.addProperty("App::PropertyFloat", attr, "IFC")
                setattr(obj, attr, value)
            elif isinstance(value, ifcopenshell.entity_instance):
                value = create(value)
                obj.addProperty("App::PropertyLink", attr, "IFC")
                setattr(obj, attr, value)
            elif isinstance(value, (list, tuple)) and value:
                if isinstance(value[0], ifcopenshell.entity_instance):
                    nvalue = []
                    for elt in value:
                        nvalue.append(create(elt))
                    obj.addProperty("App::PropertyLinkList", attr, "IFC")
                    setattr(obj, attr, nvalue)
            else:
                obj.addProperty("App::PropertyString", attr, "IFC")
                if value is not None:
                    setattr(obj, attr, str(value))
    for parent in ifcfile.get_inverse(ifcentity):
        create(parent)
    if FreeCAD.GuiUp:
        ViewProvider(obj.ViewObject)
    return obj


# main

filepath = QtWidgets.QFileDialog.getOpenFileName(
    None, "Select IFC File", None, "IFC Files (*.ifc)"
)[0]
stime = time.time()
ifcfile = ifcopenshell.open(filepath)
project = ifcfile.by_type("IfcProject")[0]
if not FreeCAD.ActiveDocument:
    FreeCAD.newDocument()
create(project)
FreeCAD.ActiveDocument.recompute()
endtime = "%02d:%02d" % (divmod(round(time.time() - stime, 1), 60))
lenobjects = str(len(FreeCAD.ActiveDocument.Objects)) + " objects"
print("Import done:", endtime, ",", lenobjects)
