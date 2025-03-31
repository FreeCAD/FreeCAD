# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD post line plot"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_lineplot
#  \ingroup FEM
#  \brief Post processing plot displaying lines

from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

# helper function to extract plot object type
def _get_extraction_subtype(obj):
    if hasattr(obj, 'Proxy') and hasattr(obj.Proxy, "Type"):
        return obj.Proxy.Type

    return "unknown"


class PostLinePlot(base_fempythonobject.BaseFemPythonObject):
    """
    A post processing extraction for plotting lines
    """

    Type = "App::FeaturePython"

    def __init__(self, obj):
        super().__init__(obj)
        obj.addExtension("App::GroupExtension")
        self._setup_properties(obj)

    def _setup_properties(self, obj):

        self.ExtractionType = "LinePlot"

        pl = obj.PropertiesList
        for prop in self._get_properties():
            if not prop.Name in pl:
                prop.add_to_object(obj)

    def _get_properties(self):
        prop = []
        return prop

    def onDocumentRestored(self, obj):
        self._setup_properties(self, obj):

    def onChanged(self, obj, prop):

        if prop == "Group":
            # check if all objects are allowed

            children = obj.Group
            for child in obj.Group:
                if _get_extraction_subtype(child) not in ["Line"]:
                    children.remove(child)

            if len(obj.Group) != len(children):
                obj.Group = children


class PostPlotLine(base_fempythonobject.BaseFemPythonObject):

    Type = "App::FeaturePython"

    def __init__(self, obj):
        super().__init__(obj)
        self._setup_properties(obj)

    def _setup_properties(self, obj):

        self.ExtractionType = "Line"

        pl = obj.PropertiesList
        for prop in self._get_properties():
            if not prop.Name in pl:
                prop.add_to_object(obj)

    def _get_properties(self):

        prop = [
            _PropHelper(
                type="App::PropertyLink",
                name="Source",
                group="Line",
                doc="The data source, the line uses",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="XField",
                group="X Data",
                doc="The field to use as X data",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="XComponent",
                group="X Data",
                doc="Which part of the X field vector to use for the X axis",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="YField",
                group="Y Data",
                doc="The field to use as Y data for the line plot",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="YComponent",
                group="Y Data",
                doc="Which part of the Y field vector to use for the X axis",
                value=None,
            ),
        ]
        return prop

    def onDocumentRestored(self, obj):
        self._setup_properties(self, obj):

    def onChanged(self, obj, prop):

        if prop == "Source":
            # check if the source is a Post object
            if obj.Source and not obj.Source.isDerivedFrom("Fem::FemPostObject"):
                FreeCAD.Console.PrintWarning("Invalid object: Line source must be FemPostObject")
                obj.XField = []
                obj.YField = []
                obj.Source = None

        if prop == "XField":
            if not obj.Source:
                obj.XComponent = []
                return

            point_data = obj.Source.Data.GetPointData()
            if not point_data.HasArray(obj.XField):
                obj.XComponent = []
                return

            match point_data.GetArray(fields.index(obj.XField)).GetNumberOfComponents:
                case 1:
                    obj.XComponent = ["Not a vector"]
                case 2:
                    obj.XComponent = ["Magnitude", "X", "Y"]
                case 3:
                    obj.XComponent = ["Magnitude", "X", "Y", "Z"]

        if prop == "YField":
            if not obj.Source:
                obj.YComponent = []
                return

            point_data = obj.Source.Data.GetPointData()
            if not point_data.HasArray(obj.YField):
                obj.YComponent = []
                return

            match point_data.GetArray(fields.index(obj.YField)).GetNumberOfComponents:
                case 1:
                    obj.YComponent = ["Not a vector"]
                case 2:
                    obj.YComponent = ["Magnitude", "X", "Y"]
                case 3:
                    obj.YComponent = ["Magnitude", "X", "Y", "Z"]

    def onExecute(self, obj):
        # we need to make sure that we show the correct fields to the user as option for data extraction

        fields = []
        if obj.Source:
            point_data = obj.Source.Data.GetPointData()
            fields = [point_data.GetArrayName(i) for i in range(point_data.GetNumberOfArrays())]

        current_X = obj.XField
        obj.XField = fields
        if current_X in fields:
            obj.XField = current_X

        current_Y = obj.YField
        obj.YField = fields
        if current_Y in fields:
            obj.YField = current_Y

        return True

