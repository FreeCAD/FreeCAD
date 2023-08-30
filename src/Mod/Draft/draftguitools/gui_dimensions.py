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
"""Provides GUI tools to create dimension objects.

The objects can be simple linear dimensions that measure between two arbitrary
points, or linear dimensions linked to an edge.
It can also be radius or diameter dimensions that measure circles
and circular arcs.
And it can also be an angular dimension measuring the angle between
two straight lines.
"""
## @package gui_dimensions
# \ingroup draftguitools
# \brief Provides GUI tools to create dimension objects.

import math
import lazy_loader.lazy_loader as lz
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
import draftutils.gui_utils as gui_utils

from draftutils.translate import translate
from draftutils.messages import _msg

DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False

## \addtogroup draftguitools
# @{


class Dimension(gui_base_original.Creator):
    """Gui command for the Dimension tool.

    This includes at the moment linear, radial, diametrical,
    and angular dimensions depending on the selected object
    and the modifier key (ALT) used.

    Maybe in the future each type can be in its own class,
    and they can inherit their basic properties from a parent class.
    """

    def __init__(self):
        super().__init__()
        self.max = 2
        self.cont = None
        self.dir = None

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Dimension',
                'Accel': "D, I",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Dimension", "Dimension"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Dimension", "Creates a dimension.\n\n- Pick three points to create a simple linear dimension.\n- Select a straight line to create a linear dimension linked to that line.\n- Select an arc or circle to create a radius or diameter dimension linked to that arc.\n- Select two straight lines to create an angular dimension between them.\nCTRL to snap, SHIFT to constrain, ALT to select an edge or arc.\n\nYou may select a single line or single circular arc before launching this command\nto create the corresponding linked dimension.\nYou may also select an 'App::MeasureDistance' object before launching this command\nto turn it into a 'Draft Dimension' object.")}

    def Activated(self):
        """Execute when the command is called."""
        if self.cont:
            self.finish()
        elif self.selected_app_measure():
            super().Activated(name="Dimension")
            self.dimtrack = trackers.dimTracker()
            self.arctrack = trackers.arcTracker()
            self.create_with_app_measure()
            self.finish()
        else:
            super().Activated(name="Dimension")
            if self.ui:
                self.ui.pointUi(title=translate("draft", "Dimension"), icon="Draft_Dimension")
                self.ui.continueCmd.show()
                self.ui.selectButton.show()
                self.altdown = False
                self.call = self.view.addEventCallback("SoEvent", self.action)
                self.dimtrack = trackers.dimTracker()
                self.arctrack = trackers.arcTracker()
                self.link = None
                self.edges = []
                self.angles = []
                self.angledata = None
                self.indices = []
                self.center = None
                self.arcmode = False
                self.point1 = None
                self.point2 = None
                self.proj_point1 = None
                self.proj_point2 = None
                self.force = None
                self.info = None
                self.selectmode = False
                self.set_selection()
                _msg(translate("draft", "Pick first point"))

    def set_selection(self):
        """Fill the nodes according to the selected geometry."""
        sel = Gui.Selection.getSelectionEx()
        if (len(sel) == 1
                and len(sel[0].SubElementNames) == 1
                and "Edge" in sel[0].SubElementNames[0]):
            # The selection is just a single `Edge`
            sel_object = sel[0]
            edge = sel_object.SubObjects[0]

            # `n` is the edge number starting from 0 not from 1.
            # The reason is lists in Python start from 0, although
            # in the object's `Shape`, they start from 1
            n = int(sel_object.SubElementNames[0].lstrip("Edge")) - 1
            self.indices.append(n)

            if DraftGeomUtils.geomType(edge) == "Line":
                self.node.extend([edge.Vertexes[0].Point,
                                  edge.Vertexes[1].Point])

                # Iterate over the vertices of the parent `Object`;
                # when the vertices match those of the selected `edge`
                # save the index of vertex in the parent object
                v1 = None
                v2 = None
                for i, v in enumerate(sel_object.Object.Shape.Vertexes):
                    if v.Point == edge.Vertexes[0].Point:
                        v1 = i
                    if v.Point == edge.Vertexes[1].Point:
                        v2 = i

                if v1 is not None and v2 is not None: # note that v1 or v2 can be zero
                    self.link = [sel_object.Object, v1, v2]
            elif DraftGeomUtils.geomType(edge) == "Circle":
                self.node.extend([edge.Curve.Center,
                                  edge.Vertexes[0].Point])
                self.edges = [edge]
                self.arcmode = "diameter"
                self.link = [sel_object.Object, n]

    def selected_app_measure(self):
        """Check if App::MeasureDistance objects are selected."""
        sel = Gui.Selection.getSelection()
        if not sel:
            return False
        for o in sel:
            if not o.isDerivedFrom("App::MeasureDistance"):
                return False
        return True

    def finish(self, cont=False):
        """Terminate the operation."""
        self.cont = None
        self.dir = None
        super().finish()
        if self.ui:
            self.dimtrack.finalize()
            self.arctrack.finalize()

    def angle_dimension_normal(self, edge1, edge2):
        rot = App.Rotation(DraftGeomUtils.vec(edge1),
                           DraftGeomUtils.vec(edge2),
                           self.wp.axis,
                           "XYZ")
        norm = rot.multVec(App.Vector(0, 0, 1))
        vnorm = gui_utils.get_3d_view().getViewDirection()
        if vnorm.getAngle(norm) < math.pi / 2:
            norm = norm.negative()
        return norm

    def create_with_app_measure(self):
        """Create on measurement objects.

        This is used when the selection is an `'App::MeasureDistance'`,
        which is created with the basic tool `Std_MeasureDistance`.
        This object is removed and in its place a `Draft Dimension`
        is created.
        """
        for o in Gui.Selection.getSelection():
            p1 = o.P1
            p2 = o.P2
            _root = o.ViewObject.RootNode
            _ch = _root.getChildren()[1].getChildren()[0].getChildren()[0]
            pt = _ch.getChildren()[3]
            p3 = App.Vector(pt.point.getValues()[2].getValue())

            Gui.addModule("Draft")
            _cmd = 'Draft.make_linear_dimension'
            _cmd += '('
            _cmd += DraftVecUtils.toString(p1) + ', '
            _cmd += DraftVecUtils.toString(p2) + ', '
            _cmd += 'dim_line=' + DraftVecUtils.toString(p3)
            _cmd += ')'
            _rem = 'FreeCAD.ActiveDocument.removeObject("' + o.Name + '")'
            _cmd_list = ['_dim_ = ' + _cmd,
                         _rem,
                         'Draft.autogroup(_dim_)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Dimension"),
                        _cmd_list)

    def create_angle_dimension(self):
        """Create an angular dimension from a center and two angles."""
        ang1 = math.degrees(self.angledata[1])
        ang2 = math.degrees(self.angledata[0])
        norm = self.angle_dimension_normal(self.edges[0], self.edges[1])

        _cmd = 'Draft.make_angular_dimension'
        _cmd += '('
        _cmd += 'center=' + DraftVecUtils.toString(self.center) + ', '
        _cmd += 'angles='
        _cmd += '['
        _cmd += str(ang1) + ', '
        _cmd += str(ang2)
        _cmd += '], '
        _cmd += 'dim_line=' + DraftVecUtils.toString(self.node[-1]) + ', '
        _cmd += 'normal=' + DraftVecUtils.toString(norm)
        _cmd += ')'
        _cmd_list = ['_dim_ = ' + _cmd,
                     'Draft.autogroup(_dim_)',
                     'FreeCAD.ActiveDocument.recompute()']
        self.commit(translate("draft", "Create Dimension"),
                    _cmd_list)

    def create_linear_dimension(self):
        """Create a simple linear dimension, not linked to an edge."""
        _cmd = 'Draft.make_linear_dimension'
        _cmd += '('
        _cmd += DraftVecUtils.toString(self.node[0]) + ', '
        _cmd += DraftVecUtils.toString(self.node[1]) + ', '
        _cmd += 'dim_line=' + DraftVecUtils.toString(self.node[2])
        _cmd += ')'
        _cmd_list = ['_dim_ = ' + _cmd,
                     'Draft.autogroup(_dim_)',
                     'FreeCAD.ActiveDocument.recompute()']
        self.commit(translate("draft", "Create Dimension"),
                    _cmd_list)

    def create_linear_dimension_obj(self, direction=None):
        """Create a linear dimension linked to an edge.

        The `link` attribute has indices of vertices as they appear
        in the list `Shape.Vertexes`, so they start as zero 0.

        The `LinearDimension` class, created by `make_linear_dimension_obj`,
        considers the vertices of a `Shape` which are numbered to start
        with 1, that is, `Vertex1`.
        Therefore the value in `link` has to be incremented by 1.
        """
        _cmd = 'Draft.make_linear_dimension_obj'
        _cmd += '('
        _cmd += 'FreeCAD.ActiveDocument.' + self.link[0].Name + ', '
        _cmd += 'i1=' + str(self.link[1] + 1) + ', '
        _cmd += 'i2=' + str(self.link[2] + 1) + ', '
        _cmd += 'dim_line=' + DraftVecUtils.toString(self.node[2])
        _cmd += ')'
        _cmd_list = ['_dim_ = ' + _cmd]

        dir_u = DraftVecUtils.toString(self.wp.u)
        dir_v = DraftVecUtils.toString(self.wp.v)
        if direction == "X":
            _cmd_list += ['_dim_.Direction = ' + dir_u]
        elif direction == "Y":
            _cmd_list += ['_dim_.Direction = ' + dir_v]

        _cmd_list += ['Draft.autogroup(_dim_)',
                      'FreeCAD.ActiveDocument.recompute()']
        self.commit(translate("draft", "Create Dimension"),
                    _cmd_list)

    def create_radial_dimension_obj(self):
        """Create a radial dimension linked to a circular edge."""
        _cmd = 'Draft.make_radial_dimension_obj'
        _cmd += '('
        _cmd += 'FreeCAD.ActiveDocument.' + self.link[0].Name + ', '
        _cmd += 'index=' + str(self.link[1] + 1) + ', '
        _cmd += 'mode="' + str(self.arcmode) + '", '
        _cmd += 'dim_line=' + DraftVecUtils.toString(self.node[2])
        _cmd += ')'
        _cmd_list = ['_dim_ = ' + _cmd,
                     'Draft.autogroup(_dim_)',
                     'FreeCAD.ActiveDocument.recompute()']
        self.commit(translate("draft", "Create Dimension (radial)"),
                    _cmd_list)

    def createObject(self):
        """Create the actual object in the current document."""
        Gui.addModule("Draft")

        if self.angledata:
            # Angle dimension, with two angles provided
            self.create_angle_dimension()
        elif self.link and not self.arcmode:
            # Linear dimension, linked to a straight edge
            if self.force == 1:
                self.create_linear_dimension_obj("Y")
            elif self.force == 2:
                self.create_linear_dimension_obj("X")
            else:
                self.create_linear_dimension_obj()
        elif self.arcmode:
            # Radius or dimeter dimension, linked to a circular edge
            self.create_radial_dimension_obj()
        else:
            # Linear dimension, not linked to any edge
            self.create_linear_dimension()

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
        self.selectmode = not self.selectmode

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

                    vnorm = gui_utils.get_3d_view().getViewDirection()
                    anorm = self.arctrack.normal

                    # Code below taken from WorkingPlane.projectPoint:
                    cos = vnorm.dot(anorm)
                    delta_ax_proj = (self.point - self.center).dot(anorm)
                    proj = self.point - delta_ax_proj / cos * vnorm
                    self.point = proj

                    r = self.point.sub(self.center)
                    self.arctrack.setRadius(r.Length)
                    a = self.arctrack.getAngle(self.point)
                    pair = DraftGeomUtils.getBoundaryAngles(a, self.angles)
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
                if self.node and self.dir and len(self.node) < 2:
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
                        if not self.point1:
                            self.point1 = self.node[0]
                        if not self.point2:
                            self.point2 = self.node[1]
                        # else:
                        #     self.node[1] = self.point2
                        self.set_constraint_node()
                else:
                    self.force = None
                    self.proj_point1 = None
                    self.proj_point2 = None
                    if self.point1:
                        self.node[0] = self.point1
                    if self.point2 and (len(self.node) > 1):
                        self.node[1] = self.point2
                        # self.point2 = None
                # update the dimline
                if self.node and not self.arcmode:
                    self.dimtrack.update(self.node
                                         + [self.point] + [self.cont])
            gui_tool_utils.redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
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
                                            self.arctrack.normal = self.angle_dimension_normal(self.edges[0], self.edges[1])
                                            self.arctrack.on()
                                            for e in self.edges:
                                                if e.Length < 0.00003: # Edge must be long enough for the tolerance of 0.00001mm to make sense.
                                                    _msg(translate("draft", "Edge too short!"))
                                                    self.finish()
                                                    return
                                                for i in [0, 1]:
                                                    pt = e.Vertexes[i].Point
                                                    if pt.isEqual(self.center, 0.00001): # A relatively high tolerance is required.
                                                        pt = e.Vertexes[i - 1].Point     # Use the other point instead.
                                                    self.angles.append(self.arctrack.getAngle(pt))
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

    def set_constraint_node(self):
        """Set constrained nodes for vertical or horizontal dimension
        by projecting on the working plane.
        """
        if not self.proj_point1 or not self.proj_point2:
            self.proj_point1 = self.wp.projectPoint(self.node[0])
            self.proj_point2 = self.wp.projectPoint(self.node[1])
            proj_u= self.wp.u.dot(self.proj_point2 - self.proj_point1)
            proj_v= self.wp.v.dot(self.proj_point2 - self.proj_point1)
            active_view = Gui.ActiveDocument.ActiveView
            cursor = active_view.getCursorPos()
            cursor_point = active_view.getPoint(cursor)
            self.point = self.wp.projectPoint(cursor_point)
            if not self.force:
                ref_point = self.point - (self.proj_point2 + self.proj_point1)*1/2
                ref_angle = abs(ref_point.getAngle(self.wp.u))
                if (ref_angle > math.pi/4) and (ref_angle <= 0.75*math.pi):
                    self.force = 2
                else:
                    self.force = 1
            if self.force == 1:
                self.node[0] = self.proj_point1
                self.node[1] = self.proj_point1 + self.wp.v*proj_v
            elif self.force == 2:
                self.node[0] = self.proj_point1
                self.node[1] = self.proj_point1 + self.wp.u*proj_u


Gui.addCommand('Draft_Dimension', Dimension())

## @}
