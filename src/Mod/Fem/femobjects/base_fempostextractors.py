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

__title__ = "FreeCAD FEM postprocessing data exxtractor base objcts"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package base_fempostextractors
#  \ingroup FEM
#  \brief base objects for data extractors

from vtkmodules.vtkCommonDataModel import vtkTable

from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

# helper functions
# ################

def is_extractor_object(obj):
    if not hasattr(obj, "Proxy"):
        return False

    return hasattr(obj.Proxy, "ExtractionType")

def get_extraction_type(obj):
    # returns the extractor type string, or throws exception if
    # not a extractor
    return obj.Proxy.ExtractionType

def get_extraction_dimension(obj):
    # returns the extractor dimension string, or throws exception if
    # not a extractor
    return obj.Proxy.ExtractionDimension


# Base class for all extractors with common source and table handling functionality
# Note: Never use directly, always subclass! This class does not create a
#       ExtractionType/Dimension variable, hence will not work correctly.
class Extractor(base_fempythonobject.BaseFemPythonObject):

    def __init__(self, obj):
        super().__init__(obj)
        self._setup_properties(obj)

    def _setup_properties(self, obj):
        pl = obj.PropertiesList
        for prop in self._get_properties():
            if not prop.name in pl:
                prop.add_to_object(obj)

    def _get_properties(self):

        prop = [
            _PropHelper(
                type="Fem::PropertyPostDataObject",
                name="Table",
                group="Base",
                doc="The data table that stores the extracted data",
                value=vtkTable(),
            ),
            _PropHelper(
                type="App::PropertyLink",
                name="Source",
                group="Base",
                doc="The data source from which the data is extracted",
                value=None,
            ),
        ]
        return prop

    def onDocumentRestored(self, obj):
        self._setup_properties(obj)

    def onChanged(self, obj, prop):

        if prop == "Source":
            # check if the source is a Post object
            if obj.Source and not obj.Source.isDerivedFrom("Fem::FemPostObject"):
                FreeCAD.Console.PrintWarning("Invalid object: Line source must be FemPostObject")
                obj.Source = None


    def get_vtk_table(self, obj):
        if not obj.DataTable:
            obj.DataTable = vtkTable()

        return obj.DataTable

    def component_options(self, num):

        match num:
            case 2:
                return ["X", "Y"]
            case 3:
                return ["X", "Y", "Z"]
            case _:
                return ["Not a vector"]


class Extractor1D(Extractor):

    ExtractionDimension = "1D"

    def __init__(self, obj):
        super().__init__(obj)


    def _get_properties(self):
        prop = [
            _PropHelper(
                type="App::PropertyEnumeration",
                name="XField",
                group="X Data",
                doc="The field to use as X data",
                value=[],
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="XComponent",
                group="X Data",
                doc="Which part of the X field vector to use for the X axis",
                value=[],
            ),
        ]

        return super()._get_properties() + prop


    def onChanged(self, obj, prop):

        super().onChanged(obj, prop)

        if prop == "XField" and obj.Source and obj.Source.getDataSet():
            point_data = obj.Source.getDataSet().GetPointData()
            self._setup_x_component_property(obj, point_data)

        if prop == "Source":
            if obj.Source:
                dset = obj.Source.getDataSet()
                if dset:
                    self._setup_x_properties(obj, dset)
                else:
                    self._clear_x_properties(obj)
            else:
                self._clear_x_properties(obj)

    def _setup_x_component_property(self, obj, point_data):

        if obj.XField == "Index":
            obj.XComponent = self.component_options(1)
        elif obj.XField == "Position":
            obj.XComponent = self.component_options(3)
        else:
            array = point_data.GetAbstractArray(obj.XField)
            obj.XComponent = self.component_options(array.GetNumberOfComponents())

    def _clear_x_properties(self, obj):
        if hasattr(obj, "XComponent"):
            obj.XComponent = []
        if hasattr(obj, "XField"):
            obj.XField = []

    def _setup_x_properties(self, obj, dataset):
        # Set all X Data properties correctly for the given dataset
        fields = ["Index", "Position"]
        point_data = dataset.GetPointData()

        for i in range(point_data.GetNumberOfArrays()):
            fields.append(point_data.GetArrayName(i))

        current_field = obj.XField
        obj.XField = fields
        if current_field in fields:
            obj.XField = current_field

        self._setup_x_component_property(obj, point_data)


