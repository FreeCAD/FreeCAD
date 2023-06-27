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

__title__="BOPTools.Utils module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "Utility code, used by various modules of BOPTools."

class HashableShape(object):
    "Decorator for Part.Shape, that can be used as key in dicts. Based on isSame method."
    def __init__(self, shape):
        self.Shape = shape
        self.hash = shape.hashCode()

    def __eq__(self, other):
        return self.Shape.isSame(other.Shape)

    def __hash__(self):
        return self.hash

class HashableShape_Deep(object):
    """Similar to HashableShape, except that the things the shape is composed of are compared.

Example:
    >>> wire2 = Part.Wire(wire1.childShapes())
    >>> wire2.isSame(wire1)
    False # <--- the wire2 is a new wire, although made of edges of wire1
    >>> HashableShape_Deep(wire2) == HashableShape_Deep(wire1)
    True # <--- made of same set of elements
    """

    def __init__(self, shape):
        self.Shape = shape
        self.hash = 0
        for el in shape.childShapes():
            self.hash = self.hash ^ el.hashCode()

    def __eq__(self, other):
        # avoiding extensive comparison for now. Just doing a few extra tests should reduce the already-low chances of false-positives
        if self.hash == other.hash:
            if len(self.Shape.childShapes()) == len(other.Shape.childShapes()):
                if self.Shape.ShapeType == other.Shape.ShapeType:
                    return True
        return False

    def __hash__(self):
        return self.hash

def compoundLeaves(shape_or_compound):
    """compoundLeaves(shape_or_compound): extracts all non-compound shapes from a nested compound.
    Note: shape_or_compound may be a non-compound; then, it is the only thing in the
    returned list."""

    if shape_or_compound.ShapeType == "Compound":
        leaves = []
        for child in shape_or_compound.childShapes():
            leaves.extend( compoundLeaves(child) )
        return leaves
    else:
        return [shape_or_compound]

def upgradeToAggregateIfNeeded(list_of_shapes, types = None):
    """upgradeToAggregateIfNeeded(list_of_shapes, types = None): upgrades non-aggregate type
    shapes to aggregate-type shapes if the list has a mix of aggregate and non-aggregate
    type shapes. Returns the new list. Recursively traverses into compounds.

    aggregate shape types are Wire, Shell, CompSolid
    non-aggregate shape types are Vertex, Edge, Face, Solid
    Compounds are something special: they are recursively traversed to upgrade the
    contained shapes.

    Examples:
    list_of_shapes contains only faces -> nothing happens
    list_of_shapes contains faces and shells -> faces are converted to shells

    'types' argument is needed for recursive traversal. Do not supply."""

    import Part
    if types is None:
        types = set()
    for shape in list_of_shapes:
        types.add(shape.ShapeType)
        subshapes = compoundLeaves(shape)
        for subshape in subshapes:
            types.add(subshape.ShapeType)
    if "Wire" in types:
        list_of_shapes = [(Part.Wire([shape]) if shape.ShapeType == "Edge" else shape) for shape in list_of_shapes]
    if "Shell" in types:
        list_of_shapes = [(Part.Shell([shape]) if shape.ShapeType == "Face" else shape) for shape in list_of_shapes]
    if "CompSolid" in types:
        list_of_shapes = [(Part.CompSolid([shape]) if shape.ShapeType == "Solid" else shape) for shape in list_of_shapes]
    if "Compound" in types:
        list_of_shapes = [(Part.Compound(upgradeToAggregateIfNeeded(shape.childShapes(), types)) if shape.ShapeType == "Compound" else shape) for shape in list_of_shapes]
    return list_of_shapes

# adapted from http://stackoverflow.com/a/3603824/6285007
class FrozenClass(object):
    '''FrozenClass: prevents adding new attributes to class outside of __init__'''
    __isfrozen = False
    def __setattr__(self, key, value):
        if self.__isfrozen and not hasattr(self, key):
            raise TypeError( "{cls} has no attribute {attr}".format(cls= self.__class__.__name__, attr= key) )
        object.__setattr__(self, key, value)

    def _freeze(self):
        self.__isfrozen = True

    def _unfreeze(self):
        self.__isfrozen = False
