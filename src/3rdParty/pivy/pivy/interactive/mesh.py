"""Mesh creation utilities for 3D graphics.

This module provides functions to create various types of 3D meshes
using Coin3D, including quad meshes and polygon meshes.
"""

from pivy import coin
from .colors import COLORS
import numpy as np


def simple_quad_mesh(points, num_u, num_v, colors=None):
    """Create a simple quadrilateral mesh.

    Creates a Coin3D SoQuadMesh from a list of 3D points arranged
    in a regular grid pattern.

    Args:
        points: List of 3D points (x, y, z tuples) representing the mesh vertices.
            Points should be ordered row by row (u-direction first, then v-direction).
        num_u: Number of vertices per row (u-direction).
        num_v: Number of vertices per column (v-direction).
        colors: Optional list of RGB color tuples (one per vertex) for per-vertex coloring.
            If None, default material coloring is used.

    Returns:
        coin.SoSeparator: A separator node containing the quad mesh with appropriate
            shape hints for rendering.

    Example:
        >>> points = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0)]
        >>> mesh = simple_quad_mesh(points, 2, 2)
    """
    msh_sep = coin.SoSeparator()
    msh = coin.SoQuadMesh()
    vertexproperty = coin.SoVertexProperty()
    vertexproperty.vertex.setValues(0, len(points), points)
    msh.verticesPerRow = num_u
    msh.verticesPerColumn = num_v
    if colors:
        vertexproperty.materialBinding = coin.SoMaterialBinding.PER_VERTEX
        for i in range(len(colors)):
            vertexproperty.orderedRGBA.set1Value(i, coin.SbColor(colors[i]).getPackedValue())
    msh.vertexProperty = vertexproperty

    shape_hint = coin.SoShapeHints()
    shape_hint.vertexOrdering = coin.SoShapeHints.COUNTERCLOCKWISE
    shape_hint.creaseAngle = np.pi / 3
    msh_sep += [shape_hint, msh]
    return msh_sep


def simple_poly_mesh(verts, poly, color=None):
    """Create a simple polygon mesh from vertices and polygon indices.

    Creates a Coin3D SoIndexedFaceSet from a list of vertices and
    polygon face definitions.

    Args:
        verts: List of 3D vertices (x, y, z tuples) defining the mesh vertices.
        poly: List of polygon definitions. Each polygon is a list of vertex indices
            that form a face. Indices refer to positions in the verts list.
        color: Optional RGB color tuple (r, g, b) in range [0.0, 1.0] for the mesh.
            If None, defaults to grey.

    Returns:
        coin.SoSeparator: A separator node containing the indexed face set with
            appropriate shape hints and material.

    Example:
        >>> vertices = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
        >>> polygons = [[0, 1, 2]]
        >>> mesh = simple_poly_mesh(vertices, polygons, color=(1.0, 0.0, 0.0))
    """
    color = color or COLORS["grey"]
    _vertices = [list(v) for v in verts]
    _polygons = []
    for pol in poly:
        _polygons += list(pol) + [-1]
    sep = coin.SoSeparator()
    vertex_property = coin.SoVertexProperty()
    face_set = coin.SoIndexedFaceSet()
    shape_hint = coin.SoShapeHints()
    shape_hint.vertexOrdering = coin.SoShapeHints.COUNTERCLOCKWISE
    shape_hint.creaseAngle = np.pi / 3
    face_mat = coin.SoMaterial()
    face_mat.diffuseColor = color
    vertex_property.vertex.setValues(0, len(_vertices), _vertices)
    face_set.coordIndex.setValues(0, len(_polygons), list(_polygons))
    vertex_property.materialBinding = coin.SoMaterialBinding.PER_VERTEX_INDEXED
    sep += [shape_hint, vertex_property, face_mat, face_set]
    return sep

