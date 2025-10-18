# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import constmethod, export, class_declarations

from App.ComplexGeoData import ComplexGeoData

@export(
    Twin="MeshObject",
    TwinPointer="MeshObject",
    Include="Mod/Mesh/App/Mesh.h",
    Namespace="Mesh",
    FatherInclude="App/ComplexGeoDataPy.h",
    FatherNamespace="Data",
    Constructor=True,
)
@class_declarations(
    """
    private:
    friend class PropertyMeshKernel;
    class PropertyMeshKernel* parentProperty = nullptr;"""
)
class Mesh(ComplexGeoData):
    """
    Mesh() -- Create an empty mesh object.

    This class allows one to manipulate the mesh object by adding new facets, deleting facets, importing from an STL file,
    transforming the mesh and much more.
    For a complete overview of what can be done see also the documentation of mesh.
    A mesh object cannot be added to an existing document directly. Therefore the document must create an object
    with a property class that supports meshes.
    Example:
    m = Mesh.Mesh()
    ... # Manipulate the mesh
    d = FreeCAD.activeDocument() # Get a reference to the actie document
    f = d.addObject("Mesh::Feature", "Mesh") # Create a mesh feature
    f.Mesh = m # Assign the mesh object to the internal property
    d.recompute()

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    def read(self, **kwargs) -> Any:
        """Read in a mesh object from file.
        mesh.read(Filename='mymesh.stl')
        mesh.read(Stream=file,Format='STL')"""
        ...

    @constmethod
    def write(self, **kwargs) -> Any:
        """Write the mesh object into file.
        mesh.write(Filename='mymesh.stl',[Format='STL',Name='Object name',Material=colors])
        mesh.write(Stream=file,Format='STL',[Name='Object name',Material=colors])"""
        ...

    @constmethod
    def writeInventor(self) -> Any:
        """Write the mesh in OpenInventor format to a string."""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this mesh"""
        ...

    def offset(self) -> Any:
        """Move the point along their normals"""
        ...

    def offsetSpecial(self) -> Any:
        """Move the point along their normals"""
        ...

    @constmethod
    def crossSections(self) -> Any:
        """Get cross-sections of the mesh through several planes"""
        ...

    @constmethod
    def unite(self) -> Any:
        """Union of this and the given mesh object."""
        ...

    @constmethod
    def intersect(self) -> Any:
        """Intersection of this and the given mesh object."""
        ...

    @constmethod
    def difference(self) -> Any:
        """Difference of this and the given mesh object."""
        ...

    @constmethod
    def inner(self) -> Any:
        """Get the part inside of the intersection"""
        ...

    @constmethod
    def outer(self) -> Any:
        """Get the part outside the intersection"""
        ...

    @constmethod
    def section(self, **kwargs) -> Any:
        """Get the section curves of this and the given mesh object.
        lines = mesh.section(mesh2, [ConnectLines=True, MinDist=0.0001])"""
        ...

    def translate(self) -> Any:
        """Apply a translation to the mesh"""
        ...

    def rotate(self) -> Any:
        """Apply a rotation to the mesh"""
        ...

    def transform(self) -> Any:
        """Apply a transformation to the mesh"""
        ...

    def transformToEigen(self) -> Any:
        """Transform the mesh to its eigenbase"""
        ...

    @constmethod
    def getEigenSystem(self) -> Any:
        """Get Eigen base of the mesh"""
        ...

    def addFacet(self) -> Any:
        """Add a facet to the mesh"""
        ...

    def addFacets(self) -> Any:
        """Add a list of facets to the mesh"""
        ...

    def removeFacets(self) -> Any:
        """Remove a list of facet indices from the mesh"""
        ...

    def removeNeedles(self) -> Any:
        """Remove all edges that are smaller than a given length"""
        ...

    def removeFullBoundaryFacets(self) -> Any:
        """Remove facets whose all three points are on the boundary"""
        ...

    @constmethod
    def getInternalFacets(self) -> Any:
        """Builds a list of facet indices with triangles that are inside a volume mesh"""
        ...

    def rebuildNeighbourHood(self) -> Any:
        """Repairs the neighbourhood which might be broken"""
        ...

    def addMesh(self) -> Any:
        """Combine this mesh with another mesh."""
        ...

    def setPoint(self) -> Any:
        """setPoint(int, Vector)
        Sets the point at index."""
        ...

    def movePoint(self) -> Any:
        """movePoint(int, Vector)
        This method moves the point in the mesh along the
        given vector. This affects the geometry of the mesh.
        Be aware that moving points may cause self-intersections."""
        ...

    @constmethod
    def getPointNormals(self) -> Any:
        """getPointNormals()
        Get the normals of the points."""
        ...

    def addSegment(self) -> Any:
        """Add a list of facet indices that describes a segment to the mesh"""
        ...

    @constmethod
    def countSegments(self) -> Any:
        """Get the number of segments which may also be 0"""
        ...

    @constmethod
    def getSegment(self) -> Any:
        """Get a list of facet indices that describes a segment"""
        ...

    @constmethod
    def getSeparateComponents(self) -> Any:
        """Returns a list containing the different
        components (separated areas) of the mesh as separate meshes

        import Mesh
        for c in mesh.getSeparatecomponents():
        Mesh.show(c)"""
        ...

    @constmethod
    def getFacetSelection(self) -> Any:
        """Get a list of the indices of selected facets"""
        ...

    @constmethod
    def getPointSelection(self) -> Any:
        """Get a list of the indices of selected points"""
        ...

    @constmethod
    def meshFromSegment(self) -> Any:
        """Create a mesh from segment"""
        ...

    def clear(self) -> Any:
        """Clear the mesh"""
        ...

    @constmethod
    def isSolid(self) -> Any:
        """Check if the mesh is a solid"""
        ...

    @constmethod
    def hasNonManifolds(self) -> Any:
        """Check if the mesh has non-manifolds"""
        ...

    def removeNonManifolds(self) -> Any:
        """Remove non-manifolds"""
        ...

    def removeNonManifoldPoints(self) -> Any:
        """Remove non-manifold points"""
        ...

    @constmethod
    def hasSelfIntersections(self) -> Any:
        """Check if the mesh intersects itself"""
        ...

    @constmethod
    def getSelfIntersections(self) -> Any:
        """Returns a tuple of indices of intersecting triangles"""
        ...

    def fixSelfIntersections(self) -> Any:
        """Repair self-intersections"""
        ...

    def removeFoldsOnSurface(self) -> Any:
        """Remove folds on surfaces"""
        ...

    @constmethod
    def hasNonUniformOrientedFacets(self) -> Any:
        """Check if the mesh has facets with inconsistent orientation"""
        ...

    @constmethod
    def countNonUniformOrientedFacets(self) -> Any:
        """Get the number of wrong oriented facets"""
        ...

    @constmethod
    def getNonUniformOrientedFacets(self) -> Any:
        """Get a tuple of wrong oriented facets"""
        ...

    @constmethod
    def hasInvalidPoints(self) -> Any:
        """Check if the mesh has points with invalid coordinates (NaN)"""
        ...

    def removeInvalidPoints(self) -> Any:
        """Remove points with invalid coordinates (NaN)"""
        ...

    @constmethod
    def hasPointsOnEdge(self) -> Any:
        """Check if points lie on edges"""
        ...

    def removePointsOnEdge(self, **kwargs) -> Any:
        """removePointsOnEdge(FillBoundary=False)
        Remove points that lie on edges.
        If FillBoundary is True then the holes by removing the affected facets
        will be re-filled."""
        ...

    @constmethod
    def hasInvalidNeighbourhood(self) -> Any:
        """Check if the mesh has invalid neighbourhood indices"""
        ...

    @constmethod
    def hasPointsOutOfRange(self) -> Any:
        """Check if the mesh has point indices that are out of range"""
        ...

    @constmethod
    def hasFacetsOutOfRange(self) -> Any:
        """Check if the mesh has facet indices that are out of range"""
        ...

    @constmethod
    def hasCorruptedFacets(self) -> Any:
        """Check if the mesh has corrupted facets"""
        ...

    @constmethod
    def countComponents(self) -> Any:
        """Get the number of topologic independent areas"""
        ...

    def removeComponents(self) -> Any:
        """Remove components with less or equal to number of given facets"""
        ...

    def fixIndices(self) -> Any:
        """Repair any invalid indices"""
        ...

    def fixCaps(self) -> Any:
        """Repair caps by swapping the edge"""
        ...

    def fixDeformations(self) -> Any:
        """Repair deformed facets"""
        ...

    def fixDegenerations(self) -> Any:
        """Remove degenerated facets"""
        ...

    def removeDuplicatedPoints(self) -> Any:
        """Remove duplicated points"""
        ...

    def removeDuplicatedFacets(self) -> Any:
        """Remove duplicated facets"""
        ...

    def refine(self) -> Any:
        """Refine the mesh"""
        ...

    def splitEdges(self) -> Any:
        """Split all edges"""
        ...

    def splitEdge(self) -> Any:
        """Split edge"""
        ...

    def splitFacet(self) -> Any:
        """Split facet"""
        ...

    def swapEdge(self) -> Any:
        """Swap the common edge with the neighbour"""
        ...

    def collapseEdge(self) -> Any:
        """Remove an edge and both facets that share this edge"""
        ...

    def collapseFacet(self) -> Any:
        """Remove a facet"""
        ...

    def collapseFacets(self) -> Any:
        """Remove a list of facets"""
        ...

    def insertVertex(self) -> Any:
        """Insert a vertex into a facet"""
        ...

    def snapVertex(self) -> Any:
        """Insert a new facet at the border"""
        ...

    @constmethod
    def printInfo(self) -> Any:
        """Get detailed information about the mesh"""
        ...

    @constmethod
    def foraminate(self) -> Any:
        """Get a list of facet indices and intersection points"""
        ...

    def cut(self) -> Any:
        """Cuts the mesh with a given closed polygon
        cut(list, int) -> None
        The argument list is an array of points, a polygon
        The argument int is the mode: 0=inner, 1=outer"""
        ...

    def trim(self) -> Any:
        """Trims the mesh with a given closed polygon
        trim(list, int) -> None
        The argument list is an array of points, a polygon
        The argument int is the mode: 0=inner, 1=outer"""
        ...

    def trimByPlane(self) -> Any:
        """Trims the mesh with a given plane
        trimByPlane(Vector, Vector) -> None
        The plane is defined by a base and normal vector. Depending on the
        direction of the normal the part above or below will be kept."""
        ...

    @constmethod
    def harmonizeNormals(self) -> Any:
        """Adjust wrong oriented facets"""
        ...

    @constmethod
    def flipNormals(self) -> Any:
        """Flip the mesh normals"""
        ...

    @constmethod
    def fillupHoles(self) -> Any:
        """Fillup holes"""
        ...

    @constmethod
    def smooth(self, **kwargs) -> Any:
        """Smooth the mesh
        smooth([iteration=1,maxError=FLT_MAX])"""
        ...

    def decimate(self) -> Any:
        """Decimate the mesh
        decimate(tolerance(Float), reduction(Float))
        tolerance: maximum error
        reduction: reduction factor must be in the range [0.0,1.0]
        Example:
        mesh.decimate(0.5, 0.1) # reduction by up to 10 percent
        mesh.decimate(0.5, 0.9) # reduction by up to 90 percent"""
        ...

    def mergeFacets(self) -> Any:
        """Merge facets to optimize topology"""
        ...

    @constmethod
    def optimizeTopology(self) -> Any:
        """Optimize the edges to get nicer facets"""
        ...

    @constmethod
    def optimizeEdges(self) -> Any:
        """Optimize the edges to get nicer facets"""
        ...

    @constmethod
    def nearestFacetOnRay(self) -> Any:
        """nearestFacetOnRay(tuple, tuple) -> dict
        Get the index and intersection point of the nearest facet to a ray.
        The first parameter is a tuple of three floats the base point of the ray,
        the second parameter is ut uple of three floats for the direction.
        The result is a dictionary with an index and the intersection point or
        an empty dictionary if there is no intersection."""
        ...

    @constmethod
    def getPlanarSegments(self) -> Any:
        """getPlanarSegments(dev,[min faces=0]) -> list
        Get all planes of the mesh as segment.
        In the worst case each triangle can be regarded as single
        plane if none of its neighbours is coplanar."""
        ...

    @constmethod
    def getSegmentsOfType(self) -> Any:
        """getSegmentsOfType(type, dev,[min faces=0]) -> list
        Get all segments of type.
        Type can be Plane, Cylinder or Sphere"""
        ...

    @constmethod
    def getSegmentsByCurvature(self) -> Any:
        """getSegmentsByCurvature(list) -> list
        The argument list gives a list if tuples where it defines the preferred maximum curvature,
        the preferred minimum curvature, the tolerances and the number of minimum faces for the segment.
        Example:
        c=(1.0, 0.0, 0.1, 0.1, 500) # search for a cylinder with radius 1.0
        p=(0.0, 0.0, 0.1, 0.1, 500) # search for a plane
        mesh.getSegmentsByCurvature([c,p])"""
        ...

    @constmethod
    def getCurvaturePerVertex(self) -> Any:
        """getCurvaturePerVertex() -> list
        The items in the list contains minimum and maximum curvature with their directions
        """
        ...
    Points: Final[list]
    """A collection of the mesh points
With this attribute it is possible to get access to the points of the mesh
for p in mesh.Points:
	print p.x, p.y, p.z"""

    CountPoints: Final[int]
    """Return the number of vertices of the mesh object."""

    CountEdges: Final[int]
    """Return the number of edges of the mesh object."""

    Facets: Final[list]
    """A collection of facets
With this attribute it is possible to get access to the facets of the mesh
for p in mesh.Facets:
	print p"""

    CountFacets: Final[int]
    """Return the number of facets of the mesh object."""

    Topology: Final[tuple]
    """Return the points and face indices as tuple."""

    Area: Final[float]
    """Return the area of the mesh object."""

    Volume: Final[float]
    """Return the volume of the mesh object."""
