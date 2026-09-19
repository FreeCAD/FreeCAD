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

"""Generic editable Forms feature backed directly by a control cage."""

import FreeCAD as App

from .feature import FormFeatureProxy
from .viewprovider import ViewProviderForm as ViewProviderFormBox


class FormProxy(FormFeatureProxy):
    """A generic Form whose editable control cage defines its geometry."""

    Type = "Forms::Form"
    ParameterNames = ()

    def __init__(self, obj):
        self._add_common_properties(obj)
        self._finish_initialization(obj)
        obj.CageMode = "Editable"

    def _topology(self, _obj):
        raise RuntimeError("An editable Form does not have parametric primitive topology")

    def onDocumentRestored(self, obj):
        # Development builds briefly used this name. Normalize saved objects.
        if getattr(obj, "FormType", "") == "Forms::Imported":
            obj.FormType = self.Type
        super().onDocumentRestored(obj)


class ViewProviderForm(ViewProviderFormBox):
    """Use the normal Forms presentation for a generic editable cage."""

    IconName = "Forms_Workbench.svg"


def create_form(document=None, name="Form"):
    """Create an empty generic editable Form in *document*."""
    document = document or App.ActiveDocument
    if document is None:
        raise RuntimeError("A document is required to create a Form")
    obj = document.addObject("Part::FeaturePython", name)
    obj.Label = App.Qt.translate("Forms", "Form")
    FormProxy(obj)
    if App.GuiUp:
        ViewProviderForm(obj.ViewObject)
    return obj
