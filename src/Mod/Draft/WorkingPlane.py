# ***************************************************************************
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provide the working plane code and utilities for the Draft Workbench.

This module provides the plane class which provides a virtual working plane
in FreeCAD and a couple of utility functions.
The working plane is mostly intended to be used in the Draft Workbench
to draw 2D objects in various orientations, not only in the standard XY,
YZ, and XZ planes.
"""
## @package WorkingPlane
#  \ingroup DRAFT
#  \brief This module handles the Working Plane and grid of the Draft module.
#
#  This module provides the plane class which provides a virtual working plane
#  in FreeCAD and a couple of utility functions.

import math
import lazy_loader.lazy_loader as lz
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD
import DraftVecUtils
from FreeCAD import Vector
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.messages import _wrn
from draftutils.translate import translate

DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Part = lz.LazyLoader("Part", globals(), "Part")
FreeCADGui = lz.LazyLoader("FreeCADGui", globals(), "FreeCADGui")

__title__ = "FreeCAD Working Plane utility"
__author__ = "Ken Cline"
__url__ = "https://www.freecad.org"


class PlaneBase:
    """PlaneBase is the base class for the Plane class and the PlaneGui class.

    Parameters
    ----------
    u: Base.Vector or WorkingPlane.PlaneBase, optional
        Defaults to Vector(1, 0, 0).
        If a WP is provided:
            A copy of the WP is created, all other parameters are then ignored.
        If a vector is provided:
            Unit vector for the `u` attribute (+X axis).

    v: Base.Vector, optional
        Defaults to Vector(0, 1, 0).
        Unit vector for the `v` attribute (+Y axis).

    w: Base.Vector, optional
        Defaults to Vector(0, 0, 1).
        Unit vector for the `axis` attribute (+Z axis).

    pos: Base.Vector, optional
        Defaults to Vector(0, 0, 0).
        Vector for the `position` attribute (origin).

    Note that the u, v and w vectors are not checked for validity.
    """

    def __init__(self,
                 u=Vector(1, 0, 0), v=Vector(0, 1, 0), w=Vector(0, 0, 1),
                 pos=Vector(0, 0, 0)):

        if isinstance(u, PlaneBase):
            self.match(u)
            return
        self.u = Vector(u)
        self.v = Vector(v)
        self.axis = Vector(w)
        self.position = Vector(pos)

    def __repr__(self):
        text = "Workplane"
        text += " x=" + str(DraftVecUtils.rounded(self.u))
        text += " y=" + str(DraftVecUtils.rounded(self.v))
        text += " z=" + str(DraftVecUtils.rounded(self.axis))
        text += " pos=" + str(DraftVecUtils.rounded(self.position))
        return text

    def copy(self):
        """Return a new WP that is a copy of the present object."""
        wp = PlaneBase()
        self.match(source=self, target=wp)
        return wp

    def _copy_value(self, val):
        """Return a copy of a value, primarily required for vectors."""
        return val.__class__(val)

    def match(self, source, target=None):
        """Match the main properties of two working planes.

        Parameters
        ----------
        source: WP object
            WP to copy properties from.
        target: WP object, optional
            Defaults to `None`.
            WP to copy properties to. If `None` the present object is used.
        """
        if target is None:
            target = self
        for prop in self._get_prop_list():
            setattr(target, prop, self._copy_value(getattr(source, prop)))

    def get_parameters(self):
        """Return a data dictionary with the main properties of the WP."""
        data = {}
        for prop in self._get_prop_list():
            data[prop] = self._copy_value(getattr(self, prop))
        return data

    def set_parameters(self, data):
        """Set the main properties of the WP according to a data dictionary."""
        for prop in self._get_prop_list():
            setattr(self, prop, self._copy_value(data[prop]))

    def align_to_3_points(self, p1, p2, p3, offset=0):
        """Align the WP to 3 points with an optional offset.

        The points must define a plane.

        Parameters
        ----------
        p1: Base.Vector
            New WP `position`.
        p2: Base.Vector
            Point on the +X axis. (p2 - p1) defines the WP `u` axis.
        p3: Base.Vector
            Defines the plane.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        return self.align_to_edge_or_wire(Part.makePolygon([p1, p2, p3]), offset)

    def align_to_edges_vertexes(self, shapes, offset=0):
        """Align the WP to the endpoints of edges and/or the points of vertexes
        with an optional offset.

        The points must define a plane.

        The first 2 points define the WP `position` and `u` axis.

        Parameters
        ----------
        shapes: iterable
            One or more edges and/or vertexes.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        points = [vert.Point for shape in shapes for vert in shape.Vertexes]
        if len(points) < 2:
            return False
        return self.align_to_edge_or_wire(Part.makePolygon(points), offset)

    def align_to_edge_or_wire(self, shape, offset=0):
        """Align the WP to an edge or wire with an optional offset.

        The shape must define a plane.

        If the shape is an edge with a `Center` then that defines the WP
        `position`. The vector between the center and its start point then
        defines the WP `u` axis.

        In other cases the start point of the first edge defines the WP
        `position` and the 1st derivative at that point the WP `u` axis.

        Parameters
        ----------
        shape: Part.Edge or Part.Wire
            Edge or wire.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        tol = 1e-7
        plane = shape.findPlane()
        if plane is None:
            return False
        self.axis = plane.Axis
        if shape.ShapeType == "Edge" and hasattr(shape.Curve, "Center"):
            pos = shape.Curve.Center
            vec = shape.Vertexes[0].Point - pos
            if vec.Length > tol:
                self.u = vec
                self.u.normalize()
                self.v = self.axis.cross(self.u)
            else:
                self.u, self.v, _ = self._axes_from_rotation(plane.Rotation)
        elif shape.Edges[0].Length > tol:
            pos = shape.Vertexes[0].Point
            self.u = shape.Edges[0].derivative1At(0)
            self.u.normalize()
            self.v = self.axis.cross(self.u)
        else:
            pos = shape.Vertexes[0].Point
            self.u, self.v, _ = self._axes_from_rotation(plane.Rotation)
        self.position = pos + (self.axis * offset)
        return True

    def align_to_face(self, face, offset=0):
        """Align the WP to a face with an optional offset.

        The face must be planar.

        The center of gravity of the face defines the WP `position` and the
        normal of the face the WP `axis`. The WP `u` and `v` vectors are
        determined by the DraftGeomUtils.uv_vectors_from_face function.
        See there.

        Parameters
        ----------
        face: Part.Face
            Face.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        if face.Surface.isPlanar() is False:
            return False
        place = DraftGeomUtils.placement_from_face(face)
        self.u, self.v, self.axis = self._axes_from_rotation(place.Rotation)
        self.position = place.Base + (self.axis * offset)
        return True

    def align_to_face_and_edge(self, face, edge, offset=0):
        """Align the WP to a face and an edge.

        The face must be planar.

        The normal of the face defines the WP `axis`, the first vertex of the
        edge its `position`, and its `u` vector is determined by the second
        vertex of the edge.

        Parameters
        ----------
        face: Part.Face
            Face.
        edge: Part.Edge
            Edge, need not be an edge of the face.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        if face.Surface.isPlanar() is False:
            return False
        axis = face.normalAt(0,0)
        point = edge.Vertexes[0].Point
        vec = edge.Vertexes[-1].Point.sub(point)
        return self.align_to_point_and_axis(point, axis, offset, axis.cross(vec))

    def align_to_placement(self, place, offset=0):
        """Align the WP to a placement with an optional offset.

        Parameters
        ----------
        place: Base.Placement
            Placement.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`
        """
        self.u, self.v, self.axis = self._axes_from_rotation(place.Rotation)
        self.position = place.Base + (self.axis * offset)
        return True

    def align_to_point_and_axis(self, point, axis, offset=0, upvec=Vector(1, 0, 0)):
        """Align the WP to a point and an axis with an optional offset and an
        optional up-vector.

        If the axis and up-vector are parallel the FreeCAD.Rotation algorithm
        will replace the up-vector: Vector(0, 0, 1) is tried first, then
        Vector(0, 1, 0), and finally Vector(1, 0, 0).

        Parameters
        ----------
        point: Base.Vector
            New WP `position`.
        axis: Base.Vector
            New WP `axis`.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        upvec: Base.Vector, optional
            Defaults to Vector(1, 0, 0).
            Up-vector.

        Returns
        -------
        `True`
        """
        tol = 1e-7
        if axis.Length < tol:
            return False
        axis = Vector(axis).normalize()
        if upvec.Length < tol or upvec.isEqual(axis, tol) or upvec.isEqual(axis.negative(), tol):
            upvec = Vector(axis)
        else:
            upvec = Vector(upvec).normalize()
        rot = FreeCAD.Rotation(Vector(), upvec, axis, "ZYX")
        self.u, self.v, _ = self._axes_from_rotation(rot)
        self.axis = axis
        self.position = point + (self.axis * offset)
        return True

    def align_to_point_and_axis_svg(self, point, axis, offset=0):
        """Align the WP to a point and an axis with an optional offset.

        It aligns `u` and `v` based on the magnitude of the components
        of `axis`.

        Parameters
        ----------
        point: Base.Vector
            The new `position` of the plane, adjusted by
            the `offset`.
        axis: Base.Vector
            A vector whose unit vector will be used as the new `axis`
            of the plane.
            The magnitudes of the `x`, `y`, `z` components of the axis
            determine the orientation of `u` and `v` of the plane.
        offset: float, optional
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.

        Returns
        -------
        `True`

        Cases
        -----
        The `u` and `v` are always calculated the same

            * `u` is the cross product of the positive or negative of `axis`
              with a `reference vector`.
              ::
                  u = [+1|-1] axis.cross(ref_vec)
            * `v` is `u` rotated 90 degrees around `axis`.

        Whether the `axis` is positive or negative, and which reference
        vector is used, depends on the absolute values of the `x`, `y`, `z`
        components of the `axis` unit vector.

         #. If `x > y`, and `y > z`
             The reference vector is +Z
             ::
                 u = -1 axis.cross(+Z)
         #. If `y > z`, and `z >= x`
             The reference vector is +X.
             ::
                 u = -1 axis.cross(+X)
         #. If `y >= x`, and `x > z`
             The reference vector is +Z.
             ::
                 u = +1 axis.cross(+Z)
         #. If `x > z`, and `z >= y`
             The reference vector is +Y.
             ::
                 u = +1 axis.cross(+Y)
         #. If `z >= y`, and `y > x`
             The reference vector is +X.
             ::
                 u = +1 axis.cross(+X)
         #. otherwise
             The reference vector is +Y.
             ::
                 u = -1 axis.cross(+Y)
        """
        self.axis = Vector(axis).normalize()
        ref_vec = Vector(0.0, 1.0, 0.0)

        if ((abs(axis.x) > abs(axis.y)) and (abs(axis.y) > abs(axis.z))):
            ref_vec = Vector(0.0, 0., 1.0)
            self.u = axis.negative().cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            # projcase = "Case new"

        elif ((abs(axis.y) > abs(axis.z)) and (abs(axis.z) >= abs(axis.x))):
            ref_vec = Vector(1.0, 0.0, 0.0)
            self.u = axis.negative().cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            # projcase = "Y>Z, View Y"

        elif ((abs(axis.y) >= abs(axis.x)) and (abs(axis.x) > abs(axis.z))):
            ref_vec = Vector(0.0, 0., 1.0)
            self.u = axis.cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            # projcase = "ehem. XY, Case XY"

        elif ((abs(axis.x) > abs(axis.z)) and (abs(axis.z) >= abs(axis.y))):
            self.u = axis.cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            # projcase = "X>Z, View X"

        elif ((abs(axis.z) >= abs(axis.y)) and (abs(axis.y) > abs(axis.x))):
            ref_vec = Vector(1.0, 0., 0.0)
            self.u = axis.cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            # projcase = "Y>X, Case YZ"

        else:
            self.u = axis.negative().cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            # projcase = "else"

        # spat_vec = self.u.cross(self.v)
        # spat_res = spat_vec.dot(axis)
        # Console.PrintMessage(projcase + " spat Prod = " + str(spat_res) + "\n")

        offsetVector = Vector(axis)
        offsetVector.multiply(offset)
        self.position = point.add(offsetVector)

        return True

    def get_global_coords(self, point, as_vector=False):
        """Translate a point or vector from the local (WP) coordinate system to
        the global coordinate system.

        Parameters
        ----------
        point: Base.Vector
            Point.
        as_vector: bool, optional
            Defaults to `False`.
            If `True` treat point as a vector.

        Returns
        -------
        Base.Vector
        """
        pos = Vector() if as_vector else self.position
        mtx = FreeCAD.Matrix(self.u, self.v, self.axis, pos)
        return mtx.multVec(point)

    def get_local_coords(self, point, as_vector=False):
        """Translate a point or vector from the global coordinate system to
        the local (WP) coordinate system.

        Parameters
        ----------
        point: Base.Vector
            Point.
        as_vector: bool, optional
            Defaults to `False`.
            If `True` treat point as a vector.

        Returns
        -------
        Base.Vector
        """
        pos = Vector() if as_vector else self.position
        mtx = FreeCAD.Matrix(self.u, self.v, self.axis, pos)
        return mtx.inverse().multVec(point)

    def get_closest_axis(self, vec):
        """Return a string indicating the positive or negative WP axis closest
        to a vector.

        Parameters
        ----------
        vec: Base.Vector
            Vector.

        Returns
        -------
        str
            `"x"`, `"y"` or `"z"`.
        """
        xyz = list(self.get_local_coords(vec, as_vector=True))
        x, y, z = [abs(coord) for coord in xyz]
        if x >= y and x >= z:
            return "x"
        elif y >= x and y >= z:
            return "y"
        else:
            return "z"

    def get_placement(self):
        """Return a placement calculated from the WP."""
        return FreeCAD.Placement(self.position, FreeCAD.Rotation(self.u, self.v, self.axis, "ZYX"))

    def is_global(self):
        """Return `True` if the WP matches the global coordinate system exactly."""
        return self.u == Vector(1, 0, 0) \
            and self.v == Vector(0, 1, 0) \
            and self.axis == Vector(0, 0, 1) \
            and self.position == Vector()

    def is_ortho(self):
        """Return `True` if all WP axes are  parallel to a global axis."""
        rot = FreeCAD.Rotation(self.u, self.v, self.axis, "ZYX")
        ypr = [round(ang, 6) for ang in rot.getYawPitchRoll()]
        return all([ang%90 == 0 for ang in ypr])

    def project_point(self, point, direction=None, force_projection=True):
        """Project a point onto the WP and return the global coordinates of the
        projected point.

        Parameters
        ----------
        point: Base.Vector
            Point to project.
        direction: Base.Vector, optional
            Defaults to `None` in which case the WP `axis` is used.
            Direction of projection.
        force_projection: bool, optional
            Defaults to `True`.
            See DraftGeomUtils.project_point_on_plane

        Returns
        -------
        Base.Vector
        """
        return DraftGeomUtils.project_point_on_plane(point,
                                                     self.position,
                                                     self.axis,
                                                     direction,
                                                     force_projection)

    def set_to_top(self, offset=0):
        """Set the WP to the top position with an optional offset."""
        self.u = Vector(1, 0, 0)
        self.v = Vector(0, 1, 0)
        self.axis = Vector(0, 0, 1)
        self.position = self.axis * offset

    def set_to_front(self, offset=0):
        """Set the WP to the front position with an optional offset."""
        self.u = Vector(1, 0, 0)
        self.v = Vector(0, 0, 1)
        self.axis = Vector(0, -1, 0)
        self.position = self.axis * offset

    def set_to_side(self, offset=0):
        """Set the WP to the right side position with an optional offset."""
        self.u = Vector(0, 1, 0)
        self.v = Vector(0, 0, 1)
        self.axis = Vector(1, 0, 0)
        self.position = self.axis * offset

    def _axes_from_rotation(self, rot):
        """Return a tuple with the `u`, `v` and `axis` vectors from a Base.Rotation."""
        mtx = rot.toMatrix()
        return mtx.col(0), mtx.col(1), mtx.col(2)

    def _axes_from_view_rotation(self, rot):
        """Return a tuple with the `u`, `v` and `axis` vectors from a Base.Rotation
        derived from a view. The Yaw, Pitch and Roll angles are rounded if they are
        near multiples of 45 degrees.
        """
        ypr = [round(ang, 3) for ang in rot.getYawPitchRoll()]
        if all([ang%45 == 0 for ang in ypr]):
            rot.setEulerAngles("YawPitchRoll", *ypr)
        return self._axes_from_rotation(rot)

    def _get_prop_list(self):
        return ["u",
                "v",
                "axis",
                "position"]


class Plane(PlaneBase):
    """The old Plane class.

    Parameters
    ----------
    u: Base.Vector or WorkingPlane.Plane, optional
        Defaults to Vector(1, 0, 0).
        If a WP is provided:
            A copy of the WP is created, all other parameters are then ignored.
        If a vector is provided:
            Unit vector for the `u` attribute (+X axis).

    v: Base.Vector, optional
        Defaults to Vector(0, 1, 0).
        Unit vector for the `v` attribute (+Y axis).

    w: Base.Vector, optional
        Defaults to Vector(0, 0, 1).
        Unit vector for the `axis` attribute (+Z axis).

    pos: Base.Vector, optional
        Defaults to Vector(0, 0, 0).
        Vector for the `position` attribute (origin).

    weak: bool, optional
        Defaults to `True`.
        If `True` the WP is in "Auto" mode and will adapt to the current view.

    Note that the u, v and w vectors are not checked for validity.

    Other attributes
    ----------------
    stored: None/list
        Placeholder for a stored state.
    """

    def __init__(self,
                 u=Vector(1, 0, 0), v=Vector(0, 1, 0), w=Vector(0, 0, 1),
                 pos=Vector(0, 0, 0),
                 weak=True):

        if isinstance(u, Plane):
            self.match(u)
            return
        super().__init__(u, v, w, pos)
        self.weak = weak
        # a placeholder for a stored state
        self.stored = None

    def copy(self):
        """See PlaneBase.copy."""
        wp = Plane()
        self.match(source=self, target=wp)
        return wp

    def offsetToPoint(self, point, direction=None):
        """Return the signed distance from a point to the plane. The direction
        argument is ignored.

        The returned value is the negative of the local Z coordinate of the point.
        """
        return -DraftGeomUtils.distance_to_plane(point, self.position, self.axis)

    def projectPoint(self, point, direction=None, force_projection=True):
        """See PlaneBase.project_point."""
        return super().project_point(point, direction, force_projection)

    def alignToPointAndAxis(self, point, axis, offset=0, upvec=None):
        """See PlaneBase.align_to_point_and_axis."""
        if upvec is None:
            upvec = Vector(1, 0, 0)
        super().align_to_point_and_axis(point, axis, offset, upvec)
        self.weak = False
        return True

    def alignToPointAndAxis_SVG(self, point, axis, offset=0):
        """See PlaneBase.align_to_point_and_axis_svg."""
        super().align_to_point_and_axis_svg(point, axis, offset)
        self.weak = False
        return True

    def alignToCurve(self, shape, offset=0):
        """Align the WP to a curve. NOT IMPLEMENTED.

        Parameters
        ----------
        shape: Part.Shape
            Edge or Wire.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `False`
        """
        return False

    def alignToEdges(self, shapes, offset=0):
        """Align the WP to edges with an optional offset.

        The eges must define a plane.

        The endpoints of the first edge defines the WP `position` and `u` axis.

        Parameters
        ----------
        shapes: iterable
            Two edges.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        if super().align_to_edges_vertexes(shapes, offset) is False:
            return False
        self.weak = False
        return True

    def alignToFace(self, shape, offset=0, parent=None):
        """Align the WP to a face with an optional offset.

        The face must be planar.

        The center of gravity of the face defines the WP `position` and the
        normal of the face the WP `axis`. The WP `u` and `v` vectors are
        determined by the DraftGeomUtils.uv_vectors_from_face function.
        See there.

        Parameters
        ----------
        shape: Part.Face
            Face.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        parent: object
            Defaults to `None`.
            The ParentGeoFeatureGroup of the object the face belongs to.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        if shape.ShapeType == "Face" and shape.Surface.isPlanar():
            place = DraftGeomUtils.placement_from_face(shape)
            if parent:
                place = parent.getGlobalPlacement() * place
            super().align_to_placement(place, offset)
            self.weak = False
            return True
        else:
            return False

    def alignTo3Points(self, p1, p2, p3, offset=0):
        """Align the plane to a temporary face created from three points.

        Parameters
        ----------
        p1: Base.Vector
            First point.
        p2: Base.Vector
            Second point.
        p3: Base.Vector
            Third point.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        w = Part.makePolygon([p1, p2, p3, p1])
        f = Part.Face(w)
        return self.alignToFace(f, offset)

    def alignToSelection(self, offset=0):
        """Align the plane to a selection with an optional offset.

        The selection must define a plane.

        Parameter
        ---------
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        sel_ex = FreeCADGui.Selection.getSelectionEx()
        if not sel_ex:
            return False

        shapes = list()
        names = list()
        for obj in sel_ex:
            # check that the geometric property is a Part.Shape object
            geom_is_shape = False
            if isinstance(obj.Object, FreeCAD.GeoFeature):
                geom = obj.Object.getPropertyOfGeometry()
                if isinstance(geom, Part.Shape):
                    geom_is_shape = True
            if not geom_is_shape:
                _wrn(translate(
                    "draft",
                    "Object without Part.Shape geometry:'{}'".format(
                        obj.ObjectName)) + "\n")
                return False
            if geom.isNull():
                _wrn(translate(
                    "draft",
                    "Object with null Part.Shape geometry:'{}'".format(
                        obj.ObjectName)) + "\n")
                return False
            if obj.HasSubObjects:
                shapes.extend(obj.SubObjects)
                names.extend([obj.ObjectName + "." + n for n in obj.SubElementNames])
            else:
                shapes.append(geom)
                names.append(obj.ObjectName)

        normal = None
        for n in range(len(shapes)):
            if not DraftGeomUtils.is_planar(shapes[n]):
                _wrn(translate(
                   "draft", "'{}' object is not planar".format(names[n])) + "\n")
                return False
            if not normal:
                normal = DraftGeomUtils.get_normal(shapes[n])
                shape_ref = n

        # test if all shapes are coplanar
        if normal:
            for n in range(len(shapes)):
                if not DraftGeomUtils.are_coplanar(shapes[shape_ref], shapes[n]):
                    _wrn(translate(
                        "draft", "{} and {} aren't coplanar".format(
                        names[shape_ref],names[n])) + "\n")
                    return False
        else:
            # suppose all geometries are straight lines or points
            points = [vertex.Point for shape in shapes for vertex in shape.Vertexes]
            if len(points) >= 3:
                poly = Part.makePolygon(points)
                if not DraftGeomUtils.is_planar(poly):
                    _wrn(translate(
                        "draft", "All Shapes must be coplanar") + "\n")
                    return False
                normal = DraftGeomUtils.get_normal(poly)
            else:
                normal = None

        if not normal:
            _wrn(translate(
                "draft", "Selected Shapes must define a plane") + "\n")
            return False

        # set center of mass
        ctr_mass = Vector(0,0,0)
        ctr_pts = Vector(0,0,0)
        mass = 0
        for shape in shapes:
            if hasattr(shape, "CenterOfMass"):
                ctr_mass += shape.CenterOfMass*shape.Mass
                mass += shape.Mass
            else:
                ctr_pts += shape.Point
        if mass > 0:
            ctr_mass /= mass
        # all shapes are vertexes
        else:
            ctr_mass = ctr_pts/len(shapes)

        super().align_to_point_and_axis(ctr_mass, normal, offset)
        self.weak = False
        return True

    def setup(self, direction=None, point=None, upvec=None, force=False):
        """Set up the working plane if it exists but is undefined.

        If `direction` and `point` are present,
        it calls `align_to_point_and_axis(point, direction, 0, upvec)`.

        Otherwise, it gets the camera orientation to define
        a working plane that is perpendicular to the current view,
        centered at the origin, and with `v` pointing up on the screen.

        This method only works when the `weak` attribute is `True`.
        This method also sets `weak` to `True`.

        This method only works when `FreeCAD.GuiUp` is `True`,
        that is, when the graphical interface is loaded.
        Otherwise it fails silently.

        Parameters
        ----------
        direction: Base.Vector, optional
            It defaults to `None`. It is the new `axis` of the plane.
        point: Base.Vector, optional
            It defaults to `None`. It is the new `position` of the plane.
        upvec: Base.Vector, optional
            It defaults to `None`. It is the new `v` orientation of the plane.
        force: bool
            If True, it sets the plane even if the plane is not in weak mode
        """
        if self.weak or force:
            if direction is not None and point is not None:
                super().align_to_point_and_axis(point, direction, 0, upvec)
            elif FreeCAD.GuiUp:
                try:
                    view = gui_utils.get_3d_view()
                    if view is not None:
                        cam = view.getCameraNode()
                        rot = FreeCAD.Rotation(*cam.getField("orientation").getValue().getValue())
                        self.u, self.v, self.axis = self._axes_from_view_rotation(rot)
                        self.position = Vector()
                except Exception:
                    pass
            if force:
                self.weak = False
            else:
                self.weak = True

    def reset(self):
        """Reset the WP.

        Sets the `weak` attribute to `True`.
        """
        self.weak = True

    def setTop(self):
        """Set the WP to the top position and updates the GUI."""
        super().set_to_top()
        self.weak = False
        if FreeCAD.GuiUp:
            from draftutils.translate import translate
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                FreeCADGui.draftToolBar.wplabel.setText(translate("draft", "Top"))

    def setFront(self):
        """Set the WP to the front position and updates the GUI."""
        super().set_to_front()
        self.weak = False
        if FreeCAD.GuiUp:
            from draftutils.translate import translate
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                FreeCADGui.draftToolBar.wplabel.setText(translate("draft", "Front"))

    def setSide(self):
        """Set the WP to the left side position and updates the GUI.

        Note that set_to_side from the parent class sets the WP to the right side position.
        Which matches the Side option from Draft_SelectPlane.
        """
        self.u = Vector(0, -1, 0)
        self.v = Vector(0, 0, 1)
        self.axis = Vector(-1, 0, 0)
        self.position = Vector()
        self.weak = False
        if FreeCAD.GuiUp:
            from draftutils.translate import translate
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                FreeCADGui.draftToolBar.wplabel.setText(translate("draft", "Side"))

    def getRotation(self):
        """Return a placement describing the WP orientation only."""
        return FreeCAD.Placement(Vector(), FreeCAD.Rotation(self.u, self.v, self.axis, "ZYX"))

    def getPlacement(self, rotated=False):
        """Return a placement calculated from the WP. The rotated argument is ignored."""
        return super().get_placement()

    def getNormal(self):
        """Return the normal vector (axis) of the WP."""
        return self.axis

    def setFromPlacement(self, place, rebase=False):
        """Align the WP to a placement.

        Parameters
        ----------
        place: Base.Placement
            Placement.
        rebase: bool, optional
            Defaults to `False`.
            If `False` the `position` of the WP is not changed.
        """
        if rebase:
            super().align_to_placement(place)
        else:
            super()._axes_from_rotation(place.Rotation)

    def inverse(self):
        """Invert the direction of the plane.

        It inverts the `u` and `axis` vectors.
        """
        self.u = self.u.negative()
        self.axis = self.axis.negative()

    def save(self):
        """Store the plane attributes.

        Store `u`, `v`, `axis`, `position` and `weak`
        in a list in `stored`.
        """
        self.stored = [self.u, self.v, self.axis, self.position, self.weak]

    def restore(self):
        """Restore the plane attributes that were saved.

        Restores the attributes `u`, `v`, `axis`, `position` and `weak`
        from `stored`, and set `stored` to `None`.
        """
        if self.stored is not None:
            self.u, self.v, self.axis, self.position, self.weak = self.stored
            self.stored = None

    def getLocalCoords(self, point):
        """Translate a point from the global coordinate system to
        the local (WP) coordinate system.
        """
        return super().get_local_coords(point)

    def getGlobalCoords(self, point):
        """Translate a point from the local (WP) coordinate system to
        the global coordinate system.
        """
        return super().get_global_coords(point)

    def getLocalRot(self, vec):
        """Translate a vector from the global coordinate system to
        the local (WP) coordinate system.
        """
        return super().get_local_coords(vec, as_vector=True)

    def getGlobalRot(self, vec):
        """Translate a vector from the local (WP) coordinate system to
        the global coordinate system.
        """
        return super().get_global_coords(vec, as_vector=True)

    def getClosestAxis(self, vec):
        """Return the closest WP axis to a vector.

        Parameters
        ----------
        vec: Base.Vector
            Vector.

        Returns
        -------
        str
            `'x'`, `'y'` or `'z'`.
        """
        return super().get_closest_axis(vec)

    def isGlobal(self):
        """Return `True` if the WP matches the global coordinate system."""
        return super().is_global()

    def isOrtho(self):
        """Return `True` if the WP axes are orthogonal to the global axes."""
        return super().is_ortho()

    def getDeviation(self):
        """Return the angle between the u axis and the horizontal plane.

        It defines a projection of `u` on the horizontal plane
        (without a Z component), and then measures the angle between
        this projection and `u`.

        It also considers the cross product of the projection
        and `u` to determine the sign of the angle.

        Returns
        -------
        float
            Angle between the `u` vector, and a projected vector
            on the global horizontal plane.

        See Also
        --------
        DraftVecUtils.angle
        """
        proj = Vector(self.u.x, self.u.y, 0)
        if self.u.getAngle(proj) == 0:
            return 0
        else:
            norm = proj.cross(self.u)
            return DraftVecUtils.angle(self.u, proj, norm)

    def getParameters(self):
        """Return a dictionary with the data which define the plane:
        `u`, `v`, `axis`, `position`, `weak`.

        Returns
        -------
        dict
            dictionary of the form:
            {"u":x, "v":v, "axis":axis, "position":position, "weak":weak}
        """
        return super().get_parameters()

    def setFromParameters(self, data):
        """Set the plane according to data.

        Parameters
        ----------
        data: dict
            dictionary of the form:
            {"u":x, "v":v, "axis":axis, "position":position, "weak":weak}
       """
        super().set_parameters(data)

    def _get_prop_list(self):
        return ["u",
                "v",
                "axis",
                "position",
                "weak"]

plane = Plane


class PlaneGui(PlaneBase):
    """The PlaneGui class.
    The class handles several GUI related aspects of the WP including a history.

    Parameters
    ----------
    u: Base.Vector or WorkingPlane.Plane, optional
        Defaults to Vector(1, 0, 0).
        If a WP is provided:
            A copy of the WP is created, all other parameters are then ignored.
        If a vector is provided:
            Unit vector for the `u` attribute (+X axis).

    v: Base.Vector, optional
        Defaults to Vector(0, 1, 0).
        Unit vector for the `v` attribute (+Y axis).

    w: Base.Vector, optional
        Defaults to Vector(0, 0, 1).
        Unit vector for the `axis` attribute (+Z axis).

    pos: Base.Vector, optional
        Defaults to Vector(0, 0, 0).
        Vector for the `position` attribute (origin).

    auto: bool, optional
        Defaults to `True`.
        If `True` the WP is in "Auto" mode and will adapt to the current view.

    icon: str, optional
        Defaults to ":/icons/view-axonometric.svg".
        Path to the icon for the draftToolBar.

    label: str, optional
        Defaults to "Auto".
        Label for the draftToolBar.

    tip: str, optional
        Defaults to "Current working plane: Auto".
        Tooltip for the draftToolBar.

    Note that the u, v and w vectors are not checked for validity.

    Other attributes
    ----------------
    _view: Gui::View3DInventor
        Reference to a 3D view.

    _stored: dict
        Dictionary for a temporary stored state.

    _history: dict
        Dictionary that holds up to 10 stored states.
    """

    def __init__(self,
                 u=Vector(1, 0, 0), v=Vector(0, 1, 0), w=Vector(0, 0, 1),
                 pos=Vector(0, 0, 0),
                 auto=True,
                 icon=":/icons/view-axonometric.svg",
                 label=QT_TRANSLATE_NOOP("draft", "Auto"),
                 tip=QT_TRANSLATE_NOOP("draft", "Current working plane:") + " " + QT_TRANSLATE_NOOP("draft", "Auto")):

        if isinstance(u, PlaneGui):
            self.match(u)
        else:
            super().__init__(u, v, w, pos)
            self.auto = auto
            self.icon = icon
            self.label = label
            self.tip = tip
        self._view = None
        self._stored = {}
        self._history = {}

    def copy(self):
        """Return a new plane that is a copy of the present object."""
        wp = PlaneGui()
        self.match(source=self, target=wp)
        return wp

    def _save(self):
        """Store the WP attributes."""
        self._stored = self.get_parameters()

    def _restore(self):
        """Restore the WP attributes that were saved."""
        if self._stored:
            self.set_parameters(self._stored)
            self._stored = {}
            self._update_all(_hist_add=False)

    def align_to_selection(self, offset=0, _hist_add=True):
        """Align the WP to a selection with an optional offset.

        The selection must define a plane.

        Parameter
        ---------
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        if not FreeCAD.GuiUp:
            return False

        sels = FreeCADGui.Selection.getSelectionEx("", 0)
        if not sels:
            return False

        objs = []
        for sel in sels:
            for sub in sel.SubElementNames if sel.SubElementNames else [""]:
                objs.append(Part.getShape(sel.Object, sub, needSubElement=True, retType=1))

        if len(objs) != 1:
            ret = False
            if all([obj[0].isNull() is False and obj[0].ShapeType in ["Edge", "Vertex"] for obj in objs]):
                ret = self.align_to_edges_vertexes([obj[0] for obj in objs], offset, _hist_add)
            elif all([obj[0].isNull() is False and obj[0].ShapeType in ["Edge", "Face"] for obj in objs]):
                    edges = [obj[0] for obj in objs if obj[0].ShapeType == "Edge"]
                    faces = [obj[0] for obj in objs if obj[0].ShapeType == "Face"]
                    if faces and edges:
                            ret = self.align_to_face_and_edge(faces[0], edges[0], offset, _hist_add)
            if ret is False:
                _wrn(translate("draft", "Selected shapes do not define a plane"))
            return ret

        shape, mtx, obj = objs[0]
        place = FreeCAD.Placement(mtx)

        typ = utils.get_type(obj)
        if typ in ["App::Part", "Part::DatumPlane", "PartDesign::Plane", "Axis", "SectionPlane"]:
            ret = self.align_to_obj_placement(obj, offset, place, _hist_add)
        elif typ == "WorkingPlaneProxy":
            ret = self.align_to_wp_proxy(obj, offset, place, _hist_add)
        elif typ == "BuildingPart":
            ret = self.align_to_wp_proxy(obj, offset, place * obj.Placement, _hist_add)
        elif typ == "IfcBuildingStorey":
            pl = FreeCAD.Placement(obj.Placement)
            pl.move(FreeCAD.Vector(0,0,obj.Elevation.Value))
            ret = self.align_to_wp_proxy(obj, offset, place * pl, _hist_add)
        elif shape.isNull():
            ret = self.align_to_obj_placement(obj, offset, place, _hist_add)
        elif shape.ShapeType == "Face":
            ret = self.align_to_face(shape, offset, _hist_add)
        elif shape.ShapeType == "Edge":
            ret = self.align_to_edge_or_wire(shape, offset, _hist_add)
        elif shape.Solids:
            ret = self.align_to_obj_placement(obj, offset, place, _hist_add)
        else:
            ret = self.align_to_edges_vertexes(shape.Vertexes, offset, _hist_add)

        if ret is False:
            _wrn(translate("draft", "Selected shapes do not define a plane"))
        return ret

    def _handle_custom(self, _hist_add):
        self.auto = False
        self.icon = ":/icons/Draft_SelectPlane.svg"
        self.label = self._get_label(translate("draft", "Custom"))
        self.tip = self._get_tip(translate("draft", "Custom"))
        self._update_all(_hist_add)

    def align_to_3_points(self, p1, p2, p3, offset=0, _hist_add=True):
        """See PlaneBase.align_to_3_points."""
        if super().align_to_3_points(p1, p2, p3, offset) is False:
            return False
        self._handle_custom(_hist_add)
        return True

    def align_to_edges_vertexes(self, shapes, offset=0, _hist_add=True):
        """See PlaneBase.align_to_edges_vertexes."""
        if super().align_to_edges_vertexes(shapes, offset) is False:
            return False
        self._handle_custom(_hist_add)
        return True

    def align_to_edge_or_wire(self, shape, offset=0, _hist_add=True):
        """See PlaneBase.align_to_edge_or_wire."""
        if super().align_to_edge_or_wire(shape, offset) is False:
            return False
        self._handle_custom(_hist_add)
        return True

    def align_to_face(self, face, offset=0, _hist_add=True):
        """See PlaneBase.align_to_face."""
        if super().align_to_face(face, offset) is False:
            return False
        self._handle_custom(_hist_add)
        return True

    def align_to_face_and_edge(self, face, edge, offset=0, _hist_add=True):
        """See PlaneBase.align_to_face."""
        if super().align_to_face_and_edge(face, edge, offset) is False:
            return False
        self._handle_custom(_hist_add)
        return True

    def align_to_placement(self, place, offset=0, _hist_add=True):
        """See PlaneBase.align_to_placement."""
        super().align_to_placement(place, offset)
        self._handle_custom(_hist_add)
        return True

    def align_to_point_and_axis(self, point, axis, offset=0, upvec=Vector(1, 0, 0), _hist_add=True):
        """See PlaneBase.align_to_point_and_axis."""
        if super().align_to_point_and_axis(point, axis, offset, upvec) is False:
            return False
        self._handle_custom(_hist_add)
        return True

    def align_to_obj_placement(self, obj, offset=0, place=None, _hist_add=True):
        """Align the WP to an object placement with an optional offset.

        Parameters
        ----------
        obj: App::DocumentObject
            Object to derive the Placement (if place is `None`), the icon and
            the label from.
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        place: Base.Placement, optional
            Defaults to `None`.
            If `None` the Placement from obj is used.
            Argument to be used if obj is inside a container.

        Returns
        -------
        `True`/`False`
            `True` if successful.
        """
        if hasattr(obj, "Placement") is False:
            return False
        if place is None:
            place = obj.Placement
        super().align_to_placement(place, offset)
        self.auto = False
        if utils.get_type(obj) == "WorkingPlaneProxy":
            self.icon = ":/icons/Draft_PlaneProxy.svg"
        elif FreeCAD.GuiUp \
                and hasattr(obj, "ViewObject") \
                and hasattr(obj.ViewObject, "Proxy") \
                and hasattr(obj.ViewObject.Proxy, "getIcon"):
            self.icon = obj.ViewObject.Proxy.getIcon()
        else:
            self.icon = ":/icons/Std_Placement.svg"
        self.label = self._get_label(obj.Label)
        self.tip = self._get_tip(obj.Label)
        self._update_all(_hist_add)
        return True

    def align_to_wp_proxy(self, obj, offset=0, place=None, _hist_add=True):
        """Align the WP to a WPProxy with an optional offset.

        See align_to_obj_placement.

        Also handles several WPProxy related features.
        """
        if self.align_to_obj_placement(obj, offset, place, _hist_add) is False:
            return False

        if not FreeCAD.GuiUp:
            return True

        vobj = obj.ViewObject

        if hasattr(vobj, "AutoWorkingPlane") \
                and vobj.AutoWorkingPlane is True:
            self.auto = True

        if hasattr(vobj, "CutView") \
                and hasattr(vobj, "AutoCutView") \
                and vobj.AutoCutView is True:
            vobj.CutView = True

        if hasattr(vobj, "RestoreView") \
                and vobj.RestoreView is True \
                and hasattr(vobj, "ViewData") \
                and len(vobj.ViewData) >= 12:
            vdat = vobj.ViewData
            if self._view is not None:
                try:
                    if len(vdat) == 13 and vdat[12] == 1:
                        camtype = "Perspective"
                    else:
                        camtype = "Orthographic"
                    if self._view.getCameraType() != camtype:
                        self._view.setCameraType(camtype)

                    cam = self._view.getCameraNode()
                    cam.position.setValue([vdat[0], vdat[1], vdat[2]])
                    cam.orientation.setValue([vdat[3], vdat[4], vdat[5], vdat[6]])
                    cam.nearDistance.setValue(vdat[7])
                    cam.farDistance.setValue(vdat[8])
                    cam.aspectRatio.setValue(vdat[9])
                    cam.focalDistance.setValue(vdat[10])
                    if camtype == "Orthographic":
                        cam.height.setValue(vdat[11])
                    else:
                        cam.heightAngle.setValue(vdat[11])
                except Exception:
                    pass

        if hasattr(vobj, "RestoreState") \
                and vobj.RestoreState is True \
                and hasattr(vobj, "VisibilityMap") \
                and vobj.VisibilityMap:
            for name, vis in vobj.VisibilityMap.items():
                obj = FreeCADGui.ActiveDocument.getObject(name)
                if obj:
                    obj.Visibility = (vis == "True")

        return True

    def auto_align(self):
        """Align the WP to the current view if self.auto is True."""
        if self.auto and self._view is not None:
            try:
                cam = self._view.getCameraNode()
                rot = FreeCAD.Rotation(*cam.getField("orientation").getValue().getValue())
                self.u, self.v, self.axis = self._axes_from_view_rotation(rot)
                self.position = Vector()
            except Exception:
                pass

    def set_to_default(self):
        """Set the WP to the default from the preferences."""
        default_wp = params.get_param("defaultWP")
        if default_wp == 0:
            self.set_to_auto()
        elif default_wp == 1:
            self.set_to_top()
        elif default_wp == 2:
            self.set_to_front()
        elif default_wp == 3:
            self.set_to_side()

    def set_to_auto(self): # Similar to Plane.reset.
        """Set the WP to auto."""
        self.auto = True
        self.auto_align()
        self.icon = ":/icons/view-axonometric.svg"
        self.label = self._get_label(translate("draft", "Auto"))
        self.tip = self._get_tip(translate("draft", "Auto"))
        self._update_all()

    def set_to_top(self, offset=0, center_on_view=False):
        """Set the WP to the top position with an optional offset.

        Parameters
        ----------
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        center_on_view: bool, optional
            Defaults to `False`.
            If `True` the WP `position` is moved along the (offset) WP to the
            center of the view
        """
        super().set_to_top(offset)
        if center_on_view:
            self._center_on_view()
        self.auto = False
        self.icon = ":/icons/view-top.svg"
        self.label = self._get_label(translate("draft", "Top"))
        self.tip = self._get_tip(translate("draft", "Top"))
        self._update_all()

    def set_to_front(self, offset=0, center_on_view=False):
        """Set the WP to the front position with an optional offset.

        Parameters
        ----------
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        center_on_view: bool, optional
            Defaults to `False`.
            If `True` the WP `position` is moved along the (offset) WP to the
            center of the view
        """
        super().set_to_front(offset)
        if center_on_view:
            self._center_on_view()
        self.auto = False
        self.icon = ":/icons/view-front.svg"
        self.label = self._get_label(translate("draft", "Front"))
        self.tip = self._get_tip(translate("draft", "Front"))
        self._update_all()

    def set_to_side(self, offset=0, center_on_view=False):
        """Set the WP to the right side position with an optional offset.

        Parameters
        ----------
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        center_on_view: bool, optional
            Defaults to `False`.
            If `True` the WP `position` is moved along the (offset) WP to the
            center of the view
        """
        super().set_to_side(offset)
        if center_on_view:
            self._center_on_view()
        self.auto = False
        self.icon = ":/icons/view-right.svg"
        self.label = self._get_label(translate("draft", "Side"))
        self.tip = self._get_tip(translate("draft", "Side"))
        self._update_all()

    def set_to_view(self, offset=0, center_on_view=False):
        """Align the WP to the view with an optional offset.

        Parameters
        ----------
        offset: float, optional
            Defaults to zero.
            Offset along the WP `axis`.
        center_on_view: bool, optional
            Defaults to `False`.
            If `True` the WP `position` is moved along the (offset) WP to the
            center of the view
        """
        if self._view is not None:
            try:
                cam = self._view.getCameraNode()
                rot = FreeCAD.Rotation(*cam.getField("orientation").getValue().getValue())
                self.u, self.v, self.axis = self._axes_from_view_rotation(rot)
                self.position = self.axis * offset
                if center_on_view:
                    self._center_on_view()
                self.auto = False
                self.icon = ":/icons/Draft_SelectPlane.svg"
                self.label = self._get_label(translate("draft", "Custom"))
                self.tip = self._get_tip(translate("draft", "Custom"))
                self._update_all()
            except Exception:
                pass

    def set_to_position(self, pos):
        """Set the `position` of the WP."""
        self.position = pos
        label = self.label.rstrip("*")
        self.label = self._get_label(label)
        self.tip = self._get_tip(label)
        self._update_all()

    def center_on_view(self):
        """Move the WP `position` along the WP to the center of the view."""
        self._center_on_view()
        label = self.label.rstrip("*")
        self.label = self._get_label(label)
        self.tip = self._get_tip(label)
        self._update_all()

    def _center_on_view(self):
        if self._view is not None:
            try:
                cam = self._view.getCameraNode()
                pos = self.project_point(Vector(cam.position.getValue().getValue()),
                                         direction=self._view.getViewDirection(),
                                         force_projection=False)
                if pos is not None:
                    self.position = pos
            except Exception:
                pass

    def align_view(self):
        """Align the view to the WP."""
        if self._view is not None:
            try:
                default_cam_dist = abs(params.get_param_view("NewDocumentCameraScale"))
                cam = self._view.getCameraNode()
                cur_cam_dist = abs(self.get_local_coords(Vector(cam.position.getValue().getValue())).z)
                cam_dist = max(default_cam_dist, cur_cam_dist)
                cam.position.setValue(self.position + DraftVecUtils.scaleTo(self.axis, cam_dist))
                cam.orientation.setValue(self.get_placement().Rotation.Q)
            except Exception:
                pass

    def _previous(self):
        idx = self._history["idx"]
        if idx == 0:
            _wrn(translate("draft", "No previous working plane"))
            return
        idx -= 1
        self.set_parameters(self._history["data_list"][idx])
        self._history["idx"] = idx
        self._update_all(_hist_add=False)
        return self._history["data_list"][idx]

    def _next(self):
        idx = self._history["idx"]
        if idx == len(self._history["data_list"]) - 1:
            _wrn(translate("draft", "No next working plane"))
            return
        idx += 1
        self.set_parameters(self._history["data_list"][idx])
        self._history["idx"] = idx
        self._update_all(_hist_add=False)
        return self._history["data_list"][idx]

    def _has_previous(self):
        return bool(self._history) and self._history["idx"] != 0

    def _has_next(self):
        return bool(self._history) and self._history["idx"] != len(self._history["data_list"]) - 1

    def _get_prop_list(self):
        return ["u",
                "v",
                "axis",
                "position",
                "auto",
                "icon",
                "label",
                "tip"]

    def _get_label(self, label):
        if self.auto or self.position.isEqual(Vector(), 0):
            return label
        else:
            return label + "*"

    def _get_tip(self, label):
        tip = translate("draft", "Current working plane:")
        tip += " " + label
        if self.auto:
            return tip
        tip += "\n" + translate("draft", "Axes:")
        tip += "\n    X = "
        tip += self._format_vector(self.u)
        tip += "\n    Y = "
        tip += self._format_vector(self.v)
        tip += "\n    Z = "
        tip += self._format_vector(self.axis)
        tip += "\n" + translate("draft", "Position:")
        tip += "\n    X = "
        tip += self._format_coord(self.position.x)
        tip += "\n    Y = "
        tip += self._format_coord(self.position.y)
        tip += "\n    Z = "
        tip += self._format_coord(self.position.z)
        return tip

    def _format_coord(self, coord):
        return FreeCAD.Units.Quantity(coord, FreeCAD.Units.Length).UserString

    def _format_vector(self, vec):
        dec = params.get_param("Decimals", path="Units")
        return f"({vec.x:.{dec}f}  {vec.y:.{dec}f}  {vec.z:.{dec}f})"

    def _update_all(self, _hist_add=True):
        if _hist_add is True:
            self._update_history()
        self._update_old_plane()  # Must happen before _update_grid.
        self._update_grid()
        self._update_gui()

    def _update_history(self):
        data = self.get_parameters()
        if not self._history:
            self._history = {"idx": 0, "data_list": [data]}
            return
        idx = self._history["idx"]
        if data == self._history["data_list"][idx]:
            return
        if self.auto is True and self._history["data_list"][idx]["auto"] is True:
            return

        max_len = 10  # Max. length of data_list.
        self._history["data_list"] = self._history["data_list"][(idx - (max_len - 2)):(idx + 1)]
        self._history["data_list"].append(data)
        self._history["idx"] = len(self._history["data_list"]) - 1

    def _update_old_plane(self):
        """ Update the old DraftWorkingPlane for compatibility.
        The tracker and snapper code currently still depend on it.
        """
        if not hasattr(FreeCAD, "DraftWorkingPlane"):
            FreeCAD.DraftWorkingPlane = Plane()
        for prop in ["u", "v", "axis", "position"]:
            setattr(FreeCAD.DraftWorkingPlane,
                    prop,
                    self._copy_value(getattr(self, prop)))
        FreeCAD.DraftWorkingPlane.weak = self.auto

    def _update_grid(self):
        # Check for draftToolBar because the trackers (grid) depend on it for its colors.
        if FreeCAD.GuiUp \
                and hasattr(FreeCADGui, "draftToolBar") \
                and hasattr(FreeCADGui, "Snapper") \
                and self._view is not None:
            FreeCADGui.Snapper.setGrid()
            FreeCADGui.Snapper.restack()  # Required??

    def _update_gui(self):
        if FreeCAD.GuiUp \
                and hasattr(FreeCADGui, "draftToolBar") \
                and self._view is not None:
            from PySide import QtGui
            button = FreeCADGui.draftToolBar.wplabel
            button.setIcon(QtGui.QIcon(self.icon))
            button.setText(self.label)
            button.setToolTip(self.tip)


def get_working_plane(update=True):

    if not hasattr(FreeCAD, "draft_working_planes"):
        FreeCAD.draft_working_planes = [[], []]

    view = gui_utils.get_3d_view()

    if view is not None and view in FreeCAD.draft_working_planes[0]:
        i = FreeCAD.draft_working_planes[0].index(view)
        wp = FreeCAD.draft_working_planes[1][i]
        if update is False:
            wp._update_old_plane()  # Currently required for tracker and snapper code.
            return wp
        wp.auto_align()
        wp._update_all(_hist_add=False)
        return wp

    wp = PlaneGui()
    if FreeCAD.GuiUp:
        wp._view = view  # Update _view before call to set_to_default, set_to_auto requires a 3D view.
        wp.set_to_default()
        if view is not None:
            FreeCAD.draft_working_planes[0].append(view)
            FreeCAD.draft_working_planes[1].append(wp)

    return wp


# View observer code to update the Draft Tray:
if FreeCAD.GuiUp:
    from PySide import QtWidgets
    from draftutils.todo import ToDo

    def _update_gui():
        try:
            view = gui_utils.get_3d_view()
            if view is None:
                return
            if not hasattr(FreeCAD, "draft_working_planes"):
                FreeCAD.draft_working_planes = [[], []]
            if view in FreeCAD.draft_working_planes[0]:
                i = FreeCAD.draft_working_planes[0].index(view)
                wp = FreeCAD.draft_working_planes[1][i]
                wp._update_gui()
            else:
                get_working_plane()
        except Exception:
            pass

    def _view_observer_callback():
        ToDo.delay(_update_gui, None)

    _view_observer_active = False

    def _view_observer_start():
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if not _view_observer_active:
            mdi.subWindowActivated.connect(_view_observer_callback)
            _view_observer_active = True
            _view_observer_callback()  # Trigger initial update.

    def _view_observer_stop():
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if _view_observer_active:
            mdi.subWindowActivated.disconnect(_view_observer_callback)
            _view_observer_active = False


# Compatibility function (v1.0, 2023):
def getPlacementFromPoints(points):
    """Return a placement from a list of 3 or 4 points. The 4th point is no longer used.

    Calls DraftGeomUtils.placement_from_points(). See there.
    """
    utils.use_instead("DraftGeomUtils.placement_from_points")
    return DraftGeomUtils.placement_from_points(*points[:3])


# Compatibility function (v1.0, 2023):
def getPlacementFromFace(face, rotated=False):
    """Return a placement from a face.

    Calls DraftGeomUtils.placement_from_face(). See there.
    """
    utils.use_instead("DraftGeomUtils.placement_from_face")
    return DraftGeomUtils.placement_from_face(face, rotated=rotated)
