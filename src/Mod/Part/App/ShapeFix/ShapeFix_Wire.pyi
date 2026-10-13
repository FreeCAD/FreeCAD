# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root
from Part.App.ShapeFix.ShapeFix_Edge import ShapeFix_Edge
from Part.TopoShape import TopoShape
from Part.TopoShapeFace import TopoShapeFace
from Part.TopoShapeWire import TopoShapeWire
from typing import overload

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

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, wire: TopoShapeWire, face: TopoShapeFace, prec: float, /) -> None: ...
    def init(self, wire: TopoShapeWire, face: TopoShapeFace, prec: float, /) -> None:
        """
        Initializes by wire, face, precision
        """
        pass

    def fixEdgeTool(self) -> ShapeFix_Edge:
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

    def load(self, wire: TopoShapeWire, /) -> None:
        """
        Load data for the wire, and drops all fixing statuses
        """
        pass

    def setFace(self, face: TopoShapeFace, /) -> None:
        """
        Set working face for the wire
        """
        pass

    def setSurface(self, surface: object, placement: object = ..., /) -> None:
        """
        setSurface(surface, [Placement])
        Set surface for the wire
        """
        pass

    def setMaxTailAngle(self, angle: float, /) -> None:
        """
        Sets the maximal allowed angle of the tails in radians
        """
        pass

    def setMaxTailWidth(self, width: float, /) -> None:
        """
        Sets the maximal allowed width of the tails
        """
        pass

    def isLoaded(self) -> bool:
        """
        Tells if the wire is loaded
        """
        pass

    def isReady(self) -> bool:
        """
        Tells if the wire and face are loaded
        """
        pass

    def numberOfEdges(self) -> int:
        """
        Returns number of edges in the working wire
        """
        pass

    def wire(self) -> TopoShape:
        """
        Makes the resulting Wire (by basic Brep_Builder)
        """
        pass

    def wireAPIMake(self) -> TopoShape:
        """
        Makes the resulting Wire (by BRepAPI_MakeWire)
        """
        pass

    def face(self) -> TopoShape:
        """
        Returns working face
        """
        pass

    def perform(self) -> bool:
        """
        Iterates on subshapes and performs fixes
        """
        pass

    def fixReorder(self) -> bool:
        """
        Performs an analysis and reorders edges in the wire
        """
        pass

    @overload
    def fixSmall(self, lock: bool, prec: float = ..., /) -> int: ...
    @overload
    def fixSmall(self, num: int, lock: bool, prec: float, /) -> bool: ...
    def fixSmall(self, *args) -> int | bool:
        """
        Applies fixSmall(...) to all edges in the wire
        """
        pass

    @overload
    def fixConnected(self, prec: float = ..., /) -> bool: ...
    @overload
    def fixConnected(self, num: int, prec: float, /) -> bool: ...
    def fixConnected(self, *args) -> bool:
        """
        Applies fixConnected(num) to all edges in the wire
        Connection between first and last edges is treated only if
        flag ClosedMode is True
        If prec is -1 then maxTolerance() is taken.
        """
        pass

    def fixEdgeCurves(self) -> bool:
        """
        Groups the fixes dealing with 3d and pcurves of the edges
        """
        pass

    def fixDegenerated(self, num: int = ..., /) -> bool:
        """
        Applies fixDegenerated(...) to all edges in the wire
        """
        pass

    def fixSelfIntersection(self) -> bool:
        """
        Applies FixSelfIntersectingEdge(num) and
         FixIntersectingEdges(num) to all edges in the wire and
         FixIntersectingEdges(num1, num2) for all pairs num1 and num2
         and removes wrong edges if any
        """
        pass

    @overload
    def fixLacking(self, force: bool = ..., /) -> bool: ...
    @overload
    def fixLacking(self, num: int, force: bool = ..., /) -> bool: ...
    def fixLacking(self, *args) -> bool:
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

    def fixClosed(self, prec: float = ..., /) -> bool:
        """
        Fixes a wire to be well closed
        """
        pass

    def fixGaps3d(self) -> bool:
        """
        Fixes gaps between ends of 3d curves on adjacent edges
        """
        pass

    def fixGaps2d(self) -> bool:
        """
        Fixes gaps between ends of pcurves on adjacent edges
        """
        pass

    def fixSeam(self, num: int, /) -> bool:
        """
        Fixes seam edges
        """
        pass

    def fixShifted(self) -> bool:
        """
        Fixes edges which have pcurves shifted by whole parameter
        range on the closed surface
        """
        pass

    def fixNotchedEdges(self) -> bool:
        """
        Fixes Notch edges.Check if there are notch edges in 2d and fix it
        """
        pass

    def fixGap3d(self, num: int, convert: bool, /) -> bool:
        """
        Fixes gap between ends of 3d curves on num-1 and num-th edges
        """
        pass

    def fixGap2d(self, num: int, convert: bool, /) -> bool:
        """
        Fixes gap between ends of pcurves on num-1 and num-th edges
        """
        pass

    def fixTails(self) -> bool:
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
