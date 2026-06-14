# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from BoundedCurve import BoundedCurve

@export(
    Twin="GeomTrimmedCurve",
    TwinPointer="GeomTrimmedCurve",
    PythonName="Part.TrimmedCurve",
    FatherInclude="Mod/Part/App/BoundedCurvePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class TrimmedCurve(BoundedCurve):
    """
    The abstract class TrimmedCurve is the root class of all trimmed curve objects.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    def setParameterRange(self, first: float, last: float, /) -> None:
        """
        Re-trims this curve to the provided parameter range ([Float=First, Float=Last])
        """
        ...
