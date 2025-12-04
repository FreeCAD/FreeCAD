# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root
from Part.App.TopoShape import TopoShape

@export(
    PythonName="Part.ShapeFix.FixSmallFace",
    Include="ShapeFix_FixSmallFace.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_FixSmallFace(ShapeFix_Root):
    """
    Class for fixing operations on faces

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def init(self) -> None:
        """
        Initializes by shape
        """
        ...

    def perform(self) -> None:
        """
        Fixing case of spot face
        """
        ...

    def fixSpotFace(self) -> None:
        """
        Fixing case of spot face, if tol = -1 used local tolerance
        """
        ...

    def replaceVerticesInCaseOfSpot(self) -> None:
        """
        Compute average vertex and replacing vertices by new one
        """
        ...

    def removeFacesInCaseOfSpot(self) -> None:
        """
        Remove spot face from compound
        """
        ...

    def fixStripFace(self) -> None:
        """
        Fixing case of strip face, if tol = -1 used local tolerance
        """
        ...

    def removeFacesInCaseOfStrip(self) -> None:
        """
        Remove strip face from compound
        """
        ...

    def fixSplitFace(self) -> TopoShape:
        """
        Fixes cases related to split faces within the given shape.
        It may return a modified shape after fixing the issues.
        """
        ...

    def fixFace(self) -> None:
        """
        Fixes issues related to the specified face and returns the modified face.
        """
        ...

    def fixShape(self) -> None:
        """
        Fixes issues in the overall geometric shape.
        This function likely encapsulates higher-level fixes that involve multiple faces or elements.
        """
        ...

    def shape(self) -> TopoShape:
        """
        Returns the current state of the geometric shape after potential modifications.
        """
        ...
