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
"""Provides GUI tools to create straight Line and Wire objects.

The Line class is used by other Gui Commands that behave in a similar way
like Wire, BSpline, and BezCurve.
"""
## @package gui_lines
# \ingroup draftguitools
# \brief Provides GUI tools to create straight Line and Wire objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
from draftguitools import gui_base_original
from draftguitools import gui_tool_utils
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils import todo
from draftutils.messages import _err, _toolmsg
from draftutils.translate import translate


class Line(gui_base_original.Creator):
    """Gui command for the Line tool."""

    def __init__(self, mode="line"):
        super().__init__()
        self.mode = mode

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Line',
                'Accel': "L,I",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Line", "Line"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Line", "Creates a 2-point line. CTRL to snap, SHIFT to constrain.")}

    def Activated(self, name=QT_TRANSLATE_NOOP("draft", "Line"), icon="Draft_Line", task_title=None):
        """Execute when the command is called."""
        super().Activated(name)
        if task_title is None:
            title = translate("draft", name)
        else:
            title = task_title
        if self.mode == "wire":
            self.ui.wireUi(title=title, icon=icon)
        elif self.mode == "leader":
            self.ui.wireUi(title=title, icon=icon)
            self.ui.closeButton.hide()
            self.ui.hasFill.hide()
        else:
            self.ui.lineUi(title=title, icon=icon)

        self.obj = self.doc.addObject("Part::Feature", self.featureName)
        gui_utils.format_object(self.obj)
        self.obj.ViewObject.ShowInTree = False

        self.call = self.view.addEventCallback("SoEvent", self.action)
        _toolmsg(translate("draft", "Pick first point"))

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
            return
        if arg["Type"] == "SoLocation2Event":
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            gui_tool_utils.redraw3DView()
            return
        if arg["Type"] != "SoMouseButtonEvent":
            return
        if arg["State"] == "UP":
            self.obj.ViewObject.Selectable = True
            return
        if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
            # Stop self.obj from being selected to avoid its display in the tree:
            self.obj.ViewObject.Selectable = False
            if arg["Position"] == self.pos:
                self.finish(cont=None)
                return
            if (not self.node) and (not self.support):
                gui_tool_utils.getSupport(arg)
                self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            if self.point:
                self.ui.redraw()
                self.pos = arg["Position"]
                self.node.append(self.point)
                self.drawSegment(self.point)
                if self.mode == "line" and len(self.node) == 2:
                    self.finish(cont=None, closed=False)
                if len(self.node) > 2:
                    # The wire is closed
                    if (self.point - self.node[0]).Length < utils.tolerance():
                        self.undolast()
                        if len(self.node) > 2:
                            self.finish(cont=None, closed=True)
                        else:
                            self.finish(cont=None, closed=False)

    def finish(self, cont=False, closed=False):
        """Terminate the operation and close the polyline if asked.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        closed: bool, optional
            Close the line if `True`.
        """
        self.end_callbacks(self.call)
        self.removeTemporaryObject()

        if len(self.node) > 1:
            Gui.addModule("Draft")
            # The command to run is built as a series of text strings
            # to be committed through the `draftutils.todo.ToDo` class.
            if (len(self.node) == 2
                    and params.get_param("UsePartPrimitives")):
                # Insert a Part::Primitive object
                p1 = self.node[0]
                p2 = self.node[-1]

                _cmd = 'FreeCAD.ActiveDocument.'
                _cmd += 'addObject("Part::Line", "Line")'
                _cmd_list = ['line = ' + _cmd,
                             'line.X1 = ' + str(p1.x),
                             'line.Y1 = ' + str(p1.y),
                             'line.Z1 = ' + str(p1.z),
                             'line.X2 = ' + str(p2.x),
                             'line.Y2 = ' + str(p2.y),
                             'line.Z2 = ' + str(p2.z),
                             'Draft.autogroup(line)',
                             'Draft.select(line)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Line"),
                            _cmd_list)
            else:
                # Insert a Draft line
                rot, sup, pts, fil = self.getStrings()

                _base = DraftVecUtils.toString(self.node[0])
                _cmd = 'Draft.make_wire'
                _cmd += '('
                _cmd += 'points, '
                _cmd += 'placement=pl, '
                _cmd += 'closed=' + str(closed) + ', '
                _cmd += 'face=' + fil + ', '
                _cmd += 'support=' + sup
                _cmd += ')'
                _cmd_list = ['pl = FreeCAD.Placement()',
                             'pl.Rotation.Q = ' + rot,
                             'pl.Base = ' + _base,
                             'points = ' + pts,
                             'line = ' + _cmd,
                             'Draft.autogroup(line)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Wire"),
                            _cmd_list)
        super().finish()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()

    def removeTemporaryObject(self):
        """Remove temporary object created."""
        if self.obj:
            try:
                old = self.obj.Name
            except ReferenceError:
                # object already deleted, for some reason
                pass
            else:
                todo.ToDo.delay(self.doc.removeObject, old)
        self.obj = None

    def undolast(self):
        """Undoes last line segment."""
        import Part
        if len(self.node) > 1:
            self.node.pop()
            # last = self.node[-1]
            if self.obj.Shape.Edges:
                edges = self.obj.Shape.Edges
                if len(edges) > 1:
                    newshape = Part.makePolygon(self.node)
                    self.obj.Shape = newshape
                else:
                    self.obj.ViewObject.hide()
                # DNC: report on removal
                # _toolmsg(translate("draft", "Removing last point"))
                _toolmsg(translate("draft", "Pick next point"))

    def drawSegment(self, point):
        """Draws new line segment."""
        import Part
        if self.planetrack and self.node:
            self.planetrack.set(self.node[-1])
        if len(self.node) == 1:
            _toolmsg(translate("draft", "Pick next point"))
        elif len(self.node) == 2:
            last = self.node[len(self.node) - 2]
            newseg = Part.LineSegment(last, point).toShape()
            self.obj.Shape = newseg
            self.obj.ViewObject.Visibility = True
            if self.mode != "line":
                _toolmsg(translate("draft", "Pick next point"))
        else:
            currentshape = self.obj.Shape.copy()
            last = self.node[len(self.node) - 2]
            if not DraftVecUtils.equals(last, point):
                newseg = Part.LineSegment(last, point).toShape()
                newshape = currentshape.fuse(newseg)
                self.obj.Shape = newshape
            _toolmsg(translate("draft", "Pick next point"))

    def wipe(self):
        """Remove all previous segments and starts from last point."""
        if len(self.node) > 1:
            # self.obj.Shape.nullify()  # For some reason this fails
            self.obj.ViewObject.Visibility = False
            self.node = [self.node[-1]]
            if self.planetrack:
                self.planetrack.set(self.node[0])
            _toolmsg(translate("draft", "Pick next point"))

    def orientWP(self):
        """Orient the working plane."""
        if len(self.node) > 1 and self.obj:
            import DraftGeomUtils
            n = DraftGeomUtils.getNormal(self.obj.Shape)
            if not n:
                n = self.wp.axis
            p = self.node[-1]
            v = self.node[-1].sub(self.node[-2])
            self.wp.align_to_point_and_axis(p, n, upvec=v, _hist_add=False)
            if self.planetrack:
                self.planetrack.set(self.node[-1])

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        self.node.append(self.point)
        self.drawSegment(self.point)
        if self.mode == "line" and len(self.node) == 2:
            self.finish(cont=None, closed=False)
        self.ui.setNextFocus()


Gui.addCommand('Draft_Line', Line())


class Wire(Line):
    """Gui command for the Wire or Polyline tool.

    It inherits the `Line` class, and calls essentially the same code,
    only this time the `mode` is set to `"wire"`,
    so we are allowed to place more than two points.
    """

    def __init__(self):
        super().__init__(mode="wire")

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Wire',
                'Accel': "P, L",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Wire", "Polyline"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Wire", "Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.")}

    def Activated(self):
        """Execute when the command is called."""
        import Part

        # If there is a selection, and this selection contains various
        # two-point lines, their shapes are extracted, and we attempt
        # to join them into a single Wire (polyline),
        # then the old lines are removed.
        if len(Gui.Selection.getSelection()) > 1:
            edges = []
            for o in Gui.Selection.getSelection():
                if utils.get_type(o) != "Wire":
                    edges = []
                    break
                edges.extend(o.Shape.Edges)
            if edges:
                try:
                    w = Part.Wire(Part.__sortEdges__(edges))
                except Exception:
                    _err(translate("draft",
                                   "Unable to create a Wire "
                                   "from selected objects"))
                else:
                    # Points of the new fused Wire in string form
                    # 'FreeCAD.Vector(x,y,z), FreeCAD.Vector(x1,y1,z1), ...'
                    pts = ", ".join([str(v.Point) for v in w.Vertexes])
                    pts = pts.replace("Vector ", "FreeCAD.Vector")

                    # List of commands to remove the old objects
                    rems = list()
                    for o in Gui.Selection.getSelection():
                        rems.append('FreeCAD.ActiveDocument.'
                                    'removeObject("' + o.Name + '")')

                    Gui.addModule("Draft")
                    # The command to run is built as a series of text strings
                    # to be committed through the `draftutils.todo.ToDo` class
                    _cmd = 'wire = Draft.make_wire('
                    _cmd += '[' + pts + '], closed=' + str(w.isClosed())
                    _cmd += ')'
                    _cmd_list = [_cmd]
                    _cmd_list.extend(rems)
                    _cmd_list.append('Draft.autogroup(wire)')
                    _cmd_list.append('FreeCAD.ActiveDocument.recompute()')

                    _op_name = translate("draft", "Convert to Wire")
                    todo.ToDo.delayCommit([(_op_name, _cmd_list)])
                    return

        # If there was no selection or the selection was just one object
        # then we proceed with the normal line creation functions,
        # only this time we will be able to input more than two points
        super().Activated(name="Polyline",
                          icon="Draft_Wire",
                          task_title=translate("draft", "Polyline"))


Gui.addCommand('Draft_Wire', Wire())

## @}
