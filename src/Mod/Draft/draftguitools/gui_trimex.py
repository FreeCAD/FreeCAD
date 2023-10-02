# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides GUI tools to trim and extend lines.

It also extends closed faces to create solids, that is, it can be used
to extrude a closed profile.

Make sure the snapping is active so that the extrusion is done following
the direction of a line, and up to the distance specified
by the snapping point.
"""
## @package gui_trimex
# \ingroup draftguitools
# \brief Provides GUI tools to trim and extend lines.

## \addtogroup draftguitools
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft
import Draft_rc
import DraftVecUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers

from draftutils.messages import _msg, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Trimex(gui_base_original.Modifier):
    """Gui Command for the Trimex tool.

    This tool trims or extends lines, wires and arcs,
    or extrudes single faces.

    SHIFT constrains to the last point
    or extrudes in direction to the face normal.
    """

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Trimex',
                'Accel': "T, R",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Trimex", "Trimex"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Trimex",
                    "Trims or extends the selected object, or extrudes single"
                    + " faces.\nCTRL snaps, SHIFT constrains to current segment"
                    + " or to normal, ALT inverts.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Trimex, self).Activated(name="Trimex")
        self.edges = []
        self.placement = None
        self.ghost = []
        self.linetrack = None
        self.color = None
        self.width = None
        if self.ui:
            if not Gui.Selection.getSelection():
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select objects to trim or extend"))
                self.call = \
                    self.view.addEventCallback("SoEvent",
                                               gui_tool_utils.selectObject)
            else:
                self.proceed()

    def proceed(self):
        """Proceed with execution of the command after proper selection."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        sel = Gui.Selection.getSelection()
        if len(sel) == 2:
            self.trimObjects(sel)
            self.finish()
            return
        self.obj = sel[0]
        self.ui.trimUi(title=translate("draft",self.featureName))
        self.linetrack = trackers.lineTracker()

        import DraftGeomUtils
        import Part

        if "Shape" not in self.obj.PropertiesList:
            return
        if "Placement" in self.obj.PropertiesList:
            self.placement = self.obj.Placement
        if len(self.obj.Shape.Faces) == 1:
            # simple extrude mode, the object itself is extruded
            self.extrudeMode = True
            self.ghost = [trackers.ghostTracker([self.obj])]
            self.normal = self.obj.Shape.Faces[0].normalAt(0.5, 0.5)
            self.ghost += [trackers.lineTracker() for _ in self.obj.Shape.Vertexes]
        elif len(self.obj.Shape.Faces) > 1:
            # face extrude mode, a new object is created
            ss = Gui.Selection.getSelectionEx()[0]
            if len(ss.SubObjects) == 1:
                if ss.SubObjects[0].ShapeType == "Face":
                    self.obj = self.doc.addObject("Part::Feature", "Face")
                    self.obj.Shape = ss.SubObjects[0]
                    self.extrudeMode = True
                    self.ghost = [trackers.ghostTracker([self.obj])]
                    self.normal = self.obj.Shape.Faces[0].normalAt(0.5, 0.5)
                    self.ghost += [trackers.lineTracker() for _ in self.obj.Shape.Vertexes]
        else:
            # normal wire trimex mode
            self.color = self.obj.ViewObject.LineColor
            self.width = self.obj.ViewObject.LineWidth
            # self.obj.ViewObject.Visibility = False
            self.obj.ViewObject.LineColor = (0.5, 0.5, 0.5)
            self.obj.ViewObject.LineWidth = 1
            self.extrudeMode = False
            if self.obj.Shape.Wires:
                self.edges = self.obj.Shape.Wires[0].Edges
                self.edges = Part.__sortEdges__(self.edges)
            else:
                self.edges = self.obj.Shape.Edges
            self.ghost = []
            lc = self.color
            sc = (lc[0], lc[1], lc[2])
            sw = self.width
            for e in self.edges:
                if DraftGeomUtils.geomType(e) == "Line":
                    self.ghost.append(trackers.lineTracker(scolor=sc,
                                                           swidth=sw))
                else:
                    self.ghost.append(trackers.arcTracker(scolor=sc,
                                                          swidth=sw))
        if not self.ghost:
            self.finish()
        for g in self.ghost:
            g.on()
        self.activePoint = 0
        self.nodes = []
        self.shift = False
        self.alt = False
        self.force = None
        self.cv = None
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick distance"))

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        arg: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            self.shift = gui_tool_utils.hasMod(arg,
                                               gui_tool_utils.MODCONSTRAIN)
            self.alt = gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT)
            self.ctrl = gui_tool_utils.hasMod(arg, gui_tool_utils.MODSNAP)
            if self.extrudeMode:
                arg["ShiftDown"] = False
            elif hasattr(Gui, "Snapper"):
                Gui.Snapper.setSelectMode(not self.ctrl)
            self.point, cp, info = gui_tool_utils.getPoint(self, arg)
            if gui_tool_utils.hasMod(arg, gui_tool_utils.MODSNAP):
                self.snapped = None
            else:
                self.snapped = self.view.getObjectInfo((arg["Position"][0],
                                                        arg["Position"][1]))
            if self.extrudeMode:
                dist, ang = (self.extrude(self.shift), None)
            else:
                # If the geomType of the edge is "Line" ang will be None,
                # else dist will be None.
                dist, ang = self.redraw(self.point, self.snapped,
                                        self.shift, self.alt)

            if dist:
                self.ui.labelRadius.setText(translate("draft", "Distance"))
                self.ui.radiusValue.setToolTip(translate("draft",
                                                         "Offset distance"))
                self.ui.setRadiusValue(dist, unit="Length")
            else:
                self.ui.labelRadius.setText(translate("draft", "Angle"))
                self.ui.radiusValue.setToolTip(translate("draft",
                                                         "Offset angle"))
                self.ui.setRadiusValue(ang, unit="Angle")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
            gui_tool_utils.redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                self.shift = gui_tool_utils.hasMod(arg,
                                                   gui_tool_utils.MODCONSTRAIN)
                self.alt = gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT)
                if gui_tool_utils.hasMod(arg, gui_tool_utils.MODSNAP):
                    self.snapped = None
                else:
                    self.snapped = self.view.getObjectInfo((cursor[0],
                                                            cursor[1]))
                self.trimObject()
                self.finish()

    def extrude(self, shift=False, real=False):
        """Redraw the ghost in extrude mode."""
        self.newpoint = self.obj.Shape.Faces[0].CenterOfMass
        dvec = self.point.sub(self.newpoint)
        if not shift:
            delta = DraftVecUtils.project(dvec, self.normal)
        else:
            delta = dvec
        if self.force and delta.Length:
            ratio = self.force/delta.Length
            delta.multiply(ratio)
        if real:
            return delta
        self.ghost[0].trans.translation.setValue([delta.x, delta.y, delta.z])
        for i in range(1, len(self.ghost)):
            base = self.obj.Shape.Vertexes[i-1].Point
            self.ghost[i].p1(base)
            self.ghost[i].p2(base.add(delta))
        return delta.Length

    def redraw(self, point, snapped=None, shift=False, alt=False, real=None):
        """Redraw the ghost normally."""
        # initializing
        reverse = False
        for g in self.ghost:
            g.off()
        if real:
            newedges = []

        import DraftGeomUtils
        import Part

        # finding the active point
        vlist = []
        for e in self.edges:
            vlist.append(e.Vertexes[0].Point)
        vlist.append(self.edges[-1].Vertexes[-1].Point)
        if shift:
            npoint = self.activePoint
        else:
            npoint = DraftGeomUtils.findClosest(point, vlist)
        if npoint > len(self.edges)/2:
            reverse = True
        if alt:
            reverse = not reverse
        self.activePoint = npoint

        # sorting out directions
        if reverse and (npoint > 0):
            npoint = npoint - 1
        if (npoint > len(self.edges) - 1):
            edge = self.edges[-1]
            ghost = self.ghost[-1]
        else:
            edge = self.edges[npoint]
            ghost = self.ghost[npoint]
        if reverse:
            v1 = edge.Vertexes[-1].Point
            v2 = edge.Vertexes[0].Point
        else:
            v1 = edge.Vertexes[0].Point
            v2 = edge.Vertexes[-1].Point

        # snapping
        if snapped:
            snapped = self.doc.getObject(snapped['Object'])
            if hasattr(snapped, "Shape"):
                pts = []
                for e in snapped.Shape.Edges:
                    int = DraftGeomUtils.findIntersection(edge, e, True, True)
                    if int:
                        pts.extend(int)
                if pts:
                    point = pts[DraftGeomUtils.findClosest(point, pts)]

        # modifying active edge
        if DraftGeomUtils.geomType(edge) == "Line":
            ang = None
            ve = DraftGeomUtils.vec(edge)
            chord = v1.sub(point)
            n = ve.cross(chord)
            if n.Length == 0:
                self.newpoint = point
            else:
                perp = ve.cross(n)
                proj = DraftVecUtils.project(chord, perp)
                self.newpoint = App.Vector.add(point, proj)
            dist = v1.sub(self.newpoint).Length
            ghost.p1(self.newpoint)
            ghost.p2(v2)
            if real:
                if self.force:
                    ray = self.newpoint.sub(v1)
                    ray.multiply(self.force / ray.Length)
                    self.newpoint = App.Vector.add(v1, ray)
                newedges.append(Part.LineSegment(self.newpoint, v2).toShape())
        else:
            dist = None
            center = edge.Curve.Center
            rad = edge.Curve.Radius
            ang1 = DraftVecUtils.angle(v2.sub(center))
            ang2 = DraftVecUtils.angle(point.sub(center))
            _rot_rad = DraftVecUtils.rotate(App.Vector(rad, 0, 0), -ang2)
            self.newpoint = App.Vector.add(center, _rot_rad)
            ang = math.degrees(-ang2)
            # if ang1 > ang2:
            #     ang1, ang2 = ang2, ang1
            # print("last calculated:",
            #       math.degrees(-ang1),
            #       math.degrees(-ang2))
            ghost.setEndAngle(-ang2)
            ghost.setStartAngle(-ang1)
            ghost.setCenter(center)
            ghost.setRadius(rad)
            if real:
                if self.force:
                    angle = math.radians(self.force)
                    newray = DraftVecUtils.rotate(App.Vector(rad, 0, 0),
                                                  -angle)
                    self.newpoint = App.Vector.add(center, newray)
                chord = self.newpoint.sub(v2)
                perp = chord.cross(App.Vector(0, 0, 1))
                scaledperp = DraftVecUtils.scaleTo(perp, rad)
                midpoint = App.Vector.add(center, scaledperp)
                _sh = Part.Arc(self.newpoint, midpoint, v2).toShape()
                newedges.append(_sh)
        ghost.on()

        # resetting the visible edges
        if not reverse:
            li = list(range(npoint + 1, len(self.edges)))
        else:
            li = list(range(npoint - 1, -1, -1))
        for i in li:
            edge = self.edges[i]
            ghost = self.ghost[i]
            if DraftGeomUtils.geomType(edge) == "Line":
                ghost.p1(edge.Vertexes[0].Point)
                ghost.p2(edge.Vertexes[-1].Point)
            else:
                ang1 = DraftVecUtils.angle(edge.Vertexes[0].Point.sub(center))
                ang2 = DraftVecUtils.angle(edge.Vertexes[-1].Point.sub(center))
                # if ang1 > ang2:
                #     ang1, ang2 = ang2, ang1
                ghost.setEndAngle(-ang2)
                ghost.setStartAngle(-ang1)
                ghost.setCenter(edge.Curve.Center)
                ghost.setRadius(edge.Curve.Radius)
            if real:
                newedges.append(edge)
            ghost.on()

        # finishing
        if real:
            return newedges
        else:
            return [dist, ang]

    def trimObject(self):
        """Trim the actual object."""
        import Part

        if self.extrudeMode:
            delta = self.extrude(self.shift, real=True)
            # print("delta", delta)
            self.doc.openTransaction("Extrude")
            Gui.addModule("Draft")
            obj = Draft.extrude(self.obj, delta, solid=True)
            self.doc.commitTransaction()
            self.obj = obj
        else:
            edges = self.redraw(self.point, self.snapped,
                                self.shift, self.alt, real=True)
            newshape = Part.Wire(edges)
            self.doc.openTransaction("Trim/extend")
            if utils.getType(self.obj) in ["Wire", "BSpline"]:
                p = []
                if self.placement:
                    invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement:
                        np = invpl.multVec(np)
                    p.append(np)
                self.obj.Points = p
            elif utils.getType(self.obj) == "Part::Line":
                p = []
                if self.placement:
                    invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement:
                        np = invpl.multVec(np)
                    p.append(np)
                if ((p[0].x == self.obj.X1)
                        and (p[0].y == self.obj.Y1)
                        and (p[0].z == self.obj.Z1)):
                    self.obj.X2 = p[-1].x
                    self.obj.Y2 = p[-1].y
                    self.obj.Z2 = p[-1].z
                elif ((p[-1].x == self.obj.X1)
                      and (p[-1].y == self.obj.Y1)
                      and (p[-1].z == self.obj.Z1)):
                    self.obj.X2 = p[0].x
                    self.obj.Y2 = p[0].y
                    self.obj.Z2 = p[0].z
                elif ((p[0].x == self.obj.X2)
                      and (p[0].y == self.obj.Y2)
                      and (p[0].z == self.obj.Z2)):
                    self.obj.X1 = p[-1].x
                    self.obj.Y1 = p[-1].y
                    self.obj.Z1 = p[-1].z
                else:
                    self.obj.X1 = p[0].x
                    self.obj.Y1 = p[0].y
                    self.obj.Z1 = p[0].z
            elif utils.getType(self.obj) == "Circle":
                angles = self.ghost[0].getAngles()
                # print("original", self.obj.FirstAngle," ",self.obj.LastAngle)
                # print("new", angles)
                if angles[0] > angles[1]:
                    angles = (angles[1], angles[0])
                self.obj.FirstAngle = angles[0]
                self.obj.LastAngle = angles[1]
            else:
                self.obj.Shape = newshape
            self.doc.commitTransaction()
        self.doc.recompute()
        for g in self.ghost:
            g.off()

    def trimObjects(self, objectslist):
        """Attempt to trim two objects together."""
        import Part
        import DraftGeomUtils

        wires = []
        for obj in objectslist:
            if not utils.getType(obj) in ["Wire", "Circle"]:
                _err(translate("draft",
                               "Unable to trim these objects, "
                               "only Draft wires and arcs are supported."))
                return
            if len(obj.Shape.Wires) > 1:
                _err(translate("draft",
                               "Unable to trim these objects, "
                               "too many wires"))
                return
            if len(obj.Shape.Wires) == 1:
                wires.append(obj.Shape.Wires[0])
            else:
                wires.append(Part.Wire(obj.Shape.Edges))
        ints = []
        edge1 = None
        edge2 = None
        for i1, e1 in enumerate(wires[0].Edges):
            for i2, e2 in enumerate(wires[1].Edges):
                i = DraftGeomUtils.findIntersection(e1, e2, dts=False)
                if len(i) == 1:
                    ints.append(i[0])
                    edge1 = i1
                    edge2 = i2
        if not ints:
            _err(translate("draft", "These objects don't intersect."))
            return
        if len(ints) != 1:
            _err(translate("draft", "Too many intersection points."))
            return

        v11 = wires[0].Vertexes[0].Point
        v12 = wires[0].Vertexes[-1].Point
        v21 = wires[1].Vertexes[0].Point
        v22 = wires[1].Vertexes[-1].Point
        if DraftVecUtils.closest(ints[0], [v11, v12]) == 1:
            last1 = True
        else:
            last1 = False
        if DraftVecUtils.closest(ints[0], [v21, v22]) == 1:
            last2 = True
        else:
            last2 = False
        for i, obj in enumerate(objectslist):
            if i == 0:
                ed = edge1
                la = last1
            else:
                ed = edge2
                la = last2
            if utils.getType(obj) == "Wire":
                if la:
                    pts = obj.Points[:ed + 1] + ints
                else:
                    pts = ints + obj.Points[ed + 1:]
                obj.Points = pts
            else:
                vec = ints[0].sub(obj.Placement.Base)
                vec = obj.Placement.inverse().Rotation.multVec(vec)
                _x = App.Vector(1, 0, 0)
                _ang = -DraftVecUtils.angle(vec,
                                            obj.Placement.Rotation.multVec(_x),
                                            obj.Shape.Edges[0].Curve.Axis)
                ang = math.degrees(_ang)
                if la:
                    obj.LastAngle = ang
                else:
                    obj.FirstAngle = ang
        self.doc.recompute()

    def finish(self, cont=False):
        """Terminate the operation of the Trimex tool."""
        super(Trimex, self).finish()
        self.force = None
        if self.ui:
            if self.linetrack:
                self.linetrack.finalize()
            if self.ghost:
                for g in self.ghost:
                    g.finalize()
            if self.obj:
                self.obj.ViewObject.Visibility = True
                if self.color:
                    self.obj.ViewObject.LineColor = self.color
                if self.width:
                    self.obj.ViewObject.LineWidth = self.width
            gui_utils.select(self.obj)

    def numericRadius(self, dist):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.force = dist
        self.trimObject()
        self.finish()


Gui.addCommand('Draft_Trimex', Trimex())

## @}
