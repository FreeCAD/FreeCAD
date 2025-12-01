# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Part.App.GeometryExtension import GeometryExtension

@export(
    PythonName="Sketcher.ExternalGeometryExtension",
    Include="Mod/Sketcher/App/ExternalGeometryExtension.h",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    Constructor=True,
)
class ExternalGeometryExtension(GeometryExtension):
    """
    Describes a ExternalGeometryExtension

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    @constmethod
    def testFlag(self) -> bool:
        """
        Returns a boolean indicating whether the given bit is set.
        """
        ...

    def setFlag(self) -> None:
        """
        Sets the given bit to true/false.
        """
        ...
    Ref: str = ""
    """Returns the reference string of this external geometry."""
