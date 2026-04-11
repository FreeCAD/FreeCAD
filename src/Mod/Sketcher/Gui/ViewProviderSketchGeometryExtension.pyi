# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.GeometryExtension import GeometryExtension

@export(
    PythonName="SketcherGui.ViewProviderSketchGeometryExtension",
    Include="Mod/Sketcher/Gui/ViewProviderSketchGeometryExtension.h",
    Namespace="SketcherGui",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    Constructor=True,
)
class ViewProviderSketchGeometryExtension(GeometryExtension):
    """
    Describes a ViewProviderSketchGeometryExtension

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    VisualLayerId: int = ...
    """Sets/returns this geometry's Visual Layer Id."""
