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
"""Provides GUI tools to stretch Draft objects.

It works with rectangles, wires, b-splines, bezier curves, and sketches.
It essentially moves the points that are located within a selection area,
while keeping other points intact. This means the lines tied by the points
that were moved are 'stretched'.
"""
## @package gui_stretch
# \ingroup draftguitools
# \brief Provides GUI tools to stretch Draft objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers

from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Stretch(gui_base_original.Modifier):
    """Gui Command for the Stretch tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Stretch',
                'Accel': "S, H",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Stretch", "Stretch"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Stretch", "Stretches the selected objects.\nSelect an object, then draw a rectangle to pick the vertices that will be stretched,\nthen draw a line to specify the distance and direction of stretching.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Stretch, self).Activated(name="Stretch")
        self.rectracker = None
        self.nodetracker = None
        if self.ui:
            if not Gui.Selection.getSelection():
                self.ui.selectUi(on_close_call=self.finish)
                _msg(translate("draft", "Select an object to stretch"))
                self.call = \
                    self.view.addEventCallback("SoEvent",
                                               gui_tool_utils.selectObject)
            else:
                self.proceed()

    def proceed(self):
        """Proceed with execution of the command after proper selection."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        supported = ["Rectangle", "Wire", "BSpline", "BezCurve", "Sketch"]
        self.sel = []
        for obj in Gui.Selection.getSelection():
            if utils.getType(obj) in supported:
                self.sel.append([obj, App.Placement()])
            elif hasattr(obj, "Base"):
                if obj.Base:
                    if utils.getType(obj.Base) in supported:
                        self.sel.append([obj.Base, obj.Placement])
                    elif utils.getType(obj.Base) in ["Offset2D", "Array"]:
                        base = None
                        if hasattr(obj.Base, "Source") and obj.Base.Source:
                            base = obj.Base.Source
                        elif hasattr(obj.Base, "Base") and obj.Base.Base:
                            base = obj.Base.Base
                        if base:
                            if utils.getType(base) in supported:
                                self.sel.append([base, obj.Placement.multiply(obj.Base.Placement)])
                    elif hasattr(obj.Base, "Base"):
                        if obj.Base.Base:
                            if utils.getType(obj.Base.Base) in supported:
                                self.sel.append([obj.Base.Base, obj.Placement.multiply(obj.Base.Placement)])
            elif utils.getType(obj) in ["Offset2D", "Array"]:
                base = None
                if hasattr(obj, "Source") and obj.Source:
                    base = obj.Source
                elif hasattr(obj, "Base") and obj.Base:
                    base = obj.Base
                if base:
                    if utils.getType(base) in supported:
                        self.sel.append([base, obj.Placement])
        if self.ui and self.sel:
            self.step = 1
            self.refpoint = None
            self.ui.pointUi(title=translate("draft", self.featureName), icon="Draft_Stretch")
            self.call = self.view.addEventCallback("SoEvent", self.action)
            self.rectracker = trackers.rectangleTracker(dotted=True,
                                                        scolor=(0.0, 0.0, 1.0),
                                                        swidth=2)
            self.nodetracker = []
            self.displacement = None
            _msg(translate("draft", "Pick first point of selection rectangle"))

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
            point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            if self.step == 2:
                self.rectracker.update(point)
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
                if arg["Position"] == self.pos:
                    # clicked twice on the same point
                    self.finish()
                else:
                    point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
                    self.addPoint(point)

    def addPoint(self, point):
        """Add point to defined selection rectangle."""
        if self.step == 1:
            # first rctangle point
            _msg(translate("draft", "Pick opposite point "
                                    "of selection rectangle"))
            self.ui.setRelative()
            self.rectracker.setorigin(point)
            self.rectracker.on()
            if self.planetrack:
                self.planetrack.set(point)
            self.step = 2
        elif self.step == 2:
            # second rectangle point
            _msg(translate("draft", "Pick start point of displacement"))
            self.rectracker.off()
            nodes = []
            self.ops = []
            for sel in self.sel:
                o = sel[0]
                vispla = sel[1]
                tp = utils.getType(o)
                if tp in ["Wire", "BSpline", "BezCurve"]:
                    np = []
                    iso = False
                    for p in o.Points:
                        p = o.Placement.multVec(p)
                        p = vispla.multVec(p)
                        isi = self.rectracker.isInside(p)
                        np.append(isi)
                        if isi:
                            iso = True
                            nodes.append(p)
                    if iso:
                        self.ops.append([o, np])
                elif tp in ["Rectangle"]:
                    p1 = App.Vector(0, 0, 0)
                    p2 = App.Vector(o.Length.Value, 0, 0)
                    p3 = App.Vector(o.Length.Value, o.Height.Value, 0)
                    p4 = App.Vector(0, o.Height.Value, 0)
                    np = []
                    iso = False
                    for p in [p1, p2, p3, p4]:
                        p = o.Placement.multVec(p)
                        p = vispla.multVec(p)
                        isi = self.rectracker.isInside(p)
                        np.append(isi)
                        if isi:
                            iso = True
                            nodes.append(p)
                    if iso:
                        self.ops.append([o, np])
                elif tp in ["Sketch"]:
                    np = []
                    iso = False
                    for p in o.Shape.Vertexes:
                        p = vispla.multVec(p.Point)
                        isi = self.rectracker.isInside(p)
                        np.append(isi)
                        if isi:
                            iso = True
                            nodes.append(p)
                    if iso:
                        self.ops.append([o, np])
                else:
                    p = o.Placement.Base
                    p = vispla.multVec(p)
                    if self.rectracker.isInside(p):
                        self.ops.append([o])
                        nodes.append(p)
            for n in nodes:
                nt = trackers.editTracker(n, inactive=True)
                nt.on()
                self.nodetracker.append(nt)
            self.step = 3
        elif self.step == 3:
            # first point of displacement line
            _msg(translate("draft", "Pick end point of displacement"))
            self.displacement = point
            # print("first point:", point)
            self.node = [point]
            self.step = 4
        elif self.step == 4:
            # print("second point:", point)
            self.displacement = point.sub(self.displacement)
            self.doStretch()
        if self.point:
            self.ui.redraw()

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        point = App.Vector(numx, numy, numz)
        self.addPoint(point)

    def finish(self, cont=False):
        """Terminate the operation of the command. and clean up."""
        if self.rectracker:
            self.rectracker.finalize()
        if self.nodetracker:
            for n in self.nodetracker:
                n.finalize()
        super(Stretch, self).finish()

    def doStretch(self):
        """Do the actual stretching once the points are selected."""
        commitops = []
        if self.displacement:
            if self.displacement.Length > 0:
                _doc = "FreeCAD.ActiveDocument."
                # print("displacement: ", self.displacement)

                # TODO: break this section into individual functions
                # depending on the type of object (wire, curve, sketch,
                # rectangle, etc.) that is selected, and use variables
                # with common strings to avoid repeating
                # the same information every time, for example, the `_doc`
                # variable.
                # This is necessary to reduce the number of indentation levels
                # and make the code easier to read.
                for ops in self.ops:
                    tp = utils.getType(ops[0])
                    _rot = ops[0].Placement.Rotation
                    localdisp = _rot.inverted().multVec(self.displacement)
                    if tp in ["Wire", "BSpline", "BezCurve"]:
                        pts = []
                        for i in range(len(ops[1])):
                            if ops[1][i] is False:
                                pts.append(ops[0].Points[i])
                            else:
                                pts.append(ops[0].Points[i].add(localdisp))
                        pts = str(pts).replace("Vector ", "FreeCAD.Vector")
                        _cmd = _doc + ops[0].Name + ".Points=" + pts
                        commitops.append(_cmd)
                    elif tp in ["Sketch"]:
                        baseverts = [ops[0].Shape.Vertexes[i].Point for i in range(len(ops[1])) if ops[1][i]]
                        for i in range(ops[0].GeometryCount):
                            j = 0
                            while True:
                                try:
                                    p = ops[0].getPoint(i, j)
                                except ValueError:
                                    break
                                else:
                                    p = ops[0].Placement.multVec(p)
                                    r = None
                                    for bv in baseverts:
                                        if DraftVecUtils.isNull(p.sub(bv)):
                                            _cmd = _doc
                                            _cmd += ops[0].Name
                                            _cmd += ".movePoint"
                                            _cmd += "("
                                            _cmd += str(i) + ", "
                                            _cmd += str(j) + ", "
                                            _cmd += "FreeCAD." + str(localdisp) + ", "
                                            _cmd += "True"
                                            _cmd += ")"
                                            commitops.append(_cmd)
                                            r = bv
                                            break
                                    if r:
                                        baseverts.remove(r)
                                    j += 1
                    elif tp in ["Rectangle"]:
                        p1 = App.Vector(0, 0, 0)
                        p2 = App.Vector(ops[0].Length.Value, 0, 0)
                        p3 = App.Vector(ops[0].Length.Value,
                                        ops[0].Height.Value,
                                        0)
                        p4 = App.Vector(0, ops[0].Height.Value, 0)
                        if ops[1] == [False, True, True, False]:
                            optype = 1
                        elif ops[1] == [False, False, True, True]:
                            optype = 2
                        elif ops[1] == [True, False, False, True]:
                            optype = 3
                        elif ops[1] == [True, True, False, False]:
                            optype = 4
                        else:
                            optype = 0
                        # print("length:", ops[0].Length,
                        #       "height:", ops[0].Height,
                        #       " - ", ops[1],
                        #       " - ", self.displacement)
                        done = False
                        if optype > 0:
                            v1 = ops[0].Placement.multVec(p2).sub(ops[0].Placement.multVec(p1))
                            a1 = round(self.displacement.getAngle(v1), 4)
                            v2 = ops[0].Placement.multVec(p4).sub(ops[0].Placement.multVec(p1))
                            a2 = round(self.displacement.getAngle(v2), 4)
                            # check if the displacement is along one
                            # of the rectangle directions
                            if a1 == 0:  # 0 degrees
                                if optype == 1:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    _cmd = _doc
                                    _cmd += ops[0].Name + ".Length=" + str(d)
                                    commitops.append(_cmd)
                                    done = True
                                elif optype == 3:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Length=" + str(d)
                                    _pl = _doc + ops[0].Name
                                    _pl += ".Placement.Base=FreeCAD."
                                    _pl += str(ops[0].Placement.Base.add(self.displacement))
                                    commitops.append(_cmd)
                                    commitops.append(_pl)
                                    done = True
                            elif a1 == 3.1416:  # pi radians, 180 degrees
                                if optype == 1:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Length=" + str(d)
                                    commitops.append(_cmd)
                                    done = True
                                elif optype == 3:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Length=" + str(d)
                                    _pl = _doc + ops[0].Name
                                    _pl += ".Placement.Base=FreeCAD."
                                    _pl += str(ops[0].Placement.Base.add(self.displacement))
                                    commitops.append(_cmd)
                                    commitops.append(_pl)
                                    done = True
                            elif a2 == 0:  # 0 degrees
                                if optype == 2:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Height=" + str(d)
                                    commitops.append(_cmd)
                                    done = True
                                elif optype == 4:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Height=" + str(d)
                                    _pl = _doc + ops[0].Name
                                    _pl += ".Placement.Base=FreeCAD."
                                    _pl += str(ops[0].Placement.Base.add(self.displacement))
                                    commitops.append(_cmd)
                                    commitops.append(_pl)
                                    done = True
                            elif a2 == 3.1416:  # pi radians, 180 degrees
                                if optype == 2:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Height=" + str(d)
                                    commitops.append(_cmd)
                                    done = True
                                elif optype == 4:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    _cmd = _doc + ops[0].Name
                                    _cmd += ".Height=" + str(d)
                                    _pl = _doc + ops[0].Name
                                    _pl += ".Placement.Base=FreeCAD."
                                    _pl += str(ops[0].Placement.Base.add(self.displacement))
                                    commitops.append(_cmd)
                                    commitops.append(_pl)
                                    done = True
                        if not done:
                            # otherwise create a wire copy and stretch it instead
                            _msg(translate("draft", "Turning one Rectangle into a Wire"))
                            pts = []
                            vts = ops[0].Shape.Vertexes
                            for i in range(4):
                                if ops[1][i] == False:
                                    pts.append(vts[i].Point)
                                else:
                                    pts.append(vts[i].Point.add(self.displacement))
                            pts = str(pts).replace("Vector ", "FreeCAD.Vector")
                            _cmd = "Draft.make_wire"
                            _cmd += "(" + pts + ", closed=True, "
                            _cmd += "face=" + str(ops[0].MakeFace)
                            _cmd += ")"
                            _format = "Draft.formatObject"
                            _format += "(w, "
                            _format += _doc + ops[0].Name
                            _format += ")"
                            _hide = _doc + ops[0].Name + ".ViewObject.hide()"
                            commitops.append("w = " + _cmd)
                            commitops.append(_format)
                            commitops.append(_hide)
                    else:
                        _pl = _doc + ops[0].Name
                        _pl += ".Placement.Base=FreeCAD."
                        _pl += str(ops[0].Placement.Base.add(self.displacement))
                        commitops.append(_pl)
        if commitops:
            commitops.append("FreeCAD.ActiveDocument.recompute()")
            Gui.addModule("Draft")
            self.commit(translate("draft", "Stretch"), commitops)
        self.finish()


Gui.addCommand('Draft_Stretch', Stretch())

## @}
