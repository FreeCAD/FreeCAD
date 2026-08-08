# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

from dataclasses import dataclass

from FreeCAD import Document, DocumentObject, PropertyContainer, Vector, Matrix
import Materials
import Part


AVERAGE_SUBSHAPE_TOLERANCE_MODE = 0
MAX_SUBSHAPE_TOLERANCE_MODE = 1
MIN_SUBSHAPE_TOLERANCE_MODE = -1


class KeyedDocumentObject:
    """Wraps a DocumentObject for comparison on Name and TypeId."""
    def __init__(self, obj: DocumentObject):
        self.obj = obj

    def __eq__(self, other):
        if not isinstance(other, KeyedDocumentObject):
            return NotImplemented
        return self.obj.Name == other.obj.Name and self.obj.TypeId == other.obj.TypeId

    def __hash__(self):
        return hash((self.obj.Name, self.obj.TypeId))


class KeyedProperty:
    """Wraps a Property for comparison on Name and TypeId."""
    def __init__(self, container: PropertyContainer, prop_name: str):
        self.name = prop_name
        self.type_id = container.getTypeIdOfProperty(self.name)

    def __eq__(self, other):
        if not isinstance(other, KeyedProperty):
            return NotImplemented
        return self.name == other.name and self.type_id == other.type_id

    def __hash__(self):
        return hash((self.name, self.type_id))


@dataclass
class ShapeFingerprint:
    volume: float
    area: float
    length: float
    bound_box: tuple[float, float, float, float, float, float]
    center_of_mass: Vector
    num_faces: int
    num_edges: int
    num_vertices: int
    inertia_matrix: Matrix
    tolerance: float

    @staticmethod
    def from_shape(shape: Part.Shape) -> 'ShapeFingerprint':
        bb = shape.BoundBox

        return ShapeFingerprint(
            volume=shape.Volume,
            area=shape.Area,
            length=shape.Length,
            bound_box=(bb.XMin, bb.YMin, bb.ZMin, bb.XMax, bb.YMax, bb.ZMax),
            center_of_mass=shape.CenterOfMass,
            num_faces=len(shape.Faces),
            num_edges=len(shape.Edges),
            num_vertices=len(shape.Vertexes),
            inertia_matrix=shape.MatrixOfInertia,
            tolerance=shape.getTolerance(AVERAGE_SUBSHAPE_TOLERANCE_MODE),
        )

    def is_equal(self, other: 'ShapeFingerprint') -> bool:
        if not isinstance(other, ShapeFingerprint):
            return NotImplemented

        if (self.num_faces != other.num_faces or
            self.num_edges != other.num_edges or
            self.num_vertices != other.num_vertices):
            return False

        tol = min(self.tolerance, other.tolerance)

        def equal_float(a: float, b: float) -> bool:
            return abs(a - b) <= tol

        def equal_vec(a: Vector, b: Vector) -> bool:
            return a.distanceToPoint(b) <= tol

        if (not equal_float(self.volume, other.volume)
            or not equal_float(self.length, other.length)
            or not equal_float(self.area, other.area)
            ):
            return False

        if not equal_vec(self.center_of_mass, other.center_of_mass):
            return False

        for v1, v2 in zip(self.bound_box, other.bound_box):
            if not equal_float(v1, v2):
                return False

        for i in range(16):
            if not equal_float(self.inertia_matrix.A[i], other.inertia_matrix.A[i]):
                return False

        return True


def is_equal_shape(shape_left: Part.Shape, shape_right: Part.Shape) -> bool:
    if shape_left.isNull() and shape_right.isNull():
        return True

    if shape_left.isNull() or shape_right.isNull():
        return False

    if shape_left.ShapeType != shape_right.ShapeType:
        return False

    fp_left = ShapeFingerprint.from_shape(shape_left)
    fp_right = ShapeFingerprint.from_shape(shape_right)

    return fp_left.is_equal(fp_right)


def _is_equal_property_value(value_left, value_right):
    if isinstance(value_left, Materials.Material) and isinstance(value_right, Materials.Material):
        return value_left.UUID == value_right.UUID
    elif isinstance(value_left, Part.Shape) and isinstance(value_right, Part.Shape):
        return is_equal_shape(value_left, value_right)
    else:
        return value_left == value_right


class DiffResultPropertyContainer:
    def __init__(self, left: PropertyContainer, right: PropertyContainer):
        self._left = left
        self._right = right

        self._find_inclusion(left, right)
        self._find_difference(left, right)

    def is_same(self):
        return (
            len(self.props_only_in_left) == 0
            and len(self.props_only_in_right) == 0
            and len(self.props_different) == 0
        )

    def _find_inclusion(self, left: PropertyContainer, right: PropertyContainer):
        props_left = {KeyedProperty(left, prop_name) for prop_name in left.PropertiesList}
        props_right = {KeyedProperty(right, prop_name) for prop_name in right.PropertiesList}

        def toSet(keyedProps: set[KeyedProperty]):
            return set(keyedProp.name for keyedProp in keyedProps)

        self.props_only_in_left = toSet(props_left - props_right)
        self.props_only_in_right = toSet(props_right - props_left)
        self.props_in_both = toSet(props_left & props_right)


    def _find_difference(self, left: PropertyContainer, right: PropertyContainer):
        self.props_same = set()
        self.props_different: dict[str, tuple[str, str]] = {}

        for prop_name in self.props_in_both:
            value_left = left.getPropertyByName(prop_name)
            value_right = right.getPropertyByName(prop_name)
            if _is_equal_property_value(value_left, value_right):
                self.props_same.add(prop_name)
            else:
                self.props_different[prop_name] = (value_left, value_right)


class DiffResultDocument:
    def __init__(self, left: Document, right: Document):
        self._left = left
        self._right = right

        objects_left = {KeyedDocumentObject(obj) for obj in left.Objects}
        objects_right = {KeyedDocumentObject(obj) for obj in right.Objects}

        def toSet(keyedObjs: set[KeyedDocumentObject]):
            return set(keyedObj.obj for keyedObj in keyedObjs)

        self.objs_only_in_left = toSet(objects_left - objects_right)
        self.objs_only_in_right = toSet(objects_right - objects_left)
        self._objs_in_both = {keyedObj.obj.Name for keyedObj in objects_left & objects_right}

        self.props = DiffResultPropertyContainer(left, right)

        self.objs_same: set[str] = set()
        self.objs_different: dict[str, DiffResultPropertyContainer] = {}

        for obj_name in self._objs_in_both:
            obj_left = left.getObject(obj_name)
            obj_right = right.getObject(obj_name)
            diff_result = DiffResultPropertyContainer(obj_left, obj_right)
            if (diff_result.is_same()):
                self.objs_same.add(obj_name)
            else:
                self.objs_different[obj_name] = diff_result

    def is_same(self):
        return (
            len(self.objs_only_in_left) == 0
            and len(self.objs_only_in_right) == 0
            and len(self.objs_different) == 0
            and self.props.is_same()
        )


def compute_diff(left: Document, right: Document) -> DiffResultDocument:
    return DiffResultDocument(left, right)

