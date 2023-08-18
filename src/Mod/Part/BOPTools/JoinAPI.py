#/***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

__title__="BOPTools.JoinAPI module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "JoinFeatures functions that operate on shapes."

import Part
from . import ShapeMerge
from .GeneralFuseResult import GeneralFuseResult
from .Utils import compoundLeaves

def shapeOfMaxSize(list_of_shapes):
    """shapeOfMaxSize(list_of_shapes): finds the shape that has the largest "
    "mass in the list and returns it. The shapes in the list must be of same dimension."""
    #first, check if shapes can be compared by size
    ShapeMerge.dimensionOfShapes(list_of_shapes)

    rel_precision = 1e-8

    #find it!
    max_size = -1e100 # max size encountered so far
    count_max = 0 # number of shapes with size equal to max_size
    shape_max = None # shape of max_size
    for sh in list_of_shapes:
        v = abs(Part.cast_to_shape(sh).Mass)
        if v > max_size*(1 + rel_precision) :
            max_size = v
            shape_max = sh
            count_max = 1
        elif (1-rel_precision) * max_size <= v and v <= (1+rel_precision) * max_size :
            count_max = count_max + 1
    if count_max > 1 :
        raise ValueError("There is more than one largest piece!")
    return shape_max

def connect(list_of_shapes, tolerance = 0.0):
    """connect(list_of_shapes, tolerance = 0.0): connects solids (walled objects), shells and
    wires by throwing off small parts that result when splitting them at intersections.

    Compounds in list_of_shapes are automatically exploded, so self-intersecting compounds
    are valid for connect."""

    # explode all compounds before GFA.
    new_list_of_shapes = []
    for sh in list_of_shapes:
        new_list_of_shapes.extend( compoundLeaves(sh) )
    list_of_shapes = new_list_of_shapes

    #test if shapes are compatible for connecting
    dim = ShapeMerge.dimensionOfShapes(list_of_shapes)
    if dim == 0:
        raise TypeError("Cannot connect vertices!")

    if len(list_of_shapes) < 2:
        return Part.makeCompound(list_of_shapes)

    pieces, map = list_of_shapes[0].generalFuse(list_of_shapes[1:], tolerance)
    ao = GeneralFuseResult(list_of_shapes, (pieces, map))
    ao.splitAggregates()
    #print len(ao.pieces)," pieces total"

    keepers = []
    all_danglers = [] # debug

    #add all biggest dangling pieces
    for src in ao.source_shapes:
        danglers = [piece for piece in ao.piecesFromSource(src) if len(ao.sourcesOfPiece(piece)) == 1]
        all_danglers.extend(danglers)
        largest = shapeOfMaxSize(danglers)
        if largest is not None:
            keepers.append(largest)

    touch_test_list = Part.Compound(keepers)
    #add all intersection pieces that touch danglers, triple intersection pieces that touch duals, and so on
    for ii in range(2, ao.largestOverlapCount()+1):
        list_ii_pieces = [piece for piece in ao.pieces if len(ao.sourcesOfPiece(piece)) == ii]
        keepers_2_add = []
        for piece in list_ii_pieces:
            if ShapeMerge.isConnected(piece, touch_test_list):
                keepers_2_add.append(piece)
        if len(keepers_2_add) == 0:
            break
        keepers.extend(keepers_2_add)
        touch_test_list = Part.Compound(keepers_2_add)


    #merge, and we are done!
    #print len(keepers)," pieces to keep"
    return ShapeMerge.mergeShapes(keepers)

def connect_legacy(shape1, shape2, tolerance = 0.0):
    """connect_legacy(shape1, shape2, tolerance = 0.0): alternative implementation of
    connect, without use of generalFuse. Slow. Provided for backwards compatibility, and
    for older OCC."""

    if tolerance>0.0:
        import FreeCAD as App
        App.Console.PrintWarning("connect_legacy does not support tolerance (yet).\n")
    cut1 = shape1.cut(shape2)
    cut1 = shapeOfMaxSize(cut1.childShapes())
    cut2 = shape2.cut(shape1)
    cut2 = shapeOfMaxSize(cut2.childShapes())
    return cut1.multiFuse([cut2, shape2.common(shape1)])

#def embed(shape_base, shape_tool, tolerance = 0.0):
#    (TODO)

def embed_legacy(shape_base, shape_tool, tolerance = 0.0):
    """embed_legacy(shape_base, shape_tool, tolerance = 0.0): alternative implementation of
    embed, without use of generalFuse. Slow. Provided for backwards compatibility, and
    for older OCC."""
    if tolerance>0.0:
        import FreeCAD as App
        App.Console.PrintWarning("embed_legacy does not support tolerance (yet).\n")

    # using legacy implementation, except adding support for shells
    pieces = compoundLeaves(shape_base.cut(shape_tool))
    piece = shapeOfMaxSize(pieces)
    result = piece.fuse(shape_tool)
    dim = ShapeMerge.dimensionOfShapes(pieces)
    if dim == 2:
        # fusing shells returns shells that are still split. Reassemble them
        result = ShapeMerge.mergeShapes(result.Faces)
    elif dim == 1:
        result = ShapeMerge.mergeShapes(result.Edges)
    return result

def cutout_legacy(shape_base, shape_tool, tolerance = 0.0):
    """cutout_legacy(shape_base, shape_tool, tolerance = 0.0): alternative implementation of
    cutout, without use of generalFuse. Slow. Provided for backwards compatibility, and
    for older OCC."""

    if tolerance>0.0:
        import FreeCAD as App
        App.Console.PrintWarning("cutout_legacy does not support tolerance (yet).\n")
    #if base is multi-piece, work on per-piece basis
    shapes_base = compoundLeaves(shape_base)
    if len(shapes_base) > 1:
        result = []
        for sh in shapes_base:
            result.append(cutout(sh, shape_tool))
        return Part.Compound(result)

    shape_base = shapes_base[0]
    pieces = compoundLeaves(shape_base.cut(shape_tool))
    return shapeOfMaxSize(pieces)
