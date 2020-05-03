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
"""Provides tools for creating dimension objects with the Draft Workbench.

The objects can be simple linear dimensions that measure between two arbitrary
points, or linear dimensions linked to an edge.
It can also be radius or diameter dimensions that measure circles
and circular arcs.
And it can also be an angular dimension measuring the angle between
two straight lines.
"""
## @package gui_texts
# \ingroup DRAFT
# \brief Provides tools for creating dimensions with the Draft Workbench.

import math
from PySide.QtCore import QT_TRANSLATE_NOOP
import sys

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
from draftutils.translate import translate
from draftutils.messages import _msg

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Dimension(gui_base_original.Creator):
    """Gui command for the Dimension tool.

    This includes at the moment linear, radial, diametrical,
    and angular dimensions depending on the selected object
    and the modifier key (ALT) used.

    Maybe in the future each type can be in its own class,
    and they can inherit their basic properties from a parent class.
    """

    def __init__(self):
        self.max = 2
        self.cont = None
        self.dir = None

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ("Creates a dimension. "
                "Select a straight line to create a linear dimension.\n"
                "Select an arc or circle to create a radius or diameter "
                "dimension.\n"
                "Select two straight lines to create an angular dimension "
                "between them.\n"
                "CTRL to snap, SHIFT to constrain, "
                "ALT to select an edge or arc.")

        return {'Pixmap': 'Draft_Dimension',
                'Accel': "D, I",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Dimension", "Dimension"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Dimension", _tip)}

    def Activated(self):
        """Execute when the command is called."""
        name = translate("draft", "Dimension")
        if self.cont:
            self.finish()
        elif self.hasMeasures():
            super(Dimension, self).Activated(name)
            self.dimtrack = trackers.dimTracker()
            self.arctrack = trackers.arcTracker()
            self.createOnMeasures()
            self.finish()
        else:
            super(Dimension, self).Activated(name)
            if self.ui:
                self.ui.pointUi(name)
                self.ui.continueCmd.show()
                self.ui.selectButton.show()
                self.altdown = False
                self.call = self.view.addEventCallback("SoEvent", self.action)
                self.dimtrack = trackers.dimTracker()
                self.arctrack = trackers.arcTracker()
                self.link = None
                self.edges = []
                self.pts = []
                self.angledata = None
                self.indices = []
                self.center = None
                self.arcmode = False
                self.point2 = None
                self.force = None
                self.info = None
                self.selectmode = False
                self.setFromSelection()
                _msg(translate("draft", "Pick first point"))
                Gui.draftToolBar.show()

    def setFromSelection(self):
        """Fill the nodes according to the selected geometry."""
        import DraftGeomUtils

        sel = Gui.Selection.getSelectionEx()
        if len(sel) == 1:
            if len(sel[0].SubElementNames) == 1:
                if "Edge" in sel[0].SubElementNames[0]:
                    edge = sel[0].SubObjects[0]
                    n = int(sel[0].SubElementNames[0].lstrip("Edge")) - 1
                    self.indices.append(n)
                    if DraftGeomUtils.geomType(edge) == "Line":
                        self.node.extend([edge.Vertexes[0].Point,
                                          edge.Vertexes[1].Point])
                        v1 = None
                        v2 = None
                        for i, v in enumerate(sel[0].Object.Shape.Vertexes):
                            if v.Point == edge.Vertexes[0].Point:
                                v1 = i
                            if v.Point == edge.Vertexes[1].Point:
                                v2 = i
                        if (v1 is not None) and (v2 is not None):
                            self.link = [sel[0].Object, v1, v2]
                    elif DraftGeomUtils.geomType(edge) == "Circle":
                        self.node.extend([edge.Curve.Center,
                                          edge.Vertexes[0].Point])
                        self.edges = [edge]
                        self.arcmode = "diameter"
                        self.link = [sel[0].Object, n]

    def hasMeasures(self):
        """Check if measurement objects are selected."""
        sel = Gui.Selection.getSelection()
        if not sel:
            return False
        for o in sel:
            if not o.isDerivedFrom("App::MeasureDistance"):
                return False
        return True

    def finish(self, closed=False):
        """Terminate the operation."""
        self.cont = None
        self.dir = None
        super(Dimension, self).finish()
        if self.ui:
            self.dimtrack.finalize()
            self.arctrack.finalize()

    def createOnMeasures(self):
        """Create on measurement objects."""
        for o in Gui.Selection.getSelection():
            p1 = o.P1
            p2 = o.P2
            _root = o.ViewObject.RootNode
            _ch = _root.getChildren()[1].getChildren()[0].getChildren()[0]
            pt = _ch.getChildren()[3]
            p3 = App.Vector(pt.point.getValues()[2].getValue())
            Gui.addModule("Draft")
            _cmd = 'Draft.makeDimension'
            _cmd += '('
            _cmd += DraftVecUtils.toString(p1) + ', '
            _cmd += DraftVecUtils.toString(p2) + ', '
            _cmd += DraftVecUtils.toString(p3)
            _cmd += ')'
            _rem = 'FreeCAD.ActiveDocument.removeObject("' + o.Name + '")'
            _cmd_list = ['dim = ' + _cmd,
                         _rem,
                         'Draft.autogroup(dim)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Dimension"),
                        _cmd_list)

    def createObject(self):
        """Create the actual object in the current document."""
        import DraftGeomUtils
        Gui.addModule("Draft")

        if self.angledata:
            normal = "None"
            if len(self.edges) == 2:
                v1 = DraftGeomUtils.vec(self.edges[0])
                v2 = DraftGeomUtils.vec(self.edges[1])
                normal = DraftVecUtils.toString((v1.cross(v2)).normalize())

            _cmd = 'Draft.makeAngularDimension('
            _cmd += 'center=' + DraftVecUtils.toString(self.center) + ', '
            _cmd += 'angles='
            _cmd += '['
            _cmd += str(self.angledata[0]) + ', '
            _cmd += str(self.angledata[1])
            _cmd += '], '
            _cmd += 'p3=' + DraftVecUtils.toString(self.node[-1]) + ', '
            _cmd += 'normal=' + normal
            _cmd += ')'
            _cmd_list = ['dim = ' + _cmd,
                         'Draft.autogroup(dim)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Dimension"),
                        _cmd_list)
        elif self.link and not self.arcmode:
            ops = []
            # Linear dimension, linked
            if self.force == 1:
                _cmd = 'Draft.makeDimension'
                _cmd += '('
                _cmd += 'FreeCAD.ActiveDocument.' + self.link[0].Name + ', '
                _cmd += str(self.link[1]) + ', '
                _cmd += str(self.link[2]) + ', '
                _cmd += DraftVecUtils.toString(self.node[2])
                _cmd += ')'
                _cmd_list = ['dim = ' + _cmd,
                             'dim.Direction = FreeCAD.Vector(0, 1, 0)',
                             'Draft.autogroup(dim)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Dimension"),
                            _cmd_list)
            elif self.force == 2:
                _cmd = 'Draft.makeDimension'
                _cmd += '('
                _cmd += 'FreeCAD.ActiveDocument.' + self.link[0].Name + ', '
                _cmd += str(self.link[1]) + ', '
                _cmd += str(self.link[2]) + ', '
                _cmd += DraftVecUtils.toString(self.node[2])
                _cmd += ')'
                _cmd_list = ['dim = ' + _cmd,
                             'dim.Direction = FreeCAD.Vector(1, 0, 0)',
                             'Draft.autogroup(dim)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Dimension"),
                            _cmd_list)
            else:
                _cmd = 'Draft.makeDimension'
                _cmd += '('
                _cmd += 'FreeCAD.ActiveDocument.' + self.link[0].Name + ', '
                _cmd += str(self.link[1]) + ', '
                _cmd += str(self.link[2]) + ', '
                _cmd += DraftVecUtils.toString(self.node[2])
                _cmd += ')'
                _cmd_list = ['dim = ' + _cmd,
                             'Draft.autogroup(dim)',
                             'FreeCAD.ActiveDocument.recompute()']
                self.commit(translate("draft", "Create Dimension"),
                            _cmd_list)
        elif self.arcmode:
            # Radius or dimeter dimension, linked
            _cmd = 'Draft.makeDimension'
            _cmd += '('
            _cmd += 'FreeCAD.ActiveDocument.' + self.link[0].Name + ', '
            _cmd += str(self.link[1]) + ', '
            _cmd += '"' + str(self.arcmode) + '", '
            _cmd += DraftVecUtils.toString(self.node[2])
            _cmd += ')'
            _cmd_list = ['dim = ' + _cmd,
                         'Draft.autogroup(dim)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Dimension"),
                        _cmd_list)
        else:
            # Linear dimension, non-linked
            _cmd = 'Draft.makeDimension'
            _cmd += '('
            _cmd += DraftVecUtils.toString(self.node[0]) + ', '
            _cmd += DraftVecUtils.toString(self.node[1]) + ', '
            _cmd += DraftVecUtils.toString(self.node[2])
            _cmd += ')'
            _cmd_list = ['dim = ' + _cmd,
                         'Draft.autogroup(dim)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Dimension"),
                        _cmd_list)
        if self.ui.continueMode:
            self.cont = self.node[2]
            if not self.dir:
                if self.link:
                    v1 = self.link[0].Shape.Vertexes[self.link[1]].Point
                    v2 = self.link[0].Shape.Vertexes[self.link[2]].Point
                    self.dir = v2.sub(v1)
                else:
                    self.dir = self.node[1].sub(self.node[0])
            self.node = [self.node[1]]
        self.link = None

    def selectEdge(self):
        """Toggle the select mode to the opposite state."""
        self.selectmode = not(self.selectmode)

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
            import DraftGeomUtils
            shift = gui_tool_utils.hasMod(arg, gui_tool_utils.MODCONSTRAIN)
            if self.arcmode or self.point2:
                gui_tool_utils.setMod(arg, gui_tool_utils.MODCONSTRAIN, False)
            (self.point,
             ctrlPoint, self.info) = gui_tool_utils.getPoint(self, arg,
                                                             noTracker=(len(self.node)>0))
            if (gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT)
                    or self.selectmode) and (len(self.node) < 3):
                self.dimtrack.off()
                if not self.altdown:
                    self.altdown = True
                    self.ui.switchUi(True)
                    if hasattr(Gui, "Snapper"):
                        Gui.Snapper.setSelectMode(True)
                snapped = self.view.getObjectInfo((arg["Position"][0],
                                                   arg["Position"][1]))
                if snapped:
                    ob = self.doc.getObject(snapped['Object'])
                    if "Edge" in snapped['Component']:
                        num = int(snapped['Component'].lstrip('Edge')) - 1
                        ed = ob.Shape.Edges[num]
                        v1 = ed.Vertexes[0].Point
                        v2 = ed.Vertexes[-1].Point
                        self.dimtrack.update([v1, v2, self.cont])
            else:
                if self.node and (len(self.edges) < 2):
                    self.dimtrack.on()
                if len(self.edges) == 2:
                    # angular dimension
                    self.dimtrack.off()
                    r = self.point.sub(self.center)
                    self.arctrack.setRadius(r.Length)
                    a = self.arctrack.getAngle(self.point)
                    pair = DraftGeomUtils.getBoundaryAngles(a, self.pts)
                    if not (pair[0] < a < pair[1]):
                        self.angledata = [4 * math.pi - pair[0],
                                          2 * math.pi - pair[1]]
                    else:
                        self.angledata = [2 * math.pi - pair[0],
                                          2 * math.pi - pair[1]]
                    self.arctrack.setStartAngle(self.angledata[0])
                    self.arctrack.setEndAngle(self.angledata[1])
                if self.altdown:
                    self.altdown = False
                    self.ui.switchUi(False)
                    if hasattr(Gui, "Snapper"):
                        Gui.Snapper.setSelectMode(False)
                if self.dir:
                    _p = DraftVecUtils.project(self.point.sub(self.node[0]),
                                               self.dir)
                    self.point = self.node[0].add(_p)
                if len(self.node) == 2:
                    if self.arcmode and self.edges:
                        cen = self.edges[0].Curve.Center
                        rad = self.edges[0].Curve.Radius
                        baseray = self.point.sub(cen)
                        v2 = DraftVecUtils.scaleTo(baseray, rad)
                        v1 = v2.negative()
                        if shift:
                            self.node = [cen, cen.add(v2)]
                            self.arcmode = "radius"
                        else:
                            self.node = [cen.add(v1), cen.add(v2)]
                            self.arcmode = "diameter"
                        self.dimtrack.update(self.node)
                # Draw constraint tracker line.
                if shift and (not self.arcmode):
                    if len(self.node) == 2:
                        if not self.point2:
                            self.point2 = self.node[1]
                        else:
                            self.node[1] = self.point2
                        if not self.force:
                            _p = self.point.sub(self.node[0])
                            a = abs(_p.getAngle(App.DraftWorkingPlane.u))
                            if (a > math.pi/4) and (a <= 0.75*math.pi):
                                self.force = 1
                            else:
                                self.force = 2
                        if self.force == 1:
                            self.node[1] = App.Vector(self.node[0].x,
                                                      self.node[1].y,
                                                      self.node[0].z)
                        elif self.force == 2:
                            self.node[1] = App.Vector(self.node[1].x,
                                                      self.node[0].y,
                                                      self.node[0].z)
                else:
                    self.force = None
                    if self.point2 and (len(self.node) > 1):
                        self.node[1] = self.point2
                        self.point2 = None
                # update the dimline
                if self.node and not self.arcmode:
                    self.dimtrack.update(self.node
                                         + [self.point] + [self.cont])
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                import DraftGeomUtils
                if self.point:
                    self.ui.redraw()
                    if (not self.node) and (not self.support):
                        gui_tool_utils.getSupport(arg)
                    if (gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT)
                            or self.selectmode) and (len(self.node) < 3):
                        # print("snapped: ",self.info)
                        if self.info:
                            ob = self.doc.getObject(self.info['Object'])
                            if 'Edge' in self.info['Component']:
                                num = int(self.info['Component'].lstrip('Edge')) - 1
                                ed = ob.Shape.Edges[num]
                                v1 = ed.Vertexes[0].Point
                                v2 = ed.Vertexes[-1].Point
                                i1 = i2 = None
                                for i in range(len(ob.Shape.Vertexes)):
                                    if v1 == ob.Shape.Vertexes[i].Point:
                                        i1 = i
                                    if v2 == ob.Shape.Vertexes[i].Point:
                                        i2 = i
                                if (i1 is not None) and (i2 is not None):
                                    self.indices.append(num)
                                    if not self.edges:
                                        # nothing snapped yet, we treat it
                                        # as a normal edge-snapped dimension
                                        self.node = [v1, v2]
                                        self.link = [ob, i1, i2]
                                        self.edges.append(ed)
                                        if DraftGeomUtils.geomType(ed) == "Circle":
                                            # snapped edge is an arc
                                            self.arcmode = "diameter"
                                            self.link = [ob, num]
                                    else:
                                        # there is already a snapped edge,
                                        # so we start angular dimension
                                        self.edges.append(ed)
                                        # self.node now has the 4 endpoints
                                        self.node.extend([v1, v2])
                                        c = DraftGeomUtils.findIntersection(self.node[0],
                                                                            self.node[1],
                                                                            self.node[2],
                                                                            self.node[3],
                                                                            True, True)
                                        if c:
                                            # print("centers:",c)
                                            self.center = c[0]
                                            self.arctrack.setCenter(self.center)
                                            self.arctrack.on()
                                            for e in self.edges:
                                                for v in e.Vertexes:
                                                    self.pts.append(self.arctrack.getAngle(v.Point))
                                            self.link = [self.link[0], ob]
                                        else:
                                            _msg(translate("draft", "Edges don't intersect!"))
                                            self.finish()
                                            return
                                self.dimtrack.on()
                    else:
                        self.node.append(self.point)
                    self.selectmode = False
                    # print("node", self.node)
                    self.dimtrack.update(self.node)
                    if len(self.node) == 2:
                        self.point2 = self.node[1]
                    if len(self.node) == 1:
                        self.dimtrack.on()
                        if self.planetrack:
                            self.planetrack.set(self.node[0])
                    elif len(self.node) == 2 and self.cont:
                        self.node.append(self.cont)
                        self.createObject()
                        if not self.cont:
                            self.finish()
                    elif len(self.node) == 3:
                        # for unlinked arc mode:
                        # if self.arcmode:
                        #     v = self.node[1].sub(self.node[0])
                        #     v.multiply(0.5)
                        #     cen = self.node[0].add(v)
                        #     self.node = [self.node[0], self.node[1], cen]
                        self.createObject()
                        if not self.cont:
                            self.finish()
                    elif self.angledata:
                        self.node.append(self.point)
                        self.createObject()
                        self.finish()

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.point = App.Vector(numx, numy, numz)
        self.node.append(self.point)
        self.dimtrack.update(self.node)
        if len(self.node) == 1:
            self.dimtrack.on()
        elif len(self.node) == 3:
            self.createObject()
            if not self.cont:
                self.finish()


Gui.addCommand('Draft_Dimension', Dimension())
