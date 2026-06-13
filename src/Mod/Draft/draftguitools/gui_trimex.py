# SPDX-License-Identifier: LGPL-2.1-or-later

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
import DraftVecUtils
from draftfunctions import extrude
from draftgeoutils import general as geo_general
from draftgeoutils import intersections as geo_intersections
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftguitools import gui_trackers as trackers
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.messages import _msg, _err, _toolmsg
from draftutils.translate import translate


def _trimex_axis_for(obj):
    """Return a Trimex adapter dict for ``obj``, or None.

    The adapter is contributed by ``obj.Proxy.trimex_axis(obj)`` when the
    proxy implements it (BIM Wall / Pipe / Structure ...). For C++ types
    without a Python proxy we fall back to a small set of built-in adapters.

    The returned dict must contain ``endpoints`` (two world-space points
    bounding the object's length / extrusion axis) and ``axes`` (matching
    outward unit vectors used to identify end faces), plus either:
        - ``redirect``: an object to operate on instead (e.g. a base wire)
        - ``set``: callable(list[Vector]) committing new world endpoints
    """
    proxy = getattr(obj, "Proxy", None)
    if proxy is not None and hasattr(proxy, "trimex_axis"):
        try:
            adapter = proxy.trimex_axis(obj)
        except Exception:
            adapter = None
        if adapter is not None:
            return adapter
    return _builtin_trimex_axis(obj)


def _builtin_trimex_axis(obj):
    """Built-in adapters for objects without a Python ``trimex_axis``."""
    try:
        if obj.isDerivedFrom("Part::Extrusion"):
            return _part_extrusion_axis(obj)
    except Exception:
        pass
    return None


def _part_extrusion_axis(obj):
    """``Part::Extrusion`` adapter: modify ``LengthFwd`` / ``LengthRev``
    when the user trims one of the two end caps along ``Dir``.

    Mirrors the executor's geometry rules (FeatureExtrusion.cpp): honours
    ``Reversed`` and the "both lengths zero -> use |Dir|" fallback.

    Symmetric mode is supported but behaves per its definition: the caps
    stay centred on the base point, so dragging one cap makes it land where
    placed while the opposite cap mirrors it (it cannot stay anchored).
    """
    try:
        dir_local = App.Vector(obj.Dir)
    except Exception:
        return None
    dir_len = dir_local.Length
    if dir_len < 1e-9:
        return None
    pl = obj.Placement
    dir_world = pl.Rotation.multVec(dir_local)
    dir_world.normalize()
    if bool(getattr(obj, "Reversed", False)):
        dir_world = dir_world.negative()
    base = App.Vector(pl.Base)
    fwd = float(obj.LengthFwd.Value) if hasattr(obj.LengthFwd, "Value") else float(obj.LengthFwd)
    rev = float(obj.LengthRev.Value) if hasattr(obj.LengthRev, "Value") else float(obj.LengthRev)
    # When both lengths are zero the executor extrudes |Dir| forward.
    if abs(fwd) < 1e-7 and abs(rev) < 1e-7:
        fwd = dir_len
    symmetric = bool(getattr(obj, "Symmetric", False))
    if symmetric:
        # In symmetric mode LengthFwd is the total, centred on the base.
        half = fwd / 2.0
        p_back = base - dir_world * half
        p_fwd = base + dir_world * half
    else:
        p_back = base - dir_world * rev
        p_fwd = base + dir_world * fwd

    def _set(pts):
        new_back = App.Vector(pts[0])
        new_fwd = App.Vector(pts[1])
        # Project onto the extrusion direction. Trimex only affects the
        # axial length: off-axis displacement is ignored on purpose since
        # changing Dir would alter the shape orientation, not just trim it.
        if symmetric:
            # Whichever cap was dragged defines the new half-length; the
            # executor recentres both caps around the base point.
            moved = new_back if (new_back - p_back).Length >= (new_fwd - p_fwd).Length else new_fwd
            obj.LengthFwd = max(0.0, 2.0 * abs((moved - base).dot(dir_world)))
        else:
            obj.LengthRev = max(0.0, (base - new_back).dot(dir_world))
            obj.LengthFwd = max(0.0, (new_fwd - base).dot(dir_world))

    return {
        "endpoints": [p_back, p_fwd],
        "axes": [App.Vector(dir_world).negative(), App.Vector(dir_world)],
        "set": _set,
    }


class Trimex(gui_base_original.Modifier):
    """Gui Command for the Trimex tool.

    This tool trims or extends lines, wires and arcs,
    or extrudes single faces.

    SHIFT constrains to the last point
    or extrudes in direction to the face normal.
    """

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {
            "Pixmap": "Draft_Trimex",
            "Accel": "T, R",
            "MenuText": QT_TRANSLATE_NOOP("Draft_Trimex", "Trimex"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Draft_Trimex", "Trims or extends the selected object, or extrudes single faces"
            ),
        }

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Trimex")
        self.edges = []
        self.placement = None
        self.ghost = []
        self.linetrack = None
        self.color = None
        self.width = None
        # Trimex-axis protocol state. Set when an object provides a
        # ``trimex_axis`` adapter (BIM Wall, Pipe, Structure, Part::Extrusion,
        # ...): the host is re-selected at the end of the command; either the
        # operation is redirected to ``self.obj`` (a base wire), or it commits
        # via ``trimexSet`` which receives the two updated world endpoints.
        self.trimexHost = None
        self.trimexSet = None
        self.trimexEndpoints = None
        self.lockedActivePoint = None
        if self.ui:
            if not Gui.Selection.getSelection():
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select objects to trim or extend"))
                self.call = self.view.addEventCallback("SoEvent", gui_tool_utils.selectObject)
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
        sel = Gui.Selection.getSelectionEx("", 0)[0]

        import Part

        # BIM / extrusion integration. If the selected object opts into
        # Trimex via the ``trimex_axis`` protocol (Wall, Pipe, Structure,
        # Part::Extrusion, ...) and an *end* face is pre-selected, route the
        # operation to its base wire or to a property-update setter instead
        # of the default face-extrude path.
        if self._setupTrimexAxis(sel):
            return

        reason = utils.get_trimex_unsupported_reason(self.obj, sel.SubObjects)
        if reason:
            self.obj = None
            self.finish()
            _err(reason)
            return
        self.ui.trimUi(title=translate("draft", self.featureName))
        self.linetrack = trackers.lineTracker()
        if hasattr(self.obj, "Placement"):
            self.placement = self.obj.Placement
        if self.obj.Shape.Faces:
            self.obj = sel.Object
            if len(self.obj.Shape.Faces) == 1:
                # simple extrude mode, the object itself is extruded
                pass
            elif len(sel.SubObjects) == 1 and sel.SubObjects[0].ShapeType == "Face":
                # face extrude mode, a new object is created
                self.obj = self.doc.addObject("Part::Feature", "Face")
                self.obj.Shape = sel.SubObjects[0]
            else:
                self.obj = None
                self.finish()
                _err(translate("draft", "Only a single face can be extruded"))
                return
            self.extrudeMode = True
            self.normal = self.obj.Shape.Faces[0].normalAt(0.5, 0.5)
            self.ghost = [trackers.ghostTracker([self.obj]), trackers.lineTracker(dotted=True)]
            self.ghost += [trackers.lineTracker() for _ in self.obj.Shape.Vertexes]
        else:
            # normal wire trimex mode
            self.color = self.obj.ViewObject.LineColor
            self.width = self.obj.ViewObject.LineWidth
            if self.obj.Shape.Wires:
                self.edges = self.obj.Shape.Wires[0].Edges
                self.edges = Part.__sortEdges__(self.edges)
            else:
                self.edges = self.obj.Shape.Edges
            for e in self.edges:
                if isinstance(e.Curve, (Part.BSplineCurve, Part.BezierCurve)):
                    self.obj = None
                    self.finish()
                    _err(translate("draft", "Trimex does not support this object type"))
                    return
            self.obj.ViewObject.LineColor = (0.5, 0.5, 0.5)
            self.obj.ViewObject.LineWidth = 1
            self.extrudeMode = False
            self.ghost = []
            lc = self.color
            sc = (lc[0], lc[1], lc[2])
            sw = self.width
            for e in self.edges:
                if geo_general.geomType(e) == "Line":
                    self.ghost.append(trackers.lineTracker(scolor=sc, swidth=sw))
                else:
                    self.ghost.append(trackers.arcTracker(scolor=sc, swidth=sw))
        if not self.ghost:
            self.obj = None
            self.finish()
            return
        for g in self.ghost:
            g.on()
        self.activePoint = 0
        self.nodes = []
        self.shift = False
        self.alt = False
        self.force = None
        self.cv = None
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _toolmsg(translate("draft", "Pick distance"))
        self.selection_done = True
        self.update_hints()

    def _setupTrimexAxis(self, sel):
        """Generic adapter dispatch.

        Resolves a ``trimex_axis`` adapter (from ``obj.Proxy.trimex_axis``
        for BIM objects, or a built-in adapter for C++ types like
        ``Part::Extrusion``) and, if the user pre-selected an *end* face,
        routes Trimex to:
          - the redirected base wire/line (``redirect`` key), or
          - a property-update setter (``set`` key) driven by a virtual edge.

        Returns True if the operation is fully set up here (set-mode);
        False otherwise. Side / top / bottom face selections always fall
        through to the default face-extrude path.
        """
        import Part

        adapter = _trimex_axis_for(self.obj)
        if adapter is None:
            return False

        endpoints = adapter.get("endpoints") or []
        axes = adapter.get("axes") or []
        if len(endpoints) < 2 or len(axes) != len(endpoints):
            return False

        ends = list(zip(endpoints, axes))
        end_idx = None
        for sub in sel.SubObjects:
            if getattr(sub, "ShapeType", None) != "Face":
                continue
            match = self._matchEndFace(sub, ends)
            if match is not None:
                end_idx = match
                break
        if end_idx is None:
            # Not an end face (side / top / bottom, or no face picked): keep
            # the default face-extrude behaviour untouched.
            return False

        redirect = adapter.get("redirect")
        setter = adapter.get("set")
        host = self.obj

        if redirect is not None:
            # Modify the base directly; the host follows on recompute. The
            # adapter reports two ends (first / last cap), but the wire-mode
            # vertex list spans every vertex of the base wire, so map the
            # matched cap to the correct vertex index: 0 -> first, last cap
            # -> the wire's final vertex (== edge count). Without this a
            # multi-segment base would move the 2nd vertex instead of the
            # last when the far end is trimmed.
            self.trimexHost = host
            self.obj = redirect
            if end_idx == 0:
                self.lockedActivePoint = 0
            else:
                # Last cap -> final vertex; its index equals the edge count.
                self.lockedActivePoint = self._wireEdgeCount(redirect)
            return False

        if setter is None:
            return False

        # Property-update mode: drive a virtual single-edge line and commit
        # the new endpoints through the setter callable.
        self.trimexHost = host
        self.trimexSet = setter
        self.trimexEndpoints = [App.Vector(p) for p in endpoints]
        self.lockedActivePoint = end_idx

        p1, p2 = self.trimexEndpoints
        self.edges = [Part.LineSegment(p1, p2).toShape()]
        self.placement = None
        self.extrudeMode = False
        self.ghost = [trackers.lineTracker(scolor=(0.5, 0.5, 0.5), swidth=1)]
        self.ui.trimUi(title=translate("draft", self.featureName))
        self.linetrack = trackers.lineTracker()
        for g in self.ghost:
            g.on()
        self.activePoint = 0
        self.nodes = []
        self.shift = False
        self.alt = False
        self.force = None
        self.cv = None
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _toolmsg(translate("draft", "Pick distance"))
        return True

    @staticmethod
    def _wireEdgeCount(obj):
        """Edge count of ``obj``'s wire, using the same edge selection as the
        wire-mode setup. The final vertex index of the vertex list equals
        this count."""
        import Part

        shape = obj.Shape
        if shape.Wires:
            return len(Part.__sortEdges__(shape.Wires[0].Edges))
        return len(shape.Edges)

    @staticmethod
    def _matchEndFace(face, ends):
        """Index of the end whose axis is parallel to ``face``'s normal and
        whose endpoint lies on the face's plane. ``None`` if the face is not
        an end face (side, top, bottom, etc.)."""
        try:
            u, v = face.Surface.parameter(face.CenterOfMass)
            normal = face.normalAt(u, v)
        except Exception:
            return None
        if normal.Length < 1e-9:
            return None
        normal = App.Vector(normal).normalize()
        center = face.CenterOfMass
        best = None
        best_dot = 0.95  # require strong alignment to discount side/top/bottom faces
        for i, (endpoint, axis) in enumerate(ends):
            ax_len = axis.Length
            if ax_len < 1e-9:
                continue
            ax = App.Vector(axis).multiply(1.0 / ax_len)
            dot = abs(normal.dot(ax))
            if dot < best_dot:
                continue
            # Endpoint must lie on the face's plane (along the normal).
            offset = abs(normal.dot(endpoint - center))
            if offset > 1e-3:
                continue
            best_dot = dot
            best = i
        return best

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
        elif not self.ui.mouse:
            pass
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            self.shift = gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_constrain_key())
            self.alt = gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_alt_key())
            self.ctrl = gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_snap_key())
            if self.extrudeMode:
                arg["ShiftDown"] = False
            elif hasattr(Gui, "Snapper"):
                Gui.Snapper.setSelectMode(not self.ctrl)
            self.point, cp, info = gui_tool_utils.getPoint(self, arg)
            if gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_snap_key()):
                self.snapped = None
            else:
                self.snapped = self.view.getObjectInfo((arg["Position"][0], arg["Position"][1]))
            if self.extrudeMode:
                dist, ang = (self.extrude(self.shift), None)
            else:
                # If the geomType of the edge is "Line" ang will be None,
                # else dist will be None.
                dist, ang = self.redraw(self.point, self.snapped, self.shift, self.alt)

            if dist:
                self.ui.labelRadius.setText(translate("draft", "Distance"))
                self.ui.radiusValue.setToolTip(translate("draft", "Offset distance"))
                self.ui.setRadiusValue(dist, unit="Length")
            elif ang:
                self.ui.labelRadius.setText(translate("draft", "Angle"))
                self.ui.radiusValue.setToolTip(translate("draft", "Offset angle"))
                self.ui.setRadiusValue(ang, unit="Angle")
            else:
                # both dist and ang are None, this indicates an impossible
                # situation. Setting 0 with no unit will show "0 ??" and not
                # compute any value
                self.ui.setRadiusValue(0)
            self.ui.setFocus("radius")
            gui_tool_utils.redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                self.shift = gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_constrain_key())
                self.alt = gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_alt_key())
                if gui_tool_utils.hasMod(arg, gui_tool_utils.get_mod_snap_key()):
                    self.snapped = None
                else:
                    self.snapped = self.view.getObjectInfo((cursor[0], cursor[1]))
                self.trimObject()
                self.finish()

    def extrude(self, shift=False, real=False):
        """Redraw the ghost in extrude mode."""
        self.newpoint = self.obj.Shape.Faces[0].CenterOfMass
        dvec = self.point.sub(self.newpoint)
        if not shift:
            delta = DraftVecUtils.project(dvec, self.normal)
            if delta.Length < 1e-7:
                # Use the normal if self.newpoint is coplanar with the face:
                delta = self.normal * dvec.Length
        else:
            delta = dvec
        if self.force and delta.Length:
            ratio = self.force / delta.Length
            delta.multiply(ratio)
        if real:
            return delta
        self.ghost[0].trans.translation.setValue([delta.x, delta.y, delta.z])
        # Update the dotted lineTracker:
        self.ghost[1].p1(self.newpoint)
        self.ghost[1].p2(self.newpoint + dvec)
        # Update the vertex lineTrackers:
        for i in range(2, len(self.ghost)):
            base = self.obj.Shape.Vertexes[i - 2].Point
            self.ghost[i].p1(base)
            self.ghost[i].p2(base + delta)
        return delta.Length

    def redraw(self, point, snapped=None, shift=False, alt=False, real=None):
        """Redraw the ghost normally."""
        # initializing
        reverse = False
        for g in self.ghost:
            g.off()
        if real:
            newedges = []

        import Part

        # finding the active point
        vlist = []
        for e in self.edges:
            vlist.append(e.Vertexes[0].Point)
        vlist.append(self.edges[-1].Vertexes[-1].Point)
        if self.lockedActivePoint is not None:
            # An endpoint was pre-selected (e.g. the start/end face of a wall).
            # Keep it locked so the user's mouse only sets the new position,
            # not which end moves.
            npoint = self.lockedActivePoint
        elif shift:
            npoint = self.activePoint
        else:
            npoint = geo_general.findClosest(point, vlist)
        if npoint > len(self.edges) / 2:
            reverse = True
        if alt:
            reverse = not reverse
        self.activePoint = npoint

        # sorting out directions
        if reverse and (npoint > 0):
            npoint = npoint - 1
        if npoint > len(self.edges) - 1:
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
            parent = snapped.get("ParentObject", None)
            if parent:
                subname = snapped["SubName"]
            else:
                parent = self.doc.getObject(snapped["Object"])
                subname = snapped["Component"]
            shape = Part.getShape(parent, subname, needSubElement=True, noElementMap=True)
            if shape.Edges:
                pts = []
                for e in shape.Edges:
                    int = geo_intersections.findIntersection(edge, e, True, True)
                    if int:
                        pts.extend(int)
                if pts:
                    point = pts[geo_general.findClosest(point, pts)]

        # modifying active edge
        if geo_general.geomType(edge) == "Line":
            ang = None
            ve = geo_general.vec(edge)
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
                    if ray.Length:
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
                    newray = DraftVecUtils.rotate(App.Vector(rad, 0, 0), -angle)
                    self.newpoint = App.Vector.add(center, newray)
                chord = self.newpoint.sub(v2)
                perp = chord.cross(App.Vector(0, 0, 1))
                scaledperp = DraftVecUtils.scaleTo(perp, rad)
                midpoint = App.Vector.add(center, scaledperp)
                _sh = Part.Arc(self.newpoint, midpoint, v2).toShape()
                newedges.append(_sh)
        ghost.on()

        # resetting the edges
        if not reverse:
            li = list(range(npoint + 1, len(self.edges)))
        else:
            li = list(range(npoint - 1, -1, -1))
        for i in li:
            edge = self.edges[i]
            ghost = self.ghost[i]
            if geo_general.geomType(edge) == "Line":
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
            obj = extrude.extrude(self.obj, delta, solid=True)
            self.doc.commitTransaction()
            self.obj = obj
        else:
            edges = self.redraw(self.point, self.snapped, self.shift, self.alt, real=True)
            newshape = Part.Wire(edges)
            self.doc.openTransaction("Trim/extend")
            if self.trimexSet is not None:
                pts = list(self.trimexEndpoints)
                idx = (
                    self.lockedActivePoint
                    if self.lockedActivePoint is not None
                    else self.activePoint
                )
                if idx is None or idx >= len(pts):
                    idx = min(self.activePoint, len(pts) - 1)
                pts[idx] = App.Vector(self.newpoint)
                self.trimexSet(pts)
            elif utils.getType(self.obj) in ["Wire", "BSpline"]:
                p = []
                if self.placement:
                    invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement:
                        np = invpl.multVec(np)
                    p.append(np)
                # When the far endpoint is trimmed, redraw rebuilds the wire
                # in reverse, which flips the Points order. That is harmless
                # for a standalone wire but flips direction-sensitive parents
                # (Arch Truss/Frame, ...) when we trim a redirected base. In
                # that case keep the original orientation by comparing both
                # ends against the previous Points.
                old = self.obj.Points
                if self.trimexHost is not None and len(p) == len(old) and len(old) >= 2:
                    same = (p[0] - old[0]).Length + (p[-1] - old[-1]).Length
                    flipped = (p[0] - old[-1]).Length + (p[-1] - old[0]).Length
                    if flipped < same:
                        p.reverse()
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
                if (p[0].x == self.obj.X1) and (p[0].y == self.obj.Y1) and (p[0].z == self.obj.Z1):
                    self.obj.X2 = p[-1].x
                    self.obj.Y2 = p[-1].y
                    self.obj.Z2 = p[-1].z
                elif (
                    (p[-1].x == self.obj.X1)
                    and (p[-1].y == self.obj.Y1)
                    and (p[-1].z == self.obj.Z1)
                ):
                    self.obj.X2 = p[0].x
                    self.obj.Y2 = p[0].y
                    self.obj.Z2 = p[0].z
                elif (
                    (p[0].x == self.obj.X2) and (p[0].y == self.obj.Y2) and (p[0].z == self.obj.Z2)
                ):
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

        wires = []
        for obj in objectslist:
            if not utils.getType(obj) in ["Wire", "Circle"]:
                _err(
                    translate(
                        "draft",
                        "Unable to trim these objects, " "only Draft wires and arcs are supported",
                    )
                )
                return
            if len(obj.Shape.Wires) > 1:
                _err(translate("draft", "Unable to trim these objects, " "too many wires"))
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
                i = geo_intersections.findIntersection(e1, e2, dts=False)
                if len(i) == 1:
                    ints.append(i[0])
                    edge1 = i1
                    edge2 = i2
        if not ints:
            _err(translate("draft", "These objects do not intersect"))
            return
        if len(ints) != 1:
            _err(translate("draft", "Too many intersection points"))
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
                    pts = obj.Points[: ed + 1] + ints
                else:
                    pts = ints + obj.Points[ed + 1 :]
                obj.Points = pts
            else:
                vec = ints[0].sub(obj.Placement.Base)
                vec = obj.Placement.inverse().Rotation.multVec(vec)
                _x = App.Vector(1, 0, 0)
                _ang = -DraftVecUtils.angle(
                    vec, obj.Placement.Rotation.multVec(_x), obj.Shape.Edges[0].Curve.Axis
                )
                ang = math.degrees(_ang)
                if la:
                    obj.LastAngle = ang
                else:
                    obj.FirstAngle = ang
        self.doc.recompute()

    def finish(self, cont=False):
        """Terminate the operation of the Trimex tool."""
        self.end_callbacks(self.call)
        self.force = None
        if self.ui:
            if self.linetrack:
                self.linetrack.finalize()
            if self.ghost:
                for g in self.ghost:
                    g.finalize()
            if self.obj:
                if self.color:
                    self.obj.ViewObject.LineColor = self.color
                if self.width:
                    self.obj.ViewObject.LineWidth = self.width
                # Re-select the original host when we operated on its base
                # or via its property setter, so the user keeps a meaningful
                # selection.
                gui_utils.select(self.trimexHost or self.obj)
        super().finish()

    def numericRadius(self, dist):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.force = dist
        self.trimObject()
        self.finish()

    def get_action_hints(self):
        # In Trimex the configured "constrain" and "alt" modifier keys don't
        # do the standard constrain/copy actions, so we describe the actual
        # Trimex-specific behavior instead of using the shared helpers.
        constrain_key = gui_tool_utils._HINT_MOD_KEYS[params.get_param("modconstrain")]
        alt_key = gui_tool_utils._HINT_MOD_KEYS[params.get_param("modalt")]
        hints = [Gui.InputHint(translate("draft", "%1 pick target"), Gui.UserInput.MouseLeft)]
        if self.extrudeMode:
            hints.append(Gui.InputHint(translate("draft", "Hold %1 free direction"), constrain_key))
        else:
            hints.append(
                Gui.InputHint(translate("draft", "Hold %1 keep active endpoint"), constrain_key)
            )
            hints.append(
                Gui.InputHint(translate("draft", "Hold %1 invert trim direction"), alt_key)
            )
        return hints + gui_tool_utils._get_hint_mod_snap()


Gui.addCommand("Draft_Trimex", Trimex())

## @}
