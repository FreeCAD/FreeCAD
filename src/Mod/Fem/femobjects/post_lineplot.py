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

from . import base_fempostextractors
from . import base_fempostvisualizations
from . import post_extract2D

from vtkmodules.vtkCommonCore import vtkDoubleArray
from vtkmodules.vtkCommonDataModel import vtkTable


from femguiutils import post_visualization

# register visualization and extractors
post_visualization.register_visualization("Lineplot",
                                          ":/icons/FEM_PostLineplot.svg",
                                          "ObjectsFem",
                                          "makePostLineplot")

post_visualization.register_extractor("Lineplot",
                                      "LineplotFieldData",
                                      ":/icons/FEM_PostField.svg",
                                      "2D",
                                      "Field",
                                      "ObjectsFem",
                                      "makePostLineplotFieldData")


# Implementation
# ##############

def is_lineplot_extractor(obj):

    if not base_fempostextractors.is_extractor_object(obj):
        return False

    if not hasattr(obj.Proxy, "VisualizationType"):
        return False

    return obj.Proxy.VisualizationType == "Lineplot"


class PostLineplotFieldData(post_extract2D.PostFieldData2D):
    """
    A 2D Field extraction for lineplot.
    """
    VisualizationType = "Lineplot"



class PostLineplot(base_fempostvisualizations.PostVisualization):
    """
    A post processing plot for showing extracted data as line plots
    """

    VisualizationType = "Lineplot"

    def __init__(self, obj):
        super().__init__(obj)
        obj.addExtension("App::GroupExtensionPython")

    def _get_properties(self):
        prop = [
            _PropHelper(
                type="Fem::PropertyPostDataObject",
                name="Table",
                group="Base",
                doc="The data table that stores the plotted data, two columns per lineplot (x,y)",
                value=vtkTable(),
            ),
        ]
        return super()._get_properties() + prop


    def onChanged(self, obj, prop):

        if prop == "Group":
            # check if all objects are allowed

            children = obj.Group
            for child in obj.Group:
                if not is_lineplot_extractor(child):
                    FreeCAD.Console.PrintWarning(f"{child.Label} is not a data lineplot data extraction object, cannot be added")
                    children.remove(child)

            if len(obj.Group) != len(children):
                obj.Group = children

    def execute(self, obj):

        # during execution we collect all child data into our table
        table = vtkTable()
        for child in obj.Group:

            c_table = child.Table
            for i in range(c_table.GetNumberOfColumns()):
                c_array = c_table.GetColumn(i)
                # TODO: check which array type it is and use that one
                array = vtkDoubleArray()
                array.DeepCopy(c_array)
                array.SetName(f"{child.Source.Label}: {c_array.GetName()}")
                table.AddColumn(array)

        obj.Table = table
        return False


