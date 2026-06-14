# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.BodyBase import BodyBase
from typing import Final

@export(
    Include="Mod/PartDesign/App/Body.h",
    FatherInclude="Mod/Part/App/BodyBasePy.h",
)
class Body(BodyBase):
    """
    PartDesign body class

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    VisibleFeature: Final[object] = ...
    """Return the visible feature of this body"""

    def insertObject(self, feature: object, target: object, after: bool = False, /) -> None:
        """
        Insert the feature into the body after the given feature.

        @param feature  The feature to insert into the body
        @param target   The feature relative which one should be inserted the given.
          If target is NULL than insert into the end if where is InsertBefore
          and into the begin if where is InsertAfter.
        @param after    if true insert the feature after the target. Default is false.

        @note the method doesn't modify the Tip unlike addObject()
        """
        ...
