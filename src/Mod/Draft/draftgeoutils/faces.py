# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions to work with faces."""
## @package faces
# \ingroup draftgeoutils
# \brief Provides various functions to work with faces.

import lazy_loader.lazy_loader as lz

import DraftVecUtils
from FreeCAD import Base
from draftgeoutils.general import precision
from draftgeoutils.geometry import are_coplanar

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def concatenate(shape):
    """Turn several faces into one."""
    boundary_edges = getBoundary(shape)
    sorted_edges = Part.sortEdges(boundary_edges)

    try:
        wires = [Part.Wire(edges) for edges in sorted_edges]
        face = Part.makeFace(wires, "Part::FaceMakerBullseye")
    except Base.FreeCADError:
        print("DraftGeomUtils: Fails to join faces into one. "
              + "The precision of the faces would be insufficient")
        return shape
    else:
        if not wires[0].isClosed():
            return wires[0]
        else:
            return face


def getBoundary(shape):
    """Return the boundary edges of a group of faces."""
    if isinstance(shape, list):
        shape = Part.makeCompound(shape)

    # Make a lookup-table where we get the number of occurrences
    # to each edge in the fused face
    table = dict()
    for f in shape.Faces:
        for e in f.Edges:
            hash_code = e.hashCode()
            if hash_code in table:
                table[hash_code] = table[hash_code] + 1
            else:
                table[hash_code] = 1

    # Filter out the edges shared by more than one sub-face
    bound = list()
    for e in shape.Edges:
        if table[e.hashCode()] == 1:
            bound.append(e)
    return bound


def is_coplanar(faces, tol=-1):
    """Return True if all faces in the given list are coplanar.

    Parameters
    ----------
    faces: list
        List of faces to check coplanarity.
    tol: float, optional
        It defaults to `-1`, the tolerance of confusion, equal to 1e-7.
        Is the maximum deviation to be considered coplanar.

    Returns
    -------
    out: bool
        True if all face are coplanar. False in other case.
    """

    first_face = faces[0]
    for face in faces:
        if not are_coplanar(first_face, face, tol):
            return False

    return True

isCoplanar = is_coplanar


def bind(w1, w2, per_segment=False):
    """Bind 2 wires by their endpoints and returns a face.

    If per_segment is True and the wires have the same number of edges, the
    wires are processed per segment: a separate face is created for each pair
    of edges (one from w1 and one from w2), and the faces are then fused. This
    avoids problems with walls based on wires that selfintersect, or that have
    a loop that ends in a T-connection (f.e. a wire shaped like a number 6).
    """

    def create_face(w1, w2):
        try:
            w3 = Part.LineSegment(w1.Vertexes[0].Point,
                                  w2.Vertexes[0].Point).toShape()
            w4 = Part.LineSegment(w1.Vertexes[-1].Point,
                                  w2.Vertexes[-1].Point).toShape()
            return Part.Face(Part.Wire(w1.Edges + [w3] + w2.Edges + [w4]))
        except Part.OCCError:
            print("DraftGeomUtils: unable to bind wires")
            return None

    if not w1 or not w2:
        print("DraftGeomUtils: unable to bind wires")
        return None

    if (per_segment
            and len(w1.Edges) > 1
            and len(w1.Edges) == len(w2.Edges)):
        faces = []
        for (edge1, edge2) in zip(w1.Edges, w2.Edges):
            face = create_face(edge1, edge2)
            if face is None:
                return None
            faces.append(face)
        # return concatenate(faces[0].fuse(faces[1:])) # Also works.
        return faces[0].fuse(faces[1:]).removeSplitter().Faces[0]
    elif w1.isClosed() and w2.isClosed():
        d1 = w1.BoundBox.DiagonalLength
        d2 = w2.BoundBox.DiagonalLength
        if d1 < d2:
            w1, w2 = w2, w1
        # return Part.Face(w1).cut(Part.Face(w2)).Faces[0] # Only works if wires do not self-intersect.
        try:
            face = Part.Face([w1, w2])
            face.fix(1e-7, 0, 1)
            return face
        except Part.OCCError:
            print("DraftGeomUtils: unable to bind wires")
            return None
    else:
        return create_face(w1, w2)


def cleanFaces(shape):
    """Remove inner edges from coplanar faces."""
    faceset = shape.Faces

    def find(hc):
        """Find a face with the given hashcode."""
        for f in faceset:
            if f.hashCode() == hc:
                return f

    def findNeighbour(hface, hfacelist):
        """Find the first neighbour of a face, and return its index."""
        eset = []
        for e in find(hface).Edges:
            eset.append(e.hashCode())
        for i in range(len(hfacelist)):
            for ee in find(hfacelist[i]).Edges:
                if ee.hashCode() in eset:
                    return i
        return None

    # build lookup table
    lut = {}
    for face in faceset:
        for edge in face.Edges:
            if edge.hashCode() in lut:
                lut[edge.hashCode()].append(face.hashCode())
            else:
                lut[edge.hashCode()] = [face.hashCode()]

    # print("lut:",lut)
    # take edges shared by 2 faces
    sharedhedges = []
    for k, v in lut.items():
        if len(v) == 2:
            sharedhedges.append(k)

    # print(len(sharedhedges)," shared edges:",sharedhedges)
    # find those with same normals
    targethedges = []
    for hedge in sharedhedges:
        faces = lut[hedge]
        n1 = find(faces[0]).normalAt(0.5, 0.5)
        n2 = find(faces[1]).normalAt(0.5, 0.5)
        if n1 == n2:
            targethedges.append(hedge)

    # print(len(targethedges)," target edges:",targethedges)
    # get target faces
    hfaces = []
    for hedge in targethedges:
        for f in lut[hedge]:
            if f not in hfaces:
                hfaces.append(f)

    # print(len(hfaces)," target faces:",hfaces)
    # sort islands
    islands = [[hfaces.pop(0)]]
    currentisle = 0
    currentface = 0
    found = True
    while hfaces:
        if not found:
            if len(islands[currentisle]) > (currentface + 1):
                currentface += 1
                found = True
            else:
                islands.append([hfaces.pop(0)])
                currentisle += 1
                currentface = 0
                found = True
        else:
            f = findNeighbour(islands[currentisle][currentface], hfaces)
            if f is not None:
                islands[currentisle].append(hfaces.pop(f))
            else:
                found = False

    # print(len(islands)," islands:",islands)
    # make new faces from islands
    newfaces = []
    treated = []
    for isle in islands:
        treated.extend(isle)
        fset = []
        for i in isle:
            fset.append(find(i))
        bounds = getBoundary(fset)
        shp = Part.Wire(Part.__sortEdges__(bounds))
        shp = Part.Face(shp)
        if shp.normalAt(0.5, 0.5) != find(isle[0]).normalAt(0.5, 0.5):
            shp.reverse()
        newfaces.append(shp)

    # print("new faces:",newfaces)
    # add remaining faces
    for f in faceset:
        if not f.hashCode() in treated:
            newfaces.append(f)

    # print("final faces")
    # finishing
    fshape = Part.makeShell(newfaces)
    if shape.isClosed():
        fshape = Part.makeSolid(fshape)
    return fshape


def removeSplitter(shape):
    """Return a face from removing the splitter in a list of faces.

    This is an alternative, shared edge-based version of Part.removeSplitter.
    Returns a face, or `None` if the operation failed.
    """
    lookup = dict()
    for f in shape.Faces:
        for e in f.Edges:
            h = e.hashCode()
            if h in lookup:
                lookup[h].append(e)
            else:
                lookup[h] = [e]

    edges = [e[0] for e in lookup.values() if len(e) == 1]

    try:
        face = Part.Face(Part.Wire(edges))
    except Part.OCCError:
        # operation failed
        return None
    else:
        if face.isValid():
            return face

    return None

## @}
