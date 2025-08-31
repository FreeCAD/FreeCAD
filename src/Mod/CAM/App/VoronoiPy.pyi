from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import constmethod, export

@export(
    Father="BaseClassPy",
    Name="VoronoiPy",
    PythonName="Path.Voronoi.Diagram",
    Twin="Voronoi",
    TwinPointer="Voronoi",
    Include="Mod/CAM/App/Voronoi.h",
    Namespace="Path",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class VoronoiPy(BaseClass):
    """
    Voronoi([segments]): Create voronoi for given collection of line segments
    """

    @constmethod
    def numCells(self) -> Any:
        """Return number of cells"""
        ...

    @constmethod
    def numEdges(self) -> Any:
        """Return number of edges"""
        ...

    @constmethod
    def numVertices(self) -> Any:
        """Return number of vertices"""
        ...

    def addPoint(self) -> Any:
        """addPoint(vector|vector2d) add given point to input collection"""
        ...

    def addSegment(self) -> Any:
        """addSegment(vector|vector2d, vector|vector2d) add given segment to input collection"""
        ...

    def construct(self) -> Any:
        """constructs the voronoi diagram from the input collections"""
        ...

    def colorExterior(self) -> Any:
        """assign given color to all exterior edges and vertices"""
        ...

    def colorTwins(self) -> Any:
        """assign given color to all twins of edges (which one is considered a twin is arbitrary)"""
        ...

    def colorColinear(self) -> Any:
        """assign given color to all edges sourced by two segments almost in line with each other (optional angle in degrees)"""
        ...

    def resetColor(self) -> Any:
        """assign color 0 to all elements with the given color"""
        ...

    @constmethod
    def getPoints(self) -> Any:
        """Get list of all input points."""
        ...

    @constmethod
    def numPoints(self) -> Any:
        """Return number of input points"""
        ...

    @constmethod
    def getSegments(self) -> Any:
        """Get list of all input segments."""
        ...

    @constmethod
    def numSegments(self) -> Any:
        """Return number of input segments"""
        ...
    Cells: Final[list]
    """List of all cells of the voronoi diagram"""

    Edges: Final[list]
    """List of all edges of the voronoi diagram"""

    Vertices: Final[list]
    """List of all vertices of the voronoi diagram"""
