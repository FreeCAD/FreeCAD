# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
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
"""Provides GUI tools to create BezCurve objects.

In particular, a cubic Bézier curve is defined, as it is one of the most
useful curves for many applications.

See https://en.wikipedia.org/wiki/B%C3%A9zier_curve
"""
## @package gui_beziers
# \ingroup draftguitools
# \brief Provides GUI tools to create BezCurve objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import draftutils.utils as utils
import draftutils.todo as todo
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_lines as gui_lines
import draftguitools.gui_trackers as trackers

from draftutils.messages import _msg, _err
from draftutils.translate import translate


class BezCurve(gui_lines.Line):
    """Gui command for the Bézier Curve tool."""

    def __init__(self):
        super(BezCurve, self).__init__(wiremode=True)
        self.degree = None

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {"Pixmap": "Draft_BezCurve",
                "Accel": "B, Z",
                "MenuText": QT_TRANSLATE_NOOP("Draft_BezCurve", "Bézier curve"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_BezCurve", "Creates an N-degree Bézier curve. The more points you pick, the higher the degree.\nCTRL to snap, SHIFT to constrain.")}

    def Activated(self):
        """Execute when the command is called.

        Activate the specific Bézier curve tracker.
        """
        super(BezCurve, self).Activated(name="BezCurve",
                                        icon="Draft_BezCurve",
                                        task_title=translate("draft","Bézier curve"))
        if self.doc:
            self.bezcurvetrack = trackers.bezcurveTracker()

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view
        by the `Activated` method of the parent class.

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
            (self.point,
             ctrlPoint, info) = gui_tool_utils.getPoint(self, arg,
                                                        noTracker=True)

            # existing points + this pointer position
            self.bezcurvetrack.update(self.node + [self.point],
                                      degree=self.degree)
            gui_tool_utils.redraw3DView()
        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["State"] == "DOWN"
              and arg["Button"] == "BUTTON1"):  # left click
            if arg["Position"] == self.pos:
                self.finish(cont=None)
                return

            if (not self.node) and (not self.support):  # first point
                gui_tool_utils.getSupport(arg)
                (self.point,
                 ctrlPoint, info) = gui_tool_utils.getPoint(self, arg,
                                                            noTracker=True)
            if self.point:
                self.ui.redraw()
                self.pos = arg["Position"]
                self.node.append(self.point)  # add point to "clicked list"
                # sb add a control point,
                # if mod(len(cpoints), 2) == 0
                # then create 2 handle points?
                self.drawUpdate(self.point)
                if not self.isWire and len(self.node) == 2:
                    self.finish(cont=None, closed=False)
                if len(self.node) > 2:
                    # does this make sense for a BCurve?
                    # DNC: allows to close the curve
                    # by placing ends close to each other
                    # with tol = Draft tolerance
                    # old code has been to insensitive
                    if (self.point-self.node[0]).Length < utils.tolerance():
                        self.undolast()
                        self.finish(cont=None, closed=True)
                        _msg(translate("draft",
                                       "Bézier curve has been closed"))

    def undolast(self):
        """Undo last line segment."""
        if len(self.node) > 1:
            self.node.pop()
            self.bezcurvetrack.update(self.node, degree=self.degree)
            self.obj.Shape = self.updateShape(self.node)
            _msg(translate("draft", "Last point has been removed"))

    def drawUpdate(self, point):
        """Draw and update to the curve."""
        if len(self.node) == 1:
            self.bezcurvetrack.on()
            if self.planetrack:
                self.planetrack.set(self.node[0])
            _msg(translate("draft", "Pick next point"))
        else:
            self.obj.Shape = self.updateShape(self.node)
            _msg(translate("draft", "Pick next point"))

    def updateShape(self, pts):
        """Create shape for display during creation process."""
        import Part
        edges = []
        if len(pts) >= 2:  # allow lower degree segment
            poles = pts[1:]
        else:
            poles = []
        if self.degree:
            segpoleslst = [poles[x:x+self.degree] for x in range(0, len(poles), (self.degree or 1))]
        else:
            segpoleslst = [pts]
        startpoint = pts[0]
        for segpoles in segpoleslst:
            c = Part.BezierCurve()  # last segment may have lower degree
            c.increase(len(segpoles))
            c.setPoles([startpoint] + segpoles)
            edges.append(Part.Edge(c))
            startpoint = segpoles[-1]
        w = Part.Wire(edges)
        return w

    def finish(self, cont=False, closed=False):
        """Terminate the operation and close the curve if asked.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        closed: bool, optional
            Close the curve if `True`.
        """
        if self.ui:
            if hasattr(self, "bezcurvetrack"):
                self.bezcurvetrack.finalize()
        if self.obj:
            # remove temporary object, if any
            old = self.obj.Name
            todo.ToDo.delay(self.doc.removeObject, old)
        if len(self.node) > 1:
            # The command to run is built as a series of text strings
            # to be committed through the `draftutils.todo.ToDo` class.
            try:
                rot, sup, pts, fil = self.getStrings()
                Gui.addModule("Draft")
                _cmd = 'Draft.make_bezcurve'
                _cmd += '('
                _cmd += 'points, '
                _cmd += 'closed=' + str(closed) + ', '
                _cmd += 'support=' + sup + ', '
                _cmd += 'degree=' + str(self.degree)
                _cmd += ')'
                _cmd_list = ['points = ' + pts,
                             'bez = ' + _cmd,
                             'Draft.autogroup(bez)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create BezCurve"),
                            _cmd_list)
            except Exception:
                _err("Draft: error delaying commit")

        # `Creator` is the grandfather class, the parent of `Line`;
        # we need to call it to perform final cleanup tasks.
        #
        # Calling it directly like this is a bit messy; maybe we need
        # another method that performs cleanup (superfinish)
        # that is not re-implemented by any of the child classes.
        gui_base_original.Creator.finish(self)
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()


Gui.addCommand('Draft_BezCurve', BezCurve())


class CubicBezCurve(gui_lines.Line):
    """Gui command for the 3rd degree Bézier Curve tool.

    The EnableSelection parameter has an impact on SoMouseButtonEvents. If the
    mouse is over a highlighted object and EnableSelection is `True` the mouse
    up event is not detected. When this command is activated EnableSelection is
    therefore temporarily set to `False`.
    See: https://github.com/FreeCAD/FreeCAD/issues/6452
    """

    def __init__(self):
        super(CubicBezCurve, self).__init__(wiremode=True)
        self.degree = 3
        self.old_EnableSelection = True

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {"Pixmap": "Draft_CubicBezCurve",
                # "Accel": "B, Z",
                "MenuText": QT_TRANSLATE_NOOP("Draft_CubicBezCurve", "Cubic Bézier curve"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_CubicBezCurve", "Creates a Bézier curve made of 2nd degree (quadratic) and 3rd degree (cubic) segments. Click and drag to define each segment.\nAfter the curve is created you can go back to edit each control point and set the properties of each knot.\nCTRL to snap, SHIFT to constrain.")}

    def Activated(self):
        """Execute when the command is called.

        Activate the specific BezCurve tracker.
        """
        param = App.ParamGet("User parameter:BaseApp/Preferences/View")
        self.old_EnableSelection = param.GetBool("EnableSelection", True)
        param.SetBool("EnableSelection", False)

        super(CubicBezCurve, self).Activated(name="CubicBezCurve",
                                             icon="Draft_CubicBezCurve",
                                             task_title=translate("draft","Cubic Bézier curve"))
        if self.doc:
            self.bezcurvetrack = trackers.bezcurveTracker()

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view
        by the `Activated` method of the parent class.

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
            (self.point,
             ctrlPoint, info) = gui_tool_utils.getPoint(self, arg,
                                                        noTracker=True)
            if (len(self.node) - 1) % self.degree == 0 and len(self.node) > 2:
                prevctrl = 2 * self.node[-1] - self.point
                # Existing points + this pointer position
                self.bezcurvetrack.update(self.node[0:-2]
                                          + [prevctrl]
                                          + [self.node[-1]]
                                          + [self.point], degree=self.degree)
            else:
                # Existing points + this pointer position
                self.bezcurvetrack.update(self.node
                                          + [self.point], degree=self.degree)
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            # Press and hold the button
            if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
                if arg["Position"] == self.pos:
                    if len(self.node) > 2:
                        self.node = self.node[0:-2]
                    else:
                        self.node = []
                    return
                else:
                    if (not self.node) and (not self.support):  # first point
                        gui_tool_utils.getSupport(arg)
                        (self.point,
                         ctrlPoint,
                         info) = gui_tool_utils.getPoint(self, arg,
                                                         noTracker=True)
                    if self.point:
                        self.ui.redraw()
                        self.pos = arg["Position"]
                        # add point to "clicked list"
                        self.node.append(self.point)
                        # sb add a control point,
                        # if mod(len(cpoints), 2) == 0
                        # then create 2 handle points?
                        self.drawUpdate(self.point)
                        if not self.isWire and len(self.node) == 2:
                            self.finish(cont=None, closed=False)
                        # does this make sense for a BCurve?
                        if len(self.node) > 2:
                            # add point to "clicked list"
                            self.node.append(self.point)
                            self.drawUpdate(self.point)
                            # DNC: allows to close the curve
                            # by placing ends close to each other
                            # with tol = Draft tolerance
                            # old code has been to insensitive
                            _diff = (self.point - self.node[0]).Length
                            if (_diff < utils.tolerance()
                                    and len(self.node) >= 4):
                                # self.undolast()
                                self.node = self.node[0:-2]
                                # close the curve with a smooth symmetric knot
                                _sym = 2 * self.node[0] - self.node[1]
                                self.node.append(_sym)
                                self.finish(cont=None, closed=True)
                                _msg(translate("draft",
                                               "Bézier curve has been closed"))

            # Release the held button
            if arg["State"] == "UP" and arg["Button"] == "BUTTON1":
                if arg["Position"] == self.pos:
                    self.node = self.node[0:-2]
                    return
                else:
                    if (not self.node) and (not self.support):  # first point
                        return
                    if self.point:
                        self.ui.redraw()
                        self.pos = arg["Position"]
                        # add point to "clicked list"
                        self.node.append(self.point)
                        # sb add a control point,
                        # if mod(len(cpoints),2) == 0
                        # then create 2 handle points?
                        self.drawUpdate(self.point)
                        if not self.isWire and len(self.node) == 2:
                            self.finish(cont=None, closed=False)
                        # Does this make sense for a BCurve?
                        if len(self.node) > 2:
                            self.node[-3] = 2 * self.node[-2] - self.node[-1]
                            self.drawUpdate(self.point)
                            # DNC: allows to close the curve
                            # by placing ends close to each other
                            # with tol = Draft tolerance
                            # old code has been to insensitive

    def undolast(self):
        """Undo last line segment."""
        if len(self.node) > 1:
            self.node.pop()
            self.bezcurvetrack.update(self.node, degree=self.degree)
            self.obj.Shape = self.updateShape(self.node)
            _msg(translate("draft", "Last point has been removed"))

    def drawUpdate(self, point):
        """Create shape for display during creation process."""
        if len(self.node) == 1:
            self.bezcurvetrack.on()
            if self.planetrack:
                self.planetrack.set(self.node[0])
            _msg(translate("draft", "Click and drag to define next knot"))
        elif (len(self.node) - 1) % self.degree == 1 and len(self.node) > 2:
            # is a knot
            self.obj.Shape = self.updateShape(self.node[:-1])
            _msg(translate("draft", "Click and drag to define next knot"))

    def updateShape(self, pts):
        """Create shape for display during creation process."""
        import Part
        # Not quite right. draws 1 big bez. sb segmented
        edges = []

        if len(pts) >= 2:  # allow lower degree segment
            poles = pts[1:]
        else:
            poles = []

        if self.degree:
            segpoleslst = [poles[x:x+self.degree] for x in range(0, len(poles), (self.degree or 1))]
        else:
            segpoleslst = [pts]

        startpoint = pts[0]

        for segpoles in segpoleslst:
            c = Part.BezierCurve()  # last segment may have lower degree
            c.increase(len(segpoles))
            c.setPoles([startpoint] + segpoles)
            edges.append(Part.Edge(c))
            startpoint = segpoles[-1]
        w = Part.Wire(edges)
        return w

    def finish(self, cont=False, closed=False):
        """Terminate the operation and close the curve if asked.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        closed: bool, optional
            Close the curve if `True`.
        """
        param = App.ParamGet("User parameter:BaseApp/Preferences/View")
        param.SetBool("EnableSelection", self.old_EnableSelection)

        if self.ui:
            if hasattr(self, "bezcurvetrack"):
                self.bezcurvetrack.finalize()
        if self.obj:
            # remove temporary object, if any
            old = self.obj.Name
            todo.ToDo.delay(self.doc.removeObject, old)
        if closed is False:
            cleannd = (len(self.node) - 1) % self.degree
            if cleannd == 0:
                self.node = self.node[0:-3]
            if cleannd > 0:
                self.node = self.node[0:-cleannd]
        if len(self.node) > 1:
            try:
                # The command to run is built as a series of text strings
                # to be committed through the `draftutils.todo.ToDo` class.
                rot, sup, pts, fil = self.getStrings()
                Gui.addModule("Draft")
                _cmd = 'Draft.make_bezcurve'
                _cmd += '('
                _cmd += 'points, '
                _cmd += 'closed=' + str(closed) + ', '
                _cmd += 'support=' + sup + ', '
                _cmd += 'degree=' + str(self.degree)
                _cmd += ')'
                _cmd_list = ['points = ' + pts,
                             'bez = ' + _cmd,
                             'Draft.autogroup(bez)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create BezCurve"),
                            _cmd_list)
            except Exception:
                _err("Draft: error delaying commit")

        # `Creator` is the grandfather class, the parent of `Line`;
        # we need to call it to perform final cleanup tasks.
        #
        # Calling it directly like this is a bit messy; maybe we need
        # another method that performs cleanup (superfinish)
        # that is not re-implemented by any of the child classes.
        gui_base_original.Creator.finish(self)
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()


Gui.addCommand('Draft_CubicBezCurve', CubicBezCurve())


class BezierGroup:
    """Gui Command group for the Bézier curve tools."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"MenuText": QT_TRANSLATE_NOOP("Draft_BezierTools", "Bézier tools"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_BezierTools", "Create various types of Bézier curves.")}

    def GetCommands(self):
        """Return a tuple of commands in the group."""
        return ('Draft_CubicBezCurve', 'Draft_BezCurve')

    def IsActive(self):
        """Return True when this command should be available.

        It is `True` when there is a document.
        """
        if Gui.ActiveDocument:
            return True
        else:
            return False


Gui.addCommand('Draft_BezierTools', BezierGroup())

## @}
