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

## @package post_histogram
#  \ingroup FEM
#  \brief Post processing plot displaying lines

from . import base_fempostextractors
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

from vtkmodules.vtkCommonCore import vtkDoubleArray
from vtkmodules.vtkCommonCore import vtkIntArray
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline

class PostFieldData1D(base_fempostextractors.Extractor1D):
    """
    A post processing extraction of one dimensional field data
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

    def __array_to_table(self, obj, array, table):
        if array.GetNumberOfComponents() == 1:
            table.AddColumn(array)
        else:
            component_array = vtkDoubleArray();
            component_array.SetNumberOfComponents(1)
            component_array.SetNumberOfTuples(array.GetNumberOfTuples())
            c_idx = obj.getEnumerationsOfProperty("XComponent").index(obj.XComponent)
            component_array.CopyComponent(0, array, c_idx)
            component_array.SetName(array.GetName())
            table.AddColumn(component_array)

    def __array_from_dataset(self, obj, dataset):
        # extracts the relevant array from the dataset and returns a copy

        match obj.XField:
            case "Index":
                num = dataset.GetPoints().GetNumberOfPoints()
                array = vtkIntArray()
                array.SetNumberOfTuples(num)
                array.SetNumberOfComponents(1)
                for i in range(num):
                    array.SetValue(i,i)

            case "Position":
                orig_array = dataset.GetPoints().GetData()
                array = vtkDoubleArray()
                array.DeepCopy(orig_array)

            case _:
                point_data = dataset.GetPointData()
                orig_array = point_data.GetAbstractArray(obj.XField)
                array = vtkDoubleArray()
                array.DeepCopy(orig_array)

        return array


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
            array = self.__array_from_dataset(obj, dataset)
            if array.GetNumberOfComponents() > 1:
                array.SetName(obj.XField + " (" + obj.XComponent + ")")
            else:
                array.SetName(obj.XField)

            self.__array_to_table(obj, array, table)

        else:
            algo = obj.Source.getOutputAlgorithm()
            for timestep in timesteps:
                algo.UpdateTimeStep(timestep)
                dataset = algo.GetOutputDataObject(0)
                array = self.__array_from_dataset(obj, dataset)

                if array.GetNumberOfComponents() > 1:
                    array.SetName(f"{obj.XField} ({obj.XComponent}) - {timestep}")
                else:
                    array.SetName(f"{obj.XField} - {timestep}")
                self.__array_to_table(obj, array, table)

        # set the final table
        obj.Table = table


class PostIndexData1D(base_fempostextractors.Extractor1D):
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
