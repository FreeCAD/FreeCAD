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

import FreeCAD

from . import base_fempostextractors
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper

from vtkmodules.vtkCommonCore import vtkDoubleArray
from vtkmodules.vtkCommonDataModel import vtkTable
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline

from PySide.QtCore import QT_TRANSLATE_NOOP


class PostFieldData1D(base_fempostextractors.Extractor1D):
    """
    A post processing extraction of one dimensional field data
    """

    ExtractionType = "Field"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop = [
            _PropHelper(
                type="App::PropertyBool",
                name="ExtractFrames",
                group="Multiframe",
                doc=QT_TRANSLATE_NOOP(
                    "FEM", "Specify if the field shall be extracted for every available frame"
                ),
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

        timesteps = []
        if obj.ExtractFrames:
            # check if we have timesteps
            info = obj.Source.getOutputAlgorithm().GetOutputInformation(0)
            if info.Has(vtkStreamingDemandDrivenPipeline.TIME_STEPS()):
                timesteps = info.Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
            else:
                FreeCAD.Console.PrintWarning(
                    'No frames available in data, ignoring "ExtractFrames" property'
                )

        if not timesteps:
            # get the dataset and extract the correct array
            array = self._x_array_from_dataset(obj, dataset)
            if array.GetNumberOfComponents() > 1:
                array.SetName(obj.XField + " (" + obj.XComponent + ")")
            else:
                array.SetName(obj.XField)

            self._x_array_component_to_table(obj, array, table)

        else:
            algo = obj.Source.getOutputAlgorithm()
            for timestep in timesteps:
                algo.UpdateTimeStep(timestep)
                dataset = algo.GetOutputDataObject(0)
                array = self._x_array_from_dataset(obj, dataset)

                if array.GetNumberOfComponents() > 1:
                    array.SetName(f"{obj.XField} ({obj.XComponent}) - {timestep}")
                else:
                    array.SetName(f"{obj.XField} - {timestep}")
                self._x_array_component_to_table(obj, array, table)

        # set the final table
        obj.Table = table


class PostIndexOverFrames1D(base_fempostextractors.Extractor1D):
    """
    A post processing extraction of one dimensional index data
    """

    ExtractionType = "Index"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop = [
            _PropHelper(
                type="App::PropertyInteger",
                name="Index",
                group="X Data",
                doc=QT_TRANSLATE_NOOP(
                    "FEM", "Specify for which index the data should be extracted"
                ),
                value=0,
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

        # check if we have timesteps
        timesteps = []
        info = obj.Source.getOutputAlgorithm().GetOutputInformation(0)
        if info.Has(vtkStreamingDemandDrivenPipeline.TIME_STEPS()):
            timesteps = info.Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS())

        algo = obj.Source.getOutputAlgorithm()
        frame_array = vtkDoubleArray()
        idx = obj.Index

        if timesteps:
            setup = False
            for i, timestep in enumerate(timesteps):

                algo.UpdateTimeStep(timestep)
                dataset = algo.GetOutputDataObject(0)
                array = self._x_array_from_dataset(obj, dataset, copy=False)

                # safeguard for invalid access
                if idx < 0 or array.GetNumberOfTuples() - 1 < idx:
                    raise Exception(
                        f"Invalid index: {idx} is not in range 0 - {array.GetNumberOfTuples()-1}"
                    )

                if not setup:
                    frame_array.SetNumberOfComponents(array.GetNumberOfComponents())
                    frame_array.SetNumberOfTuples(len(timesteps))
                    setup = True

                frame_array.SetTuple(i, idx, array)
        else:
            algo.Update()
            dataset = algo.GetOutputDataObject(0)
            array = self._x_array_from_dataset(obj, dataset, copy=False)

            # safeguard for invalid access
            if idx < 0 or array.GetNumberOfTuples() - 1 < idx:
                raise Exception(
                    f"Invalid index: {idx} is not in range 0 - {array.GetNumberOfTuples()-1}"
                )

            frame_array.SetNumberOfComponents(array.GetNumberOfComponents())
            frame_array.SetNumberOfTuples(1)
            frame_array.SetTuple(0, idx, array)

        if frame_array.GetNumberOfComponents() > 1:
            frame_array.SetName(f"{obj.XField} ({obj.XComponent}) @Idx {obj.Index}")
        else:
            frame_array.SetName(f"{obj.XField} @Idx {obj.Index}")

        self._x_array_component_to_table(obj, frame_array, table)

        # set the final table
        obj.Table = table
