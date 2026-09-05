# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Parametric Forms box object and its Python view provider."""

import FreeCAD as App

from .feature import FormFeatureProxy
from .viewprovider import ViewProviderForm as ViewProviderFormBox, FORMS_WORKBENCH, ACTIVE_FORM_KEY
from .feedback import MODELING_ERRORS, report_modeling_error
from .topology import box_control_cage, cage_edges


class FormBoxProxy(FormFeatureProxy):
    """Application-side implementation of a form box."""

    Type = "Forms::Box"
    ParameterNames = (
        "Length",
        "Width",
        "Height",
        "XSegments",
        "YSegments",
        "ZSegments",
    )

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "Length", "Box", "Length along the X axis")
        obj.addProperty("App::PropertyLength", "Width", "Box", "Width along the Y axis")
        obj.addProperty("App::PropertyLength", "Height", "Box", "Height along the Z axis")
        obj.addProperty(
            "App::PropertyIntegerConstraint", "XSegments", "Box", "Control faces along X"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint", "YSegments", "Box", "Control faces along Y"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint", "ZSegments", "Box", "Control faces along Z"
        )
        obj.Length = 20.0
        obj.Width = 20.0
        obj.Height = 20.0
        obj.XSegments = (2, 1, 100, 1)
        obj.YSegments = (2, 1, 100, 1)
        obj.ZSegments = (2, 1, 100, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return box_control_cage(
            obj.Length.Value,
            obj.Width.Value,
            obj.Height.Value,
            obj.XSegments,
            obj.YSegments,
            obj.ZSegments,
        )



def create_box(document=None, name="FormBox"):
    """Create and return a new form box."""
    document = document or App.ActiveDocument
    if document is None:
        raise RuntimeError("A document is required to create a form box")
    obj = document.addObject("Part::FeaturePython", name)
    obj.Label = App.Qt.translate("Forms_Create", "Form Box")
    FormBoxProxy(obj)
    if App.GuiUp:
        ViewProviderFormBox(obj.ViewObject)
    obj.recompute()
    return obj
