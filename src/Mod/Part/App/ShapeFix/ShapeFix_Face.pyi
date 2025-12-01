# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.ShapeFix_Root import ShapeFix_Root
from Part.TopoShapeFace import TopoShapeFace
from Part.TopoShapeShell import TopoShapeShell
from typing import Union

@export(
    PythonName="Part.ShapeFix.Face",
    Include="ShapeFix_Face.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_Face(ShapeFix_Root):
    """
    Class for fixing operations on faces

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    FixWireMode: bool = ...
    """Mode for applying fixes of ShapeFix_Wire"""

    FixOrientationMode: bool = ...
    """
    Mode for applying fixes of orientation
    If True, wires oriented to border limited square
    """

    FixAddNaturalBoundMode: bool = ...
    """
    If true, natural boundary is added on faces that miss them.
    Default is False for faces with single wire (they are
    handled by FixOrientation in that case) and True for others.
    """

    FixMissingSeamMode: bool = ...
    """If True, tries to insert seam if missing"""

    FixSmallAreaWireMode: bool = ...
    """If True, drops small wires"""

    RemoveSmallAreaFaceMode: bool = ...
    """If True, drops small wires"""

    FixIntersectingWiresMode: bool = ...
    """Mode for applying fixes of intersecting wires"""

    FixLoopWiresMode: bool = ...
    """Mode for applying fixes of loop wires"""

    FixSplitFaceMode: bool = ...
    """Mode for applying fixes of split face"""

    AutoCorrectPrecisionMode: bool = ...
    """Mode for applying auto-corrected precision"""

    FixPeriodicDegeneratedMode: bool = ...
    """Mode for applying periodic degeneration"""

    def init(self) -> None:
        """
        Initializes by face
        """
        ...

    def fixWireTool(self):
        """
        Returns tool for fixing wires
        """
        ...

    def clearModes(self) -> None:
        """
        Sets all modes to default
        """
        ...

    def add(self) -> None:
        """
        Add a wire to current face using BRep_Builder.
        Wire is added without taking into account orientation of face
        (as if face were FORWARD)
        """
        ...

    def fixOrientation(self) -> bool:
        """
        Fixes orientation of wires on the face
        It tries to make all wires lie outside all others (according
        to orientation) by reversing orientation of some of them.
        If face lying on sphere or torus has single wire and
        AddNaturalBoundMode is True, that wire is not reversed in
        any case (supposing that natural bound will be added).
        Returns True if wires were reversed
        """
        ...

    def fixAddNaturalBound(self) -> bool:
        """
        Adds natural boundary on face if it is missing.
        Two cases are supported:
         - face has no wires
         - face lies on geometrically double-closed surface
        (sphere or torus) and none of wires is left-oriented
        Returns True if natural boundary was added
        """
        ...

    def fixMissingSeam(self) -> bool:
        """
        Detects and fixes the special case when face on a closed
        surface is given by two wires closed in 3d but with gap in 2d.
        In that case it creates a new wire from the two, and adds a
        missing seam edge
        Returns True if missing seam was added
        """
        ...

    def fixSmallAreaWire(self) -> bool:
        """
        Detects wires with small area (that is less than
        100*Precision.PConfusion(). Removes these wires if they are internal.
        Returns True if at least one small wire removed, False nothing is done.
        """
        ...

    def fixLoopWire(self) -> None:
        """
        Detects if wire has a loop and fixes this situation by splitting on the few parts.
        """
        ...

    def fixIntersectingWires(self) -> None:
        """
        Detects and fixes the special case when face has more than one wire
        and this wires have intersection point
        """
        ...

    def fixWiresTwoCoincidentEdges(self) -> None:
        """
        If wire contains two coincidence edges it must be removed
        """
        ...

    def fixPeriodicDegenerated(self) -> None:
        """
        Fixes topology for a specific case when face is composed
        by a single wire belting a periodic surface. In that case
        a degenerated edge is reconstructed in the degenerated pole
        of the surface. Initial wire gets consistent orientation.
        Must be used in couple and before FixMissingSeam routine
        """
        ...

    def perform(self) -> None:
        """
        Iterates on subshapes and performs fixes
        """
        ...

    def face(self) -> TopoShapeFace:
        """
        Returns a face which corresponds to the current state
        """
        ...

    def result(self) -> Union[TopoShapeFace, TopoShapeShell]:
        """
        Returns resulting shape (Face or Shell if split)
        To be used instead of face() if FixMissingSeam involved
        """
        ...
