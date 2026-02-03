# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.BaseClass import PyObjectBase
from Base.Metadata import export

@export(
    Include="Mod/Surface/App/Blending/BlendCurve.h",
    Namespace="Surface",
    Constructor=True,
    Delete=True,
)
class BlendCurve(PyObjectBase):
    """
    Create a BlendCurve that interpolate 2 BlendPoints.
        curve = BlendCurve(BlendPoint1, BlendPoint2)

    Author: Mattéo Grellier (matteogrellier@gmail.com)
    License: LGPL-2.1-or-later
    """

    def compute(self) -> Any:
        """
        Return the BezierCurve that interpolate the input BlendPoints.
        """
        ...

    def setSize(self) -> Any:
        """
        Set the tangent size of the blendpoint at given index.
        If relative is true, the size is considered relative to the distance between the two blendpoints.
        myBlendCurve.setSize(idx, size, relative)
        """
        ...
