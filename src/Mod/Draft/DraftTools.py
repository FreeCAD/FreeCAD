# -*- coding: utf8 -*-
# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provide GUI commands of the Draft Workbench.

This module loads all graphical commands of the Draft Workbench,
that is, those actions that can be called from menus and buttons.
This module must be imported only when the graphical user interface
is available, for example, during the workbench definition in `IntiGui.py`.
"""
## @package DraftTools
#  \ingroup DRAFT
#  \brief Provide GUI commands of the Draft workbench.
#
#  This module contains all the graphical commands of the Draft workbench,
#  that is, those actions that can be called from menus and buttons.

# ---------------------------------------------------------------------------
# Generic stuff
# ---------------------------------------------------------------------------
import math
import sys
from PySide import QtCore, QtGui
from pivy import coin

import FreeCAD
import FreeCADGui
from FreeCAD import Vector

import Draft
import Draft_rc
import DraftGui  # Initializes the DraftToolBar class
import DraftVecUtils
import WorkingPlane
from draftutils.todo import ToDo
from draftutils.translate import translate
import draftguitools.gui_snapper as gui_snapper
import draftguitools.gui_trackers as trackers

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False
True if DraftGui.__name__ else False

__title__ = "FreeCAD Draft Workbench GUI Tools"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin")
__url__ = "https://www.freecadweb.org"

if not hasattr(FreeCADGui, "Snapper"):
    FreeCADGui.Snapper = gui_snapper.Snapper()

if not hasattr(FreeCAD, "DraftWorkingPlane"):
    FreeCAD.DraftWorkingPlane = WorkingPlane.plane()

# ---------------------------------------------------------------------------
# Commands that have been migrated to their own modules
# ---------------------------------------------------------------------------
import draftguitools.gui_edit
import draftguitools.gui_selectplane
import draftguitools.gui_planeproxy
from draftguitools.gui_lineops import FinishLine
from draftguitools.gui_lineops import CloseLine
from draftguitools.gui_lineops import UndoLine
from draftguitools.gui_togglemodes import ToggleConstructionMode
from draftguitools.gui_togglemodes import ToggleContinueMode
from draftguitools.gui_togglemodes import ToggleDisplayMode
from draftguitools.gui_groups import AddToGroup
from draftguitools.gui_groups import SelectGroup
from draftguitools.gui_groups import SetAutoGroup
from draftguitools.gui_groups import Draft_AddConstruction
from draftguitools.gui_grid import ToggleGrid
from draftguitools.gui_heal import Heal
from draftguitools.gui_dimension_ops import Draft_FlipDimension
from draftguitools.gui_lineslope import Draft_Slope
import draftguitools.gui_arrays
import draftguitools.gui_annotationstyleeditor
# import DraftFillet

# ---------------------------------------------------------------------------
# Preflight stuff
# ---------------------------------------------------------------------------
# update the translation engine
FreeCADGui.updateLocale()

# sets the default working plane
plane = WorkingPlane.plane()
FreeCAD.DraftWorkingPlane = plane
defaultWP = Draft.getParam("defaultWP",1)
if defaultWP == 1: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,0,1), 0)
elif defaultWP == 2: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,1,0), 0)
elif defaultWP == 3: plane.alignToPointAndAxis(Vector(0,0,0), Vector(1,0,0), 0)

# last snapped objects, for quick intersection calculation
lastObj = [0,0]

# Set modifier keys
from draftguitools.gui_tool_utils import MODCONSTRAIN
from draftguitools.gui_tool_utils import MODSNAP
from draftguitools.gui_tool_utils import MODALT

# ---------------------------------------------------------------------------
# General functions
# ---------------------------------------------------------------------------
from draftguitools.gui_tool_utils import formatUnit

from draftguitools.gui_tool_utils import selectObject

from draftguitools.gui_tool_utils import getPoint

from draftguitools.gui_tool_utils import getSupport

from draftguitools.gui_tool_utils import setWorkingPlaneToObjectUnderCursor

from draftguitools.gui_tool_utils import setWorkingPlaneToSelectedObject

from draftguitools.gui_tool_utils import hasMod

from draftguitools.gui_tool_utils import setMod

# ---------------------------------------------------------------------------
# Base Class
# ---------------------------------------------------------------------------
from draftguitools.gui_base_original import DraftTool

# ---------------------------------------------------------------------------
# Geometry constructors
# ---------------------------------------------------------------------------
from draftguitools.gui_tool_utils import redraw3DView

from draftguitools.gui_base_original import Creator

from draftguitools.gui_lines import Line
from draftguitools.gui_lines import Wire
from draftguitools.gui_splines import BSpline
from draftguitools.gui_beziers import BezCurve
from draftguitools.gui_beziers import CubicBezCurve
from draftguitools.gui_beziers import BezierGroup
from draftguitools.gui_rectangles import Rectangle
from draftguitools.gui_arcs import Arc
from draftguitools.gui_arcs import Draft_Arc_3Points
from draftguitools.gui_circles import Circle
from draftguitools.gui_polygons import Polygon
from draftguitools.gui_ellipses import Ellipse
from draftguitools.gui_texts import Text
from draftguitools.gui_dimensions import Dimension
from draftguitools.gui_shapestrings import ShapeString
from draftguitools.gui_points import Point
from draftguitools.gui_facebinders import Draft_Facebinder
from draftguitools.gui_labels import Draft_Label

# ---------------------------------------------------------------------------
# Modifier functions
# ---------------------------------------------------------------------------
from draftguitools.gui_base_original import Modifier

from draftguitools.gui_subelements import SubelementHighlight
from draftguitools.gui_move import Move
from draftguitools.gui_styles import ApplyStyle
from draftguitools.gui_rotate import Rotate
from draftguitools.gui_offset import Offset
from draftguitools.gui_stretch import Stretch
from draftguitools.gui_join import Join
from draftguitools.gui_split import Split
from draftguitools.gui_upgrade import Upgrade
from draftguitools.gui_downgrade import Downgrade
from draftguitools.gui_trimex import Trimex
from draftguitools.gui_scale import Scale
from draftguitools.gui_drawing import Drawing
from draftguitools.gui_wire2spline import WireToBSpline
from draftguitools.gui_shape2dview import Shape2DView


class Draft2Sketch(Modifier):
    """The Draft2Sketch FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Draft2Sketch',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Draft2Sketch", "Draft to Sketch"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Draft2Sketch", "Convert bidirectionally between Draft and Sketch objects")}

    def Activated(self):
        Modifier.Activated(self)
        if not FreeCADGui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to convert")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        sel = FreeCADGui.Selection.getSelection()
        allSketches = True
        allDraft = True
        FreeCADGui.addModule("Draft")
        for obj in sel:
            if obj.isDerivedFrom("Sketcher::SketchObject"):
                allDraft = False
            elif obj.isDerivedFrom("Part::Part2DObjectPython"):
                allSketches = False
            else:
                allDraft = False
                allSketches = False
        if not sel:
            return
        elif allDraft:
            lines = ["Draft.makeSketch(FreeCADGui.Selection.getSelection(),autoconstraints=True)"]
            self.commit(translate("draft","Convert to Sketch"),
                        lines + ['FreeCAD.ActiveDocument.recompute()'])
        elif allSketches:
            lines = ["Draft.draftify(FreeCAD.ActiveDocument."+o.Name+",delete=False)" for o in sel]
            self.commit(translate("draft","Convert to Draft"),
                        lines + ['FreeCAD.ActiveDocument.recompute()'])
        else:
            lines = []
            for obj in sel:
                if obj.isDerivedFrom("Sketcher::SketchObject"):
                    lines.append("Draft.draftify(FreeCAD.ActiveDocument."+obj.Name+",delete=False)")
                elif obj.isDerivedFrom("Part::Part2DObjectPython"):
                    lines.append("Draft.makeSketch(FreeCAD.ActiveDocument."+obj.Name+",autoconstraints=True)")
                elif obj.isDerivedFrom("Part::Feature"):
                    #if (len(obj.Shape.Wires) == 1) or (len(obj.Shape.Edges) == 1):
                    lines.append("Draft.makeSketch(FreeCAD.ActiveDocument."+obj.Name+",autoconstraints=True)")
            self.commit(translate("draft","Convert"),
                        lines + ['FreeCAD.ActiveDocument.recompute()'])
        self.finish()


class Array(Modifier):
    """GuiCommand for the Draft_Array tool.

    Parameters
    ----------
    use_link: bool, optional
        It defaults to `False`. If it is `True`, the created object
        will be a `Link array`.
    """

    def __init__(self, use_link=False):
        Modifier.__init__(self)
        self.use_link = use_link

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Array',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Array", "Array"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Array", "Creates a polar or rectangular array from a selected object")}

    def Activated(self):
        Modifier.Activated(self)
        if not FreeCADGui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to array")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        if FreeCADGui.Selection.getSelection():
            obj = FreeCADGui.Selection.getSelection()[0]
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Array"),
                        ['obj = Draft.makeArray(FreeCAD.ActiveDocument.{},FreeCAD.Vector(1,0,0),FreeCAD.Vector(0,1,0),2,2,use_link={})'.format(obj.Name,self.use_link),
                         'Draft.autogroup(obj)',
                         'FreeCAD.ActiveDocument.recompute()'])
        self.finish()


class LinkArray(Array):
    """GuiCommand for the Draft_LinkArray tool."""

    def __init__(self):
        Array.__init__(self, use_link=True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_LinkArray',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_LinkArray", "LinkArray"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_LinkArray", "Creates a polar or rectangular link array from a selected object")}

class PathArray(Modifier):
    """The PathArray FreeCAD command definition"""

    def __init__(self,use_link=False):
        Modifier.__init__(self)
        self.use_link = use_link

    def GetResources(self):
        return {'Pixmap'  : 'Draft_PathArray',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_PathArray", "PathArray"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_PathArray", "Creates copies of a selected object along a selected path.")}

    def Activated(self):
        Modifier.Activated(self)
        if not FreeCADGui.Selection.getSelectionEx():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Please select base and path objects")+"\n")
#                print("Please select base and path objects")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            base = sel[0].Object
            path = sel[1].Object
            pathsubs = list(sel[1].SubElementNames)
            defXlate = FreeCAD.Vector(0,0,0)
            defCount = 4
            defAlign = False
            FreeCAD.ActiveDocument.openTransaction("PathArray")
            Draft.makePathArray(base,path,defCount,defXlate,defAlign,pathsubs,use_link=self.use_link)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()                                  # feature won't appear until recompute.
        self.finish()

class PathLinkArray(PathArray):
    "The PathLinkArray FreeCAD command definition"

    def __init__(self):
        PathArray.__init__(self,True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_PathLinkArray',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_PathLinkArray", "PathLinkArray"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_PathLinkArray", "Creates links of a selected object along a selected path.")}

class PointArray(Modifier):
    """The PointArray FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_PointArray',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_PointArray", "PointArray"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_PointArray", "Creates copies of a selected object on the position of points.")}

    def Activated(self):
        Modifier.Activated(self)
        if not FreeCADGui.Selection.getSelectionEx():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Please select base and pointlist objects\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            base  = sel[0].Object
            ptlst = sel[1].Object
            FreeCAD.ActiveDocument.openTransaction("PointArray")
            Draft.makePointArray(base, ptlst)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        self.finish()


class Draft_Clone(Modifier):
    """The Draft Clone command definition"""

    def __init__(self):
        Modifier.__init__(self)
        self.moveAfterCloning = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Clone',
                'Accel' : "C,L",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Clone", "Clone"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Clone", "Clones the selected object(s)")}

    def Activated(self):
        Modifier.Activated(self)
        if not FreeCADGui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to clone")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        if FreeCADGui.Selection.getSelection():
            l = len(FreeCADGui.Selection.getSelection())
            FreeCADGui.addModule("Draft")
            FreeCAD.ActiveDocument.openTransaction("Clone")
            nonRepeatList = []
            for obj in FreeCADGui.Selection.getSelection():
                if obj not in nonRepeatList:
                    FreeCADGui.doCommand("Draft.clone(FreeCAD.ActiveDocument.getObject(\""+obj.Name+"\"))")
                    nonRepeatList.append(obj)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            FreeCADGui.Selection.clearSelection()
            for i in range(l):
                FreeCADGui.Selection.addSelection(FreeCAD.ActiveDocument.Objects[-(1+i)])
        self.finish()

    def finish(self,close=False):
        Modifier.finish(self,close=False)
        if self.moveAfterCloning:
            ToDo.delay(FreeCADGui.runCommand, "Draft_Move")


class Mirror(Modifier):
    """The Draft_Mirror FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Mirror',
                'Accel' : "M, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Mirror", "Mirror"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Mirror", "Mirrors the selected objects along a line defined by two points")}

    def Activated(self):
        self.name = translate("draft","Mirror", utf8_decode=True)
        Modifier.Activated(self,self.name)
        self.ghost = None
        if self.ui:
            if not FreeCADGui.Selection.getSelection():
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to mirror")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = FreeCADGui.Selection.getSelection()
        self.ui.pointUi(self.name)
        self.ui.modUi()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        # self.ghost = trackers.ghostTracker(self.sel) TODO: solve this (see below)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick start point of mirror line")+"\n")
        self.ui.isCopy.hide()

    def finish(self,closed=False,cont=False):
        if self.ghost:
            self.ghost.finalize()
        Modifier.finish(self)
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def mirror(self,p1,p2,copy=False):
        """mirroring the real shapes"""
        sel = '['
        for o in self.sel:
            if len(sel) > 1:
                sel += ','
            sel += 'FreeCAD.ActiveDocument.'+o.Name
        sel += ']'
        FreeCADGui.addModule("Draft")
        self.commit(translate("draft","Mirror"),
                    ['Draft.mirror('+sel+','+DraftVecUtils.toString(p1)+','+DraftVecUtils.toString(p2)+')',
                     'FreeCAD.ActiveDocument.recompute()'])

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg)
            if (len(self.node) > 0):
                last = self.node[-1]
                if self.ghost:
                    if self.point != last:
                        # TODO : the following doesn't work at the moment
                        mu = self.point.sub(last).normalize()
                        if FreeCAD.GuiUp:
                            mv = FreeCADGui.ActiveDocument.ActiveView.getViewDirection().negative()
                        else:
                            mv = FreeCAD.Vector(0,0,1)
                        mw = mv.cross(mu)
                        import WorkingPlane
                        tm = WorkingPlane.plane(u=mu,v=mv,w=mw,pos=last).getPlacement().toMatrix()
                        m = self.ghost.getMatrix()
                        m = m.multiply(tm.inverse())
                        m.scale(FreeCAD.Vector(1,1,-1))
                        m = m.multiply(tm)
                        m.scale(FreeCAD.Vector(-1,1,1))
                        self.ghost.setMatrix(m)
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    self.ui.redraw()
                    if (self.node == []):
                        self.node.append(self.point)
                        self.ui.isRelative.show()
                        if self.ghost:
                            self.ghost.on()
                        FreeCAD.Console.PrintMessage(translate("draft", "Pick end point of mirror line")+"\n")
                        if self.planetrack:
                            self.planetrack.set(self.point)
                    else:
                        last = self.node[0]
                        if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                            self.mirror(last,self.point,True)
                        else:
                            self.mirror(last,self.point)
                        if hasMod(arg,MODALT):
                            self.extendedCopy = True
                        else:
                            self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        if not self.node:
            self.node.append(self.point)
            if self.ghost:
                self.ghost.on()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick end point of mirror line")+"\n")
        else:
            last = self.node[-1]
            if self.ui.isCopy.isChecked():
                self.mirror(last,self.point,True)
            else:
                self.mirror(last,self.point)
            self.finish()


# ---------------------------------------------------------------------------
# Snap tools
# ---------------------------------------------------------------------------
from draftguitools.gui_snaps import Draft_Snap_Lock
from draftguitools.gui_snaps import Draft_Snap_Midpoint
from draftguitools.gui_snaps import Draft_Snap_Perpendicular
from draftguitools.gui_snaps import Draft_Snap_Grid
from draftguitools.gui_snaps import Draft_Snap_Intersection
from draftguitools.gui_snaps import Draft_Snap_Parallel
from draftguitools.gui_snaps import Draft_Snap_Endpoint
from draftguitools.gui_snaps import Draft_Snap_Angle
from draftguitools.gui_snaps import Draft_Snap_Center
from draftguitools.gui_snaps import Draft_Snap_Extension
from draftguitools.gui_snaps import Draft_Snap_Near
from draftguitools.gui_snaps import Draft_Snap_Ortho
from draftguitools.gui_snaps import Draft_Snap_Special
from draftguitools.gui_snaps import Draft_Snap_Dimensions
from draftguitools.gui_snaps import Draft_Snap_WorkingPlane
from draftguitools.gui_snaps import ShowSnapBar

# ---------------------------------------------------------------------------
# Adds the icons & commands to the FreeCAD command manager, and sets defaults
# ---------------------------------------------------------------------------

# drawing commands

# modification commands
FreeCADGui.addCommand('Draft_Draft2Sketch',Draft2Sketch())
FreeCADGui.addCommand('Draft_Array',Array())
FreeCADGui.addCommand('Draft_LinkArray',LinkArray())
FreeCADGui.addCommand('Draft_Clone',Draft_Clone())
FreeCADGui.addCommand('Draft_PathArray',PathArray())
FreeCADGui.addCommand('Draft_PathLinkArray',PathLinkArray())
FreeCADGui.addCommand('Draft_PointArray',PointArray())
FreeCADGui.addCommand('Draft_Mirror',Mirror())

# context commands

# a global place to look for active draft Command
FreeCAD.activeDraftCommand = None
