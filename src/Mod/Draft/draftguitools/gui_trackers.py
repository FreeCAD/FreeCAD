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
"""Provide Coin based objects used for previews in the Draft Workbench.

This module provides Coin (pivy) based objects
that are used by the Draft Workbench to draw temporary geometry,
that is, previews, of the real objects that will be created on the 3D view.
"""
## @package DraftTrackers
#  \ingroup DRAFT
#  \brief Provide Coin based objects used for previews in the Draft Workbench.
#
# This module provides Coin (pivy) based objects
# that are used by the Draft Workbench to draw temporary geometry,
# that is, previews, of the real objects that will be created on the 3D view.

import math
from pivy import coin
import re

import FreeCAD
import FreeCADGui
import Draft
import DraftVecUtils
from FreeCAD import Vector
from draftutils.todo import ToDo
from draftutils.messages import _msg

__title__ = "FreeCAD Draft Trackers"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecadweb.org"


class Tracker:
    """A generic Draft Tracker, to be used by other specific trackers."""

    def __init__(self, dotted=False, scolor=None, swidth=None,
                 children=[], ontop=False, name=None):
        global Part, DraftGeomUtils
        import Part, DraftGeomUtils
        self.ontop = ontop
        color = coin.SoBaseColor()
        color.rgb = scolor or FreeCADGui.draftToolBar.getDefaultColor("ui")
        drawstyle = coin.SoDrawStyle()
        if swidth:
            drawstyle.lineWidth = swidth
        if dotted:
            drawstyle.style = coin.SoDrawStyle.LINES
            drawstyle.lineWeight = 3
            drawstyle.linePattern = 0x0f0f  # 0xaa
        node = coin.SoSeparator()
        for c in [drawstyle, color] + children:
            node.addChild(c)
        self.switch = coin.SoSwitch()  # this is the on/off switch
        if name:
            self.switch.setName(name)
        self.switch.addChild(node)
        self.switch.whichChild = -1
        self.Visible = False
        ToDo.delay(self._insertSwitch, self.switch)

    def finalize(self):
        """Finish the command by removing the switch."""
        ToDo.delay(self._removeSwitch, self.switch)
        self.switch = None

    def _insertSwitch(self, switch):
        """Insert self.switch into the scene graph.

        Must not be called
        from an event handler (or other scene graph traversal).
        """
        sg = Draft.get3DView().getSceneGraph()
        if self.ontop:
            sg.insertChild(switch, 0)
        else:
            sg.addChild(switch)

    def _removeSwitch(self, switch):
        """Remove self.switch from the scene graph.

        As with _insertSwitch,
        must not be called during scene graph traversal).
        """
        sg = Draft.get3DView().getSceneGraph()
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
            sg = Draft.get3DView().getSceneGraph()
            sg.removeChild(self.switch)
            sg.addChild(self.switch)

    def raiseTracker(self):
        """Raise the tracker to the top of the scenegraph.

        So it obscures the other objects.
        """
        if self.switch:
            sg = Draft.get3DView().getSceneGraph()
            sg.removeChild(self.switch)
            sg.insertChild(self.switch, 0)


class snapTracker(Tracker):
    """Define Snap Mark tracker, used by tools that support snapping."""

    def __init__(self):
        color = coin.SoBaseColor()
        color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.marker = coin.SoMarkerSet()  # this is the marker symbol
        self.marker.markerIndex = FreeCADGui.getMarkerIndex("", 9)
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        self.coords.point.setValue((0, 0, 0))
        node = coin.SoAnnotation()
        node.addChild(self.coords)
        node.addChild(color)
        node.addChild(self.marker)
        Tracker.__init__(self, children=[node], name="snapTracker")

    def setMarker(self, style):
        """Set the marker index."""
        self.marker.markerIndex = FreeCADGui.getMarkerIndex(style, 9)

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
        Tracker.__init__(self, dotted, scolor, swidth,
                         [self.coords, line],
                         ontop, name="lineTracker")

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


class rectangleTracker(Tracker):
    """A Rectangle tracker, used by the rectangle tool."""

    def __init__(self, dotted=False, scolor=None, swidth=None, face=False):
        self.origin = Vector(0, 0, 0)
        line = coin.SoLineSet()
        line.numVertices.setValue(5)
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        self.coords.point.setValues(0, 50, [[0, 0, 0],
                                            [2, 0, 0],
                                            [2, 2, 0],
                                            [0, 2, 0],
                                            [0, 0, 0]])
        if face:
            m1 = coin.SoMaterial()
            m1.transparency.setValue(0.5)
            m1.diffuseColor.setValue([0.5, 0.5, 1.0])
            f = coin.SoIndexedFaceSet()
            f.coordIndex.setValues([0, 1, 2, 3])
            Tracker.__init__(self, dotted, scolor, swidth,
                             [self.coords, line, m1, f],
                             name="rectangleTracker")
        else:
            Tracker.__init__(self, dotted, scolor, swidth,
                             [self.coords, line],
                             name="rectangleTracker")
        self.u = FreeCAD.DraftWorkingPlane.u
        self.v = FreeCAD.DraftWorkingPlane.v

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
            norm = FreeCAD.DraftWorkingPlane.u.cross(FreeCAD.DraftWorkingPlane.v)
            self.v = self.u.cross(norm)

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
        return ((DraftVecUtils.project(diag, self.u)).Length,
                (DraftVecUtils.project(diag, self.v)).Length)

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
        self.coords.point.setValues(0, 4,
                                    [[0, 0, 0],
                                     [0, 0, 0],
                                     [0, 0, 0],
                                     [0, 0, 0]])
        Tracker.__init__(self, dotted, scolor, swidth,
                         [self.coords, line], name="dimTracker")
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
            points = [DraftVecUtils.tup(self.p1, True),
                      DraftVecUtils.tup(self.p2, True),
                      DraftVecUtils.tup(self.p1, True),
                      DraftVecUtils.tup(self.p2, True)]
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
                points = [DraftVecUtils.tup(p1),
                          DraftVecUtils.tup(p2),
                          DraftVecUtils.tup(p3),
                          DraftVecUtils.tup(p4)]
            self.coords.point.setValues(0, 4, points)


class bsplineTracker(Tracker):
    """A bspline tracker."""

    def __init__(self, dotted=False, scolor=None, swidth=None, points=[]):
        self.bspline = None
        self.points = points
        self.trans = coin.SoTransform()
        self.sep = coin.SoSeparator()
        self.recompute()
        Tracker.__init__(self, dotted, scolor, swidth,
                         [self.trans, self.sep], name="bsplineTracker")

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
            c =  Part.BSplineCurve()
            # DNC: allows to close the curve by placing ends close to each other
            if len(self.points) >= 3 and ( (self.points[0] - self.points[-1]).Length < Draft.tolerance() ):
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
                pts = re.findall("point \[(.*?)\]", buf)[0]
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
                    FreeCAD.Console.PrintWarning("bsplineTracker.recompute() failed to read-in Inventor string\n")


class bezcurveTracker(Tracker):
    """A bezcurve tracker."""

    def __init__(self, dotted=False, scolor=None, swidth=None, points=[]):
        self.bezcurve = None
        self.points = points
        self.degree = None
        self.trans = coin.SoTransform()
        self.sep = coin.SoSeparator()
        self.recompute()
        Tracker.__init__(self, dotted, scolor, swidth,
                         [self.trans, self.sep], name="bezcurveTracker")

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

        if (len(self.points) >= 2):
            if self.degree:
                poles = self.points[1:]
                segpoleslst = [poles[x:x+self.degree] for x in range(0, len(poles), (self.degree or 1))]
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
                    buf = buf.replace("\n","")
                    pts = re.findall("point \[(.*?)\]", buf)[0]
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
                        FreeCAD.Console.PrintWarning("bezcurveTracker.recompute() failed to read-in Inventor string\n")
                self.bezcurve.append(bezcurveseg)


class arcTracker(Tracker):
    """An arc tracker."""

    def __init__(self, dotted=False, scolor=None, swidth=None,
                 start=0, end=math.pi*2, normal=None):
        self.circle = None
        self.startangle = math.degrees(start)
        self.endangle = math.degrees(end)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        self.sep = coin.SoSeparator()
        self.autoinvert = True
        if normal:
            self.normal = normal
        else:
            self.normal = FreeCAD.DraftWorkingPlane.axis
        self.basevector = self.getDeviation()
        self.recompute()
        Tracker.__init__(self, dotted, scolor, swidth,
                         [self.trans, self.sep], name="arcTracker")

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
        rad = pt.sub(center)
        a = DraftVecUtils.angle(rad, self.basevector, self.normal)
        # print(a)
        return a

    def getAngles(self):
        """Return the start and end angles in degrees."""
        return(self.startangle, self.endangle)

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
        self.basevector = self.getDeviation()
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
            c = Part.makeCircle(1, Vector(0, 0, 0),
                                self.normal, self.endangle, self.startangle)
        else:
            c = Part.makeCircle(1, Vector(0, 0, 0),
                                self.normal, self.startangle, self.endangle)
        buf = c.writeInventor(2, 0.01)
        try:
            ivin = coin.SoInput()
            ivin.setBuffer(buf)
            ivob = coin.SoDB.readAll(ivin)
        except Exception:
            # workaround for pivy SoInput.setBuffer() bug
            buf = buf.replace("\n", "")
            pts = re.findall("point \[(.*?)\]", buf)[0]
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
                FreeCAD.Console.PrintWarning("arcTracker.recompute() failed to read-in Inventor string\n")


class ghostTracker(Tracker):
    """A Ghost tracker, that allows to copy whole object representations.

    You can pass it an object or a list of objects, or a shape.
    """

    def __init__(self, sel, dotted=False, scolor=None, swidth=None):
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        self.children = [self.trans]
        rootsep = coin.SoSeparator()
        if not isinstance(sel, list):
            sel = [sel]
        for obj in sel:
            import Part
            if not isinstance(obj, Part.Vertex):
                rootsep.addChild(self.getNode(obj))
            else:
                self.coords = coin.SoCoordinate3()
                self.coords.point.setValue((obj.X, obj.Y, obj.Z))
                color = coin.SoBaseColor()
                color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
                self.marker = coin.SoMarkerSet()  # this is the marker symbol
                self.marker.markerIndex = FreeCADGui.getMarkerIndex("quad", 9)
                node = coin.SoAnnotation()
                selnode = coin.SoSeparator()
                selnode.addChild(self.coords)
                selnode.addChild(color)
                selnode.addChild(self.marker)
                node.addChild(selnode)
                rootsep.addChild(node)
        self.children.append(rootsep)
        Tracker.__init__(self, dotted, scolor, swidth,
                         children=self.children, name="ghostTracker")

    def update(self, obj):
        """Recreate the ghost from a new object."""
        obj.ViewObject.show()
        self.finalize()
        sep = self.getNode(obj)
        Tracker.__init__(self, children=[self.sep])
        self.on()
        obj.ViewObject.hide()

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

    def getNode(self, obj):
        """Return a coin node representing the given object."""
        import Part
        if isinstance(obj, Part.Shape):
            return self.getNodeLight(obj)
        elif obj.isDerivedFrom("Part::Feature"):
            return self.getNodeFull(obj)
        else:
            return self.getNodeFull(obj)

    def getNodeFull(self, obj):
        """Get a coin node which is a copy of the current representation."""
        sep = coin.SoSeparator()
        try:
            sep.addChild(obj.ViewObject.RootNode.copy())
            # add Part container offset
            if hasattr(obj, "getGlobalPlacement"):
                if obj.Placement != obj.getGlobalPlacement():
                    if sep.getChild(0).getNumChildren() > 0:
                        if isinstance(sep.getChild(0).getChild(0),coin.SoTransform):
                            gpl = obj.getGlobalPlacement()
                            sep.getChild(0).getChild(0).translation.setValue(tuple(gpl.Base))
                            sep.getChild(0).getChild(0).rotation.setValue(gpl.Rotation.Q)
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
        r = FreeCADGui.ActiveDocument.ActiveView.getViewer().getSoRenderManager().getViewportRegion()
        v = coin.SoGetMatrixAction(r)
        m = self.trans.getMatrix(v)
        if m:
            m = m.getValue()
            return FreeCAD.Matrix(m[0][0], m[0][1], m[0][2], m[0][3],
                                  m[1][0], m[1][1], m[1][2], m[1][3],
                                  m[2][0], m[2][1], m[2][2], m[2][3],
                                  m[3][0], m[3][1], m[3][2], m[3][3])
        else:
            return FreeCAD.Matrix()

    def setMatrix(self, matrix):
        """Set the transformation matrix."""
        m = coin.SbMatrix(matrix.A11, matrix.A12, matrix.A13, matrix.A14,
                          matrix.A21, matrix.A22, matrix.A23, matrix.A24,
                          matrix.A31, matrix.A32, matrix.A33, matrix.A34,
                          matrix.A41, matrix.A42, matrix.A43, matrix.A44)
        self.trans.setMatrix(m)


class editTracker(Tracker):
    """A node edit tracker."""

    def __init__(self, pos=Vector(0, 0, 0), name=None, idx=0, objcol=None,
                 marker=FreeCADGui.getMarkerIndex("quad", 9),
                 inactive=False):
        self.color = coin.SoBaseColor()
        if objcol:
            self.color.rgb = objcol[:3]
        else:
            self.color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.marker = coin.SoMarkerSet()  # this is the marker symbol
        self.marker.markerIndex = marker
        self.coords = coin.SoCoordinate3()  # this is the coordinate
        self.coords.point.setValue((pos.x, pos.y, pos.z))
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
        self.selnode.addChild(self.color)
        self.selnode.addChild(self.marker)
        node.addChild(self.selnode)
        ontop = not inactive
        Tracker.__init__(self, children=[node],
                         ontop=ontop, name="editTracker")
        self.on()

    def set(self, pos):
        """Set the point to the position."""
        self.coords.point.setValue((pos.x, pos.y, pos.z))

    def get(self):
        """Get a vector from the point."""
        p = self.coords.point.getValues()[0]
        return Vector(p[0], p[1], p[2])

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

    def setColor(self, color):
        """Set the color."""
        if color:
            self.color.rgb = color
        else:
            self.color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")


class PlaneTracker(Tracker):
    """A working plane tracker."""

    def __init__(self):
        # getting screen distance
        p1 = Draft.get3DView().getPoint((100, 100))
        p2 = Draft.get3DView().getPoint((110, 100))
        bl = (p2.sub(p1)).Length * (Draft.getParam("snapRange", 8)/2.0)
        pick = coin.SoPickStyle()
        pick.style.setValue(coin.SoPickStyle.UNPICKABLE)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        m1 = coin.SoMaterial()
        m1.transparency.setValue(0.8)
        m1.diffuseColor.setValue([0.4, 0.4, 0.6])
        c1 = coin.SoCoordinate3()
        c1.point.setValues([[-bl, -bl, 0],
                            [bl, -bl, 0],
                            [bl, bl, 0],
                            [-bl, bl, 0]])
        f = coin.SoIndexedFaceSet()
        f.coordIndex.setValues([0, 1, 2, 3])
        m2 = coin.SoMaterial()
        m2.transparency.setValue(0.7)
        m2.diffuseColor.setValue([0.2, 0.2, 0.3])
        c2 = coin.SoCoordinate3()
        c2.point.setValues([[0, bl, 0], [0, 0, 0],
                            [bl, 0, 0], [-0.05*bl, 0.95*bl, 0],
                            [0, bl, 0], [0.05*bl, 0.95*bl, 0],
                            [0.95*bl, 0.05*bl, 0], [bl, 0, 0],
                            [0.95*bl, -0.05*bl, 0]])
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
        Tracker.__init__(self, children=[s], name="planeTracker")

    def set(self, pos=None):
        """Set the translation to the position."""
        if pos:
            Q = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
        else:
            plm = FreeCAD.DraftWorkingPlane.getPlacement()
            Q = plm.Rotation.Q
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
            self.line.numVertices.setValue(len(wire.Vertexes)+1)
        else:
            self.line.numVertices.setValue(len(wire.Vertexes))
        self.coords = coin.SoCoordinate3()
        self.update(wire)
        Tracker.__init__(self, children=[self.coords, self.line],
                         name="wireTracker")

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
        col = self.getGridColor()
        pick = coin.SoPickStyle()
        pick.style.setValue(coin.SoPickStyle.UNPICKABLE)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0, 0, 0])
        mat1 = coin.SoMaterial()
        mat1.transparency.setValue(0.7)
        mat1.diffuseColor.setValue(col)
        self.coords1 = coin.SoCoordinate3()
        self.lines1 = coin.SoLineSet()
        mat2 = coin.SoMaterial()
        mat2.transparency.setValue(0.3)
        mat2.diffuseColor.setValue(col)
        self.coords2 = coin.SoCoordinate3()
        self.lines2 = coin.SoLineSet()
        mat3 = coin.SoMaterial()
        mat3.transparency.setValue(0)
        mat3.diffuseColor.setValue(col)
        self.coords3 = coin.SoCoordinate3()
        self.lines3 = coin.SoLineSet()
        self.pts = []
        s = coin.SoSeparator()
        s.addChild(pick)
        s.addChild(self.trans)
        s.addChild(mat1)
        s.addChild(self.coords1)
        s.addChild(self.lines1)
        s.addChild(mat2)
        s.addChild(self.coords2)
        s.addChild(self.lines2)
        s.addChild(mat3)
        s.addChild(self.coords3)
        s.addChild(self.lines3)
        Tracker.__init__(self, children=[s], name="gridTracker")
        self.reset()

    def getGridColor(self):
        """Get the grid color from the parameter editor."""
        color = Draft.getParam("gridColor", 842157055)
        r = ((color >> 24) & 0xFF) / 255
        g = ((color >> 16) & 0xFF) / 255
        b = ((color >> 8) & 0xFF) / 255
        return [r, g, b]

    def update(self):
        """Redraw the grid."""
        # Resize the grid to make sure it fits
        # an exact pair number of main lines
        numlines = self.numlines // self.mainlines // 2 * 2 * self.mainlines
        bound = (numlines // 2) * self.space
        pts = []
        mpts = []
        apts = []
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
            else:
                pts.extend([[-bound, curr, z], [bound, curr, z]])
                pts.extend([[curr, -bound, z], [curr, bound, z]])
        if pts != self.pts:
            idx = []
            midx = []
            aidx = []
            for p in range(0, len(pts), 2):
                idx.append(2)
            for mp in range(0, len(mpts), 2):
                midx.append(2)
            for ap in range(0, len(apts), 2):
                aidx.append(2)
            self.lines1.numVertices.deleteValues(0)
            self.lines2.numVertices.deleteValues(0)
            self.lines3.numVertices.deleteValues(0)
            self.coords1.point.setValues(pts)
            self.lines1.numVertices.setValues(idx)
            self.coords2.point.setValues(mpts)
            self.lines2.numVertices.setValues(midx)
            self.coords3.point.setValues(apts)
            self.lines3.numVertices.setValues(aidx)
            self.pts = pts

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
        self.space = Draft.getParam("gridSpacing", 1)
        self.mainlines = Draft.getParam("gridEvery", 10)
        self.numlines = Draft.getParam("gridSize", 100)
        self.update()

    def set(self):
        """Move and rotate the grid according to the current working plane."""
        self.reset()
        Q = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
        P = FreeCAD.DraftWorkingPlane.position
        self.trans.rotation.setValue([Q[0], Q[1], Q[2], Q[3]])
        self.trans.translation.setValue([P.x, P.y, P.z])
        self.on()

    def getClosestNode(self, point):
        """Return the closest node from the given point."""
        # get the 2D coords.
        # point = FreeCAD.DraftWorkingPlane.projectPoint(point)
        pt = FreeCAD.DraftWorkingPlane.getLocalCoords(point)
        pu = round(pt.x / self.space, 0) * self.space
        pv = round(pt.y / self.space, 0) * self.space
        pt = FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(pu, pv, 0))
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
            Tracker.__init__(self, children=[self.trans, m, self.cube],
                             name="boxTracker")
        else:
            Tracker.__init__(self, children=[self.trans, w, self.cube],
                             name="boxTracker")

    def update(self, line=None, normal=None):
        """Update the tracker."""
        import WorkingPlane, DraftGeomUtils
        if not normal:
            normal = FreeCAD.DraftWorkingPlane.axis
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
        right = lvec.cross(normal)
        self.cube.width.setValue(lvec.Length)
        p = WorkingPlane.getPlacementFromPoints([bp,
                                                 bp.add(lvec),
                                                 bp.add(right)])
        if p:
            self.trans.rotation.setValue(p.Rotation.Q)
        bp = bp.add(lvec.multiply(0.5))
        bp = bp.add(DraftVecUtils.scaleTo(normal, self.cube.depth.getValue()/2.0))
        self.pos(bp)

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
    """A tracker that displays a transparent sphere to inicate a radius."""

    def __init__(self, position=FreeCAD.Vector(0, 0, 0), radius=1):
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([position.x, position.y, position.z])
        m = coin.SoMaterial()
        m.transparency.setValue(0.9)
        m.diffuseColor.setValue([0, 1, 0])
        self.sphere = coin.SoSphere()
        self.sphere.radius.setValue(radius)
        self.baseline = None
        Tracker.__init__(self, children=[self.trans, m, self.sphere],
                         name="radiusTracker")

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

    def __init__(self,
                 p1=FreeCAD.Vector(0, 0, 0),
                 p2=FreeCAD.Vector(1, 0, 0), mode=1):
        import SketcherGui
        self.dimnode = coin.SoType.fromName("SoDatumLabel").createInstance()
        p1node = coin.SbVec3f([p1.x, p1.y, p1.z])
        p2node = coin.SbVec3f([p2.x, p2.y, p2.z])
        self.dimnode.pnts.setValues([p1node, p2node])
        self.dimnode.lineWidth = 1
        color = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.dimnode.textColor.setValue(coin.SbVec3f(color))
        self.setString()
        self.setMode(mode)
        Tracker.__init__(self, children=[self.dimnode], name="archDimTracker")

    def setString(self, text=None):
        """Set the dim string to the given value or auto value."""
        self.dimnode.param1.setValue(.5)
        p1 = Vector(self.dimnode.pnts.getValues()[0].getValue())
        p2 = Vector(self.dimnode.pnts.getValues()[-1].getValue())
        m = self.dimnode.datumtype.getValue()
        if m == 2:
            self.Distance = (DraftVecUtils.project(p2.sub(p1), Vector(1, 0, 0))).Length
        elif m == 3:
            self.Distance = (DraftVecUtils.project(p2.sub(p1), Vector(0, 1, 0))).Length
        else:
            self.Distance = (p2.sub(p1)).Length
        text = FreeCAD.Units.Quantity(self.Distance, FreeCAD.Units.Length).UserString
        self.dimnode.string.setValue(text.encode('utf8'))

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
        if point:
            self.dimnode.pnts.set1Value(0, point.x, point.y, point.z)
            self.setString()
        else:
            return Vector(self.dimnode.pnts.getValues()[0].getValue())

    def p2(self, point=None):
        """Set or get the second point of the dim."""
        if point:
            self.dimnode.pnts.set1Value(1, point.x, point.y, point.z)
            self.setString()
        else:
            return Vector(self.dimnode.pnts.getValues()[-1].getValue())
