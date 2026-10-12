# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``ReverseEngineering`` module helpers.

This source-adjacent stub file carries the point-cloud approximation,
reconstruction, filtering, segmentation, and sample-consensus helpers exposed
directly by the ReverseEngineering application module.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import Literal, TypeAlias, TypedDict, overload

from FreeCAD.Base import Vector
from Mesh import Mesh
from Part import BSplineCurve, BSplineSurface
from Points import Points

_Point3: TypeAlias = tuple[float, float, float]
_VectorLike: TypeAlias = Vector | _Point3
_CurvePointCloud: TypeAlias = Points | Sequence[_VectorLike]
_SurfacePointCloud: TypeAlias = Points | Mesh | Sequence[_VectorLike]
_Normals: TypeAlias = Sequence[_VectorLike]
_SegmentationCluster: TypeAlias = tuple[int, ...]
_UvDirections: TypeAlias = tuple[_VectorLike, _VectorLike]
_ParametrizationType: TypeAlias = Literal["Uniform", "Centripetal", "ChordLength"] | str
_SacModel: TypeAlias = Literal["Plane", "Cylinder", "Sphere", "Cone"] | str
SampleConsensusResult = TypedDict(
    "SampleConsensusResult",
    {
        "Probability": float,
        "Parameters": tuple[float, ...],
        "Model": tuple[int, ...],
    },
)

# Approximation helpers
@overload
def approxCurve(
    Points: _CurvePointCloud,
    Closed: bool = False,
    MinDegree: int = 3,
    MaxDegree: int = 8,
    Continuity: int = 2,
    Tolerance: float = 1.0e-3,
) -> BSplineCurve:
    """Approximate one point sequence with one B-spline curve."""
    ...

@overload
def approxCurve(
    Points: _CurvePointCloud,
    ParametrizationType: _ParametrizationType,
    Closed: bool = False,
    MinDegree: int = 3,
    MaxDegree: int = 8,
    Continuity: int = 2,
    Tolerance: float = 1.0e-3,
) -> BSplineCurve:
    """Approximate one point sequence with one explicitly parametrized B-spline curve."""
    ...

@overload
def approxCurve(
    Points: _CurvePointCloud,
    Weight1: float,
    Weight2: float,
    Weight3: float,
    Closed: bool = False,
    MaxDegree: int = 8,
    Continuity: int = 2,
    Tolerance: float = 1.0e-3,
) -> BSplineCurve:
    """Approximate one point sequence with weighted energy terms."""
    ...

def approxSurface(
    Points: _SurfacePointCloud,
    UDegree: int = 3,
    VDegree: int = 3,
    NbUPoles: int = 6,
    NbVPoles: int = 6,
    Smooth: bool = True,
    Weight: float = 0.1,
    Grad: float = 1.0,
    Bend: float = 0.0,
    Curv: float = 0.0,
    Iterations: int = 5,
    Correction: bool = True,
    PatchFactor: float = 1.0,
    UVDirs: _UvDirections | None = None,
) -> BSplineSurface:
    """Approximate one point cloud or mesh with one B-spline surface."""
    ...

# Point-cloud to mesh reconstruction
def triangulate(
    Points: Points,
    SearchRadius: float,
    Mu: float = 2.5,
    KSearch: int = 5,
    Normals: _Normals | None = None,
) -> Mesh:
    """Triangulate one point cloud into one mesh with greedy surface reconstruction."""
    ...

def poissonReconstruction(
    Points: Points,
    KSearch: int = 5,
    OctreeDepth: int = -1,
    SolverDivide: int = -1,
    SamplesPerNode: float = -1.0,
    Normals: _Normals | None = None,
) -> Mesh:
    """Reconstruct one mesh from one point cloud with the Poisson method."""
    ...

def viewTriangulation(Points: Points, Width: int, Height: int) -> Mesh:
    """Triangulate one regularly sampled height field into one mesh."""
    ...

def gridProjection(
    Points: Points,
    KSearch: int = 5,
    Normals: _Normals | None = None,
) -> Mesh:
    """Reconstruct one mesh from one point cloud with grid projection."""
    ...

def marchingCubesRBF(
    Points: Points,
    KSearch: int = 5,
    Normals: _Normals | None = None,
) -> Mesh:
    """Reconstruct one mesh from one point cloud with radial-basis marching cubes."""
    ...

def marchingCubesHoppe(
    Points: Points,
    KSearch: int = 5,
    Normals: _Normals | None = None,
) -> Mesh:
    """Reconstruct one mesh from one point cloud with Hoppe marching cubes."""
    ...

def fitBSpline(
    Points: Points,
    Degree: int = 2,
    Refinement: int = 4,
    Iterations: int = 10,
    InteriorSmoothness: float = 0.2,
    InteriorWeight: float = 1.0,
    BoundarySmoothness: float = 0.2,
    BoundaryWeight: float = 0.0,
) -> BSplineSurface:
    """Fit one B-spline surface directly to one point cloud."""
    ...

# Filtering and analysis
def filterVoxelGrid(
    Points: Points,
    DimX: float,
    DimY: float = 0.0,
    DimZ: float = 0.0,
) -> Points:
    """Downsample one point cloud with one voxel-grid filter."""
    ...

def normalEstimation(
    Points: Points,
    KSearch: int = 0,
    SearchRadius: float = 0.0,
) -> list[Vector]:
    """Estimate one normal vector per point in one point cloud."""
    ...

def regionGrowingSegmentation(
    Points: Points,
    KSearch: int = 5,
    Normals: _Normals | None = None,
) -> list[_SegmentationCluster]:
    """Segment one point cloud into smooth regions and return point-index clusters."""
    ...

def featureSegmentation(Points: Points, KSearch: int = 5) -> list[_SegmentationCluster]:
    """Segment one point cloud by geometric features and return point-index clusters."""
    ...

def sampleConsensus(
    SacModel: _SacModel,
    Points: Points,
    Normals: _Normals | None = None,
) -> SampleConsensusResult:
    """Fit one sample-consensus primitive model to one point cloud."""
    ...
