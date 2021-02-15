# ***************************************************************************
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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

import FreeCAD
import DraftVecUtils
from FreeCAD import Vector

__title__ = "FreeCAD Working Plane utility"
__author__ = "Ken Cline"
__url__ = "https://www.freecadweb.org"


class Plane:
    """A WorkPlane object.

    Parameters
    ----------
    u: Base::Vector3, optional
        An axis (vector) that helps define the working plane.
        It defaults to `(1, 0, 0)`, or the +X axis.

    v: Base::Vector3, optional
        An axis (vector) that helps define the working plane.
        It defaults to `(0, 1, 0)`, or the +Y axis.

    w: Base::Vector3, optional
        An axis that is supposed to be perpendicular to `u` and `v`;
        it is redundant.
        It defaults to `(0, 0, 1)`, or the +Z axis.

    pos: Base::Vector3, optional
        A point through which the plane goes through.
        It defaults to the origin `(0, 0, 0)`.

    Attributes
    ----------
    doc: App::Document
        The active document. Reset view when `doc` changes.

    weak: bool
        It is `True` if the plane has been defined by `setup()`
        or has been reset. A weak plane can be changed
        (it is the "auto" mode), while a strong plane will keep
        its position until weakened (it is "locked")

    u: Base::Vector3
        An axis (vector) that helps define the working plane.

    v: Base::Vector3
        An axis (vector) that helps define the working plane.

    axis: Base::Vector3
        A vector that is supposed to be perpendicular to `u` and `v`;
        it is helpful although redundant.

    position: Base::Vector3
        A point, which the plane goes through,
        that helps define the working plane.

    stored: bool
        A placeholder for a stored state.
    """

    def __init__(self,
                 u=Vector(1, 0, 0), v=Vector(0, 1, 0), w=Vector(0, 0, 1),
                 pos=Vector(0, 0, 0)):

        # keep track of active document.  Reset view when doc changes.
        self.doc = None
        self.weak = True
        self.u = u
        self.v = v
        self.axis = w
        self.position = pos
        # a placeholder for a stored state
        self.stored = None

    def __repr__(self):
        """Show the string representation of the object."""
        text = "Workplane"
        text += " x=" + str(DraftVecUtils.rounded(self.u))
        text += " y=" + str(DraftVecUtils.rounded(self.v))
        text += " z=" + str(DraftVecUtils.rounded(self.axis))
        return text

    def copy(self):
        """Return a new plane that is a copy of the present object."""
        return plane(u=self.u, v=self.v, w=self.axis, pos=self.position)

    def offsetToPoint(self, p, direction=None):
        """Return the signed distance from a point to the plane.

        Parameters
        ----------
        p : Base::Vector3
            The external point to consider.

        direction : Base::Vector3, optional
            The unit vector that indicates the direction of the distance.

            It defaults to `None`, which then uses the `plane.axis` (normal)
            value, meaning that the measured distance is perpendicular
            to the plane.

        Returns
        -------
        float
            The distance from the point to the plane.

        Notes
        -----
        The signed distance `d`, from `p` to the plane, is such that
        ::
            x = p + d*direction,

        where `x` is a point that lies on the plane.

        The `direction` is a unit vector that specifies the direction
        in which the distance is measured.
        It defaults to `plane.axis`,
        meaning that it is the perpendicular distance.

        A picture will help explain the computation
        ::
                                            p
                                          //|
                                        / / |
                                    d /  /  | axis
                                    /   /   |
                                  /    /    |
            -------- plane -----x-----c-----a--------

        The points are as follows

         * `p` is an arbitrary point outside the plane.
         * `c` is a known point on the plane,
           for example, `plane.position`.
         * `x` is the intercept on the plane from `p` in
           the desired `direction`.
         * `a` is the perpendicular intercept on the plane,
           i.e. along `plane.axis`.

        The distance is calculated through the dot product
        of the vector `pc` (going from point `p` to point `c`,
        both of which are known) with the unit vector `direction`
        (which is provided or defaults to `plane.axis`).
        ::
            d = pc . direction
            d = (c - p) . direction

        **Warning:** this implementation doesn't calculate the entire
        distance `|xp|`, only the distance `|pc|` projected onto `|xp|`.

        Trigonometric relationships
        ---------------------------
        In 2D the distances can be calculated by trigonometric relationships
        ::
            |ap| = |cp| cos(apc) = |xp| cos(apx)

        Then the desired distance is `d = |xp|`
        ::
            |xp| = |cp| cos(apc) / cos(apx)

        The cosines can be obtained from the definition of the dot product
        ::
            A . B = |A||B| cos(angleAB)

        If one vector is a unit vector
        ::
            A . uB = |A| cos(angleAB)
            cp . axis = |cp| cos(apc)

        and if both vectors are unit vectors
        ::
            uA . uB = cos(angleAB).
            direction . axis = cos(apx)

        Then
        ::
            d = (cp . axis) / (direction . axis)

        **Note:** for 2D these trigonometric operations
        produce the full `|xp|` distance.
        """
        if direction is None:
            direction = self.axis
        return direction.dot(self.position.sub(p))

    def projectPoint(self, p, direction=None):
        """Project a point onto the plane, by default orthogonally.

        Parameters
        ----------
        p : Base::Vector3
            The point to project.
        direction : Base::Vector3, optional
            The unit vector that indicates the direction of projection.

            It defaults to `None`, which then uses the `plane.axis` (normal)
            value, meaning that the point is projected perpendicularly
            to the plane.

        Returns
        -------
        Base::Vector3
            The projected vector, scaled to the appropriate distance.
        """
        if not direction:
            direction = self.axis
        lp = self.getLocalCoords(p)
        gp = self.getGlobalCoords(Vector(lp.x, lp.y, 0))
        a = direction.getAngle(gp.sub(p))
        if a > math.pi/2:
            direction = direction.negative()
            a = math.pi - a
        ld = self.getLocalRot(direction)
        gd = self.getGlobalRot(Vector(ld.x, ld.y, 0))
        hyp = abs(math.tan(a) * lp.z)
        return gp.add(DraftVecUtils.scaleTo(gd, hyp))

    def projectPointOld(self, p, direction=None):
        """Project a point onto the plane. OBSOLETE.

        Parameters
        ----------
        p : Base::Vector3
            The point to project.
        direction : Base::Vector3, optional
            The unit vector that indicates the direction of projection.

            It defaults to `None`, which then uses the `plane.axis` (normal)
            value, meaning that the point is projected perpendicularly
            to the plane.

        Returns
        -------
        Base::Vector3
            The projected point,
            or the original point if the angle between the `direction`
            and the `plane.axis` is 90 degrees.
        """
        if not direction:
            direction = self.axis
        t = Vector(direction)
        # t.normalize()
        a = round(t.getAngle(self.axis), DraftVecUtils.precision())
        pp = round((math.pi)/2, DraftVecUtils.precision())
        if a == pp:
            return p
        t.multiply(self.offsetToPoint(p, direction))
        return p.add(t)

    def alignToPointAndAxis(self, point, axis, offset=0, upvec=None):
        """Align the working plane to a point and an axis (vector).

        Set `v` as the cross product of `axis` with `(1, 0, 0)` or `+X`,
        and `u` as `v` rotated -90 degrees around the `axis`.
        Also set `weak` to `False`.

        Parameters
        ----------
        point : Base::Vector3
            The new `position` of the plane, adjusted by
            the `offset`.
        axis : Base::Vector3
            A vector whose unit vector will be used as the new `axis`
            of the plane.
            If it is very close to the `X` or `-X` axes,
            it will use this axis exactly, and will adjust `u` and `v`
            to `+Y` and `+Z`, or `-Y` and `+Z`, respectively.
        offset : float, optional
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.
        upvec : Base::Vector3, optional
            Defaults to `None`.
            If it exists, its unit vector will be used as `v`,
            and will set `u` as the cross product of `v` with `axis`.
        """
        self.doc = FreeCAD.ActiveDocument
        self.axis = axis
        self.axis.normalize()
        if axis.getAngle(Vector(1, 0, 0)) < 0.00001:
            self.axis = Vector(1, 0, 0)
            self.u = Vector(0, 1, 0)
            self.v = Vector(0, 0, 1)
        elif axis.getAngle(Vector(-1, 0, 0)) < 0.00001:
            self.axis = Vector(-1, 0, 0)
            self.u = Vector(0, -1, 0)
            self.v = Vector(0, 0, 1)
        elif upvec:
            self.v = upvec
            self.v.normalize()
            self.u = self.v.cross(self.axis)
        else:
            self.v = axis.cross(Vector(1, 0, 0))
            self.v.normalize()
            self.u = DraftVecUtils.rotate(self.v, -math.pi/2, self.axis)
        offsetVector = Vector(axis)
        offsetVector.multiply(offset)
        self.position = point.add(offsetVector)
        self.weak = False
        # Console.PrintMessage("(position = " + str(self.position) + ")\n")
        # Console.PrintMessage(self.__repr__() + "\n")

    def alignToPointAndAxis_SVG(self, point, axis, offset=0):
        """Align the working plane to a point and an axis (vector).

        It aligns `u` and `v` based on the magnitude of the components
        of `axis`.
        Also set `weak` to `False`.

        Parameters
        ----------
        point : Base::Vector3
            The new `position` of the plane, adjusted by
            the `offset`.
        axis : Base::Vector3
            A vector whose unit vector will be used as the new `axis`
            of the plane.
            The magnitudes of the `x`, `y`, `z` components of the axis
            determine the orientation of `u` and `v` of the plane.
        offset : float, optional
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.

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
        self.doc = FreeCAD.ActiveDocument
        self.axis = axis
        self.axis.normalize()
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
        self.weak = False
        # Console.PrintMessage("(position = " + str(self.position) + ")\n")
        # Console.PrintMessage(self.__repr__() + "\n")

    def alignToCurve(self, shape, offset=0):
        """Align plane to curve. NOT YET IMPLEMENTED.

        Parameters
        ----------
        shape : Part.Shape
            A curve that will serve to align the plane.
            It can be an `'Edge'` or `'Wire'`.
        offset : float
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.

        Returns
        -------
        False
            Returns `False` if the shape is null.
            Currently it always returns `False`.
        """
        if shape.isNull():
            return False
        elif shape.ShapeType == 'Edge':
            # ??? TODO: process curve here.  look at shape.edges[0].Curve
            return False
        elif shape.ShapeType == 'Wire':
            # ??? TODO: determine if edges define a plane
            return False
        else:
            return False

    def alignToEdges(self, edges):
        """Align plane to two edges.

        Uses the two points of the first edge to define the direction
        of the unit vector `u`, the other two points of the other edge
        to define the other unit vector `v`, and then the cross product
        of `u` with `v` to define the `axis`.

        Parameters
        ----------
        edges : list
            A list of two edges.

        Returns
        -------
        False
            Return `False` if `edges` is a list of more than 2 elements.
        """
        # use a list of edges to find a plane position
        if len(edges) > 2:
            return False
        # for axes systems, we suppose the 2 first edges are parallel
        # ??? TODO: exclude other cases first
        v1 = edges[0].Vertexes[-1].Point.sub(edges[0].Vertexes[0].Point)
        v2 = edges[1].Vertexes[0].Point.sub(edges[0].Vertexes[0].Point)
        v3 = v1.cross(v2)
        v1.normalize()
        v2.normalize()
        v3.normalize()
        # print(v1,v2,v3)
        self.u = v1
        self.v = v2
        self.axis = v3

    def alignToFace(self, shape, offset=0):
        """Align the plane to a face.

        It uses the center of mass of the face as `position`,
        and its normal in the center of the face as `axis`,
        then calls `alignToPointAndAxis(position, axis, offset)`.

        If the face is a quadrilateral, then it adjusts the position
        of the plane according to its reported X direction and Y direction.

        Also set `weak` to `False`.

        Parameter
        --------
        shape : Part.Face
            A shape of type `'Face'`.

        offset : float
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.

        Returns
        -------
        bool
            `True` if the operation was successful, and `False` if the shape
            is not a `'Face'`.

        See Also
        --------
        alignToPointAndAxis, DraftGeomUtils.getQuad
        """
        # Set face to the unique selected face, if found
        if shape.ShapeType == 'Face':
            self.alignToPointAndAxis(shape.Faces[0].CenterOfMass,
                                     shape.Faces[0].normalAt(0, 0),
                                     offset)
            import DraftGeomUtils
            q = DraftGeomUtils.getQuad(shape)
            if q:
                self.u = q[1]
                self.v = q[2]
                if not DraftVecUtils.equals(self.u.cross(self.v), self.axis):
                    self.u = q[2]
                    self.v = q[1]
                if DraftVecUtils.equals(self.u, Vector(0, 0, 1)):
                    # the X axis is vertical: rotate 90 degrees
                    self.u, self.v = self.v.negative(), self.u
                elif DraftVecUtils.equals(self.u, Vector(0, 0, -1)):
                    self.u, self.v = self.v, self.u.negative()

            self.weak = False
            return True
        else:
            return False

    def alignTo3Points(self, p1, p2, p3, offset=0):
        """Align the plane to three points.

        It makes a closed quadrilateral face with the three points,
        and then calls `alignToFace(shape, offset)`.

        Parameter
        ---------
        p1 : Base::Vector3
            The first point.
        p2 : Base::Vector3
            The second point.
        p3 : Base::Vector3
            The third point.

        offset : float
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.

        Returns
        -------
        bool
            `True` if the operation was successful, and `False` otherwise.
        """
        import Part
        w = Part.makePolygon([p1, p2, p3, p1])
        f = Part.Face(w)
        return self.alignToFace(f, offset)

    def alignToSelection(self, offset=0):
        """Align the plane to a selection if it defines a plane.

        If the selection uniquely defines a plane it will be used.
        Currently it only works with one object selected, a `'Face'`.
        It extracts the shape of the object or subobject
        and then calls `alignToFace(shape, offset)`.

        This method only works when `FreeCAD.GuiUp` is `True`,
        that is, when the graphical interface is loaded.

        Parameter
        ---------
        offset : float
            Defaults to zero. A value which will be used to offset
            the plane in the direction of its `axis`.

        Returns
        -------
        bool
            `True` if the operation was successful, and `False` otherwise.
            It returns `False` if the selection has no elements,
            or if it has more than one element,
            or if the object is not derived from `'Part::Feature'`
            or if the object doesn't have a `Shape`.

        To do
        -----
        The method returns `False` if the selection list has more than
        one element.
        The method should search the list for objects like faces, points,
        edges, wires, etc., and call the appropriate aligning submethod.

        The method could work for curves (`'Edge'`  or `'Wire'`) but
        `alignToCurve()` isn't fully implemented.

        When the interface is not loaded it should fail and print
        a message, `FreeCAD.Console.PrintError()`.

        See Also
        --------
        alignToFace, alignToCurve
        """
        import FreeCADGui
        sex = FreeCADGui.Selection.getSelectionEx(FreeCAD.ActiveDocument.Name)
        if len(sex) == 0:
            return False
        elif len(sex) == 1:
            if (not sex[0].Object.isDerivedFrom("Part::Feature")
                    or not sex[0].Object.Shape):
                return False
            return (self.alignToFace(sex[0].Object.Shape, offset)
                    or (len(sex[0].SubObjects) == 1
                        and self.alignToFace(sex[0].SubObjects[0], offset))
                    or self.alignToCurve(sex[0].Object.Shape, offset))
        else:
            # len(sex) > 2, look for point and line, three points, etc.
            return False

    def setup(self, direction=None, point=None, upvec=None, force=False):
        """Set up the working plane if it exists but is undefined.

        If `direction` and `point` are present,
        it calls `alignToPointAndAxis(point, direction, 0, upvec)`.

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
        direction : Base::Vector3, optional
            It defaults to `None`. It is the new `axis` of the plane.
        point : Base::Vector3, optional
            It defaults to `None`. It is the new `position` of the plane.
        upvec : Base::Vector3, optional
            It defaults to `None`. It is the new `v` orientation of the plane.
        force : Bool
            If True, it sets the plane even if the plane is not in weak mode

        To do
        -----
        When the interface is not loaded it should fail and print
        a message, `FreeCAD.Console.PrintError()`.
        """
        if self.weak or force:
            if direction and point:
                self.alignToPointAndAxis(point, direction, 0, upvec)
            elif FreeCAD.GuiUp:
                try:
                    import FreeCADGui
                    from pivy import coin
                    view = FreeCADGui.ActiveDocument.ActiveView
                    camera = view.getCameraNode()
                    rot = camera.getField("orientation").getValue()
                    coin_up = coin.SbVec3f(0, 1, 0)
                    upvec = Vector(rot.multVec(coin_up).getValue())
                    vdir = view.getViewDirection()
                    # The angle is between 0 and 180 degrees.
                    angle = vdir.getAngle(self.axis)
                    if (angle > 0.001) and (angle < 3.14159):
                        # don't change the plane if it is already
                        # perpendicular to the current view
                        self.alignToPointAndAxis(Vector(0, 0, 0),
                                                 vdir.negative(), 0, upvec)
                except Exception:
                    pass
            if force:
                self.weak = False
            else:
                self.weak = True

    def reset(self):
        """Reset the plane.

        Set the `doc` attribute to `None`, and `weak` to `True`.
        """
        self.doc = None
        self.weak = True

    def setTop(self):
        """sets the WP to top position and updates the GUI"""
        self.alignToPointAndAxis(FreeCAD.Vector(0.0, 0.0, 0.0), FreeCAD.Vector(0, 0, 1), 0.0)
        if FreeCAD.GuiUp:
            import FreeCADGui
            from draftutils.translate import translate
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                FreeCADGui.draftToolBar.wplabel.setText(translate("draft", "Top"))

    def setFront(self):
        """sets the WP to front position and updates the GUI"""
        self.alignToPointAndAxis(FreeCAD.Vector(0.0, 0.0, 0.0), FreeCAD.Vector(0, 1, 0), 0.0)
        if FreeCAD.GuiUp:
            import FreeCADGui
            from draftutils.translate import translate
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                FreeCADGui.draftToolBar.wplabel.setText(translate("draft", "Front"))

    def setSide(self):
        """sets the WP to top position and updates the GUI"""
        self.alignToPointAndAxis(FreeCAD.Vector(0.0, 0.0, 0.0), FreeCAD.Vector(-1, 0, 0), 0.0)
        if FreeCAD.GuiUp:
            import FreeCADGui
            from draftutils.translate import translate
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                FreeCADGui.draftToolBar.wplabel.setText(translate("draft", "Side"))

    def getRotation(self):
        """Return a placement describing the plane orientation only.

        If `FreeCAD.GuiUp` is `True`, that is, if the graphical interface
        is loaded, it will test if the active object is an `Arch` container
        and will calculate the placement accordingly.

        Returns
        -------
        Base::Placement
            A placement, comprised of a `Base` (`Base::Vector3`),
            and a `Rotation` (`Base::Rotation`).
        """
        m = DraftVecUtils.getPlaneRotation(self.u, self.v, self.axis)
        p = FreeCAD.Placement(m)
        # Arch active container
        if FreeCAD.GuiUp:
            import FreeCADGui
            if FreeCADGui.ActiveDocument:
                view = FreeCADGui.ActiveDocument.ActiveView
                if view:
                    a = view.getActiveObject("Arch")
                    if a:
                        p = a.Placement.inverse().multiply(p)
        return p

    def getPlacement(self, rotated=False):
        """Return the placement of the plane.

        Parameters
        ----------
        rotated : bool, optional
            It defaults to `False`. If it is `True`, it switches `axis`
            with `-v` to produce a rotated placement.

        Returns
        -------
        Base::Placement
            A placement, comprised of a `Base` (`Base::Vector3`),
            and a `Rotation` (`Base::Rotation`).
        """
        if rotated:
            m = FreeCAD.Matrix(
                self.u.x, self.axis.x, -self.v.x, self.position.x,
                self.u.y, self.axis.y, -self.v.y, self.position.y,
                self.u.z, self.axis.z, -self.v.z, self.position.z,
                0.0, 0.0, 0.0, 1.0)
        else:
            m = FreeCAD.Matrix(
                self.u.x, self.v.x, self.axis.x, self.position.x,
                self.u.y, self.v.y, self.axis.y, self.position.y,
                self.u.z, self.v.z, self.axis.z, self.position.z,
                0.0, 0.0, 0.0, 1.0)
        p = FreeCAD.Placement(m)
        # Arch active container if based on App Part
        # if FreeCAD.GuiUp:
        #    import FreeCADGui
        #    view = FreeCADGui.ActiveDocument.ActiveView
        #    a = view.getActiveObject("Arch")
        #    if a:
        #        p = a.Placement.inverse().multiply(p)
        return p

    def getNormal(self):
        """Return the normal vector of the plane (axis).

        Returns
        -------
        Base::Vector3
            The `axis` attribute of the plane.
        """
        n = self.axis
        # Arch active container if based on App Part
        # if FreeCAD.GuiUp:
        #    import FreeCADGui
        #    view = FreeCADGui.ActiveDocument.ActiveView
        #    a = view.getActiveObject("Arch")
        #    if a:
        #        n = a.Placement.inverse().Rotation.multVec(n)
        return n

    def setFromPlacement(self, pl, rebase=False):
        """Set the plane from a placement.

        It normally uses only the rotation, unless `rebase` is `True`.

        Parameters
        ----------
        pl : Base::Placement or Base::Matrix4D
            A placement, comprised of a `Base` (`Base::Vector3`),
            and a `Rotation` (`Base::Rotation`),
            or a `Base::Matrix4D` that defines a placement.
        rebase : bool, optional
            It defaults to `False`.
            If `True`, it will use `pl.Base` as the new `position`
            of the plane. Otherwise it will only consider `pl.Rotation`.

        To do
        -----
        If `pl` is a `Base::Matrix4D`, it shouldn't try to use `pl.Base`
        because a matrix has no `Base`.
        """
        rot = FreeCAD.Placement(pl).Rotation
        self.u = rot.multVec(FreeCAD.Vector(1, 0, 0))
        self.v = rot.multVec(FreeCAD.Vector(0, 1, 0))
        self.axis = rot.multVec(FreeCAD.Vector(0, 0, 1))
        if rebase:
            self.position = pl.Base

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
        if self.stored:
            self.u = self.stored[0]
            self.v = self.stored[1]
            self.axis = self.stored[2]
            self.position = self.stored[3]
            self.weak = self.stored[4]
            self.stored = None

    def getLocalCoords(self, point):
        """Return the coordinates of the given point, from the plane.

        If the `point` was constructed using the plane as origin,
        return the relative coordinates from the `point` to the plane.

        A vector is calculated from the plane's `position`
        to the external `point`, and this vector is projected onto
        each of the `u`, `v` and `axis` of the plane to determine
        the local, relative vector.

        Parameters
        ----------
        point : Base::Vector3
            The point external to the plane.

        Returns
        -------
        Base::Vector3
            The relative coordinates of the point from the plane.

        See Also
        --------
        getGlobalCoords, getLocalRot, getGlobalRot

        Notes
        -----
        The following graphic explains the coordinates.
        ::
                                  g GlobalCoords (1, 11)
                                  |
                                  |
                                  |
                              (n) p point (1, 6)
                                  | LocalCoords (1, 1)
                                  |
            ----plane--------c-------- position (0, 5)

        In the graphic

            * `p` is an arbitrary point, external to the plane
            * `c` is the plane's `position`
            * `g` is the global coordinates of `p` when added to the plane
            * `n` is the relative coordinates of `p` when referred to the plane

        To do
        -----
        Maybe a better name would be getRelativeCoords?
        """
        pt = point.sub(self.position)
        xv = DraftVecUtils.project(pt, self.u)
        x = xv.Length
        # If the angle between the projection xv and u
        # is larger than 1 radian (57.29 degrees), use the negative
        # of the magnitude. Why exactly 1 radian?
        if xv.getAngle(self.u) > 1:
            x = -x
        yv = DraftVecUtils.project(pt, self.v)
        y = yv.Length
        if yv.getAngle(self.v) > 1:
            y = -y
        zv = DraftVecUtils.project(pt, self.axis)
        z = zv.Length
        if zv.getAngle(self.axis) > 1:
            z = -z
        return Vector(x, y, z)

    def getGlobalCoords(self, point):
        """Return the coordinates of the given point, added to the plane.

        If the `point` was constructed using the plane as origin,
        return the absolute coordinates from the `point`
        to the global origin.

        The `u`, `v`, and `axis` vectors scale the components of `point`,
        and the result is added to the planes `position`.

        Parameters
        ----------
        point : Base::Vector3
            The external point.

        Returns
        -------
        Base::Vector3
            The coordinates of the point from the absolute origin.

        See Also
        --------
        getLocalCoords, getLocalRot, getGlobalRot

        Notes
        -----
        The following graphic explains the coordinates.
        ::
                                  g GlobalCoords (1, 11)
                                  |
                                  |
                                  |
                              (n) p point (1, 6)
                                  | LocalCoords (1, 1)
                                  |
            ----plane--------c-------- position (0, 5)

        In the graphic

            * `p` is an arbitrary point, external to the plane
            * `c` is the plane's `position`
            * `g` is the global coordinates of `p` when added to the plane
            * `n` is the relative coordinates of `p` when referred to the plane

        """
        vx = Vector(self.u).multiply(point.x)
        vy = Vector(self.v).multiply(point.y)
        vz = Vector(self.axis).multiply(point.z)
        pt = (vx.add(vy)).add(vz)
        return pt.add(self.position)

    def getLocalRot(self, point):
        """Like getLocalCoords, but doesn't use the plane's position.

        If the `point` was constructed using the plane as origin,
        return the relative coordinates from the `point` to the plane.
        However, in this case, the plane is assumed to have its `position`
        at the global origin, therefore, the returned coordinates
        will only consider the orientation of the plane.

        The external `point` is a vector, which is projected onto
        each of the `u`, `v` and `axis` of the plane to determine
        the local, relative vector.

        Parameters
        ----------
        point : Base::Vector3
            The point external to the plane.

        Returns
        -------
        Base::Vector3
            The relative coordinates of the point from the plane,
            if the plane had its `position` at the global origin.

        See Also
        --------
        getLocalCoords, getGlobalCoords, getGlobalRot
        """
        xv = DraftVecUtils.project(point, self.u)
        x = xv.Length
        if xv.getAngle(self.u) > 1:
            x = -x
        yv = DraftVecUtils.project(point, self.v)
        y = yv.Length
        if yv.getAngle(self.v) > 1:
            y = -y
        zv = DraftVecUtils.project(point, self.axis)
        z = zv.Length
        if zv.getAngle(self.axis) > 1:
            z = -z
        return Vector(x, y, z)

    def getGlobalRot(self, point):
        """Like getGlobalCoords, but doesn't use the plane's position.

        If the `point` was constructed using the plane as origin,
        return the absolute coordinates from the `point`
        to the global origin.
        However, in this case, the plane is assumed to have its `position`
        at the global origin, therefore, the returned coordinates
        will only consider the orientation of the plane.

        The `u`, `v`, and `axis` vectors scale the components of `point`.

        Parameters
        ----------
        point : Base::Vector3
            The external point.

        Returns
        -------
        Base::Vector3
            The coordinates of the point from the absolute origin.

        See Also
        --------
        getGlobalCoords, getLocalCoords, getLocalRot
        """
        vx = Vector(self.u).multiply(point.x)
        vy = Vector(self.v).multiply(point.y)
        vz = Vector(self.axis).multiply(point.z)
        pt = (vx.add(vy)).add(vz)
        return pt

    def getClosestAxis(self, point):
        """Return the closest axis of the plane to the given point (vector).

        It tests the angle that the `point` vector makes with the unit vectors
        `u`, `v`, and `axis`, as well their negatives.
        The smallest angle indicates the closest axis.

        Parameters
        ----------
        point : Base::Vector3
            The external point to test.

        Returns
        -------
        str
            * It is `'x'` if the closest axis is `u` or `-u`.
            * It is `'y'` if the closest axis is `v` or `-v`.
            * It is `'z'` if the closest axis is `axis` or `-axis`.
        """
        ax = point.getAngle(self.u)
        ay = point.getAngle(self.v)
        az = point.getAngle(self.axis)
        bx = point.getAngle(self.u.negative())
        by = point.getAngle(self.v.negative())
        bz = point.getAngle(self.axis.negative())
        b = min(ax, ay, az, bx, by, bz)
        if b in [ax, bx]:
            return "x"
        elif b in [ay, by]:
            return "y"
        elif b in [az, bz]:
            return "z"
        else:
            return None

    def isGlobal(self):
        """Return True if the plane axes are equal to the global axes.

        Return `False` if any of `u`, `v`, or `axis` does not correspond
        to `+X`, `+Y`, or `+Z`, respectively.
        """
        if self.u != Vector(1, 0, 0):
            return False
        if self.v != Vector(0, 1, 0):
            return False
        if self.axis != Vector(0, 0, 1):
            return False
        return True

    def isOrtho(self):
        """Return True if the plane axes are orthogonal with the global axes.

        Orthogonal means that the angle between `u` and the global axis `+Y`
        is a multiple of 90 degrees, meaning 0, -90, 90, -180, 180,
        -270, 270, or 360 degrees.
        And similarly for `v` and `axis`.
        All three axes should be orthogonal to the `+Y` axis.

        Due to rounding errors, the angle difference is rounded
        to 6 decimal digits to do the test.

        Returns
        -------
        bool
            Returns `True` if all three `u`, `v`, and `axis`
            are orthogonal with the global axis `+Y`.
            Otherwise it returns `False`.
        """
        ortho = [0, -1.570796, 1.570796,
                 -3.141593, 3.141593,
                 -4.712389, 4.712389, 6.283185]
        # Shouldn't the angle difference be calculated with
        # the other global axes `+X` and `+Z` as well?
        if round(self.u.getAngle(Vector(0, 1, 0)), 6) in ortho:
            if round(self.v.getAngle(Vector(0, 1, 0)), 6) in ortho:
                if round(self.axis.getAngle(Vector(0, 1, 0)), 6) in ortho:
                    return True
        return False

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


plane = Plane


def getPlacementFromPoints(points):
    """Return a placement from a list of 3 or 4 points.

    With these points a temporary `plane` is defined.

    Then it returns the `Base::Placement` returned from `plane.getPlacement()`.

    Parameters
    ----------
    points : list of Base::Vector3
        A list with 3 or 4 points to create a temporary plane
        from which to extract the placement.

        The first point is the plane's `position`.
        The other two points are used to define the `u` and `v` axes,
        as originating from the first point.

        If the fourth point exists, it is used to define the plane's `axis`
        as originating from the first point.
        If no fourth point is provided, the cross product bewtween
        the previously defined `u` and `v` is used as `axis`.

    Return
    ------
    Base::Placement
        A placement obtained from the temporary plane
        defined by `points`,
        or `None` is it fails to use the points.

    See Also
    --------
    getPlacement
    """
    pl = plane()
    try:
        pl.position = points[0]
        pl.u = (points[1].sub(points[0]).normalize())
        pl.v = (points[2].sub(points[0]).normalize())
        if len(points) == 4:
            pl.axis = (points[3].sub(points[0]).normalize())
        else:
            pl.axis = ((pl.u).cross(pl.v)).normalize()
    except Exception:
        return None
    p = pl.getPlacement()
    del pl
    return p


def getPlacementFromFace(face, rotated=False):
    """Return a placement from a face.

    It creates a temporary `plane` and uses `alignToFace(face)`
    to set its orientation.

    Then it returns the `Base::Placement` returned
    from `plane.getPlacement(rotated)`.

    Parameter
    ---------
    face : Part.Face
        A shape of type `'Face'`.
    rotated : bool, optional
        It defaults to `False`. If it is `True`, the temporary plane
        switches `axis` with `-v` to produce a rotated placement.

    Returns
    -------
    Base::Placement
        A placement obtained from the temporary plane
        defined by `face`,
        or `None` if it fails to use `face`.

    See Also
    --------
    alignToFace, getPlacement
    """
    pl = plane()
    try:
        pl.alignToFace(face)
    except Exception:
        return None
    p = pl.getPlacement(rotated)
    del pl
    return p
