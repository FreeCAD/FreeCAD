# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root

@export(
    PythonName="Part.ShapeFix.Wire",
    Twin="ShapeFix_Wire",
    TwinPointer="ShapeFix_Wire",
    Include="ShapeFix_Wire.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_Wire(ShapeFix_Root):
    """
    Class for fixing operations on wires

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def init(self) -> None:
        """
        Initializes by wire, face, precision
        """
        pass

    def fixEdgeTool(self) -> None:
        """
        Returns tool for fixing wires
        """
        pass

    def clearModes(self) -> None:
        """
        Sets all modes to default
        """
        pass

    def clearStatuses(self) -> None:
        """
        Clears all statuses
        """
        pass

    def load(self) -> None:
        """
        Load data for the wire, and drops all fixing statuses
        """
        pass

    def setFace(self) -> None:
        """
        Set working face for the wire
        """
        pass

    def setSurface(self, surface: object, Placement: object = ..., /) -> None:
        """
        setSurface(surface, [Placement])
        Set surface for the wire
        """
        pass

    def setMaxTailAngle(self) -> None:
        """
        Sets the maximal allowed angle of the tails in radians
        """
        pass

    def setMaxTailWidth(self) -> None:
        """
        Sets the maximal allowed width of the tails
        """
        pass

    def isLoaded(self) -> None:
        """
        Tells if the wire is loaded
        """
        pass

    def isReady(self) -> None:
        """
        Tells if the wire and face are loaded
        """
        pass

    def numberOfEdges(self) -> None:
        """
        Returns number of edges in the working wire
        """
        pass

    def wire(self) -> None:
        """
        Makes the resulting Wire (by basic Brep_Builder)
        """
        pass

    def wireAPIMake(self) -> None:
        """
        Makes the resulting Wire (by BRepAPI_MakeWire)
        """
        pass

    def face(self) -> None:
        """
        Returns working face
        """
        pass

    def perform(self) -> None:
        """
        Iterates on subshapes and performs fixes
        """
        pass

    def fixReorder(self) -> None:
        """
        Performs an analysis and reorders edges in the wire
        """
        pass

    def fixSmall(self) -> None:
        """
        Applies fixSmall(...) to all edges in the wire
        """
        pass

    def fixConnected(self, num: int, /) -> None:
        """
        Applies fixConnected(num) to all edges in the wire
        Connection between first and last edges is treated only if
        flag ClosedMode is True
        If prec is -1 then maxTolerance() is taken.
        """
        pass

    def fixEdgeCurves(self) -> None:
        """
        Groups the fixes dealing with 3d and pcurves of the edges
        """
        pass

    def fixDegenerated(self) -> None:
        """
        Applies fixDegenerated(...) to all edges in the wire
        """
        pass

    def fixSelfIntersection(self) -> None:
        """
        Applies FixSelfIntersectingEdge(num) and
         FixIntersectingEdges(num) to all edges in the wire and
         FixIntersectingEdges(num1, num2) for all pairs num1 and num2
         and removes wrong edges if any
        """
        pass

    def fixLacking(self) -> None:
        """
        Applies FixLacking(num) to all edges in the wire
        Connection between first and last edges is treated only if
        flag ClosedMode is True
        If 'force' is False (default), test for connectness is done with
        precision of vertex between edges, else it is done with minimal
        value of vertex tolerance and Analyzer.Precision().
        Hence, 'force' will lead to inserting lacking edges in replacement
        of vertices which have big tolerances.
        """
        pass

    def fixClosed(self) -> None:
        """
        Fixes a wire to be well closed
        """
        pass

    def fixGaps3d(self, num: int, /) -> None:
        """
        Fixes gaps between ends of 3d curves on adjacent edges
        """
        pass

    def fixGaps2d(self, num: int, /) -> None:
        """
        Fixes gaps between ends of pcurves on adjacent edges
        """
        pass

    def fixSeam(self) -> None:
        """
        Fixes seam edges
        """
        pass

    def fixShifted(self) -> None:
        """
        Fixes edges which have pcurves shifted by whole parameter
        range on the closed surface
        """
        pass

    def fixNotchedEdges(self) -> None:
        """
        Fixes Notch edges.Check if there are notch edges in 2d and fix it
        """
        pass

    def fixGap3d(self, num: int, /) -> None:
        """
        Fixes gap between ends of 3d curves on num-1 and num-th edges
        """
        pass

    def fixGap2d(self, num: int, /) -> None:
        """
        Fixes gap between ends of pcurves on num-1 and num-th edges
        """
        pass

    def fixTails(self) -> None:
        """
        Fixes issues related to 'tails' in the geometry.
        Tails are typically small, undesired protrusions or deviations in the curves or edges that need correction.
        This method examines the geometry and applies corrective actions to eliminate or reduce the presence of tails.
        """
        pass
    ModifyTopologyMode: bool = ...
    """Mode for modifying topology of the wire"""

    ModifyGeometryMode: bool = ...
    """Mode for modifying geometry of vertexes and edges"""

    ModifyRemoveLoopMode: bool = ...
    """Mode for modifying edges"""

    ClosedWireMode: bool = ...
    """
    Mode which defines whether the wire
    is to be closed (by calling methods like fixDegenerated()
    and fixConnected() for last and first edges)
    """

    PreferencePCurveMode: bool = ...
    """
    Mode which defines whether the 2d 'True'
    representation of the wire is preferable over 3d one in the
    case of ambiguity in FixEdgeCurves
    """

    FixGapsByRangesMode: bool = ...
    """
    Mode which defines whether tool
    tries to fix gaps first by changing curves ranges (i.e.
    using intersection, extrema, projections) or not
    """

    FixReorderMode: bool = ...
    """
    Mode which performs an analysis and reorders edges in the wire using class WireOrder.
    Flag 'theModeBoth' determines the use of miscible mode if necessary.
    """

    FixSmallMode: bool = ...
    """Mode which applies FixSmall(num) to all edges in the wire"""

    FixConnectedMode: bool = ...
    """
    Mode which applies FixConnected(num) to all edges in the wire
    Connection between first and last edges is treated only if
    flag ClosedMode is True
    If 'prec' is -1 then MaxTolerance() is taken.
    """

    FixEdgeCurvesMode: bool = ...
    """
    Mode which groups the fixes dealing with 3d and pcurves of the edges.
    The order of the fixes and the default behaviour are:
    ShapeFix_Edge::FixReversed2d
    ShapeFix_Edge::FixRemovePCurve (only if forced)
    ShapeFix_Edge::FixAddPCurve
    ShapeFix_Edge::FixRemoveCurve3d (only if forced)
    ShapeFix_Edge::FixAddCurve3d
    FixSeam,
    FixShifted,
    ShapeFix_Edge::FixSameParameter
    """

    FixDegeneratedMode: bool = ...
    """
    Mode which applies FixDegenerated(num) to all edges in the wire
    Connection between first and last edges is treated only if
    flag ClosedMode is True
    """

    FixSelfIntersectionMode: bool = ...
    """
    Mode which applies FixSelfIntersectingEdge(num) and
    FixIntersectingEdges(num) to all edges in the wire and
    FixIntersectingEdges(num1, num2) for all pairs num1 and num2
    and removes wrong edges if any
    """

    FixLackingMode: bool = ...
    """
    Mode which applies FixLacking(num) to all edges in the wire
    Connection between first and last edges is treated only if
    flag ClosedMode is True
    If 'force' is False (default), test for connectness is done with
    precision of vertex between edges, else it is done with minimal
    value of vertex tolerance and Analyzer.Precision().
    Hence, 'force' will lead to inserting lacking edges in replacement
    of vertices which have big tolerances.
    """

    FixGaps3dMode: bool = ...
    """
    Mode which fixes gaps between ends of 3d curves on adjacent edges
    myPrecision is used to detect the gaps.
    """

    FixGaps2dMode: bool = ...
    """
    Mode whixh fixes gaps between ends of pcurves on adjacent edges
    myPrecision is used to detect the gaps.
    """

    FixReversed2dMode: bool = ...
    """Mode which fixes the reversed in 2d"""

    FixRemovePCurveMode: bool = ...
    """Mode which removePCurve in 2d"""

    FixAddPCurveMode: bool = ...
    """Mode which fixes addCurve in 2d"""

    FixRemoveCurve3dMode: bool = ...
    """Mode which fixes removeCurve in 3d """

    FixAddCurve3dMode: bool = ...
    """Mode which fixes addCurve in 3d"""

    FixSeamMode: bool = ...
    """Mode which fixes Seam """

    FixShiftedMode: bool = ...
    """Mode which fixes Shifted"""

    FixSameParameterMode: bool = ...
    """Mode which fixes sameParameter in 2d"""

    FixVertexToleranceMode: bool = ...
    """Mode which fixes VertexTolerence in 2d"""

    FixNotchedEdgesMode: bool = ...
    """Mode which fixes NotchedEdges in 2d"""

    FixSelfIntersectingEdgeMode: bool = ...
    """Mode which fixes SelfIntersectionEdge in 2d"""

    FixIntersectingEdgesMode: bool = ...
    """Mode which fixes IntersectingEdges in 2d"""

    FixNonAdjacentIntersectingEdgesMode: bool = ...
    """Mode which fixes NonAdjacentIntersectingEdges in 2d"""

    FixTailMode: bool = ...
    """Mode which fixes Tails in 2d"""
