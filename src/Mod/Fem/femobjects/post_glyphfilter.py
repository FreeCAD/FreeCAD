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

__title__ = "FreeCAD post glyph filter"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_glyphfilter
#  \ingroup FEM
#  \brief Post processing filter creating glyphs for vector fields

import FreeCAD

# check vtk version to potentially find missmatchs
from femguiutils.vtk_module_handling import vtk_module_handling

vtk_module_handling()

# IMPORTANT: Never import vtk directly. Often vtk is compiled with different QT
# version than FreeCAD, and "import vtk" crashes by importing qt components.
# Always import the filter and data modules only.
from vtkmodules.vtkFiltersCore import vtkMaskPoints
from vtkmodules.vtkFiltersCore import vtkGlyph3D
import vtkmodules.vtkFiltersSources as vtkSources

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class PostGlyphFilter(base_fempythonobject.BaseFemPythonObject):
    """
    A post processing filter adding glyphs
    """

    Type = "Fem::PostFilterPython"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

        self.__setupFilterPipeline(obj)

    def _get_properties(self):

        prop = [
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Glyph",
                group="Glyph",
                doc="The form of the glyph",
                value=["Arrow", "Cone", "Cube", "Cylinder", "Line", "Sphere"],
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="OrientationData",
                group="Glyph",
                doc="Which vector field is used to orient the glyphs",
                value=["None"],
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ScaleData",
                group="Scale",
                doc="Which data field is used to scale the glyphs",
                value=["None"],
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="VectorScaleMode",
                group="Scale",
                doc="If the scale data is a vector, this property decides if the glyph is scaled by vector magnitude or by the individual components",
                value=["Not a vector"],
            ),
            _PropHelper(
                type="App::PropertyFloatConstraint",
                name="ScaleFactor",
                group="Scale",
                doc="A constant multiplier the glyphs are scaled with",
                value=(1, 0, 1e12, 1e-12),
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="MaskMode",
                group="Masking",
                doc="Which vertices are used as glyph locations",
                value=["Use All", "Every Nth", "Uniform Sampling"],
            ),
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="Stride",
                group="Masking",
                doc='Define the stride for "Every Nth" masking mode',
                value=(2, 1, 999999999, 1),
            ),
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="MaxNumber",
                group="Masking",
                doc='Defines the maximum number of vertices used for "Uniform Sampling" masking mode',
                value=(1000, 1, 999999999, 1),
            ),
        ]
        return prop

    def __setupMaskingFilter(self, obj, masking):

        if obj.MaskMode == "Use All":
            masking.RandomModeOff()
            masking.SetOnRatio(1)
            masking.SetMaximumNumberOfPoints(int(1e10))
        elif obj.MaskMode == "Every Nth":
            masking.RandomModeOff()
            masking.SetOnRatio(obj.Stride)
            masking.SetMaximumNumberOfPoints(int(1e10))
        else:
            masking.SetOnRatio(1)
            masking.SetMaximumNumberOfPoints(obj.MaxNumber)
            masking.RandomModeOn()

    def __setupGlyphFilter(self, obj, glyph):

        # scaling
        if obj.ScaleData != "None":

            glyph.ScalingOn()
            if obj.ScaleData in obj.getInputVectorFields():

                # make sure the vector mode is set correctly
                if obj.VectorScaleMode == "Not a vector":
                    obj.VectorScaleMode = ["Scale by magnitude", "Scale by components"]
                    obj.VectorScaleMode = "Scale by magnitude"

                if obj.VectorScaleMode == "Scale by magnitude":
                    glyph.SetScaleModeToScaleByVector()
                else:
                    glyph.SetScaleModeToScaleByVectorComponents()

                glyph.SetInputArrayToProcess(2, 0, 0, 0, obj.ScaleData)

            else:
                # scalar scaling mode
                if obj.VectorScaleMode != "Not a vector":
                    obj.VectorScaleMode = ["Not a vector"]

                glyph.SetInputArrayToProcess(2, 0, 0, 0, obj.ScaleData)
                glyph.SetScaleModeToScaleByScalar()
        else:
            glyph.ScalingOff()

        glyph.SetScaleFactor(obj.ScaleFactor)

        # Orientation
        if obj.OrientationData != "None":
            glyph.OrientOn()
            glyph.SetInputArrayToProcess(1, 0, 0, 0, obj.OrientationData)
        else:
            glyph.OrientOff()

    def __setupFilterPipeline(self, obj):

        # store of all algorithms for later access
        # its map filter_name : [source, mask, glyph]
        self._algorithms = {}

        # create all vtkalgorithm combinations and set them as filter pipeline
        sources = {
            "Arrow": vtkSources.vtkArrowSource,
            "Cone": vtkSources.vtkConeSource,
            "Cube": vtkSources.vtkCubeSource,
            "Cylinder": vtkSources.vtkCylinderSource,
            "Line": vtkSources.vtkLineSource,
            "Sphere": vtkSources.vtkSphereSource,
        }

        for source_name in sources:

            source = sources[source_name]()

            masking = vtkMaskPoints()
            self.__setupMaskingFilter(obj, masking)

            glyph = vtkGlyph3D()
            glyph.SetSourceConnection(source.GetOutputPort(0))
            glyph.SetInputConnection(masking.GetOutputPort(0))
            self.__setupGlyphFilter(obj, glyph)

            self._algorithms[source_name] = [source, masking, glyph]
            obj.addFilterPipeline(source_name, masking, glyph)

        obj.setActiveFilterPipeline(obj.Glyph)

    def onDocumentRestored(self, obj):
        # resetup the pipeline
        self.__setupFilterPipeline(obj)

    def execute(self, obj):
        # we check what new inputs

        vector_fields = obj.getInputVectorFields()
        all_fields = vector_fields + obj.getInputScalarFields()

        vector_fields.sort()
        all_fields.sort()

        current_orient = obj.OrientationData
        enumeration = ["None"] + vector_fields
        obj.OrientationData = enumeration
        if current_orient in enumeration:
            obj.OrientationData = current_orient

        current_scale = obj.ScaleData
        enumeration = ["None"] + all_fields
        obj.ScaleData = enumeration
        if current_scale in enumeration:
            obj.ScaleData = current_scale

        # make sure parent class execute is called!
        return False

    def onChanged(self, obj, prop):

        # check if we are setup already
        if not hasattr(self, "_algorithms"):
            return

        if prop == "Glyph":
            obj.setActiveFilterPipeline(obj.Glyph)

        if prop == "MaskMode":
            for filter in self._algorithms:
                masking = self._algorithms[filter][1]
                self.__setupMaskingFilter(obj, masking)

        if prop == "Stride":
            # if mode is use all stride setting needs to stay at one
            if obj.MaskMode == "Every Nth":
                for filter in self._algorithms:
                    masking = self._algorithms[filter][1]
                    masking.SetOnRatio(obj.Stride)

        if prop == "MaxNumber":
            if obj.MaskMode == "Uniform Sampling":
                for filter in self._algorithms:
                    masking = self._algorithms[filter][1]
                    masking.SetMaximumNumberOfPoints(obj.MaxNumber)

        if prop == "OrientationData" or prop == "ScaleData":
            for filter in self._algorithms:
                glyph = self._algorithms[filter][2]
                self.__setupGlyphFilter(obj, glyph)

        if prop == "ScaleFactor":
            for filter in self._algorithms:
                glyph = self._algorithms[filter][2]
                glyph.SetScaleFactor(obj.ScaleFactor)
