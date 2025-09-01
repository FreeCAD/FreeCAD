from typing import Any

from Base.Metadata import export
from TechDraw.rawView import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawViewPartPy",
    Twin="DrawViewPart",
    TwinPointer="DrawViewPart",
    Include="Mod/TechDraw/App/DrawViewPart.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewPartPy(DrawView):
    """
    Feature for creating and manipulating Technical Drawing Part Views
    """

    def getVisibleEdges(self) -> Any:
        """
        getVisibleEdges([conventionalCoords]) - get the visible edges in the View as Part::TopoShapeEdges. Edges are returned
        in conventional coordinates if conventionalCoords is True.  The default is to return Qt inverted Y coordinates.
        """
        ...

    def getVisibleVertexes(self) -> Any:
        """
        getVisibleVertexes() - get the visible vertexes as App.Vector in the View's coordinate system.  App.Vectors are returned
        in conventional coordinates if conventionalCoords is True.  The default is to return Qt inverted Y coordinates.
        """
        ...

    def getHiddenEdges(self) -> Any:
        """
        getHiddenEdges([conventionalCoords]) - get the hidden edges in the View as Part::TopoShapeEdges.  Edges are returned
        in conventional coordinates if conventionalCoords is True.  The default is to return Qt inverted Y coordinates.
        """
        ...

    def getHiddenVertexes(self) -> Any:
        """
        getHiddenVertexes() - get the hidden vertexes as App.Vector in the View's coordinate system.  App.Vectors are returned
        in conventional coordinates if conventionalCoords is True.  The default is to return Qt inverted Y coordinates.
        """
        ...

    def makeCosmeticVertex(self) -> Any:
        """id = makeCosmeticVertex(p1) - add a CosmeticVertex at p1 (View coordinates). Returns unique id vertex."""
        ...

    def makeCosmeticVertex3d(self) -> Any:
        """id = makeCosmeticVertex3d(p1) - add a CosmeticVertex at p1 (3d model coordinates). Returns unique id vertex."""
        ...

    def getCosmeticVertex(self) -> Any:
        """cv = getCosmeticVertex(id) - returns CosmeticVertex with unique id."""
        ...

    def getCosmeticVertexBySelection(self) -> Any:
        """cv = getCosmeticVertexBySelection(name) - returns CosmeticVertex with name (Vertex6).  Used in selections."""
        ...

    def removeCosmeticVertex(self) -> Any:
        """removeCosmeticVertex(cv) - remove CosmeticVertex from View. Returns None."""
        ...

    def clearCosmeticVertices(self) -> Any:
        """clearCosmeticVertices() - remove all CosmeticVertices from the View. Returns None."""
        ...

    def makeCosmeticLine(self) -> Any:
        """tag = makeCosmeticLine(p1, p2) - add a CosmeticEdge from p1 to p2(View coordinates). Returns tag of new CosmeticEdge."""
        ...

    def makeCosmeticLine3D(self) -> Any:
        """tag = makeCosmeticLine3D(p1, p2) - add a CosmeticEdge from p1 to p2(3D coordinates). Returns tag of new CosmeticEdge."""
        ...

    def makeCosmeticCircle(self) -> Any:
        """tag = makeCosmeticCircle(center, radius) - add a CosmeticEdge at center with radius radius(View coordinates). Returns tag of new CosmeticEdge."""
        ...

    def makeCosmeticCircleArc(self) -> Any:
        """tag = makeCosmeticCircleArc(center, radius, start, end) - add a CosmeticEdge at center with radius radius(View coordinates) from start angle to end angle. Returns tag of new CosmeticEdge."""
        ...

    def makeCosmeticCircle3d(self) -> Any:
        """tag = makeCosmeticCircle3d(center, radius) - add a CosmeticEdge at center (3d point) with radius. Returns tag of new CosmeticEdge."""
        ...

    def makeCosmeticCircleArc3d(self) -> Any:
        """tag = makeCosmeticCircleArc3d(center, radius, start, end) - add a CosmeticEdge at center (3d point) with radius from start angle to end angle. Returns tag of new CosmeticEdge."""
        ...

    def getCosmeticEdge(self) -> Any:
        """ce = getCosmeticEdge(id) - returns CosmeticEdge with unique id."""
        ...

    def getCosmeticEdgeBySelection(self) -> Any:
        """ce = getCosmeticEdgeBySelection(name) - returns CosmeticEdge by name (Edge25).  Used in selections"""
        ...

    def removeCosmeticEdge(self) -> Any:
        """removeCosmeticEdge(ce) - remove CosmeticEdge ce from View. Returns None."""
        ...

    def makeCenterLine(self) -> Any:
        """makeCenterLine(subNames, mode) - draw a center line on this viewPart. SubNames is a list of n Faces, 2 Edges or 2 Vertices (ex [Face1,Face2,Face3]. Returns unique tag of added CenterLine."""
        ...

    def getCenterLine(self) -> Any:
        """cl = getCenterLine(id) - returns CenterLine with unique id."""
        ...

    def getCenterLineBySelection(self) -> Any:
        """cl = getCenterLineBySelection(name) - returns CenterLine by name (Edge25).  Used in selections"""
        ...

    def removeCenterLine(self) -> Any:
        """removeCenterLine(cl) - remove CenterLine cl from View. Returns None."""
        ...

    def clearCosmeticEdges(self) -> Any:
        """clearCosmeticEdges() - remove all CosmeticLines from the View. Returns None."""
        ...

    def clearCenterLines(self) -> Any:
        """clearCenterLines() - remove all CenterLines from the View. Returns None."""
        ...

    def clearGeomFormats(self) -> Any:
        """clearGeomFormats() - remove all GeomFormats from the View. Returns None."""
        ...

    def formatGeometricEdge(self) -> Any:
        """formatGeometricEdge(index, style, weight, color, visible). Returns None."""
        ...

    def getEdgeByIndex(self) -> Any:
        """getEdgeByIndex(edgeIndex). Returns Part.TopoShape."""
        ...

    def getEdgeBySelection(self) -> Any:
        """getEdgeBySelection(edgeName). Returns Part.TopoShape."""
        ...

    def getVertexByIndex(self) -> Any:
        """getVertexByIndex(vertexIndex). Returns Part.TopoShape."""
        ...

    def getVertexBySelection(self) -> Any:
        """getVertexBySelection(vertexName). Returns Part.TopoShape."""
        ...

    def projectPoint(self) -> Any:
        """
        projectPoint(vector3d point, [bool invert]). Returns the projection of point in the
        projection coordinate system of this DrawViewPart. Optionally inverts the Y coordinate of the
        result.
        """
        ...

    def getGeometricCenter(self) -> Any:
        """point3d = getGeometricCenter() - returns the geometric center of the source shapes."""
        ...

    def requestPaint(self) -> Any:
        """requestPaint(). Redraw the graphic for this View."""
        ...
