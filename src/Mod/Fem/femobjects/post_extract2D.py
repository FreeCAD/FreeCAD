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

__title__ = "FreeCAD post extractors 2D"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_histogram
#  \ingroup FEM
#  \brief Post processing plot displaying lines

from . import base_fempostextractors
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline

class PostFieldData2D(base_fempostextractors.Extractor2D):
    """
    A post processing extraction of two dimensional field data
    """

    ExtractionType = "Field"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop =[ _PropHelper(
                type="App::PropertyBool",
                name="ExtractFrames",
                group="Multiframe",
                doc="Specify if the field shall be extracted for every available frame",
                value=False,
            ),
        ]
        return super()._get_properties() + prop


    def execute(self, obj):

        # on execution we populate the vtk table
        table = vtkTable()

        if not obj.Source:
            obj.Table = table
            return

        dataset = obj.Source.getDataSet()
        if not dataset:
            obj.Table = table
            return

        frames = False
        if obj.ExtractFrames:
            # check if we have timesteps
            info = obj.Source.getOutputAlgorithm().GetOutputInformation(0)
            if info.Has(vtkStreamingDemandDrivenPipeline.TIME_STEPS()):
                timesteps = info.Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
                frames = True
            else:
                FreeCAD.Console.PrintWarning("No frames available in data, ignoring \"ExtractFrames\" property")

        if not frames:
            # get the dataset and extract the correct array
            xarray = self._x_array_from_dataset(obj, dataset)
            if xarray.GetNumberOfComponents() > 1:
                xarray.SetName(obj.XField + " (" + obj.XComponent + ")")
            else:
                xarray.SetName(obj.XField)

            self._x_array_component_to_table(obj, xarray, table)

            yarray = self._y_array_from_dataset(obj, dataset)
            if yarray.GetNumberOfComponents() > 1:
                yarray.SetName(obj.YField + " (" + obj.YComponent + ")")
            else:
                yarray.SetName(obj.YField)

            self._y_array_component_to_table(obj, yarray, table)

        else:
            algo = obj.Source.getOutputAlgorithm()
            for timestep in timesteps:
                algo.UpdateTimeStep(timestep)
                dataset = algo.GetOutputDataObject(0)

                xarray = self._x_array_from_dataset(obj, dataset)
                if xarray.GetNumberOfComponents() > 1:
                    xarray.SetName(f"X - {obj.XField} ({obj.XComponent}) - {timestep}")
                else:
                    xarray.SetName(f"X - {obj.XField} - {timestep}")
                self._x_array_component_to_table(obj, xarray, table)

                yarray = self._y_array_from_dataset(obj, dataset)
                if yarray.GetNumberOfComponents() > 1:
                    yarray.SetName(f"{obj.YField} ({obj.YComponent}) - {timestep}")
                else:
                    yarray.SetName(f"{obj.YField} - {timestep}")
                self._y_array_component_to_table(obj, yarray, table)

        # set the final table
        obj.Table = table


class PostIndexData2D(base_fempostextractors.Extractor2D):
    """
    A post processing extraction of one dimensional index data
    """

    ExtractionType = "Index"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop =[ _PropHelper(
                type="App::PropertyBool",
                name="ExtractFrames",
                group="Multiframe",
                doc="Specify if the data at the index should be extracted for each frame",
                value=False,
            ),
            _PropHelper(
                type="App::PropertyInteger",
                name="XIndex",
                group="X Data",
                doc="Specify for which point index the data should be extracted",
                value=0,
            ),
        ]
        return super()._get_properties() + prop
