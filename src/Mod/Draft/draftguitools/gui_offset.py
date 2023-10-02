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
"""Provides GUI tools to create offsets from objects.

It mostly works on lines, polylines, and similar objects with
regular geometrical shapes, like rectangles.
"""
## @package gui_offset
# \ingroup draftguitools
# \brief Provides GUI tools to create offsets from objects.

## \addtogroup draftguitools
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers

from draftutils.messages import _msg, _wrn, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Offset(gui_base_original.Modifier):
    """Gui Command for the Offset tool."""

    def __init__(self):
        self.param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Offset',
                'Accel': "O, S",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Offset", "Offset"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Offset", "Offsets of the selected object.\nIt can also create an offset copy of the original object.\nCTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.")}

    def Activated(self):
        """Execute when the command is called."""
        self.running = False
        super().Activated(name="Offset")
        self.ghost = None
        self.linetrack = None
        self.arctrack = None
        if self.ui:
            if not Gui.Selection.getSelection():
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to offset"))
                self.call = self.view.addEventCallback(
                    "SoEvent",
                    gui_tool_utils.selectObject)
            elif len(Gui.Selection.getSelection()) > 1:
                _wrn(translate("draft", "Offset only works "
                                        "on one object at a time."))
            else:
                self.proceed()

    def proceed(self):
        """Proceed with the command if one object was selected."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.sel = Gui.Selection.getSelection()[0]
        if not self.sel.isDerivedFrom("Part::Feature"):
            _wrn(translate("draft", "Cannot offset this object type"))
            self.finish()
        else:
            self.step = 0
            self.dvec = None
            self.npts = None
            self.constrainSeg = None

            self.ui.offsetUi()
            occmode = self.param.GetBool("Offset_OCC", False)
            self.ui.occOffset.setChecked(occmode)

            self.linetrack = trackers.lineTracker()
            self.faces = False
            self.shape = self.sel.Shape
            self.mode = None
            if utils.getType(self.sel) in ("Circle", "Arc"):
                self.ghost = trackers.arcTracker()
                self.mode = "Circle"
                self.center = self.shape.Edges[0].Curve.Center
                self.ghost.setCenter(self.center)
                if self.sel.FirstAngle <= self.sel.LastAngle:
                    self.ghost.setStartAngle(math.radians(self.sel.FirstAngle))
                else:
                    self.ghost.setStartAngle(math.radians(self.sel.FirstAngle) - 2 * math.pi)
                self.ghost.setEndAngle(math.radians(self.sel.LastAngle))
            elif utils.getType(self.sel) == "BSpline":
                self.ghost = trackers.bsplineTracker(points=self.sel.Points)
                self.mode = "BSpline"
            elif utils.getType(self.sel) == "BezCurve":
                _wrn(translate("draft", "Offset of Bezier curves "
                                        "is currently not supported"))
                self.finish()
                return
            else:
                if len(self.sel.Shape.Edges) == 1:
                    import Part

                    if isinstance(self.sel.Shape.Edges[0].Curve, Part.Circle):
                        self.ghost = trackers.arcTracker()
                        self.mode = "Circle"
                        self.center = self.shape.Edges[0].Curve.Center
                        self.ghost.setCenter(self.center)
                        if len(self.sel.Shape.Vertexes) > 1:
                            _edge = self.sel.Shape.Edges[0]
                            self.ghost.setStartAngle(_edge.FirstParameter)
                            self.ghost.setEndAngle(_edge.LastParameter)
                if not self.ghost:
                    self.ghost = trackers.wireTracker(self.shape)
                    self.mode = "Wire"
            self.call = self.view.addEventCallback("SoEvent", self.action)
            _msg(translate("draft", "Pick distance"))
            if self.planetrack:
                self.planetrack.set(self.shape.Vertexes[0].Point)
            self.running = True

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        arg: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        import DraftGeomUtils

        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            if (gui_tool_utils.hasMod(arg, gui_tool_utils.MODCONSTRAIN)
                    and self.constrainSeg):
                dist = DraftGeomUtils.findPerpendicular(self.point,
                                                        self.shape,
                                                        self.constrainSeg[1])
            else:
                dist = DraftGeomUtils.findPerpendicular(self.point,
                                                        self.shape.Edges)
            if dist:
                self.ghost.on()
                if self.mode == "Wire":
                    d = dist[0].negative()
                    v1 = DraftGeomUtils.getTangent(self.shape.Edges[0],
                                                   self.point)
                    v2 = DraftGeomUtils.getTangent(self.shape.Edges[dist[1]],
                                                   self.point)
                    a = -DraftVecUtils.angle(v1, v2, self.wp.axis)
                    self.dvec = DraftVecUtils.rotate(d, a, self.wp.axis)
                    occmode = self.ui.occOffset.isChecked()
                    self.param.SetBool("Offset_OCC", occmode)
                    _wire = DraftGeomUtils.offsetWire(self.shape,
                                                      self.dvec,
                                                      occ=occmode)
                    self.ghost.update(_wire, forceclosed=occmode)
                elif self.mode == "BSpline":
                    d = dist[0].negative()
                    e = self.shape.Edges[0]
                    basetan = DraftGeomUtils.getTangent(e, self.point)
                    self.npts = []
                    for p in self.sel.Points:
                        currtan = DraftGeomUtils.getTangent(e, p)
                        a = -DraftVecUtils.angle(currtan, basetan, self.wp.axis)
                        self.dvec = DraftVecUtils.rotate(d, a, self.wp.axis)
                        self.npts.append(p.add(self.dvec))
                    self.ghost.update(self.npts)
                elif self.mode == "Circle":
                    self.dvec = self.point.sub(self.center).Length
                    self.ghost.setRadius(self.dvec)
                self.constrainSeg = dist
                self.linetrack.on()
                self.linetrack.p1(self.point)
                self.linetrack.p2(self.point.add(dist[0]))
                self.ui.setRadiusValue(dist[0].Length, unit="Length")
            else:
                self.dvec = None
                self.ghost.off()
                self.constrainSeg = None
                self.linetrack.off()
                self.ui.radiusValue.setText("off")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
            if self.extendedCopy:
                if not gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                    self.finish()
            gui_tool_utils.redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                copymode = False
                occmode = self.ui.occOffset.isChecked()
                self.param.SetBool("Offset_OCC", occmode)
                if (gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT)
                        or self.ui.isCopy.isChecked()):
                    copymode = True
                Gui.addModule("Draft")
                if self.npts:
                    # _msg("offset:npts= " + str(self.npts))
                    _cmd = 'Draft.offset'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += self.sel.Name + ', '
                    _cmd += DraftVecUtils.toString(self.npts) + ', '
                    _cmd += 'copy=' + str(copymode)
                    _cmd += ')'
                    _cmd_list = ['offst = ' + _cmd,
                                 'FreeCAD.ActiveDocument.recompute()']
                    self.commit(translate("draft", "Offset"),
                                _cmd_list)
                elif self.dvec:
                    if isinstance(self.dvec, float):
                        delta = str(self.dvec)
                    else:
                        delta = DraftVecUtils.toString(self.dvec)
                    _cmd = 'Draft.offset'
                    _cmd += '('
                    _cmd += 'FreeCAD.ActiveDocument.'
                    _cmd += self.sel.Name + ', '
                    _cmd += delta + ', '
                    _cmd += 'copy=' + str(copymode) + ', '
                    _cmd += 'occ=' + str(occmode)
                    _cmd += ')'
                    _cmd_list = ['offst = ' + _cmd,
                                 'FreeCAD.ActiveDocument.recompute()']
                    self.commit(translate("draft", "Offset"),
                                _cmd_list)
                if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                    self.extendedCopy = True
                else:
                    self.finish()

    def finish(self, cont=False):
        """Finish the offset operation."""
        if self.running:
            if self.linetrack:
                self.linetrack.finalize()
            if self.ghost:
                self.ghost.finalize()
        super().finish()

    def numericRadius(self, rad):
        """Validate the radius entry field in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid radius has been entered in the input field.
        """
        # print("dvec:", self.dvec)
        # print("rad:", rad)
        if self.dvec:
            if isinstance(self.dvec, float):
                if self.mode == "Circle":
                    r1 = self.shape.Edges[0].Curve.Radius
                    r2 = self.ghost.getRadius()
                    if r2 >= r1:
                        rad = r1 + rad
                    else:
                        rad = r1 - rad
                    delta = str(rad)
                else:
                    _err("Draft.Offset error: Unhandled case")
            # to offset bspline
            elif self.mode == "BSpline":
                new_points = []
                for old_point, new_point in zip(self.sel.Points, self.npts):
                    diff_direction = new_point.sub(old_point).normalize()
                    new_points.append(old_point.add(diff_direction*rad))
                delta = DraftVecUtils.toString(new_points)
            else:
                self.dvec.normalize()
                self.dvec.multiply(rad)
                delta = DraftVecUtils.toString(self.dvec)
            copymode = False
            occmode = self.ui.occOffset.isChecked()
            self.param.SetBool("Offset_OCC", occmode)

            if self.ui.isCopy.isChecked():
                copymode = True
            Gui.addModule("Draft")
            _cmd = 'Draft.offset'
            _cmd += '('
            _cmd += 'FreeCAD.ActiveDocument.'
            _cmd += self.sel.Name + ', '
            _cmd += delta + ', '
            _cmd += 'copy=' + str(copymode) + ', '
            _cmd += 'occ=' + str(occmode)
            _cmd += ')'
            _cmd_list = ['offst = ' + _cmd,
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Offset"),
                        _cmd_list)
            self.finish()
        else:
            _err(translate("Draft",
                           "Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction"))


Gui.addCommand('Draft_Offset', Offset())

## @}
