from typing import Any

from Base.Metadata import export

from App.GeoFeature import GeoFeature

@export(
    Father="GeoFeaturePy",
    Name="MeshFeaturePy",
    Twin="Feature",
    TwinPointer="Feature",
    Include="Mod/Mesh/App/MeshFeature.h",
    Namespace="Mesh",
    FatherInclude="App/GeoFeaturePy.h",
    FatherNamespace="App",
)
class MeshFeaturePy(GeoFeature):
    """
    The Mesh::Feature class handles meshes.
    The Mesh.MeshFeature() function is for internal use only and cannot be used to create instances of this class.
    Therefore you must have a reference to a document, e.g. 'd' then you can create an instance with
    d.addObject("Mesh::Feature").
    """

    def countPoints(self) -> Any:
        """Return the number of vertices of the mesh object"""
        ...

    def countFacets(self) -> Any:
        """Return the number of facets of the mesh object"""
        ...

    def harmonizeNormals(self) -> Any:
        """Adjust wrong oriented facets"""
        ...

    def smooth(self) -> Any:
        """Smooth the mesh data"""
        ...

    def decimate(self) -> Any:
        """
        Decimate the mesh
        decimate(tolerance(Float), reduction(Float))
        tolerance: maximum error
        reduction: reduction factor must be in the range [0.0,1.0]
        Example:
        mesh.decimate(0.5, 0.1) # reduction by up to 10 percent
        mesh.decimate(0.5, 0.9) # reduction by up to 90 percent

        or

        decimate(targwt size(int))
        mesh.decimate(mesh.CountFacets/2)
        """
        ...

    def removeNonManifolds(self) -> Any:
        """Remove non-manifolds"""
        ...

    def removeNonManifoldPoints(self) -> Any:
        """Remove non-manifold points"""
        ...

    def fixIndices(self) -> Any:
        """Repair any invalid indices"""
        ...

    def fixDegenerations(self) -> Any:
        """Remove degenerated facets"""
        ...

    def removeDuplicatedFacets(self) -> Any:
        """Remove duplicated facets"""
        ...

    def removeDuplicatedPoints(self) -> Any:
        """Remove duplicated points"""
        ...

    def fixSelfIntersections(self) -> Any:
        """Repair self-intersections"""
        ...

    def removeFoldsOnSurface(self) -> Any:
        """Remove folds on surfaces"""
        ...

    def removeInvalidPoints(self) -> Any:
        """Remove points with invalid coordinates (NaN)"""
        ...
