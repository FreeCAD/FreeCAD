"""
    Copyright (C) 2011-2015  Parametric Products Intellectual Holdings, LLC

    This file is part of CadQuery.

    CadQuery is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    CadQuery is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; If not, see <http://www.gnu.org/licenses/>
"""
from __future__ import division
import math
from copy import copy
import cadquery
import FreeCAD
import Part as FreeCADPart


def sortWiresByBuildOrder(wireList, plane, result=[]):
    """Tries to determine how wires should be combined into faces.

    Assume:
        The wires make up one or more faces, which could have 'holes'
        Outer wires are listed ahead of inner wires
        there are no wires inside wires inside wires
        ( IE, islands -- we can deal with that later on )
        none of the wires are construction wires

    Compute:
        one or more sets of wires, with the outer wire listed first, and inner
        ones

    Returns, list of lists.
    """
    result = []

    remainingWires = list(wireList)
    while remainingWires:
        outerWire = remainingWires.pop(0)
        group = [outerWire]
        otherWires = list(remainingWires)
        for w in otherWires:
            if plane.isWireInside(outerWire, w):
                group.append(w)
                remainingWires.remove(w)
        result.append(group)

    return result


class Vector(object):
    """Create a 3-dimensional vector

        **you can provide any of:**

        * nothing (in which case a zero vector is return)
        * a :class:`FreeCAD.Base.Vector` instance to wrap
        * a :class:`Vector` (in which case it is copied)
        * a 2-tuple of :class:`float`, ``(x, y)`` (z=0)
        * a 3-tuple of :class:`float`, ``(x, y, z)``
        * 2 :class:`float` values, as ``x``, and ``y``
        * 3 :class:`float` values, as ``x``, ``y``, and ``z``
        * named coordinates, like ``Vector(x=1, z=2)``

        .. doctest::

            >>> from cadquery import Vector
            >>> import FreeCAD
            >>> Vector()
            Vector (0.0, 0.0, 0.0)
            >>> Vector(FreeCAD.Base.Vector(1,2,3))
            Vector (1.0, 2.0, 3.0)
            >>> Vector(Vector(4,5,6))
            Vector (4.0, 5.0, 6.0)
            >>> Vector((1, 2))
            Vector (1.0, 2.0, 0.0)
            >>> Vector((1, 2, 3))
            Vector (1.0, 2.0, 3.0)
            >>> Vector(4, 5)
            Vector (4.0, 5.0, 0.0)
            >>> Vector(4, 5, 6)
            Vector (4.0, 5.0, 6.0)
            >>> Vector(x=1, z=2)
            Vector (1.0, 0.0, 2.0)
    """
    def __init__(self, *args, **kwargs):
        fV = None

        if args and not kwargs:
            if (1 <= len(args) <= 3):  # 1, 2, or 3 arguments (no keyword arguments given)
                if isinstance(args[0], (int, float)):
                    # First argument is a number, pass list through to freecad
                    fV = FreeCAD.Base.Vector(*args)
                elif len(args) == 1:
                    arg = args[0]  # only 1 argument given
                    if isinstance(arg, Vector):
                        v = arg.wrapped
                        fV = FreeCAD.Base.Vector(v.x, v.y, v.z)  # create copy
                    elif isinstance(arg, FreeCAD.Base.Vector):
                        v = arg
                        fV = FreeCAD.Base.Vector(v.x, v.y, v.z)  # create copy
                    elif isinstance(arg, (list, tuple)):
                        if (1 <= len(arg) <= 3):
                            fV = FreeCAD.Base.Vector(*arg)
        elif not args:
            if set(kwargs.keys()) - set('xyz'):
                pass  # kwargs contains invalid arguments, raise exception
            else:
                fV = FreeCAD.Base.Vector(
                    kwargs.get('x', 0),
                    kwargs.get('y', 0),
                    kwargs.get('z', 0),
                )

        if fV is None:
            raise ValueError(
                "Expected 2 or 3 floats, FreeCAD Vector, 2 or 3-tuple, or named lower-case coords. " + \
                ("%r %r" % (args, kwargs))
            )

        self._wrapped = fV

    @property
    def x(self):
        return self._wrapped.x

    @x.setter
    def x(self, value):
        self._wrapped.x = value

    @property
    def y(self):
        return self._wrapped.y

    @y.setter
    def y(self, value):
        self._wrapped.y = value

    @property
    def z(self):
        return self._wrapped.z

    @z.setter
    def z(self, value):
        self._wrapped.z = value

    @property
    def Length(self):
        return self._wrapped.Length

    @property
    def wrapped(self):
        return self._wrapped

    def toTuple(self):
        v = self._wrapped
        return (v.x, v.y, v.z)

    # TODO: is it possible to create a dynamic proxy without all this code?
    def cross(self, v):
        return type(self)(self._wrapped.cross(v.wrapped))

    def dot(self, v):
        return self._wrapped.dot(v.wrapped)

    def sub(self, v):
        return type(self)(self._wrapped.sub(v.wrapped))

    def add(self, v):
        return type(self)(self._wrapped.add(v.wrapped))

    def multiply(self, scale):
        """Return a copy multiplied by the provided scalar"""
        tmp_fc_vector = FreeCAD.Base.Vector(self._wrapped)
        return type(self)(tmp_fc_vector.multiply(scale))

    def normalized(self):
        """Return a normalized version of this vector"""
        tmp_fc_vector = FreeCAD.Base.Vector(self._wrapped)
        tmp_fc_vector.normalize()
        return type(self)(tmp_fc_vector)

    def Center(self):
        """Return the vector itself

        The center of myself is myself.
        Provided so that vectors, vertices, and other shapes all support a
        common interface, when Center() is requested for all objects on the
        stack.
        """
        return self

    def getAngle(self, v):
        return self._wrapped.getAngle(v.wrapped)

    def distanceToLine(self):
        raise NotImplementedError("Have not needed this yet, but FreeCAD supports it!")

    def projectToLine(self):
        raise NotImplementedError("Have not needed this yet, but FreeCAD supports it!")

    def distanceToPlane(self):
        raise NotImplementedError("Have not needed this yet, but FreeCAD supports it!")

    def projectToPlane(self):
        raise NotImplementedError("Have not needed this yet, but FreeCAD supports it!")

    def __bool__(self):
        return any(coord != 0 for coord in self.toTuple())

    __nonzero__ = __bool__  # python 2.x compatibility

    def __add__(self, v):
        return self.add(v)

    def __sub__(self, v):
        return self.sub(v)

    def __mul__(self, scalar):
        return self.multiply(scalar)

    def __truediv__(self, scalar):
        if scalar == 0:
            raise ZeroDivisionError("%r division by zero" % (type(self)))
        return self.multiply(1. / scalar)

    def __neg__(self):
        return type(self)(self.wrapped.negative())

    def __abs__(self):
        return self.Length

    def __repr__(self):
        return self._wrapped.__repr__()

    def __str__(self):
        return self._wrapped.__str__()

    def __ne__(self, other):
        if isinstance(other, Vector):
            return self.wrapped.__ne__(other.wrapped)
        return False

    def __eq__(self, other):
        if isinstance(other, Vector):
            return self.wrapped.__eq__(other.wrapped)
        return False

    def __copy__(self):
        return type(self)(self._wrapped)


class Matrix:
    """A 3d , 4x4 transformation matrix.

    Used to move geometry in space.
    """
    def __init__(self, matrix=None):
        if matrix is None:
            self.wrapped = FreeCAD.Base.Matrix()
        else:
            self.wrapped = matrix

    def rotateX(self, angle):
        self.wrapped.rotateX(angle)

    def rotateY(self, angle):
        self.wrapped.rotateY(angle)


class Plane(object):
    """A 2D coordinate system in space

    A 2D coordinate system in space, with the x-y axes on the plane, and a
    particular point as the origin.

    A plane allows the use of 2-d coordinates, which are later converted to
    global, 3d coordinates when the operations are complete.

    Frequently, it is not necessary to create work planes, as they can be
    created automatically from faces.
    """

    # equality tolerances
    _eq_tolerance_origin = 1e-6
    _eq_tolerance_dot = 1e-6

    @classmethod
    def named(cls, stdName, origin=(0, 0, 0)):
        """Create a predefined Plane based on the conventional names.

        :param stdName: one of (XY|YZ|ZX|XZ|YX|ZY|front|back|left|right|top|bottom)
        :type stdName: string
        :param origin: the desired origin, specified in global coordinates
        :type origin: 3-tuple of the origin of the new plane, in global coorindates.

        Available named planes are as follows. Direction references refer to
        the global directions.

        =========== ======= ======= ======
        Name        xDir    yDir    zDir
        =========== ======= ======= ======
        XY          +x      +y      +z
        YZ          +y      +z      +x
        ZX          +z      +x      +y
        XZ          +x      +z      -y
        YX          +y      +x      -z
        ZY          +z      +y      -x
        front       +x      +y      +z
        back        -x      +y      -z
        left        +z      +y      -x
        right       -z      +y      +x
        top         +x      -z      +y
        bottom      +x      +z      -y
        =========== ======= ======= ======
        """

        namedPlanes = {
            # origin, xDir, normal
            'XY': cls(origin, (1, 0, 0), (0, 0, 1)),
            'YZ': cls(origin, (0, 1, 0), (1, 0, 0)),
            'ZX': cls(origin, (0, 0, 1), (0, 1, 0)),
            'XZ': cls(origin, (1, 0, 0), (0, -1, 0)),
            'YX': cls(origin, (0, 1, 0), (0, 0, -1)),
            'ZY': cls(origin, (0, 0, 1), (-1, 0, 0)),
            'front': cls(origin, (1, 0, 0), (0, 0, 1)),
            'back': cls(origin, (-1, 0, 0), (0, 0, -1)),
            'left': cls(origin, (0, 0, 1), (-1, 0, 0)),
            'right': cls(origin, (0, 0, -1), (1, 0, 0)),
            'top': cls(origin, (1, 0, 0), (0, 1, 0)),
            'bottom': cls(origin, (1, 0, 0), (0, -1, 0))
        }

        try:
            return namedPlanes[stdName]
        except KeyError:
            raise ValueError('Supported names are {}'.format(
                list(namedPlanes.keys())))

    @classmethod
    def XY(cls, origin=(0, 0, 0), xDir=Vector(1, 0, 0)):
        plane = cls.named('XY', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def YZ(cls, origin=(0, 0, 0), xDir=Vector(0, 1, 0)):
        plane = cls.named('YZ', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def ZX(cls, origin=(0, 0, 0), xDir=Vector(0, 0, 1)):
        plane = cls.named('ZX', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def XZ(cls, origin=(0, 0, 0), xDir=Vector(1, 0, 0)):
        plane = cls.named('XZ', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def YX(cls, origin=(0, 0, 0), xDir=Vector(0, 1, 0)):
        plane = cls.named('YX', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def ZY(cls, origin=(0, 0, 0), xDir=Vector(0, 0, 1)):
        plane = cls.named('ZY', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def front(cls, origin=(0, 0, 0), xDir=Vector(1, 0, 0)):
        plane = cls.named('front', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def back(cls, origin=(0, 0, 0), xDir=Vector(-1, 0, 0)):
        plane = cls.named('back', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def left(cls, origin=(0, 0, 0), xDir=Vector(0, 0, 1)):
        plane = cls.named('left', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def right(cls, origin=(0, 0, 0), xDir=Vector(0, 0, -1)):
        plane = cls.named('right', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def top(cls, origin=(0, 0, 0), xDir=Vector(1, 0, 0)):
        plane = cls.named('top', origin)
        plane._setPlaneDir(xDir)
        return plane

    @classmethod
    def bottom(cls, origin=(0, 0, 0), xDir=Vector(1, 0, 0)):
        plane = cls.named('bottom', origin)
        plane._setPlaneDir(xDir)
        return plane

    def __init__(self, origin, xDir, normal):
        """Create a Plane with an arbitrary orientation

        TODO: project x and y vectors so they work even if not orthogonal
        :param origin: the origin
        :type origin: a three-tuple of the origin, in global coordinates
        :param xDir: a vector representing the xDirection.
        :type xDir: a three-tuple representing a vector, or a FreeCAD Vector
        :param normal: the normal direction for the new plane
        :type normal: a FreeCAD Vector
        :raises: ValueError if the specified xDir is not orthogonal to the provided normal.
        :return: a plane in the global space, with the xDirection of the plane in the specified direction.

        **Non-orthogonal vectors**

        If the ``xDir`` and ``normal`` vectors are not orthogonal, the ``normal``
        (or ``z`` axis) vector direction is preserved, and truly orthogonal
        ``x`` and ``y`` axes are calculated with cross products.

        * ``y = z.cross(x)``, and
        * ``x = y.cross(z)``
        """
        # z-axis
        normal = Vector(normal)
        if (normal.Length == 0.0):
            raise ValueError('normal should be non null')
        self.zDir = normal.normalized()

        # x-axis
        xDir = Vector(xDir)
        if (xDir.Length == 0.0):
            raise ValueError('xDir should be non null')

        self._setPlaneDir(xDir)

        self.invZDir = -self.zDir

        self.origin = origin

    @property
    def origin(self):
        return self._origin

    @origin.setter
    def origin(self, value):
        self._origin = Vector(value)
        self._calcTransforms()

    def setOrigin2d(self, x, y):
        """
        Set a new origin in the plane itself

        Set a new origin in the plane itself. The plane's orientation and
        xDrection are unaffected.

        :param float x: offset in the x direction
        :param float y: offset in the y direction
        :return: void

        The new coordinates are specified in terms of the current 2-d system.
        As an example:

        p = Plane.XY()
        p.setOrigin2d(2, 2)
        p.setOrigin2d(2, 2)

        results in a plane with its origin at (x, y) = (4, 4) in global
        coordinates. Both operations were relative to local coordinates of the
        plane.
        """
        self.origin = self.toWorldCoords((x, y))

    def isWireInside(self, baseWire, testWire):
        """Determine if testWire is inside baseWire

        Determine if testWire is inside baseWire, after both wires are projected
        into the current plane.

        :param baseWire: a reference wire
        :type baseWire: a FreeCAD wire
        :param testWire: another wire
        :type testWire: a FreeCAD wire
        :return: True if testWire is inside baseWire, otherwise False

        If either wire does not lie in the current plane, it is projected into
        the plane first.

        *WARNING*:  This method is not 100% reliable. It uses bounding box
        tests, but needs more work to check for cases when curves are complex.

        Future Enhancements:
            * Discretizing points along each curve to provide a more reliable
              test.
        """
        # TODO: also use a set of points along the wire to test as well.
        # TODO: would it be more efficient to create objects in the local
        #       coordinate system, and then transform to global
        #       coordinates upon extrusion?

        tBaseWire = baseWire.transformGeometry(self.fG)
        tTestWire = testWire.transformGeometry(self.fG)

        # These bounding boxes will have z=0, since we transformed them into the
        # space of the plane.
        bb = tBaseWire.BoundingBox()
        tb = tTestWire.BoundingBox()

        # findOutsideBox actually inspects both ways, here we only want to
        # know if one is inside the other
        return bb == BoundBox.findOutsideBox2D(bb, tb)

    def toLocalCoords(self, obj):
        """Project the provided coordinates onto this plane

        :param obj: an object or vector to convert
        :type vector: a vector or shape
        :return: an object of the same type, but converted to local coordinates


        Most of the time, the z-coordinate returned will be zero, because most
        operations based on a plane are all 2-d. Occasionally, though, 3-d
        points outside of the current plane are transformed. One such example is
        :py:meth:`Workplane.box`, where 3-d corners of a box are transformed to
        orient the box in space correctly.

        """
        if isinstance(obj, Vector):
            return Vector(self.fG.multiply(obj.wrapped))
        elif isinstance(obj, cadquery.Shape):
            return obj.transformShape(self.rG)
        else:
            raise ValueError(
                "Don't know how to convert type {} to local coordinates".format(
                    type(obj)))

    def toWorldCoords(self, tuplePoint):
        """Convert a point in local coordinates to global coordinates

        :param tuplePoint: point in local coordinates to convert.
        :type tuplePoint: a 2 or three tuple of float. The third value is taken to be zero if not supplied.
        :return: a Vector in global coordinates
        """
        if isinstance(tuplePoint, Vector):
            v = tuplePoint
        elif len(tuplePoint) == 2:
            v = Vector(tuplePoint[0], tuplePoint[1], 0)
        else:
            v = Vector(tuplePoint)
        return Vector(self.rG.multiply(v.wrapped))

    def rotated(self, rotate=(0, 0, 0)):
        """Returns a copy of this plane, rotated about the specified axes

        Since the z axis is always normal the plane, rotating around Z will
        always produce a plane that is parallel to this one.

        The origin of the workplane is unaffected by the rotation.

        Rotations are done in order x, y, z. If you need a different order,
        manually chain together multiple rotate() commands.

        :param rotate: Vector [xDegrees, yDegrees, zDegrees]
        :return: a copy of this plane rotated as requested.
        """
        rotate = Vector(rotate)
        # Convert to radians.
        rotate = rotate.multiply(math.pi / 180.0)

        # Compute rotation matrix.
        m = FreeCAD.Base.Matrix()
        m.rotateX(rotate.x)
        m.rotateY(rotate.y)
        m.rotateZ(rotate.z)

        # Compute the new plane.
        newXdir = Vector(m.multiply(self.xDir.wrapped))
        newZdir = Vector(m.multiply(self.zDir.wrapped))

        return type(self)(self.origin, newXdir, newZdir)

    def rotateShapes(self, listOfShapes, rotationMatrix):
        """Rotate the listOfShapes by the supplied rotationMatrix

        @param listOfShapes is a list of shape objects
        @param rotationMatrix is a geom.Matrix object.
        returns a list of shape objects rotated according to the rotationMatrix.
        """
        # Compute rotation matrix (global --> local --> rotate --> global).
        # rm = self.plane.fG.multiply(matrix).multiply(self.plane.rG)
        # rm = self.computeTransform(rotationMatrix)

        # There might be a better way, but to do this rotation takes 3 steps:
        # - transform geometry to local coordinates
        # - then rotate about x
        # - then transform back to global coordinates.

        resultWires = []
        for w in listOfShapes:
            mirrored = w.transformGeometry(rotationMatrix.wrapped)

            # If the first vertex of the second wire is not coincident with the
            # first or last vertices of the first wire we have to fix the wire
            # so that it will mirror correctly.
            if ((mirrored.wrapped.Vertexes[0].X == w.wrapped.Vertexes[0].X and
                 mirrored.wrapped.Vertexes[0].Y == w.wrapped.Vertexes[0].Y and
                 mirrored.wrapped.Vertexes[0].Z == w.wrapped.Vertexes[0].Z) or
                (mirrored.wrapped.Vertexes[0].X == w.wrapped.Vertexes[-1].X and
                 mirrored.wrapped.Vertexes[0].Y == w.wrapped.Vertexes[-1].Y and
                 mirrored.wrapped.Vertexes[0].Z == w.wrapped.Vertexes[-1].Z)):

                resultWires.append(mirrored)
            else:
                # Make sure that our mirrored edges meet up and are ordered
                # properly.
                aEdges = w.wrapped.Edges
                aEdges.extend(mirrored.wrapped.Edges)
                comp = FreeCADPart.Compound(aEdges)
                mirroredWire = comp.connectEdgesToWires(False).Wires[0]

                resultWires.append(cadquery.Shape.cast(mirroredWire))

        return resultWires

    def _setPlaneDir(self, xDir):
        """Set the vectors parallel to the plane, i.e. xDir and yDir"""
        self.xDir = Vector(xDir)
        self.yDir = self.zDir.cross(self.xDir).normalized()
        self.xDir = self.yDir.cross(self.zDir).normalized()

    def _calcTransforms(self):
        """Computes transformation matrices to convert between coordinates

        Computes transformation matrices to convert between local and global
        coordinates.
        """
        # r is the forward transformation matrix from world to local coordinates
        # ok i will be really honest, i cannot understand exactly why this works
        # something bout the order of the translation and the rotation.
        # the double-inverting is strange, and I don't understand it.
        r = FreeCAD.Base.Matrix()

        # Forward transform must rotate and adjust for origin.
        (r.A11, r.A12, r.A13) = (self.xDir.x, self.xDir.y, self.xDir.z)
        (r.A21, r.A22, r.A23) = (self.yDir.x, self.yDir.y, self.yDir.z)
        (r.A31, r.A32, r.A33) = (self.zDir.x, self.zDir.y, self.zDir.z)

        invR = r.inverse()
        invR.A14 = self.origin.x
        invR.A24 = self.origin.y
        invR.A34 = self.origin.z

        self.rG = invR
        self.fG = invR.inverse()

    def computeTransform(self, tMatrix):
        """Computes the 2-d projection of the supplied matrix"""

        return Matrix(self.fG.multiply(tMatrix.wrapped).multiply(self.rG))

    # Equality
    def __eq__(self, other):
        def equality_iter():
            cls = type(self)
            yield isinstance(other, Plane)  # comparison is with another Plane
            # origins are the same
            yield abs(self.origin - other.origin) < cls._eq_tolerance_origin
            # z-axis vectors are parallel (assumption: both are unit vectors)
            yield abs(self.zDir.dot(other.zDir) - 1) < cls._eq_tolerance_dot
            # x-axis vectors are parallel (assumption: both are unit vectors)
            yield abs(self.xDir.dot(other.xDir) - 1) < cls._eq_tolerance_dot
        return all(equality_iter())

    def __ne__(self, other):
        return not self.__eq__(other)


class BoundBox(object):
    """A BoundingBox for an object or set of objects. Wraps the FreeCAD one"""
    def __init__(self, bb):
        self.wrapped = bb
        self.xmin = bb.XMin
        self.xmax = bb.XMax
        self.xlen = bb.XLength
        self.ymin = bb.YMin
        self.ymax = bb.YMax
        self.ylen = bb.YLength
        self.zmin = bb.ZMin
        self.zmax = bb.ZMax
        self.zlen = bb.ZLength
        self.center = Vector(bb.Center)
        self.DiagonalLength = bb.DiagonalLength

    def add(self, obj):
        """Returns a modified (expanded) bounding box

        obj can be one of several things:
            1. a 3-tuple corresponding to x,y, and z amounts to add
            2. a vector, containing the x,y,z values to add
            3. another bounding box, where a new box will be created that
               encloses both.

        This bounding box is not changed.
        """
        tmp = FreeCAD.Base.BoundBox(self.wrapped)
        if isinstance(obj, tuple):
            tmp.add(obj[0], obj[1], obj[2])
        elif isinstance(obj, Vector):
            tmp.add(obj.fV)
        elif isinstance(obj, BoundBox):
            tmp.add(obj.wrapped)

        return BoundBox(tmp)

    @classmethod
    def findOutsideBox2D(cls, b1, b2):
        """Compares bounding boxes

        Compares bounding boxes. Returns none if neither is inside the other.
        Returns the outer one if either is outside the other.

        BoundBox.isInside works in 3d, but this is a 2d bounding box, so it
        doesn't work correctly plus, there was all kinds of rounding error in
        the built-in implementation i do not understand.
        """
        fc_bb1 = b1.wrapped
        fc_bb2 = b2.wrapped
        if (fc_bb1.XMin < fc_bb2.XMin and
            fc_bb1.XMax > fc_bb2.XMax and
            fc_bb1.YMin < fc_bb2.YMin and
            fc_bb1.YMax > fc_bb2.YMax):
            return b1

        if (fc_bb2.XMin < fc_bb1.XMin and
            fc_bb2.XMax > fc_bb1.XMax and
            fc_bb2.YMin < fc_bb1.YMin and
            fc_bb2.YMax > fc_bb1.YMax):
            return b2

        return None

    def isInside(self, anotherBox):
        """Is the provided bounding box inside this one?"""
        return self.wrapped.isInside(anotherBox.wrapped)
