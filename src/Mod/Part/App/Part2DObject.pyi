# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from PartFeature import PartFeature

@export(
    Twin="Part2DObject",
    TwinPointer="Part2DObject",
    Include="Mod/Part/App/Part2DObject.h",
    FatherInclude="Mod/Part/App/PartFeaturePy.h",
)
class Part2DObject(PartFeature):
    """
    This object represents a 2D Shape in a 3D World

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    ...
