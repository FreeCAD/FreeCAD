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

__title__ = "FreeCAD FEM postprocessing data visualization base object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package base_fempostextractors
#  \ingroup FEM
#  \brief base objects for data visualizations

from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkCommonCore import vtkDoubleArray

from . import base_fempythonobject
from . import base_fempostextractors

# helper functions
# ################


def is_visualization_object(obj):
    if not obj:
        return False

    if not hasattr(obj, "Proxy"):
        return False

    return hasattr(obj.Proxy, "VisualizationType")


def get_visualization_type(obj):
    # returns the extractor type string, or throws exception if
    # not a extractor
    return obj.Proxy.VisualizationType


def is_visualization_extractor_type(obj, vistype):

    # must be extractor
    if not base_fempostextractors.is_extractor_object(obj):
        return False

    # must be visualization object
    if not is_visualization_object(obj):
        return False

    # must be correct type
    if get_visualization_type(obj) != vistype:
        return False

    return True


# Base class for all visualizations
# It collects all data from its extraction objects into a table.
# Note: Never use directly, always subclass! This class does not create a
#       Visualization variable, hence will not work correctly.
class PostVisualization(base_fempythonobject.BaseFemPythonObject):

    def __init__(self, obj):
        super().__init__(obj)
        obj.addExtension("App::GroupExtensionPython")
        self._setup_properties(obj)

    def _setup_properties(self, obj):
        pl = obj.PropertiesList
        for prop in self._get_properties():
            if not prop.name in pl:
                prop.add_to_object(obj)

    def _get_properties(self):
        # override if subclass wants to add additional properties

        prop = [
            base_fempostextractors._PropHelper(
                type="Fem::PropertyPostDataObject",
                name="Table",
                group="Base",
                doc="The data table that stores the data for visualization",
                value=vtkTable(),
            ),
        ]
        return prop

    def onDocumentRestored(self, obj):
        # if a new property was added we handle it by setup
        # Override if subclass needs to handle changed property type

        self._setup_properties(obj)

    def onChanged(self, obj, prop):
        # Ensure only correct child object types are in the group

        if prop == "Group":
            # check if all objects are allowed

            children = obj.Group
            for child in obj.Group:
                if not is_visualization_extractor_type(child, self.VisualizationType):
                    FreeCAD.Console.PrintWarning(
                        f"{child.Label} is not a {self.VisualizationType} extraction object, cannot be added"
                    )
                    children.remove(child)

            if len(obj.Group) != len(children):
                obj.Group = children

    def execute(self, obj):
        # Collect all extractor child data into our table
        # Note: Each childs table can have different number of rows. We need
        # to pad the date for our table in this case

        rows = self.getLongestColumnLength(obj)
        table = vtkTable()
        for child in obj.Group:

            # If child has no Source, its table should be empty. However,
            # it would theoretical be possible that child source was set
            # to none without recompute, and the visualization was manually
            # recomputed afterwards
            if not child.Source and (child.Table.GetNumberOfColumns() > 0):
                FreeCAD.Console.PrintWarning(
                    f"{child.Label} has data, but no Source object. Will be ignored"
                )
                continue

            c_table = child.Table
            for i in range(c_table.GetNumberOfColumns()):
                c_array = c_table.GetColumn(i)
                array = vtkDoubleArray()

                if c_array.GetNumberOfTuples() == rows:
                    # simple deep copy is enough
                    array.DeepCopy(c_array)

                else:
                    array.SetNumberOfComponents(c_array.GetNumberOfComponents())
                    array.SetNumberOfTuples(rows)
                    array.Fill(0)  # so that all non-used entries are set to 0
                    for j in range(c_array.GetNumberOfTuples()):
                        array.SetTuple(j, c_array.GetTuple(j))

                array.SetName(f"{child.Source.Name}: {c_array.GetName()}")
                table.AddColumn(array)

        obj.Table = table
        return False

    def getLongestColumnLength(self, obj):
        # iterate all extractor children and get the column lengths

        length = 0
        for child in obj.Group:
            if base_fempostextractors.is_extractor_object(child):
                table = child.Table
                if table.GetNumberOfColumns() > 0:
                    # we assume all columns of an extractor have same length
                    num = table.GetColumn(0).GetNumberOfTuples()
                    if num > length:
                        length = num

        return length
