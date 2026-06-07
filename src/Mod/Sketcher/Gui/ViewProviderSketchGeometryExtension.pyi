# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.GeometryExtension import GeometryExtension

@export(
    Namespace="SketcherGui",
)
class ViewProviderSketchGeometryExtension(GeometryExtension):
    """
    Describes a ViewProviderSketchGeometryExtension

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    def __init__(self) -> None: ...

    VisualLayerId: int = ...
    """Sets/returns this geometry's Visual Layer Id."""
