# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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
"""Provides Coin based objects used to preview objects being built.

This module provides Coin (pivy) based objects
that are used by the Draft Workbench to draw temporary geometry,
that is, previews, of the real objects that will be created on the 3D view.
"""
## @package gui_trackers
# \ingroup draftguitools
# \brief Provides Coin based objects used to preview objects being built.
#
# This module provides Coin (pivy) based objects
# that are used by the Draft Workbench to draw temporary geometry,
# that is, previews, of the real objects that will be created on the 3D view.

## \addtogroup draftguitools
# @{
import math
import re
import pivy.coin as coin

import FreeCAD
import FreeCADGui
import Draft
import DraftVecUtils
from FreeCAD import Vector
from draftutils import grid_observer
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.messages import _msg
from draftutils.todo import ToDo

__title__ = "FreeCAD Draft Trackers"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"


class Tracker:
    """A generic Draft Tracker, to be used by other specific trackers."""

    def __init__(self, dotted=False, scolor=None, swidth=None, children=[], ontop=False, name=None):
        global Part, DraftGeomUtils
        import Part
        import DraftGeomUtils

        self.ontop = ontop
        self.color = coin.SoBaseColor()
        drawstyle = coin.SoDrawStyle()
        if swidth:
            drawstyle.lineWidth = swidth
        if dotted:
            drawstyle.style = coin.SoDrawStyle.LINES
            drawstyle.lineWeight = 3
            drawstyle.linePattern = 0x0F0F  # 0xaa
        node = coin.SoSeparator()
        for c in [drawstyle, self.color] + children:
            node.addChild(c)
        self.switch = coin.SoSwitch()  # this is the on/off switch
        if name:
            self.switch.setName(name)
        self.switch.addChild(node)
        self.switch.whichChild = -1
        self.setColor(scolor)
        self.Visible = False
        ToDo.delay(self._insertSwitch, self.switch)

    def finalize(self):
        """Finish the command by removing the switch.
        Also called by ghostTracker.remove.
        """
        ToDo.delay(self._removeSwitch, self.switch)
        self.switch = None

    def get_scene_graph(self):
        """Returns the current scenegraph or None if this is not a 3D view"""
        v = Draft.get3DView()
        if v:
            return v.getSceneGraph()
        else:
            return None

    def _insertSwitch(self, switch):
        """Insert self.switch into the scene graph.

        Must not be called
        from an event handler (or other scene graph traversal).
        """
        sg = self.get_scene_graph()
        if not sg:
            return
        if self.ontop:
            sg.insertChild(switch, 0)
        else:
            sg.addChild(switch)

    def _removeSwitch(self, switch):
        """Remove self.switch from the scene graph.

        As with _insertSwitch,
        must not be called during scene graph traversal).
        """
        sg = self.get_scene_graph()
        if not sg:
            return
        if sg.findChild(switch) >= 0:
            sg.removeChild(switch)

    def on(self):
        """Set the visibility to True."""
        self.switch.whichChild = 0
        self.Visible = True

    def off(self):
        """Set the visibility to False."""
        self.switch.whichChild = -1
        self.Visible = False

    def lowerTracker(self):
        """Lower the tracker to the bottom of the scenegraph.

        So it doesn't obscure the other objects.
        """
        if self.switch:
            sg = self.get_scene_graph()
            if not sg:
                return
            sg.removeChild(self.switch)
            sg.addChild(self.switch)

    def raiseTracker(self):
        """Raise the tracker to the top of the scenegraph.

        So it obscures the other objects.
        """
        if self.switch:
            sg = self.get_scene_graph()
            if not sg:
                return
            sg.removeChild(self.switch)
            sg.insertChild(self.switch, 0)

    def setColor(self, color=None):
        """Set the color."""
        if color is not None:
            self.color.rgb = color
        elif (
            hasattr(FreeCAD, "activeDraftCommand")
            and FreeCAD.activeDraftCommand is not None
            and hasattr(FreeCAD.activeDraftCommand, "featureName")
            and FreeCAD.activeDraftCommand.featureName in ("Dimension", "Label", "Text")
        ):
            self.color.rgb = utils.get_rgba_tuple(params.get_param("DefaultAnnoLineColor"))[:3]
        else:
            self.color.rgb = utils.get_rgba_tuple(params.get_param_view("DefaultShapeLineColor"))[
                :3
            ]

    def _get_wp(self):
        return FreeCAD.DraftWorkingPlane


class snapTracker(Tracker):
    """Define Snap Mark tracker, used by tools that support snapping."""

    def __init__(self):
        self.marker = coin.SoMarkerSet()  # this is the marker symbol
        self.marker.markerIndex = FreeCADGui.getMarkerIndex(
            "CIRCLE_FILLED", params.get_param_view("MarkerSize")
        )
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        self.coords.point.setValue((0, 0, 0))
        node = coin.SoAnnotation()
        node.addChild(self.coords)
        node.addChild(self.marker)
        super().__init__(children=[node], name="snapTracker")
        self.setColor()

    def setMarker(self, style):
        """Set the marker index."""
        self.marker.markerIndex = FreeCADGui.getMarkerIndex(
            style, params.get_param_view("MarkerSize")
        )

    def setColor(self, color=None):
        """Set the color."""
        if color is None:
            self.color.rgb = utils.get_rgba_tuple(params.get_param("snapcolor"))[:3]
        else:
            self.color.rgb = color

    def setCoords(self, point):
        """Set the coordinates to the point."""
        self.coords.point.setValue((point.x, point.y, point.z))

    def addCoords(self, point):
        """Add the point to the current point."""
        l = self.coords.point.getValues()
        l.append(coin.SbVec3f(point.x, point.y, point.z))
        self.coords.point.setValues(l)

    def clear(self):
        """Delete the values of the point."""
        self.coords.point.deleteValues(0)


class lineTracker(Tracker):
    """A Line tracker, used by the tools that need to draw temporary lines"""

    def __init__(self, dotted=False, scolor=None, swidth=None, ontop=False):
        line = coin.SoLineSet()
        line.numVertices.setValue(2)
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        self.coords.point.setValues(0, 2, [[0, 0, 0], [1, 0, 0]])
        super().__init__(dotted, scolor, swidth, [self.coords, line], ontop, name="lineTracker")

    def p1(self, point=None):
        """Set or get the first point of the line."""
        if point:
            if self.coords.point.getValues()[0].getValue() != tuple(point):
                self.coords.point.set1Value(0, point.x, point.y, point.z)
        else:
            return Vector(self.coords.point.getValues()[0].getValue())

    def p2(self, point=None):
        """Set or get the second point of the line."""
        if point:
            if self.coords.point.getValues()[-1].getValue() != tuple(point):
                self.coords.point.set1Value(1, point.x, point.y, point.z)
        else:
            return Vector(self.coords.point.getValues()[-1].getValue())

    def getLength(self):
        """Return the length of the line."""
        p1 = Vector(self.coords.point.getValues()[0].getValue())
        p2 = Vector(self.coords.point.getValues()[-1].getValue())
        return (p2.sub(p1)).Length


class polygonTracker(Tracker):
    """A Polygon tracker, used by the polygon tool."""

    def __init__(self, dotted=False, scolor=None, swidth=None, face=False, sides=None):
        self.origin = Vector(0, 0, 0)
        self.line = coin.SoLineSet()
        self.sides = sides if sides is not None else 3
        self.base_angle = None
        self.line.numVertices.setValue(self.sides + 1)
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        # fmt: off
        self.coords.point.setValues(0, 50, [[0, 0, 0],
                                            [2, 0, 0],
                                            [1, 2, 0],
                                            [0, 0, 0]])
        # fmt: on
        if face:
            m1 = coin.SoMaterial()
            m1.transparency.setValue(0.5)
            m1.diffuseColor.setValue([0.5, 0.5, 1.0])
            f = coin.SoIndexedFaceSet()
            f.coordIndex.setValues([0, 1, 2, 3])
            super().__init__(
                dotted, scolor, swidth, [self.coords, self.line, m1, f], name="polygonTracker"
            )
        else:
            super().__init__(
                dotted, scolor, swidth, [self.coords, self.line], name="polygonTracker"
            )

    def setNumVertices(self, num):
        self.line.numVertices.setValue(num + 1)
        self.sides = num

    def _drawPolygon(self, origin, radius):
        wp = self._get_wp()
        local_origin = wp.get_local_coords(origin)
        for i in range(self.sides):
            angle = 2 * math.pi * i / self.sides
            px = local_origin.x + radius * math.cos(angle)
            py = local_origin.y + radius * math.sin(angle)

            # Back to global space
            global_point = wp.get_global_coords(Vector(px, py, 0))
            self.coords.point.set1Value(i, *global_point)

        # Close the polygon by repeating the first point
        first = self.coords.point[0].getValue()
        self.coords.point.set1Value(self.sides, *first)

    def setOrigin(self, point, radius=1.0):
        """Set the origin of the polygon and initialize the shape."""
        self.origin = point
        self._drawPolygon(self.origin, radius)

    def update(self, point):
        """Update polygon size based on current mouse point."""
        local_vec = self._get_wp().get_local_coords(point - self.origin, as_vector=True)
        radius = math.hypot(local_vec.x, local_vec.y)
        self._drawPolygon(self.origin, radius)


class rectangleTracker(Tracker):
    """A Rectangle tracker, used by the rectangle tool."""

    def __init__(self, dotted=False, scolor=None, swidth=None, face=False):
        self.origin = Vector(0, 0, 0)
        line = coin.SoLineSet()
        line.numVertices.setValue(5)
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        # fmt: off
        self.coords.point.setValues(0, 50, [[0, 0, 0],
                                            [2, 0, 0],
                                            [2, 2, 0],
                                            [0, 2, 0],
                                            [0, 0, 0]])
        # fmt: on
        if face:
            m1 = coin.SoMaterial()
            m1.transparency.setValue(0.5)
            m1.diffuseColor.setValue([0.5, 0.5, 1.0])
            f = coin.SoIndexedFaceSet()
            f.coordIndex.setValues([0, 1, 2, 3])
            super().__init__(
                dotted, scolor, swidth, [self.coords, line, m1, f], name="rectangleTracker"
            )
        else:
            super().__init__(dotted, scolor, swidth, [self.coords, line], name="rectangleTracker")
        wp = self._get_wp()
        self.u = wp.u
        self.v = wp.v

    def setorigin(self, point):
        """Set the base point of the rectangle."""
        self.coords.point.set1Value(0, point.x, point.y, point.z)
        self.coords.point.set1Value(4, point.x, point.y, point.z)
        self.origin = point

    def update(self, point):
        """Set the opposite (diagonal) point of the rectangle."""
        diagonal = point.sub(self.origin)
        inpoint1 = self.origin.add(DraftVecUtils.project(diagonal, self.v))
        inpoint2 = self.origin.add(DraftVecUtils.project(diagonal, self.u))
        self.coords.point.set1Value(1, inpoint1.x, inpoint1.y, inpoint1.z)
        self.coords.point.set1Value(2, point.x, point.y, point.z)
        self.coords.point.set1Value(3, inpoint2.x, inpoint2.y, inpoint2.z)

    def setPlane(self, u, v=None):
        """Set given (u,v) vectors as working plane.

        You can give only `u` and `v` will be deduced automatically
        given the current working plane.
        """
        self.u = u
        if v:
            self.v = v
        else:
            norm = self._get_wp().axis
            self.v = norm.cross(self.u)

    def p1(self, point=None):
        """Set or get the base point of the rectangle."""
        if point:
            self.setorigin(point)
        else:
            return Vector(self.coords.point.getValues()[0].getValue())

    def p2(self):
        """Get the second point (on u axis) of the rectangle."""
        return Vector(self.coords.point.getValues()[3].getValue())

    def p3(self, point=None):
        """Set or get the opposite (diagonal) point of the rectangle."""
        if point:
            self.update(point)
        else:
            return Vector(self.coords.point.getValues()[2].getValue())

    def p4(self):
        """Get the fourth point (on v axis) of the rectangle."""
        return Vector(self.coords.point.getValues()[1].getValue())

    def getSize(self):
        """Return (length, width) of the rectangle."""
        p1 = Vector(self.coords.point.getValues()[0].getValue())
        p2 = Vector(self.coords.point.getValues()[2].getValue())
        diag = p2.sub(p1)
        return (
            (DraftVecUtils.project(diag, self.u)).Length,
            (DraftVecUtils.project(diag, self.v)).Length,
        )

    def getNormal(self):
        """Return the normal of the rectangle."""
        return (self.u.cross(self.v)).normalize()

    def isInside(self, point):
        """Return True if the given point is inside the rectangle."""
        vp = point.sub(self.p1())
        uv = self.p2().sub(self.p1())
        vv = self.p4().sub(self.p1())
        uvp = DraftVecUtils.project(vp, uv)
        vvp = DraftVecUtils.project(vp, vv)
        if uvp.getAngle(uv) < 1:
            if vvp.getAngle(vv) < 1:
                if uvp.Length <= uv.Length:
                    if vvp.Length <= vv.Length:
                        return True
        return False


class dimTracker(Tracker):
    """A Dimension tracker, used by the dimension tool."""

    def __init__(self, dotted=False, scolor=None, swidth=None):
        line = coin.SoLineSet()
        line.numVertices.setValue(4)
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        # fmt: off
        self.coords.point.setValues(0, 4,
                                    [[0, 0, 0],
                                     [0, 0, 0],
                                     [0, 0, 0],
                                     [0, 0, 0]])
        # fmt: on
        super().__init__(dotted, scolor, swidth, [self.coords, line], name="dimTracker")
        self.p1 = self.p2 = self.p3 = None

    def update(self, pts):
        """Update the points and calculate."""
        if not pts:
            return
        elif len(pts) == 1:
            self.p3 = pts[0]
        else:
            self.p1 = pts[0]
            self.p2 = pts[1]
            if len(pts) > 2:
                self.p3 = pts[2]
        self.calc()

    def calc(self):
        """Calculate the new points from p1 and p2."""
        import Part

        if (self.p1 is not None) and (self.p2 is not None):
            # fmt: off
            points = [DraftVecUtils.tup(self.p1, True),
                      DraftVecUtils.tup(self.p2, True),
                      DraftVecUtils.tup(self.p1, True),
                      DraftVecUtils.tup(self.p2, True)]
            # fmt: on
            if self.p3 is not None:
                p1 = self.p1
                p4 = self.p2
                if DraftVecUtils.equals(p1, p4):
                    proj = None
                else:
                    base = Part.LineSegment(p1, p4).toShape()
                    proj = DraftGeomUtils.findDistance(self.p3, base)
                if not proj:
                    p2 = p1
                    p3 = p4
                else:
                    p2 = p1.add(proj.negative())
                    p3 = p4.add(proj.negative())
                # fmt: off
                points = [DraftVecUtils.tup(p1),
                          DraftVecUtils.tup(p2),
                          DraftVecUtils.tup(p3),
                          DraftVecUtils.tup(p4)]
                # fmt: on
            self.coords.point.setValues(0, 4, points)


class bsplineTracker(Tracker):
    """A bspline tracker."""

    def __init__(self, dotted=False, scolor=None, swidth=None, points=[]):
        self.bspline = None
        self.points = points
        self.trans = coin.SoTransform()
        self.sep = coin.SoSeparator()
        self.recompute()
        super().__init__(dotted, scolor, swidth, [self.trans, self.sep], name="bsplineTracker")

    def update(self, points):
        """Update the points and recompute."""
        self.points = points
        self.recompute()

    def recompute(self):
        """Recompute the tracker."""
        if len(self.points) >= 2:
            if self.bspline:
                self.sep.removeChild(self.bspline)
            self.bspline = None
            c = Part.BSplineCurve()
            # DNC: allows one to close the curve by placing ends close to each other
            if len(self.points) >= 3 and (
                (self.points[0] - self.points[-1]).Length < Draft.tolerance()
            ):
                # YVH: Added a try to bypass some hazardous situations
                try:
                    c.interpolate(self.points[:-1], True)
                except Part.OCCError:
                    pass
            elif self.points:
                try:
                    c.interpolate(self.points, False)
                except Part.OCCError:
                    pass
            c = c.toShape()
            buf = c.writeInventor(2, 0.01)
            # fp = open("spline.iv", "w")
            # fp.write(buf)
            # fp.close()
            try:
                ivin = coin.SoInput()
                ivin.setBuffer(buf)
                ivob = coin.SoDB.readAll(ivin)
            except Exception:
                # workaround for pivy SoInput.setBuffer() bug
                buf = buf.replace("\n", "")
                pts = re.findall(r"point \\[(.*?)\\]", buf)[0]
                pts = pts.split(",")
                pc = []
                for p in pts:
                    v = p.strip().split()
                    pc.append([float(v[0]), float(v[1]), float(v[2])])
                coords = coin.SoCoordinate3()
                coords.point.setValues(0, len(pc), pc)
                line = coin.SoLineSet()
                line.numVertices.setValue(-1)
                self.bspline = coin.SoSeparator()
                self.bspline.addChild(coords)
                self.bspline.addChild(line)
                self.sep.addChild(self.bspline)
            else:
                if ivob and ivob.getNumChildren() > 1:
                    self.bspline = ivob.getChild(1).getChild(0)
                    self.bspline.removeChild(self.bspline.getChild(0))
                    self.bspline.removeChild(self.bspline.getChild(0))
                    self.sep.addChild(self.bspline)
                else:
                    FreeCAD.Console.PrintWarning(
                        "bsplineTracker.recompute() failed to read-in Inventor string\n"
                    )


class bezcurveTracker(Tracker):
    """A bezcurve tracker."""

    def __init__(self, dotted=False, scolor=None, swidth=None, points=[]):
        self.bezcurve = None
        self.points = points
        self.degree = None
        self.trans = coin.SoTransform()
        self.sep = coin.SoSeparator()
        self.recompute()
        super().__init__(dotted, scolor, swidth, [self.trans, self.sep], name="bezcurveTracker")

    def update(self, points, degree=None):
        """Update the points and recompute."""
        self.points = points
        if degree:
            self.degree = degree
        self.recompute()

    def recompute(self):
        """Recompute the tracker."""
        if self.bezcurve:
            for seg in self.bezcurve:
                self.sep.removeChild(seg)
                seg = None

        self.bezcurve = []

        if len(self.points) >= 2:
            if self.degree:
                poles = self.points[1:]
                segpoleslst = [
                    poles[x : x + self.degree] for x in range(0, len(poles), (self.degree or 1))
                ]
            else:
                segpoleslst = [self.points]
            startpoint = self.points[0]

            for segpoles in segpoleslst:
                c = Part.BezierCurve()  # last segment may have lower degree
                c.increase(len(segpoles))
                c.setPoles([startpoint] + segpoles)
                c = c.toShape()
                startpoint = segpoles[-1]
                buf = c.writeInventor(2, 0.01)
                # fp=open("spline.iv", "w")
                # fp.write(buf)
                # fp.close()
                try:
                    ivin = coin.SoInput()
                    ivin.setBuffer(buf)
                    ivob = coin.SoDB.readAll(ivin)
                except Exception:
                    # workaround for pivy SoInput.setBuffer() bug
                    buf = buf.replace("\n", "")
                    pts = re.findall(r"point \\[(.*?)\\]", buf)[0]
                    pts = pts.split(",")
                    pc = []
                    for p in pts:
                        v = p.strip().split()
                        pc.append([float(v[0]), float(v[1]), float(v[2])])
                    coords = coin.SoCoordinate3()
                    coords.point.setValues(0, len(pc), pc)
                    line = coin.SoLineSet()
                    line.numVertices.setValue(-1)
                    bezcurveseg = coin.SoSeparator()
                    bezcurveseg.addChild(coords)
                    bezcurveseg.addChild(line)
                    self.sep.addChild(bezcurveseg)
                else:
                    if ivob and ivob.getNumChildren() > 1:
                        bezcurveseg = ivob.getChild(1).getChild(0)
                        bezcurveseg.removeChild(bezcurveseg.getChild(0))
                        bezcurveseg.removeChild(bezcurveseg.getChild(0))
                        self.sep.addChild(bezcurveseg)
                    else:
                        FreeCAD.Console.PrintWarning(
                            "bezcurveTracker.recompute() failed to read-in Inventor string\n"
                        )
                self.bezcurve.append(bezcurveseg)


class arcTracker(Tracker):
    """An arc tracker."""

    # Note: used by the Arc command but also for angular dimensions.
    def __init__(self, dotted=False, scolor=None, swidth=None, start=0, end=math.pi * 2):
        self.circle = None
        self.startangle = math.degrees(start)
        self.endangle = math.degrees(end)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        self.sep = coin.SoSeparator()
        self.autoinvert = True
        self.normal = self._get_wp().axis
        self.recompute()
        super().__init__(dotted, scolor, swidth, [self.trans, self.sep], name="arcTracker")

    def getDeviation(self):
        """Return a deviation vector that represents the base of the circle."""
        import Part

        c = Part.makeCircle(1, Vector(0, 0, 0), self.normal)
        return c.Vertexes[0].Point

    def setCenter(self, cen):
        """Set the center point."""
        self.trans.translation.setValue([cen.x, cen.y, cen.z])

    def setRadius(self, rad):
        """Set the radius."""
        self.trans.scaleFactor.setValue([rad, rad, rad])

    def getRadius(self):
        """Return the current radius."""
        return self.trans.scaleFactor.getValue()[0]

    def setStartAngle(self, ang):
        """Set the start angle."""
        self.startangle = math.degrees(ang)
        self.recompute()

    def setEndAngle(self, ang):
        """Set the end angle."""
        self.endangle = math.degrees(ang)
        self.recompute()

    def getAngle(self, pt):
        """Return the angle of a given vector in radians."""
        c = self.trans.translation.getValue()
        center = Vector(c[0], c[1], c[2])
        return DraftVecUtils.angle(pt.sub(center), self.getDeviation(), self.normal)

    def getAngles(self):
        """Return the start and end angles in degrees."""
        return (self.startangle, self.endangle)

    def setStartPoint(self, pt):
        """Set the start angle from a point."""
        self.setStartAngle(-self.getAngle(pt))

    def setEndPoint(self, pt):
        """Set the end angle from a point."""
        self.setEndAngle(-self.getAngle(pt))

    def setApertureAngle(self, ang):
        """Set the end angle by giving the aperture angle."""
        ap = math.degrees(ang)
        self.endangle = self.startangle + ap
        self.recompute()

    def setBy3Points(self, p1, p2, p3):
        """Set the arc by three points."""
        import Part

        try:
            arc = Part.ArcOfCircle(p1, p2, p3)
        except Exception:
            return
        e = arc.toShape()
        self.autoinvert = False
        self.normal = e.Curve.Axis.negative()  # axis is always in wrong direction
        self.setCenter(e.Curve.Center)
        self.setRadius(e.Curve.Radius)
        self.setStartPoint(p1)
        self.setEndPoint(p3)

    def recompute(self):
        """Recompute the tracker."""
        import Part

        if self.circle:
            self.sep.removeChild(self.circle)
        self.circle = None
        if (self.endangle < self.startangle) or not self.autoinvert:
            c = Part.makeCircle(1, Vector(0, 0, 0), self.normal, self.endangle, self.startangle)
        else:
            c = Part.makeCircle(1, Vector(0, 0, 0), self.normal, self.startangle, self.endangle)
        buf = c.writeInventor(2, 0.01)
        try:
            ivin = coin.SoInput()
            ivin.setBuffer(buf)
            ivob = coin.SoDB.readAll(ivin)
        except Exception:
            # workaround for pivy SoInput.setBuffer() bug
            buf = buf.replace("\n", "")
            pts = re.findall(r"point \\[(.*?)\\]", buf)[0]
            pts = pts.split(",")
            pc = []
            for p in pts:
                v = p.strip().split()
                pc.append([float(v[0]), float(v[1]), float(v[2])])
            coords = coin.SoCoordinate3()
            coords.point.setValues(0, len(pc), pc)
            line = coin.SoLineSet()
            line.numVertices.setValue(-1)
            self.circle = coin.SoSeparator()
            self.circle.addChild(coords)
            self.circle.addChild(line)
            self.sep.addChild(self.circle)
        else:
            if ivob and ivob.getNumChildren() > 1:
                self.circle = ivob.getChild(1).getChild(0)
                self.circle.removeChild(self.circle.getChild(0))
                self.circle.removeChild(self.circle.getChild(0))
                self.sep.addChild(self.circle)
            else:
                FreeCAD.Console.PrintWarning(
                    "arcTracker.recompute() failed to read-in Inventor string\n"
                )


class ghostTracker(Tracker):
    """A Ghost tracker, that allows one to copy whole object representations.

    You can pass it an object or a list of objects, or a shape.
    """

    def __init__(
        self, sel, dotted=False, scolor=None, swidth=None, mirror=False, parent_places=None
    ):
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        self.children = [self.trans]
        self.rootsep = coin.SoSeparator()
        if not isinstance(sel, list):
            sel = [sel]
        for idx, obj in enumerate(sel):
            if parent_places is not None:
                parent_place = parent_places[idx]
            else:
                parent_place = None
            import Part

            if not isinstance(obj, Part.Vertex):
                self.rootsep.addChild(self.getNode(obj, parent_place))
            else:
                self.coords = coin.SoCoordinate3()
                self.coords.point.setValue((obj.X, obj.Y, obj.Z))
                self.marker = coin.SoMarkerSet()  # this is the marker symbol
                self.marker.markerIndex = FreeCADGui.getMarkerIndex(
                    "SQUARE_FILLED", params.get_param_view("MarkerSize")
                )
                node = coin.SoAnnotation()
                selnode = coin.SoSeparator()
                selnode.addChild(self.coords)
                selnode.addChild(self.marker)
                node.addChild(selnode)
                self.rootsep.addChild(node)
        if mirror is True:
            self._do_flip(self.rootsep)
            self.flipped = True
        else:
            self.flipped = False
        self.children.append(self.rootsep)
        super().__init__(dotted, scolor, swidth, children=self.children, name="ghostTracker")
        self.setColor(scolor)

    def setColor(self, color=None):
        """Set the color."""
        if color is None:
            self.color.rgb = utils.get_rgba_tuple(params.get_param("snapcolor"))[:3]
        else:
            self.color.rgb = color

    def remove(self):
        """Remove the ghost when switching to and from subelement mode."""
        if self.switch:
            self.finalize()

    def move(self, delta):
        """Move the ghost to a given position.

        Relative from its start position.
        """
        self.trans.translation.setValue([delta.x, delta.y, delta.z])

    def rotate(self, axis, angle):
        """Rotate the ghost of a given angle."""
        self.trans.rotation.setValue(coin.SbVec3f(DraftVecUtils.tup(axis)), angle)

    def center(self, point):
        """Set the rotation/scale center of the ghost."""
        self.trans.center.setValue(point.x, point.y, point.z)

    def scale(self, delta):
        """Scale the ghost by the given factor."""
        self.trans.scaleFactor.setValue([delta.x, delta.y, delta.z])

    def getNode(self, obj, parent_place=None):
        """Return a coin node representing the given object."""
        import Part

        if isinstance(obj, Part.Shape):
            return self.getNodeLight(obj)
        else:
            return self.getNodeFull(obj, parent_place)

    def getNodeFull(self, obj, parent_place=None):
        """Get a coin node which is a copy of the current representation."""
        sep = coin.SoSeparator()
        try:
            sep.addChild(obj.ViewObject.RootNode.copy())
            # add Part container offset
            if parent_place is not None:
                if hasattr(obj, "Placement") and utils.get_type(obj) != "Label":
                    gpl = parent_place * obj.Placement
                else:
                    gpl = parent_place
            elif hasattr(obj, "getGlobalPlacement"):
                gpl = obj.getGlobalPlacement()
            else:
                gpl = None
            if gpl is not None:
                transform = gui_utils.find_coin_node(sep.getChild(0), coin.SoTransform)
                if transform is not None:
                    transform.translation.setValue(tuple(gpl.Base))
                    transform.rotation.setValue(gpl.Rotation.Q)
        except Exception:
            _msg("ghostTracker: Error retrieving coin node (full)")
        return sep

    def getNodeLight(self, shape):
        """Extract a lighter version directly from a shape."""
        # error-prone
        sep = coin.SoSeparator()
        try:
            inputstr = coin.SoInput()
            inputstr.setBuffer(shape.writeInventor())
            coinobj = coin.SoDB.readAll(inputstr)
            # only add wireframe or full node?
            sep.addChild(coinobj.getChildren()[1])
            # sep.addChild(coinobj)
        except Exception:
            _msg("ghostTracker: Error retrieving coin node (light)")
        return sep

    def getMatrix(self):
        """Get matrix of the active view."""
        r = (
            FreeCADGui.ActiveDocument.ActiveView.getViewer()
            .getSoRenderManager()
            .getViewportRegion()
        )
        v = coin.SoGetMatrixAction(r)
        m = self.trans.getMatrix(v)
        if m:
            m = m.getValue()
            # fmt: off
            return FreeCAD.Matrix(m[0][0], m[0][1], m[0][2], m[0][3],
                                  m[1][0], m[1][1], m[1][2], m[1][3],
                                  m[2][0], m[2][1], m[2][2], m[2][3],
                                  m[3][0], m[3][1], m[3][2], m[3][3])
            # fmt: on
        else:
            return FreeCAD.Matrix()

    def setMatrix(self, matrix):
        """Set the transformation matrix.

        The 4th column of the matrix (the position) is ignored.
        """
        # fmt: off
        m = coin.SbMatrix(matrix.A11, matrix.A12, matrix.A13, matrix.A14,
                          matrix.A21, matrix.A22, matrix.A23, matrix.A24,
                          matrix.A31, matrix.A32, matrix.A33, matrix.A34,
                          matrix.A41, matrix.A42, matrix.A43, matrix.A44)
        # fmt: on
        self.trans.setMatrix(m)

    def flip_normals(self, flip):
        if flip:
            if not self.flipped:
                self._do_flip(self.rootsep)
                self.flipped = True
        else:
            if self.flipped:
                self._do_flip(self.rootsep)
                self.flipped = False

    def _do_flip(self, root):
        """Flip the normals of the coin faces."""
        # Code by wmayer:
        # https://forum.freecad.org/viewtopic.php?p=702640#p702640
        search = coin.SoSearchAction()
        search.setType(coin.SoIndexedFaceSet.getClassTypeId())
        search.apply(root)
        path = search.getPath()
        if path:
            node = path.getTail()
            index = node.coordIndex.getValues()
            if len(index) % 4 == 0:
                for i in range(0, len(index), 4):
                    tmp = index[i]
                    index[i] = index[i + 1]
                    index[i + 1] = tmp

                node.coordIndex.setValues(index)


class editTracker(Tracker):
    """A node edit tracker."""

    def __init__(
        self, pos=Vector(0, 0, 0), name=None, idx=0, objcol=None, marker=None, inactive=False
    ):
        self.marker = coin.SoMarkerSet()  # this is the marker symbol
        if marker is None:
            self.marker.markerIndex = FreeCADGui.getMarkerIndex(
                "SQUARE_FILLED", params.get_param_view("MarkerSize")
            )
        else:
            self.marker.markerIndex = marker
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        self.coords.point.setValue((pos.x, pos.y, pos.z))
        self.position = pos
        if inactive:
            self.selnode = coin.SoSeparator()
        else:
            self.selnode = coin.SoType.fromName("SoFCSelection").createInstance()
            if name:
                self.selnode.useNewSelection = False
                self.selnode.documentName.setValue(FreeCAD.ActiveDocument.Name)
                self.selnode.objectName.setValue(name)
                self.selnode.subElementName.setValue("EditNode" + str(idx))
        node = coin.SoAnnotation()
        self.selnode.addChild(self.coords)
        self.selnode.addChild(self.marker)
        node.addChild(self.selnode)
        ontop = not inactive
        super().__init__(children=[node], ontop=ontop, name="editTracker")
        if objcol is None:
            self.setColor()
        else:
            self.setColor(objcol[:3])
        self.on()

    def set(self, pos):
        """Set the point to the position."""
        self.coords.point.setValue((pos.x, pos.y, pos.z))
        self.position = pos

    def get(self):
        """Get a vector from the point."""
        return self.position

    def get_doc_name(self):
        """Get the document name."""
        return str(self.selnode.documentName.getValue())

    def get_obj_name(self):
        """Get the object name."""
        return str(self.selnode.objectName.getValue())

    def get_subelement_name(self):
        """Get the subelement name."""
        return str(self.selnode.subElementName.getValue())

    def get_subelement_index(self):
        """Get the subelement index."""
        subElement = self.get_subelement_name()
        idx = int(subElement[8:])
        return idx

    def move(self, delta):
        """Get the point and add a delta, and set the new point."""
        self.set(self.get().add(delta))

    def setColor(self, color=None):
        """Set the color."""
        if color is None:
            self.color.rgb = utils.get_rgba_tuple(params.get_param("snapcolor"))[:3]
        else:
            self.color.rgb = color


class PlaneTracker(Tracker):
    """A working plane tracker."""

    def __init__(self):
        # getting screen distance
        p1 = Draft.get3DView().getPoint((100, 100))
        p2 = Draft.get3DView().getPoint((110, 100))
        bl = (p2.sub(p1)).Length * (params.get_param("snapRange") / 2.0)
        pick = coin.SoPickStyle()
        pick.style.setValue(coin.SoPickStyle.UNPICKABLE)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        m1 = coin.SoMaterial()
        m1.transparency.setValue(0.8)
        m1.diffuseColor.setValue([0.4, 0.4, 0.6])
        c1 = coin.SoCoordinate3()
        # fmt: off
        c1.point.setValues([[-bl, -bl, 0],
                            [bl, -bl, 0],
                            [bl, bl, 0],
                            [-bl, bl, 0]])
        # fmt: on
        f = coin.SoIndexedFaceSet()
        f.coordIndex.setValues([0, 1, 2, 3])
        m2 = coin.SoMaterial()
        m2.transparency.setValue(0.7)
        m2.diffuseColor.setValue([0.2, 0.2, 0.3])
        c2 = coin.SoCoordinate3()
        # fmt: off
        c2.point.setValues([[0, bl, 0], [0, 0, 0],
                            [bl, 0, 0], [-0.05*bl, 0.95*bl, 0],
                            [0, bl, 0], [0.05*bl, 0.95*bl, 0],
                            [0.95*bl, 0.05*bl, 0], [bl, 0, 0],
                            [0.95*bl, -0.05*bl, 0]])
        # fmt: on
        l = coin.SoLineSet()
        l.numVertices.setValues([3, 3, 3])
        s = coin.SoSeparator()
        s.addChild(pick)
        s.addChild(self.trans)
        s.addChild(m1)
        s.addChild(c1)
        s.addChild(f)
        s.addChild(m2)
        s.addChild(c2)
        s.addChild(l)
        super().__init__(children=[s], name="planeTracker")

    def set(self, pos=None):
        """Set the translation to the position."""
        plm = self._get_wp().get_placement()
        Q = plm.Rotation.Q
        if pos is None:
            pos = plm.Base
        self.trans.translation.setValue([pos.x, pos.y, pos.z])
        self.trans.rotation.setValue([Q[0], Q[1], Q[2], Q[3]])
        self.on()


class wireTracker(Tracker):
    """A wire tracker."""

    def __init__(self, wire):
        self.line = coin.SoLineSet()
        self.closed = DraftGeomUtils.isReallyClosed(wire)
        if self.closed:
            self.line.numVertices.setValue(len(wire.Vertexes) + 1)
        else:
            self.line.numVertices.setValue(len(wire.Vertexes))
        self.coords = coin.SoCoordinate3()
        self.update(wire)
        super().__init__(children=[self.coords, self.line], name="wireTracker")

    def update(self, wire, forceclosed=False):
        """Update the tracker."""
        if wire:
            if self.closed or forceclosed:
                self.line.numVertices.setValue(len(wire.Vertexes) + 1)
            else:
                self.line.numVertices.setValue(len(wire.Vertexes))
            for i in range(len(wire.Vertexes)):
                p = wire.Vertexes[i].Point
                self.coords.point.set1Value(i, [p.x, p.y, p.z])
            if self.closed or forceclosed:
                t = len(wire.Vertexes)
                p = wire.Vertexes[0].Point
                self.coords.point.set1Value(t, [p.x, p.y, p.z])

    def updateFromPointlist(self, points, forceclosed=False):
        """Update the tracker from points."""
        if points:
            for i in range(len(points)):
                p = points[i]
                self.coords.point.set1Value(i, [p.x, p.y, p.z])


class gridTracker(Tracker):
    """A grid tracker."""

    def __init__(self):

        col, red, green, blue, gtrans = self.getGridColors()
        pick = coin.SoPickStyle()
        pick.style.setValue(coin.SoPickStyle.UNPICKABLE)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])

        # small squares
        self.mat1 = coin.SoMaterial()
        self.mat1.transparency.setValue(0.8 * (1 - gtrans))
        self.mat1.diffuseColor.setValue(col)
        self.font = coin.SoFont()
        self.coords1 = coin.SoCoordinate3()
        self.lines1 = coin.SoLineSet()  # small squares

        # texts
        texts = coin.SoSeparator()
        t1 = coin.SoSeparator()
        self.textpos1 = coin.SoTransform()
        self.text1 = coin.SoAsciiText()
        self.text1.string = " "
        t2 = coin.SoSeparator()
        self.textpos2 = coin.SoTransform()
        self.textpos2.rotation.setValue((0.0, 0.0, 0.7071067811865475, 0.7071067811865476))
        self.text2 = coin.SoAsciiText()
        self.text2.string = " "
        t1.addChild(self.textpos1)
        t1.addChild(self.text1)
        t2.addChild(self.textpos2)
        t2.addChild(self.text2)
        texts.addChild(self.font)
        texts.addChild(t1)
        texts.addChild(t2)

        # big squares
        self.mat2 = coin.SoMaterial()
        self.mat2.transparency.setValue(0.2 * (1 - gtrans))
        self.mat2.diffuseColor.setValue(col)
        self.coords2 = coin.SoCoordinate3()
        self.lines2 = coin.SoLineSet()  # big squares

        # human figure
        mat_human = coin.SoMaterial()
        mat_human.transparency.setValue(0.3 * (1 - gtrans))
        mat_human.diffuseColor.setValue(col)
        self.coords_human = coin.SoCoordinate3()
        self.human = coin.SoLineSet()

        # axes
        self.mat3 = coin.SoMaterial()
        self.mat3.transparency.setValue(gtrans)
        self.mat3.diffuseColor.setValues([col, red, green, blue])
        self.coords3 = coin.SoCoordinate3()
        self.lines3 = coin.SoIndexedLineSet()  # axes
        self.lines3.coordIndex.setValues(0, 5, [0, 1, -1, 2, 3])
        self.lines3.materialIndex.setValues(0, 2, [0, 0])
        mbind3 = coin.SoMaterialBinding()
        mbind3.value = coin.SoMaterialBinding.PER_PART_INDEXED

        self.pts = []
        s = coin.SoType.fromName("SoSkipBoundingGroup").createInstance()
        s.addChild(pick)
        s.addChild(self.trans)
        s.addChild(self.mat1)
        s.addChild(self.coords1)
        s.addChild(self.lines1)
        s.addChild(self.mat2)
        s.addChild(self.coords2)
        s.addChild(self.lines2)
        s.addChild(mat_human)
        s.addChild(self.coords_human)
        s.addChild(self.human)
        s.addChild(mbind3)
        s.addChild(self.mat3)
        s.addChild(self.coords3)
        s.addChild(self.lines3)
        s.addChild(texts)

        super().__init__(children=[s], name="gridTracker")
        self.show_during_command = False
        self.show_always = False
        self.reset()

    def update(self):
        """Redraw the grid."""
        # Resize the grid to make sure it fits
        # an exact pair number of main lines
        if self.space == 0:
            self.lines1.numVertices.deleteValues(0)
            self.lines2.numVertices.deleteValues(0)
            self.pts = []
            FreeCAD.Console.PrintWarning("Draft Grid: Spacing value is zero\n")
            return
        if self.mainlines == 0:
            self.lines1.numVertices.deleteValues(0)
            self.lines2.numVertices.deleteValues(0)
            self.pts = []
            return
        if self.numlines == 0:
            self.lines1.numVertices.deleteValues(0)
            self.lines2.numVertices.deleteValues(0)
            self.pts = []
            return
        numlines = self.numlines // self.mainlines // 2 * 2 * self.mainlines
        bound = (numlines // 2) * self.space
        border = (numlines // 2 + self.mainlines / 2) * self.space
        cursor = self.mainlines // 4 * self.space
        pts = []
        mpts = []
        apts = []
        cpts = []
        for i in range(numlines + 1):
            curr = -bound + i * self.space
            z = 0
            if i / float(self.mainlines) == i // self.mainlines:
                if round(curr, 4) == 0:
                    apts.extend([[-bound, curr, z], [bound, curr, z]])
                    apts.extend([[curr, -bound, z], [curr, bound, z]])
                else:
                    mpts.extend([[-bound, curr, z], [bound, curr, z]])
                    mpts.extend([[curr, -bound, z], [curr, bound, z]])
                cpts.extend([[-border, curr, z], [-border + cursor, curr, z]])
                cpts.extend([[border - cursor, curr, z], [border, curr, z]])
                cpts.extend([[curr, -border, z], [curr, -border + cursor, z]])
                cpts.extend([[curr, border - cursor, z], [curr, border, z]])
            else:
                pts.extend([[-bound, curr, z], [bound, curr, z]])
                pts.extend([[curr, -bound, z], [curr, bound, z]])
        if pts != self.pts:
            idx = []
            midx = []
            # aidx = []
            cidx = []
            for p in range(0, len(pts), 2):
                idx.append(2)
            for mp in range(0, len(mpts), 2):
                midx.append(2)
            # for ap in range(0, len(apts), 2):
            #    aidx.append(2)
            for cp in range(0, len(cpts), 2):
                cidx.append(2)

            if params.get_param("gridBorder"):
                # extra border
                border = (numlines // 2 + self.mainlines / 2) * self.space
                mpts.extend(
                    [
                        [-border, -border, z],
                        [border, -border, z],
                        [border, border, z],
                        [-border, border, z],
                        [-border, -border, z],
                    ]
                )
                midx.append(5)
                # cursors
                mpts.extend(cpts)
                midx.extend(cidx)
                # texts
                self.font.size = self.space * (self.mainlines // 4) or 1
                self.font.name = params.get_param("textfont")
                txt = FreeCAD.Units.Quantity(
                    self.space * self.mainlines, FreeCAD.Units.Length
                ).UserString
                self.text1.string = txt
                self.text2.string = txt
                self.textpos1.translation.setValue((-bound + self.space, -border + self.space, z))
                self.textpos2.translation.setValue((-bound - self.space, -bound + self.space, z))
            else:
                self.text1.string = " "
                self.text2.string = " "

            self.lines1.numVertices.deleteValues(0)
            self.lines2.numVertices.deleteValues(0)
            # self.lines3.numVertices.deleteValues(0)
            self.coords1.point.setValues(pts)
            self.lines1.numVertices.setValues(idx)
            self.coords2.point.setValues(mpts)
            self.lines2.numVertices.setValues(midx)
            self.coords3.point.setValues(apts)
            # self.lines3.numVertices.setValues(aidx)
            self.pts = pts

        # update the grid colors
        col, red, green, blue, gtrans = self.getGridColors()
        self.mat1.diffuseColor.setValue(col)
        self.mat2.diffuseColor.setValue(col)
        self.mat3.diffuseColor.setValues([col, red, green, blue])

    def getGridColors(self):
        """Returns grid colors stored in the preferences"""
        gtrans = params.get_param("gridTransparency") / 100.0
        col = utils.get_rgba_tuple(params.get_param("gridColor"))[:3]
        if params.get_param("coloredGridAxes"):
            vp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
            red = vp.GetUnsigned("AxisXColor", 0xCC333300)
            green = vp.GetUnsigned("AxisYColor", 0x33CC3300)
            blue = vp.GetUnsigned("AxisZColor", 0x3333CC00)
            red = utils.get_rgba_tuple(red)[:3]
            green = utils.get_rgba_tuple(green)[:3]
            blue = utils.get_rgba_tuple(blue)[:3]
        else:
            red = col
            green = col
            blue = col
        return col, red, green, blue, gtrans

    def get_human_figure(self, loc=None):
        """Return a list of points defining a human figure,
        optionally translated to a given location.
        Based on "HumanFigure.brep" from the BIM Workbench.
        """
        pts = [
            Vector(131.2, 0.0, 175.8),
            Vector(135.7, 0.0, 211.7),
            Vector(142.1, 0.0, 229.3),
            Vector(154.0, 0.0, 276.3),
            Vector(163.6, 0.0, 333.4),
            Vector(173.8, 0.0, 411.5),
            Vector(186.0, 0.0, 491.7),
            Vector(200.2, 0.0, 591.8),
            Vector(215.4, 0.0, 714.8),
            Vector(224.1, 0.0, 650.0),
            Vector(234.1, 0.0, 575.8),
            Vector(251.9, 0.0, 425.5),
            Vector(240.8, 0.0, 273.3),
            Vector(235.9, 0.0, 243.2),
            Vector(197.3, 0.0, 81.7),
            Vector(188.8, 0.0, 37.6),
            Vector(195.1, 0.0, 0.0),
            Vector(278.8, 0.0, 0.0),
            Vector(294.5, 0.0, 121.0),
            Vector(308.4, 0.0, 184.3),
            Vector(318.1, 0.0, 225.4),
            Vector(322.0, 0.0, 235.2),
            Vector(340.1, 0.0, 283.5),
            Vector(349.0, 0.0, 341.4),
            Vector(354.2, 0.0, 523.7),
            Vector(383.1, 0.0, 719.7),
            Vector(399.6, 0.0, 820.3),
            Vector(381.2, 0.0, 1029.6),
            Vector(385.6, 0.0, 1164.0),
            Vector(390.1, 0.0, 1184.6),
            Vector(398.3, 0.0, 1201.9),
            Vector(430.2, 0.0, 1273.1),
            Vector(438.2, 0.0, 1321.2),
            Vector(441.3, 0.0, 1368.2),
            Vector(428.1, 0.0, 1405.3),
            Vector(395.0, 0.0, 1424.3),
            Vector(365.5, 0.0, 1446.6),
            Vector(349.8, 0.0, 1460.2),
            Vector(345.6, 0.0, 1476.7),
            Vector(337.9, 0.0, 1570.3),
            Vector(332.8, 0.0, 1624.7),
            Vector(322.2, 0.0, 1666.9),
            Vector(272.6, 0.0, 1697.4),
            Vector(216.2, 0.0, 1680.2),
            Vector(178.1, 0.0, 1625.8),
            Vector(163.5, 0.0, 1559.1),
            Vector(179.8, 0.0, 1475.5),
            Vector(195.6, 0.0, 1459.9),
            Vector(193.1, 0.0, 1447.8),
            Vector(171.0, 0.0, 1429.4),
            Vector(132.3, 0.0, 1399.8),
            Vector(96.8, 0.0, 1374.7),
            Vector(74.0, 0.0, 1275.1),
            Vector(46.2, 0.0, 1095.6),
            Vector(38.0, 0.0, 1041.5),
            Vector(25.2, 0.0, 958.5),
            Vector(15.9, 0.0, 889.7),
            Vector(2.5, 0.0, 842.1),
            Vector(-5.8, 0.0, 785.4),
            Vector(15.8, 0.0, 755.1),
            Vector(38.0, 0.0, 750.4),
            Vector(45.0, 0.0, 755.4),
            Vector(53.1, 0.0, 736.1),
            Vector(76.4, 0.0, 572.2),
            Vector(83.6, 0.0, 512.0),
            Vector(81.3, 0.0, 499.0),
            Vector(74.0, 0.0, 460.9),
            Vector(61.9, 0.0, 363.4),
            Vector(53.4, 0.0, 269.3),
            Vector(44.5, 0.0, 179.7),
            Vector(25.6, 0.0, 121.6),
            Vector(0.6, 0.0, 76.5),
            Vector(-23.7, 0.0, 54.5),
            Vector(-43.8, 0.0, 33.5),
            Vector(-25.6, 0.0, 0.0),
            Vector(117.9, 0.0, 0.0),
            Vector(131.2, 0.0, 175.8),
        ]
        if loc:
            pts = [p.add(loc) for p in pts]
        return pts

    def displayHumanFigure(self, wp):
        """Display the human figure at the grid corner.
        The silhouette is displayed only if:
        - preference BaseApp/Preferences/Mod/Draft/gridBorder is True;
        - preference BaseApp/Preferences/Mod/Draft/gridShowHuman is True;
        - the working plane normal is vertical.
        """
        numlines = self.numlines // self.mainlines // 2 * 2 * self.mainlines
        bound = (numlines // 2) * self.space
        pts = []
        pidx = []
        if (
            params.get_param("gridBorder")
            and params.get_param("gridShowHuman")
            and wp.axis.getAngle(FreeCAD.Vector(0, 0, 1)) < 0.001
        ):
            loc = FreeCAD.Vector(-bound + self.space / 2, -bound + self.space / 2, 0)
            hpts = self.get_human_figure(loc)
            pts.extend([tuple(p) for p in hpts])
            pidx.append(len(hpts))
        self.human.numVertices.deleteValues(0)
        self.coords_human.point.setValues(pts)
        self.human.numVertices.setValues(pidx)

    def setAxesColor(self, wp):
        """set axes color"""
        cols = [0, 0]
        if params.get_param("coloredGridAxes"):
            if round(wp.u.getAngle(FreeCAD.Vector(1, 0, 0)), 2) in (0, 3.14):
                cols[0] = 1
            elif round(wp.u.getAngle(FreeCAD.Vector(0, 1, 0)), 2) in (0, 3.14):
                cols[0] = 2
            elif round(wp.u.getAngle(FreeCAD.Vector(0, 0, 1)), 2) in (0, 3.14):
                cols[0] = 3
            if round(wp.v.getAngle(FreeCAD.Vector(1, 0, 0)), 2) in (0, 3.14):
                cols[1] = 1
            elif round(wp.v.getAngle(FreeCAD.Vector(0, 1, 0)), 2) in (0, 3.14):
                cols[1] = 2
            elif round(wp.v.getAngle(FreeCAD.Vector(0, 0, 1)), 2) in (0, 3.14):
                cols[1] = 3
        self.lines3.materialIndex.setValues(0, 2, cols)

    def setSize(self, size):
        """Set size of the lines and update."""
        self.numlines = size
        self.update()

    def setSpacing(self, space):
        """Set spacing and update."""
        self.space = space
        self.update()

    def setMainlines(self, ml):
        """Set mainlines and update."""
        self.mainlines = ml
        self.update()

    def reset(self):
        """Reset the grid according to preferences settings."""
        try:
            self.space = FreeCAD.Units.Quantity(params.get_param("gridSpacing")).Value
        except ValueError:
            self.space = 1
        self.mainlines = params.get_param("gridEvery")
        self.numlines = params.get_param("gridSize")
        self.update()

    def set(self):
        """Move and rotate the grid according to the current working plane."""
        self.reset()
        wp = self._get_wp()
        Q = wp.get_placement().Rotation.Q
        P = wp.position
        self.trans.rotation.setValue([Q[0], Q[1], Q[2], Q[3]])
        self.trans.translation.setValue([P.x, P.y, P.z])
        self.displayHumanFigure(wp)
        self.setAxesColor(wp)
        self.on()

    def on(self):
        """Set the visibility to True and update the checked state of the grid button."""
        super().on()
        grid_observer._update_grid_gui()

    def off(self):
        """Set the visibility to False and update the checked state of the grid button."""
        super().off()
        grid_observer._update_grid_gui()

    def getClosestNode(self, point):
        """Return the closest node from the given point."""
        wp = self._get_wp()
        pt = wp.get_local_coords(point)
        pu = round(pt.x / self.space, 0) * self.space
        pv = round(pt.y / self.space, 0) * self.space
        pt = wp.get_global_coords(Vector(pu, pv, 0))
        return pt


class boxTracker(Tracker):
    """A box tracker, can be based on a line object."""

    def __init__(self, line=None, width=0.1, height=1, shaded=False):
        self.trans = coin.SoTransform()
        m = coin.SoMaterial()
        m.transparency.setValue(0.8)
        m.diffuseColor.setValue([0.4, 0.4, 0.6])
        w = coin.SoDrawStyle()
        w.style = coin.SoDrawStyle.LINES
        self.cube = coin.SoCube()
        self.cube.height.setValue(width)
        self.cube.depth.setValue(height)
        self.baseline = None
        if line:
            self.baseline = line
            self.update()
        if shaded:
            super().__init__(children=[self.trans, m, self.cube], name="boxTracker")
        else:
            super().__init__(children=[self.trans, w, self.cube], name="boxTracker")

    def update(self, line=None, normal=None):
        """Update the tracker."""
        import DraftGeomUtils

        if not normal:
            normal = self._get_wp().axis
        if line:
            if isinstance(line, list):
                bp = line[0]
                lvec = line[1].sub(line[0])
            else:
                lvec = DraftGeomUtils.vec(line.Shape.Edges[0])
                bp = line.Shape.Edges[0].Vertexes[0].Point
        elif self.baseline:
            lvec = DraftGeomUtils.vec(self.baseline.Shape.Edges[0])
            bp = self.baseline.Shape.Edges[0].Vertexes[0].Point
        else:
            return
        self.cube.width.setValue(lvec.Length)
        bp = bp.add(lvec.multiply(0.5))
        bp = bp.add(DraftVecUtils.scaleTo(normal, self.cube.depth.getValue() / 2.0))
        self.pos(bp)
        tol = 1e-6
        if lvec.Length > tol and normal.Length > tol:
            lvec.normalize()
            normal.normalize()
            if not lvec.isEqual(normal, tol) and not lvec.isEqual(normal.negative(), tol):
                rot = FreeCAD.Rotation(lvec, FreeCAD.Vector(), normal, "XZY")
                self.trans.rotation.setValue(rot.Q)

    def setRotation(self, rot):
        """Set the rotation."""
        self.trans.rotation.setValue(rot.Q)

    def pos(self, p):
        """Set the translation."""
        self.trans.translation.setValue(DraftVecUtils.tup(p))

    def width(self, w=None):
        """Set the width."""
        if w:
            self.cube.height.setValue(w)
        else:
            return self.cube.height.getValue()

    def length(self, l=None):
        """Set the length."""
        if l:
            self.cube.width.setValue(l)
        else:
            return self.cube.width.getValue()

    def height(self, h=None):
        """Set the height."""
        if h:
            self.cube.depth.setValue(h)
            self.update()
        else:
            return self.cube.depth.getValue()


class radiusTracker(Tracker):
    """A tracker that displays a transparent sphere to indicate a radius."""

    def __init__(self, position=FreeCAD.Vector(0, 0, 0), radius=1):
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([position.x, position.y, position.z])
        m = coin.SoMaterial()
        m.transparency.setValue(0.9)
        m.diffuseColor.setValue([0, 1, 0])
        self.sphere = coin.SoSphere()
        self.sphere.radius.setValue(radius)
        self.baseline = None
        super().__init__(children=[self.trans, m, self.sphere], name="radiusTracker")

    def update(self, arg1, arg2=None):
        """Update the tracker."""
        if isinstance(arg1, FreeCAD.Vector):
            self.trans.translation.setValue([arg1.x, arg1.y, arg1.z])
        else:
            self.sphere.radius.setValue(arg1)
        if arg2 is not None:
            if isinstance(arg2, FreeCAD.Vector):
                self.trans.translation.setValue([arg2.x, arg2.y, arg2.z])
            else:
                self.sphere.radius.setValue(arg2)


class archDimTracker(Tracker):
    """A wrapper around a Sketcher dim."""

    def __init__(self, p1=FreeCAD.Vector(0, 0, 0), p2=FreeCAD.Vector(1, 0, 0), mode=1):
        import SketcherGui

        self.transform = coin.SoMatrixTransform()
        self.dimnode = coin.SoType.fromName("SoDatumLabel").createInstance()
        p1node = coin.SbVec3f([p1.x, p1.y, p1.z])
        p2node = coin.SbVec3f([p2.x, p2.y, p2.z])
        self.dimnode.pnts.setValues([p1node, p2node])
        self.dimnode.lineWidth = 1
        self.setColor()
        self.setSize()
        self.offset = 0.5
        self.mode = mode
        self.matrix = self.transform.matrix
        self.norm = self.dimnode.norm
        self.param1 = self.dimnode.param1
        self.param2 = self.dimnode.param2
        self.pnts = self.dimnode.pnts
        self.string = self.dimnode.string
        self.view = Draft.get3DView()
        self.camera = self.view.getCameraNode()
        self.setMode(mode)
        self.setString()
        super().__init__(children=[self.transform, self.dimnode], name="archDimTracker")

    def setColor(self, color=None):
        """Set the color."""
        if color is None:
            self.color = utils.get_rgba_tuple(params.get_param("snapcolor"))[:3]
        else:
            self.color.rgb = color
        self.dimnode.textColor.setValue(coin.SbVec3f(self.color))

    def setSize(self):
        """Set the text size."""
        self.dimnode.size = params.get_param_view("MarkerSize") * 2
        self.size_pixel = self.dimnode.size.getValue() * 96 / 72

    def setString(self, text=None):
        """Set the dim string to the given value or auto value."""
        plane = self._get_wp()
        p1 = Vector(self.pnts.getValues()[0].getValue())
        p2 = Vector(self.pnts.getValues()[-1].getValue())
        self.norm.setValue(plane.axis)
        # set the offset sign to prevent the dim line from intersecting the curve near the cursor
        sign_dx = math.copysign(1, (p2.sub(p1)).x)
        sign_dy = math.copysign(1, (p2.sub(p1)).y)
        sign = sign_dx * sign_dy
        if self.mode == 2:
            self.Distance = abs((p2.sub(p1)).x)
            self.param1.setValue(sign * self.offset)
        elif self.mode == 3:
            self.Distance = abs((p2.sub(p1)).y)
            self.param1.setValue(-1 * sign * self.offset)
        else:
            self.Distance = (p2.sub(p1)).Length

        text = FreeCAD.Units.Quantity(self.Distance, FreeCAD.Units.Length).UserString
        self.matrix.setValue(*plane.get_placement().Matrix.transposed().A)
        self.string.setValue(text.encode("utf8"))
        # change the text position to external depending on the distance and scale values
        volume = self.camera.getViewVolume()
        scale = self.view.getSize()[1] / volume.getHeight()
        if scale * self.Distance > self.size_pixel * len(text):
            self.param2.setValue(0)
        else:
            self.param2.setValue(
                1 / 2 * self.Distance + 3 / 5 * self.size_pixel * len(text) / scale
            )

    def setMode(self, mode=1):
        """Set the mode.

        0 = without lines (a simple mark)
        1 = aligned (default)
        2 = horizontal
        3 = vertical.
        """
        self.dimnode.datumtype.setValue(mode)

    def p1(self, point=None):
        """Set or get the first point of the dim."""
        plane = self._get_wp()
        if point:
            p1_proj = plane.project_point(point)
            p1_proj_u = (p1_proj - plane.position).dot(plane.u.normalize())
            p1_proj_v = (p1_proj - plane.position).dot(plane.v.normalize())
            self.pnts.set1Value(0, p1_proj_u, p1_proj_v, 0)
            self.setString()
        else:
            return Vector(self.pnts.getValues()[0].getValue())

    def p2(self, point=None):
        """Set or get the second point of the dim."""
        plane = self._get_wp()
        if point:
            p2_proj = plane.project_point(point)
            p2_proj_u = (p2_proj - plane.position).dot(plane.u.normalize())
            p2_proj_v = (p2_proj - plane.position).dot(plane.v.normalize())
            self.pnts.set1Value(1, p2_proj_u, p2_proj_v, 0)
            self.setString()
        else:
            return Vector(self.pnts.getValues()[-1].getValue())


## @}
