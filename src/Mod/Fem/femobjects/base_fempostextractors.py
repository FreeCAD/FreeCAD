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

__title__ = "FreeCAD FEM postprocessing data exxtractor base objects"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package base_fempostextractors
#  \ingroup FEM
#  \brief base objects for data extractors

from vtkmodules.vtkCommonCore import vtkIntArray
from vtkmodules.vtkCommonCore import vtkDoubleArray
from vtkmodules.vtkCommonDataModel import vtkTable

from PySide.QtCore import QT_TRANSLATE_NOOP

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
                doc=QT_TRANSLATE_NOOP("FEM", "The data table that stores the extracted data"),
                value=vtkTable(),
            ),
            _PropHelper(
                type="App::PropertyLink",
                name="Source",
                group="Base",
                doc=QT_TRANSLATE_NOOP("FEM", "The data source from which the data is extracted"),
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
            case 6:
                return ["XX", "YY", "ZZ", "XY", "XZ", "YZ"]
            case _:
                return ["Not a vector"]

    def get_representive_fieldname(self, obj):
        # should return the representative field name, e.g. Position (X)
        return ""


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
                doc=QT_TRANSLATE_NOOP("FEM", "The field to use as X data"),
                value=[],
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="XComponent",
                group="X Data",
                doc=QT_TRANSLATE_NOOP(
                    "FEM", "Which part of the X field vector to use for the X axis"
                ),
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

    def _x_array_component_to_table(self, obj, array, table):
        # extracts the component out of the array according to XComponent setting
        # Note: Uses the array name unchanged

        if array.GetNumberOfComponents() == 1:
            table.AddColumn(array)
        else:
            component_array = vtkDoubleArray()
            component_array.SetNumberOfComponents(1)
            component_array.SetNumberOfTuples(array.GetNumberOfTuples())
            c_idx = obj.getEnumerationsOfProperty("XComponent").index(obj.XComponent)
            component_array.CopyComponent(0, array, c_idx)
            component_array.SetName(array.GetName())
            table.AddColumn(component_array)

    def _x_array_from_dataset(self, obj, dataset, copy=True):
        # extracts the relevant array from the dataset and returns a copy
        # indices = None uses all indices, otherwise the values in this list

        match obj.XField:
            case "Index":
                # index needs always to be build, ignore copy argument
                num = dataset.GetPoints().GetNumberOfPoints()
                array = vtkIntArray()
                array.SetNumberOfTuples(num)
                array.SetNumberOfComponents(1)
                for i in range(num):
                    array.SetValue(i, i)

            case "Position":

                orig_array = dataset.GetPoints().GetData()
                if copy:
                    array = vtkDoubleArray()
                    array.DeepCopy(orig_array)
                else:
                    array = orig_array

            case _:
                point_data = dataset.GetPointData()
                orig_array = point_data.GetAbstractArray(obj.XField)
                if copy:
                    array = vtkDoubleArray()
                    array.DeepCopy(orig_array)
                else:
                    array = orig_array

        return array

    def get_representive_fieldname(self, obj):
        # representative field is the x field
        label = obj.XField
        if not label:
            return ""

        if len(obj.getEnumerationsOfProperty("XComponent")) > 1:
            label += f" ({obj.XComponent})"

        return label


class Extractor2D(Extractor1D):

    ExtractionDimension = "2D"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop = [
            _PropHelper(
                type="App::PropertyEnumeration",
                name="YField",
                group="Y Data",
                doc=QT_TRANSLATE_NOOP("FEM", "The field to use as Y data"),
                value=[],
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="YComponent",
                group="Y Data",
                doc=QT_TRANSLATE_NOOP(
                    "FEM", "Which part of the Y field vector to use for the Y axis"
                ),
                value=[],
            ),
        ]

        return super()._get_properties() + prop

    def onChanged(self, obj, prop):

        super().onChanged(obj, prop)

        if prop == "YField" and obj.Source and obj.Source.getDataSet():
            point_data = obj.Source.getDataSet().GetPointData()
            self._setup_y_component_property(obj, point_data)

        if prop == "Source":
            if obj.Source:
                dset = obj.Source.getDataSet()
                if dset:
                    self._setup_y_properties(obj, dset)
                else:
                    self._clear_y_properties(obj)
            else:
                self._clear_y_properties(obj)

    def _setup_y_component_property(self, obj, point_data):

        if obj.YField == "Position":
            obj.YComponent = self.component_options(3)
        else:
            array = point_data.GetAbstractArray(obj.YField)
            obj.YComponent = self.component_options(array.GetNumberOfComponents())

    def _clear_y_properties(self, obj):
        if hasattr(obj, "YComponent"):
            obj.YComponent = []
        if hasattr(obj, "YField"):
            obj.YField = []

    def _setup_y_properties(self, obj, dataset):
        # Set all X Data properties correctly for the given dataset
        fields = ["Position"]
        point_data = dataset.GetPointData()

        for i in range(point_data.GetNumberOfArrays()):
            fields.append(point_data.GetArrayName(i))

        current_field = obj.YField
        obj.YField = fields
        if current_field in fields:
            obj.YField = current_field

        self._setup_y_component_property(obj, point_data)

    def _y_array_component_to_table(self, obj, array, table):
        # extracts the component out of the array according to XComponent setting

        if array.GetNumberOfComponents() == 1:
            table.AddColumn(array)
        else:
            component_array = vtkDoubleArray()
            component_array.SetNumberOfComponents(1)
            component_array.SetNumberOfTuples(array.GetNumberOfTuples())
            c_idx = obj.getEnumerationsOfProperty("YComponent").index(obj.YComponent)
            component_array.CopyComponent(0, array, c_idx)
            component_array.SetName(array.GetName())
            table.AddColumn(component_array)

    def _y_array_from_dataset(self, obj, dataset, copy=True):
        # extracts the relevant array from the dataset and returns a copy
        # indices = None uses all indices, otherwise the values in this list

        match obj.YField:
            case "Position":

                orig_array = dataset.GetPoints().GetData()
                if copy:
                    array = vtkDoubleArray()
                    array.DeepCopy(orig_array)
                else:
                    array = orig_array

            case _:
                point_data = dataset.GetPointData()
                orig_array = point_data.GetAbstractArray(obj.YField)

                if copy:
                    array = vtkDoubleArray()
                    array.DeepCopy(orig_array)
                else:
                    array = orig_array

        return array

    def get_representive_fieldname(self, obj):
        # representative field is the y field
        label = obj.YField
        if not label:
            return ""

        if len(obj.getEnumerationsOfProperty("YComponent")) > 1:
            label += f" ({obj.YComponent})"

        return label
