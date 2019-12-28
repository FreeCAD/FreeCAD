# -*- coding: utf8 -*-
#***************************************************************************
#*   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
#*   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__="FreeCAD Draft Workbench GUI Tools"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin"
__url__ = "https://www.freecadweb.org"

## @package DraftTools
#  \ingroup DRAFT
#  \brief GUI Commands of the Draft workbench
#
#  This module contains all the FreeCAD commands
#  of the Draft module

#---------------------------------------------------------------------------
# Generic stuff
#---------------------------------------------------------------------------

import sys, os, FreeCAD, FreeCADGui, WorkingPlane, math, re, Draft, Draft_rc, DraftVecUtils
from FreeCAD import Vector
from PySide import QtCore,QtGui
from DraftGui import todo, translate, utf8_decode
from DraftSnap import *
from DraftTrackers import *
from pivy import coin

#---------------------------------------------------------------------------
# Commands that have been migrated to their own modules
#---------------------------------------------------------------------------

import DraftEdit
# import DraftFillet
import DraftSelectPlane

#---------------------------------------------------------------------------
# Preflight stuff
#---------------------------------------------------------------------------

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

# set modifier keys
MODS = ["shift","ctrl","alt"]
MODCONSTRAIN = MODS[Draft.getParam("modconstrain",0)]
MODSNAP = MODS[Draft.getParam("modsnap",1)]
MODALT = MODS[Draft.getParam("modalt",2)]

#---------------------------------------------------------------------------
# General functions
#---------------------------------------------------------------------------

def formatUnit(exp,unit="mm"):
    '''returns a formatting string to set a number to the correct unit'''
    return FreeCAD.Units.Quantity(exp,FreeCAD.Units.Length).UserString

def selectObject(arg):
    '''this is a scene even handler, to be called from the Draft tools
    when they need to select an object'''
    if (arg["Type"] == "SoKeyboardEvent"):
        if (arg["Key"] == "ESCAPE"):
            FreeCAD.activeDraftCommand.finish()
            # TODO : this part raises a coin3D warning about scene traversal, to be fixed.
    elif (arg["Type"] == "SoMouseButtonEvent"):
        if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
            cursor = arg["Position"]
            snapped = Draft.get3DView().getObjectInfo((cursor[0],cursor[1]))
            if snapped:
                obj = FreeCAD.ActiveDocument.getObject(snapped['Object'])
                FreeCADGui.Selection.addSelection(obj)
                FreeCAD.activeDraftCommand.component=snapped['Component']
                FreeCAD.activeDraftCommand.proceed()

def getPoint(target,args,mobile=False,sym=False,workingplane=True,noTracker=False):
    """Function used by the Draft Tools.
    returns a constrained 3d point and its original point.
    if mobile=True, the constraining occurs from the location of
    mouse cursor when Shift is pressed, otherwise from last entered
    point. If sym=True, x and y values stay always equal. If workingplane=False,
    the point won't be projected on the Working Plane. if noTracker is True, the
    tracking line will not be displayed
    """

    ui = FreeCADGui.draftToolBar

    if target.node:
        last = target.node[-1]
    else:
        last = None

    amod = hasMod(args, MODSNAP)
    cmod = hasMod(args, MODCONSTRAIN)
    point = None

    if hasattr(FreeCADGui, "Snapper"):
        point = FreeCADGui.Snapper.snap(args["Position"],lastpoint=last,active=amod,constrain=cmod,noTracker=noTracker)
        info = FreeCADGui.Snapper.snapInfo
        mask = FreeCADGui.Snapper.affinity
    if not point:
        p = FreeCADGui.ActiveDocument.ActiveView.getCursorPos()
        point = FreeCADGui.ActiveDocument.ActiveView.getPoint(p)
        info = FreeCADGui.ActiveDocument.ActiveView.getObjectInfo(p)
        mask = None

    ctrlPoint = Vector(point)
    if target.node:
        if target.featureName == "Rectangle":
            ui.displayPoint(point, target.node[0], plane=plane, mask=mask)
        else:
            ui.displayPoint(point, target.node[-1], plane=plane, mask=mask)
    else:
        ui.displayPoint(point, plane=plane, mask=mask)
    return point,ctrlPoint,info

def getSupport(mouseEvent=None):
    """returns the supporting object and sets the working plane"""
    plane.save()
    if mouseEvent:
        return setWorkingPlaneToObjectUnderCursor(mouseEvent)
    return setWorkingPlaneToSelectedObject()

def setWorkingPlaneToObjectUnderCursor(mouseEvent):
    objectUnderCursor = Draft.get3DView().getObjectInfo((
        mouseEvent["Position"][0],
        mouseEvent["Position"][1]))

    if not objectUnderCursor:
        return None

    try:
        componentUnderCursor = getattr(
            FreeCAD.ActiveDocument.getObject(
                objectUnderCursor['Object']
                ).Shape,
            objectUnderCursor["Component"])

        if not plane.weak:
            return None

        if "Face" in objectUnderCursor["Component"]:
            plane.alignToFace(componentUnderCursor)
        else:
            plane.alignToCurve(componentUnderCursor)
        plane.weak = True
        return objectUnderCursor
    except:
        pass

    return None

def setWorkingPlaneToSelectedObject():
    sel = FreeCADGui.Selection.getSelectionEx()
    if len(sel) != 1:
        return None
    sel = sel[0]
    if sel.HasSubObjects \
        and len(sel.SubElementNames) == 1 \
        and "Face" in sel.SubElementNames[0]:
        if plane.weak:
            plane.alignToFace(sel.SubObjects[0])
            plane.weak = True
        return sel.Object
    return None

def hasMod(args,mod):
    """checks if args has a specific modifier"""
    if mod == "shift":
        return args["ShiftDown"]
    elif mod == "ctrl":
        return args["CtrlDown"]
    elif mod == "alt":
        return args["AltDown"]

def setMod(args,mod,state):
    """sets a specific modifier state in args"""
    if mod == "shift":
        args["ShiftDown"] = state
    elif mod == "ctrl":
        args["CtrlDown"] = state
    elif mod == "alt":
        args["AltDown"] = state




#---------------------------------------------------------------------------
# Base Class
#---------------------------------------------------------------------------

class DraftTool:
    """The base class of all Draft Tools"""

    def __init__(self):
        self.commitList = []

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self, name="None", noplanesetup=False, is_subtool=False):
        if FreeCAD.activeDraftCommand and not is_subtool:
            FreeCAD.activeDraftCommand.finish()

        global Part, DraftGeomUtils
        import Part, DraftGeomUtils

        self.ui = None
        self.call = None
        self.support = None
        self.point = None
        self.commitList = []
        self.doc = FreeCAD.ActiveDocument
        if not self.doc:
            self.finish()
            return

        FreeCAD.activeDraftCommand = self
        self.view = Draft.get3DView()
        self.ui = FreeCADGui.draftToolBar
        self.featureName = name
        self.ui.sourceCmd = self
        self.ui.setTitle(name)
        self.ui.show()
        if not noplanesetup:
            plane.setup()
        self.node = []
        self.pos = []
        self.constrain = None
        self.obj = None
        self.extendedCopy = False
        self.ui.setTitle(name)
        self.planetrack = None
        if Draft.getParam("showPlaneTracker",False):
            self.planetrack = PlaneTracker()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.setTrackers()

    def finish(self,close=False):
        self.node = []
        FreeCAD.activeDraftCommand = None
        if self.ui:
            self.ui.offUi()
            self.ui.sourceCmd = None
        if self.planetrack:
            self.planetrack.finalize()
        plane.restore()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.off()
        if self.call:
            try:
                self.view.removeEventCallback("SoEvent",self.call)
            except RuntimeError:
                # the view has been deleted already
                pass
            self.call = None
        if self.commitList:
            todo.delayCommit(self.commitList)
        self.commitList = []

    def commit(self,name,func):
        """stores actions to be committed to the FreeCAD document"""
        self.commitList.append((name,func))

    def getStrings(self,addrot=None):
        """returns a couple of useful strings for building python commands"""

        # current plane rotation
        p = plane.getRotation()
        qr = p.Rotation.Q
        qr = '('+str(qr[0])+','+str(qr[1])+','+str(qr[2])+','+str(qr[3])+')'

        # support object
        if self.support and FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("useSupport",False):
            sup = 'FreeCAD.ActiveDocument.getObject("' + self.support.Name + '")'
        else:
            sup = 'None'

        # contents of self.node
        points='['
        for n in self.node:
            if len(points) > 1:
                points += ','
            points += DraftVecUtils.toString(n)
        points += ']'

        # fill mode
        if self.ui:
            fil = str(bool(self.ui.fillmode))
        else:
            fil = "True"

        return qr,sup,points,fil


#---------------------------------------------------------------------------
# Geometry constructors
#---------------------------------------------------------------------------

def redraw3DView():
    """redraw3DView(): forces a redraw of 3d view."""
    try:
        FreeCADGui.ActiveDocument.ActiveView.redraw()
    except AttributeError as err:
        pass

class Creator(DraftTool):
    """A generic Draft Creator Tool used by creation tools such as line or arc"""

    def __init__(self):
        DraftTool.__init__(self)

    def Activated(self,name="None",noplanesetup=False):
        DraftTool.Activated(self,name,noplanesetup)
        if not noplanesetup:
            self.support = getSupport()

class Line(Creator):
    """The Line FreeCAD command definition"""

    def __init__(self, wiremode=False):
        Creator.__init__(self)
        self.isWire = wiremode

    def GetResources(self):
        return {'Pixmap': 'Draft_Line',
                'Accel': "L,I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Line", "Line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Line", "Creates a 2-point line. CTRL to snap, SHIFT to constrain")}

    def Activated(self,name=translate("draft","Line")):
        Creator.Activated(self,name)
        if not self.doc:
            return
        self.obj = None # stores the temp shape
        self.oldWP = None # stores the WP if we modify it
        if self.isWire:
            self.ui.wireUi(name)
        else:
            self.ui.lineUi(name)
        self.ui.setTitle(translate("draft", "Line"))
        if sys.version_info.major < 3:
            if isinstance(self.featureName,unicode):
                self.featureName = self.featureName.encode("utf8")
        self.obj=self.doc.addObject("Part::Feature",self.featureName)
        Draft.formatObject(self.obj)
        self.call = self.view.addEventCallback("SoEvent", self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick first point")+"\n")

    def action(self, arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point, ctrlPoint, info = getPoint(self, arg)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent" and \
            arg["State"] == "DOWN" and \
            arg["Button"] == "BUTTON1":
                if (arg["Position"] == self.pos):
                    return self.finish(False,cont=True)
                if (not self.node) and (not self.support):
                    getSupport(arg)
                    self.point,ctrlPoint,info = getPoint(self,arg)
                if self.point:
                    self.ui.redraw()
                    self.pos = arg["Position"]
                    self.node.append(self.point)
                    self.drawSegment(self.point)
                    if (not self.isWire and len(self.node) == 2):
                        self.finish(False,cont=True)
                    if (len(self.node) > 2):
                        if ((self.point-self.node[0]).Length < Draft.tolerance()):
                            self.undolast()
                            self.finish(True,cont=True)

    def finish(self,closed=False,cont=False):
        """terminates the operation and closes the poly if asked"""
        self.removeTemporaryObject()
        if self.oldWP:
            FreeCAD.DraftWorkingPlane = self.oldWP
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
                FreeCADGui.Snapper.restack()
        self.oldWP = None
        if (len(self.node) > 1):
            FreeCADGui.addModule("Draft")
            if (len(self.node) == 2) and Draft.getParam("UsePartPrimitives",False):
                # use Part primitive
                p1 = self.node[0]
                p2 = self.node[-1]
                self.commit(translate("draft","Create Line"),
                            ['line = FreeCAD.ActiveDocument.addObject("Part::Line","Line")',
                             'line.X1 = '+str(p1.x),
                             'line.Y1 = '+str(p1.y),
                             'line.Z1 = '+str(p1.z),
                             'line.X2 = '+str(p2.x),
                             'line.Y2 = '+str(p2.y),
                             'line.Z2 = '+str(p2.z),
                             'Draft.autogroup(line)',
                             'FreeCAD.ActiveDocument.recompute()'])
            else:
                # building command string
                rot,sup,pts,fil = self.getStrings()
                self.commit(translate("draft","Create Wire"),
                            ['pl = FreeCAD.Placement()',
                             'pl.Rotation.Q = '+rot,
                             'pl.Base = '+DraftVecUtils.toString(self.node[0]),
                             'points = '+pts,
                             'line = Draft.makeWire(points,placement=pl,closed='+str(closed)+',face='+fil+',support='+sup+')',
                             'Draft.autogroup(line)',
                             'FreeCAD.ActiveDocument.recompute()'])
        Creator.finish(self)
        if self.ui and self.ui.continueMode:
            self.Activated()

    def removeTemporaryObject(self):
        if self.obj:
            try:
                old = self.obj.Name
            except ReferenceError:
                # object already deleted, for some reason
                pass
            else:
                todo.delay(self.doc.removeObject,old)
        self.obj = None

    def undolast(self):
        """undoes last line segment"""
        if (len(self.node) > 1):
            self.node.pop()
            last = self.node[-1]
            if self.obj.Shape.Edges:
                edges = self.obj.Shape.Edges
                if len(edges) > 1:
                    newshape = Part.makePolygon(self.node)
                    self.obj.Shape = newshape
                else:
                    self.obj.ViewObject.hide()
                # DNC: report on removal
                #FreeCAD.Console.PrintMessage(translate("draft", "Removing last point")+"\n")
                FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")

    def drawSegment(self,point):
        """draws a new segment"""
        if self.planetrack and self.node:
            self.planetrack.set(self.node[-1])
        if (len(self.node) == 1):
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")
        elif (len(self.node) == 2):
            last = self.node[len(self.node)-2]
            newseg = Part.LineSegment(last,point).toShape()
            self.obj.Shape = newseg
            self.obj.ViewObject.Visibility = True
            if self.isWire:
                FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")
        else:
            currentshape = self.obj.Shape.copy()
            last = self.node[len(self.node)-2]
            if not DraftVecUtils.equals(last,point):
                newseg = Part.LineSegment(last,point).toShape()
                newshape=currentshape.fuse(newseg)
                self.obj.Shape = newshape
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")

    def wipe(self):
        """removes all previous segments and starts from last point"""
        if len(self.node) > 1:
            # self.obj.Shape.nullify() - for some reason this fails
            self.obj.ViewObject.Visibility = False
            self.node = [self.node[-1]]
            if self.planetrack:
                self.planetrack.set(self.node[0])
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")

    def orientWP(self):
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            if (len(self.node) > 1) and self.obj:
                import DraftGeomUtils
                n = DraftGeomUtils.getNormal(self.obj.Shape)
                if not n:
                    n = FreeCAD.DraftWorkingPlane.axis
                p = self.node[-1]
                v = self.node[-2].sub(self.node[-1])
                v = v.negative()
                if not self.oldWP:
                    self.oldWP = FreeCAD.DraftWorkingPlane.copy()
                FreeCAD.DraftWorkingPlane.alignToPointAndAxis(p,n,upvec=v)
                if hasattr(FreeCADGui,"Snapper"):
                    FreeCADGui.Snapper.setGrid()
                    FreeCADGui.Snapper.restack()
                if self.planetrack:
                    self.planetrack.set(self.node[-1])

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        self.node.append(self.point)
        self.drawSegment(self.point)
        if (not self.isWire and len(self.node) == 2):
            self.finish(False,cont=True)
        self.ui.setNextFocus()

class Wire(Line):
    """a FreeCAD command for creating a wire"""

    def __init__(self):
        Line.__init__(self,wiremode=True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Wire',
                'Accel' : "P, L",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Wire", "Polyline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Wire", "Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain")}

    def Activated(self):

        # allow to convert several Draft Lines to a Wire
        if len(FreeCADGui.Selection.getSelection()) > 1:
            edges = []
            for o in FreeCADGui.Selection.getSelection():
                if Draft.getType(o) != "Wire":
                    edges  = []
                    break
                edges.extend(o.Shape.Edges)
            if edges:
                try:
                    import Part
                    w = Part.Wire(edges)
                except:
                    FreeCAD.Console.PrintError(translate("draft", "Unable to create a Wire from selected objects")+"\n")
                else:
                    pts = ",".join([str(v.Point) for v in w.Vertexes])
                    pts = pts.replace("Vector","FreeCAD.Vector")
                    rems = ["FreeCAD.ActiveDocument.removeObject(\""+o.Name+"\")" for o in FreeCADGui.Selection.getSelection()]
                    FreeCADGui.addModule("Draft")
                    todo.delayCommit([(translate("draft","Convert to Wire"),
                            ['wire = Draft.makeWire(['+pts+'])']+rems+['Draft.autogroup(wire)',
                             'FreeCAD.ActiveDocument.recompute()'])])
                    return

        Line.Activated(self,name=translate("draft","Polyline"))

 
class BSpline(Line):
    """a FreeCAD command for creating a B-spline"""

    def __init__(self):
        Line.__init__(self,wiremode=True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_BSpline',
                'Accel' : "B, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_BSpline", "B-spline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_BSpline", "Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Line.Activated(self,name=translate("draft","BSpline"))
        if self.doc:
            self.bsplinetrack = bsplineTracker()

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
            self.bsplinetrack.update(self.node + [self.point])
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish(False,cont=True)
                else:
                    if (not self.node) and (not self.support):
                        getSupport(arg)
                        self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
                    if self.point:
                        self.ui.redraw()
                        self.pos = arg["Position"]
                        self.node.append(self.point)
                        self.drawUpdate(self.point)
                        if (not self.isWire and len(self.node) == 2):
                            self.finish(False,cont=True)
                        if (len(self.node) > 2):
                            # DNC: allows to close the curve
                            # by placing ends close to each other
                            # with tol = Draft tolerance
                            # old code has been to insensitive
                            if ((self.point-self.node[0]).Length < Draft.tolerance()):
                                self.undolast()
                                self.finish(True,cont=True)
                                FreeCAD.Console.PrintMessage(translate("draft", "Spline has been closed")+"\n")

    def undolast(self):
        """undoes last line segment"""
        if (len(self.node) > 1):
            self.node.pop()
            self.bsplinetrack.update(self.node)
            spline = Part.BSplineCurve()
            spline.interpolate(self.node, False)
            self.obj.Shape = spline.toShape()
            FreeCAD.Console.PrintMessage(translate("draft", "Last point has been removed")+"\n")

    def drawUpdate(self,point):
        if (len(self.node) == 1):
            self.bsplinetrack.on()
            if self.planetrack:
                self.planetrack.set(self.node[0])
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")
        else:
            spline = Part.BSplineCurve()
            spline.interpolate(self.node, False)
            self.obj.Shape = spline.toShape()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point, or Finish (shift-F) or close (o)")+"\n")

    def finish(self,closed=False,cont=False):
        """terminates the operation and closes the poly if asked"""
        if self.ui:
            self.bsplinetrack.finalize()
        if not Draft.getParam("UiMode",1):
            FreeCADGui.Control.closeDialog()
        if self.obj:
            # remove temporary object, if any
            old = self.obj.Name
            todo.delay(self.doc.removeObject,old)
        if (len(self.node) > 1):
            try:
                # building command string
                rot,sup,pts,fil = self.getStrings()
                FreeCADGui.addModule("Draft")
                self.commit(translate("draft","Create B-spline"),
                            ['points = '+pts,
                             'spline = Draft.makeBSpline(points,closed='+str(closed)+',face='+fil+',support='+sup+')',
                             'Draft.autogroup(spline)',
                             'FreeCAD.ActiveDocument.recompute()'])
            except:
                print("Draft: error delaying commit")
        Creator.finish(self)
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

class BezCurve(Line):
    """a FreeCAD command for creating a Bezier Curve"""

    def __init__(self):
        Line.__init__(self,wiremode=True)
        self.degree = None

    def GetResources(self):
        return {'Pixmap'  : 'Draft_BezCurve',
                'Accel' : "B, Z",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_BezCurve", "BezCurve"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_BezCurve", "Creates a Bezier curve. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Line.Activated(self,name=translate("draft","BezCurve"))
        if self.doc:
            self.bezcurvetrack = bezcurveTracker()

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
            self.bezcurvetrack.update(self.node + [self.point],degree=self.degree)                 #existing points + this pointer position
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):       #left click
                if (arg["Position"] == self.pos):                               #double click?
                    self.finish(False,cont=True)
                else:
                    if (not self.node) and (not self.support):                  #first point
                        getSupport(arg)
                        self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
                    if self.point:
                        self.ui.redraw()
                        self.pos = arg["Position"]
                        self.node.append(self.point)                            #add point to "clicked list"
                        # sb add a control point, if mod(len(cpoints),2) == 0) then create 2 handle points?
                        self.drawUpdate(self.point)                             #???
                        if (not self.isWire and len(self.node) == 2):
                            self.finish(False,cont=True)
                        if (len(self.node) > 2):                                #does this make sense for a BCurve?
                            # DNC: allows to close the curve
                            # by placing ends close to each other
                            # with tol = Draft tolerance
                            # old code has been to insensitive
                            if ((self.point-self.node[0]).Length < Draft.tolerance()):
                                self.undolast()
                                self.finish(True,cont=True)
                                FreeCAD.Console.PrintMessage(translate("draft", "Bezier curve has been closed")+"\n")

    def undolast(self):
        """undoes last line segment"""
        if (len(self.node) > 1):
            self.node.pop()
            self.bezcurvetrack.update(self.node,degree=self.degree)
            self.obj.Shape = self.updateShape(self.node)
            FreeCAD.Console.PrintMessage(translate("draft", "Last point has been removed")+"\n")

    def drawUpdate(self,point):
        if (len(self.node) == 1):
            self.bezcurvetrack.on()
            if self.planetrack:
                self.planetrack.set(self.node[0])
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point")+"\n")
        else:
            self.obj.Shape = self.updateShape(self.node)
            FreeCAD.Console.PrintMessage(translate("draft", "Pick next point, or Finish (shift-F) or close (o)")+"\n")

    def updateShape(self, pts):
        '''creates shape for display during creation process.'''
        edges = []
        if len(pts) >= 2: #allow lower degree segment
            poles=pts[1:]
        else:
            poles=[]
        if self.degree:
            segpoleslst = [poles[x:x+self.degree] for x in range(0, len(poles), (self.degree or 1))]
        else:
            segpoleslst = [pts]
        startpoint=pts[0]
        for segpoles in segpoleslst:
            c = Part.BezierCurve() #last segment may have lower degree
            c.increase(len(segpoles))
            c.setPoles([startpoint]+segpoles)
            edges.append(Part.Edge(c))
            startpoint = segpoles[-1]
        w = Part.Wire(edges)
        return(w)

    def finish(self,closed=False,cont=False):
        """terminates the operation and closes the poly if asked"""
        if self.ui:
            if hasattr(self,"bezcurvetrack"):
                self.bezcurvetrack.finalize()
        if not Draft.getParam("UiMode",1):
            FreeCADGui.Control.closeDialog()
        if self.obj:
            # remove temporary object, if any
            old = self.obj.Name
            todo.delay(self.doc.removeObject,old)
        if (len(self.node) > 1):
            try:
                # building command string
                rot,sup,pts,fil = self.getStrings()
                FreeCADGui.addModule("Draft")
                self.commit(translate("draft","Create BezCurve"),
                            ['points = '+pts,
                             'bez = Draft.makeBezCurve(points,closed='+str(closed)+',support='+sup+',degree='+str(self.degree)+')',
                             'Draft.autogroup(bez)'])
            except:
                print("Draft: error delaying commit")
        Creator.finish(self)
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

class CubicBezCurve(Line):
    """a FreeCAD command for creating a 3rd degree Bezier Curve"""

    def __init__(self):
        Line.__init__(self,wiremode=True)
        self.degree = 3

    def GetResources(self):
        return {'Pixmap'  : 'Draft_CubicBezCurve',
                #'Accel' : "B, Z",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_CubicBezCurve", "CubicBezCurve"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_CubicBezCurve", "Creates a Cubic Bezier curve \nClick and drag to define control points. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Line.Activated(self,name=translate("draft","CubicBezCurve"))
        if self.doc:
            self.bezcurvetrack = bezcurveTracker()

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
            if (len(self.node)-1) % self.degree == 0 and len(self.node) > 2 :
                    prevctrl = 2 * self.node[-1] - self.point
                    self.bezcurvetrack.update(self.node[0:-2] + [prevctrl] + [self.node[-1]] +[self.point],degree=self.degree)                 #existing points + this pointer position
            else:
                self.bezcurvetrack.update(self.node + [self.point],degree=self.degree)                 #existing points + this pointer position
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):       #left click
                if (arg["Position"] == self.pos):                               #double click?
                    if len(self.node) > 2:
                        self.node = self.node[0:-2]
                    else:
                        self.node = []
                    return
                else:
                    if (not self.node) and (not self.support):                  #first point
                        getSupport(arg)
                        self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
                    if self.point:
                        self.ui.redraw()
                        self.pos = arg["Position"]
                        self.node.append(self.point)                            #add point to "clicked list"
                        # sb add a control point, if mod(len(cpoints),2) == 0) then create 2 handle points?
                        self.drawUpdate(self.point)                             #???
                        if (not self.isWire and len(self.node) == 2):
                            self.finish(False,cont=True)
                        if (len(self.node) > 2):                                #does this make sense for a BCurve?
                            self.node.append(self.point)                            #add point to "clicked list"
                            self.drawUpdate(self.point)
                            # DNC: allows to close the curve
                            # by placing ends close to each other
                            # with tol = Draft tolerance
                            # old code has been to insensitive
                            if ((self.point-self.node[0]).Length < Draft.tolerance()) and len(self.node) >= 4:
                                #self.undolast()
                                self.node=self.node[0:-2]
                                self.node.append(2 * self.node[0] - self.node[1]) #close the curve with a smooth symmetric knot
                                self.finish(True,cont=True)
                                FreeCAD.Console.PrintMessage(translate("draft", "Bezier curve has been closed")+"\n")
            if (arg["State"] == "UP") and (arg["Button"] == "BUTTON1"):       #left click
                if (arg["Position"] == self.pos):                               #double click?
                    self.node = self.node[0:-2]
                    return
                else:
                    if (not self.node) and (not self.support):                  #first point
                        return
                    if self.point:
                        self.ui.redraw()
                        self.pos = arg["Position"]
                        self.node.append(self.point)                            #add point to "clicked list"
                        # sb add a control point, if mod(len(cpoints),2) == 0) then create 2 handle points?
                        self.drawUpdate(self.point)                             #???
                        if (not self.isWire and len(self.node) == 2):
                            self.finish(False,cont=True)
                        if (len(self.node) > 2):                                #does this make sense for a BCurve?
                            self.node[-3] = 2 * self.node[-2] - self.node[-1]
                            self.drawUpdate(self.point)
                            # DNC: allows to close the curve
                            # by placing ends close to each other
                            # with tol = Draft tolerance
                            # old code has been to insensitive

    def undolast(self):
        """undoes last line segment"""
        if (len(self.node) > 1):
            self.node.pop()
            self.bezcurvetrack.update(self.node,degree=self.degree)
            self.obj.Shape = self.updateShape(self.node)
            FreeCAD.Console.PrintMessage(translate("draft", "Last point has been removed")+"\n")


    def drawUpdate(self,point):
        if (len(self.node) == 1):
            self.bezcurvetrack.on()
            if self.planetrack:
                self.planetrack.set(self.node[0])
            FreeCAD.Console.PrintMessage(translate("draft", "Click and drag to define next knot")+"\n")
        elif (len(self.node)-1) % self.degree == 1 and len(self.node) > 2 : #is a knot
            self.obj.Shape = self.updateShape(self.node[:-1])
            FreeCAD.Console.PrintMessage(translate("draft", "Click and drag to define next knot: ESC to Finish or close (o)")+"\n")

    def updateShape(self, pts):
        '''creates shape for display during creation process.'''
# not quite right. draws 1 big bez.  sb segmented
        edges = []

        if len(pts) >= 2: #allow lower degree segment
            poles=pts[1:]
        else:
            poles=[]

        if self.degree:
            segpoleslst = [poles[x:x+self.degree] for x in range(0, len(poles), (self.degree or 1))]
        else:
            segpoleslst = [pts]

        startpoint=pts[0]

        for segpoles in segpoleslst:
            c = Part.BezierCurve() #last segment may have lower degree
            c.increase(len(segpoles))
            c.setPoles([startpoint]+segpoles)
            edges.append(Part.Edge(c))
            startpoint = segpoles[-1]
        w = Part.Wire(edges)
        return(w)

    def finish(self,closed=False,cont=False):
        """terminates the operation and closes the poly if asked"""
        if self.ui:
            if hasattr(self,"bezcurvetrack"):
                self.bezcurvetrack.finalize()
        if not Draft.getParam("UiMode",1):
            FreeCADGui.Control.closeDialog()
        if self.obj:
            # remove temporary object, if any
            old = self.obj.Name
            todo.delay(self.doc.removeObject,old)
        if closed == False :
            cleannd=(len(self.node)-1) % self.degree
            if cleannd == 0 : self.node = self.node[0:-3]
            if cleannd > 0 : self.node = self.node[0:-cleannd]
        if (len(self.node) > 1):
            try:
                # building command string
                rot,sup,pts,fil = self.getStrings()
                FreeCADGui.addModule("Draft")
                self.commit(translate("draft","Create BezCurve"),
                            ['points = '+pts,
                             'bez = Draft.makeBezCurve(points,closed='+str(closed)+',support='+sup+',degree='+str(self.degree)+')',
                             'Draft.autogroup(bez)',
                             'FreeCAD.ActiveDocument.recompute()'])
            except:
                print("Draft: error delaying commit")
        Creator.finish(self)
        if self.ui:
            if self.ui.continueMode:
                self.Activated()


class FinishLine:
    """a FreeCAD command to finish any running Line drawing operation"""

    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.finish(False)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Finish',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_FinishLine", "Finish line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_FinishLine", "Finishes a line without closing it")}

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False


class CloseLine:
    """a FreeCAD command to close any running Line drawing operation"""

    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.finish(True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Lock',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_CloseLine", "Close Line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_CloseLine", "Closes the line being drawn")}

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False


class UndoLine:
    """a FreeCAD command to undo last drawn segment of a line"""

    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.undolast()

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_UndoLine", "Undo last segment"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_UndoLine", "Undoes the last drawn segment of the line being drawn")}

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False


class Rectangle(Creator):
    """the Draft_Rectangle FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rectangle',
                'Accel' : "R, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Rectangle", "Rectangle"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Rectangle", "Creates a 2-point rectangle. CTRL to snap")}

    def Activated(self):
        name = translate("draft","Rectangle")
        Creator.Activated(self,name)
        if self.ui:
            self.refpoint = None
            self.ui.pointUi(name)
            self.ui.extUi()
            if Draft.getParam("UsePartPrimitives",False):
                self.fillstate = self.ui.hasFill.isChecked()
                self.ui.hasFill.setChecked(True)
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.rect = rectangleTracker()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick first point")+"\n")

    def finish(self,closed=False,cont=False):
        """terminates the operation and closes the poly if asked"""
        Creator.finish(self)
        if self.ui:
            if hasattr(self,"fillstate"):
                self.ui.hasFill.setChecked(self.fillstate)
                del self.fillstate
            self.rect.off()
            self.rect.finalize()
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        """creates the final object in the current doc"""
        p1 = self.node[0]
        p3 = self.node[-1]
        diagonal = p3.sub(p1)
        p2 = p1.add(DraftVecUtils.project(diagonal, plane.v))
        p4 = p1.add(DraftVecUtils.project(diagonal, plane.u))
        length = p4.sub(p1).Length
        if abs(DraftVecUtils.angle(p4.sub(p1),plane.u,plane.axis)) > 1: length = -length
        height = p2.sub(p1).Length
        if abs(DraftVecUtils.angle(p2.sub(p1),plane.v,plane.axis)) > 1: height = -height
        try:
            # building command string
            rot,sup,pts,fil = self.getStrings()
            base = p1
            if length < 0:
                length = -length
                base = base.add((p1.sub(p4)).negative())
            if height < 0:
                height = -height
                base = base.add((p1.sub(p2)).negative())
            FreeCADGui.addModule("Draft")
            if Draft.getParam("UsePartPrimitives",False):
                # Use Part Primitive
                self.commit(translate("draft","Create Plane"),
                            ['plane = FreeCAD.ActiveDocument.addObject("Part::Plane","Plane")',
                             'plane.Length = '+str(length),
                             'plane.Width = '+str(height),
                             'pl = FreeCAD.Placement()',
                             'pl.Rotation.Q='+rot,
                             'pl.Base = '+DraftVecUtils.toString(base),
                             'plane.Placement = pl',
                             'Draft.autogroup(plane)',
                             'FreeCAD.ActiveDocument.recompute()'])
            else:
                self.commit(translate("draft","Create Rectangle"),
                            ['pl = FreeCAD.Placement()',
                             'pl.Rotation.Q = '+rot,
                             'pl.Base = '+DraftVecUtils.toString(base),
                             'rec = Draft.makeRectangle(length='+str(length)+',height='+str(height)+',placement=pl,face='+fil+',support='+sup+')',
                             'Draft.autogroup(rec)',
                             'FreeCAD.ActiveDocument.recompute()'])
        except:
            print("Draft: error delaying commit")
        self.finish(cont=True)

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg,mobile=True,noTracker=True)
            self.rect.update(self.point)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish()
                else:
                    if (not self.node) and (not self.support):
                        getSupport(arg)
                        self.point,ctrlPoint,info = getPoint(self,arg,mobile=True,noTracker=True)
                    if self.point:
                        self.ui.redraw()
                        self.appendPoint(self.point)

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        self.appendPoint(self.point)

    def appendPoint(self,point):
        self.node.append(point)
        if (len(self.node) > 1):
            self.rect.update(point)
            self.createObject()
        else:
            FreeCAD.Console.PrintMessage(translate("draft", "Pick opposite point")+"\n")
            self.ui.setRelative()
            self.rect.setorigin(point)
            self.rect.on()
            if self.planetrack:
                self.planetrack.set(point)


class Arc(Creator):
    """the Draft_Arc FreeCAD command definition"""

    def __init__(self):
        self.closedCircle=False
        self.featureName = "Arc"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Arc',
                'Accel' : "A, R",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Arc", "Arc"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Arc", "Creates an arc by center point and radius. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Creator.Activated(self,self.featureName)
        if self.ui:
            self.step = 0
            self.center = None
            self.rad = None
            self.angle = 0 # angle inscribed by arc
            self.tangents = []
            self.tanpoints = []
            if self.featureName == "Arc": self.ui.arcUi()
            else: self.ui.circleUi()
            self.altdown = False
            self.ui.sourceCmd = self
            self.linetrack = lineTracker(dotted=True)
            self.arctrack = arcTracker()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            FreeCAD.Console.PrintMessage(translate("draft", "Pick center point")+"\n")

    def finish(self,closed=False,cont=False):
        """finishes the arc"""
        Creator.finish(self)
        if self.ui:
            self.linetrack.finalize()
            self.arctrack.finalize()
            self.doc.recompute()
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

    def updateAngle(self, angle):
        # previous absolute angle
        lastangle = self.firstangle + self.angle
        if lastangle <= -2*math.pi: lastangle += 2*math.pi
        if lastangle >= 2*math.pi: lastangle -= 2*math.pi
        # compute delta = change in angle:
        d0 = angle-lastangle
        d1 = d0 + 2*math.pi
        d2 = d0 - 2*math.pi
        if abs(d0) < min(abs(d1), abs(d2)):
            delta = d0
        elif abs(d1) < abs(d2):
            delta = d1
        else:
            delta = d2
        newangle = self.angle + delta
        # normalize angle, preserving direction
        if newangle >= 2*math.pi: newangle -= 2*math.pi
        if newangle <= -2*math.pi: newangle += 2*math.pi
        self.angle = newangle

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point,ctrlPoint,info = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            if self.center and DraftVecUtils.dist(self.point,self.center) > 0:
                viewdelta = DraftVecUtils.project(self.point.sub(self.center), plane.axis)
                if not DraftVecUtils.isNull(viewdelta):
                    self.point = self.point.add(viewdelta.negative())
            if (self.step == 0): # choose center
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.altdown = True
                        self.ui.switchUi(True)
                    else:
                        if self.altdown:
                            self.altdown = False
                            self.ui.switchUi(False)
            elif (self.step == 1): # choose radius
                if len(self.tangents) == 2:
                    cir = DraftGeomUtils.circleFrom2tan1pt(self.tangents[0], self.tangents[1], self.point)
                    self.center = DraftGeomUtils.findClosestCircle(self.point,cir).Center
                    self.arctrack.setCenter(self.center)
                elif self.tangents and self.tanpoints:
                    cir = DraftGeomUtils.circleFrom1tan2pt(self.tangents[0], self.tanpoints[0], self.point)
                    self.center = DraftGeomUtils.findClosestCircle(self.point,cir).Center
                    self.arctrack.setCenter(self.center)
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.altdown = True
                    if info:
                        ob = self.doc.getObject(info['Object'])
                        num = int(info['Component'].lstrip('Edge'))-1
                        ed = ob.Shape.Edges[num]
                        if len(self.tangents) == 2:
                            cir = DraftGeomUtils.circleFrom3tan(self.tangents[0], self.tangents[1], ed)
                            cl = DraftGeomUtils.findClosestCircle(self.point,cir)
                            self.center = cl.Center
                            self.rad = cl.Radius
                            self.arctrack.setCenter(self.center)
                        else:
                            self.rad = self.center.add(DraftGeomUtils.findDistance(self.center,ed).sub(self.center)).Length
                    else:
                        self.rad = DraftVecUtils.dist(self.point,self.center)
                else:
                    if self.altdown:
                        self.altdown = False
                    self.rad = DraftVecUtils.dist(self.point,self.center)
                self.ui.setRadiusValue(self.rad, "Length")
                self.arctrack.setRadius(self.rad)
                self.linetrack.p1(self.center)
                self.linetrack.p2(self.point)
                self.linetrack.on()
            elif (self.step == 2): # choose first angle
                currentrad = DraftVecUtils.dist(self.point,self.center)
                if currentrad != 0:
                    angle = DraftVecUtils.angle(plane.u, self.point.sub(self.center), plane.axis)
                else: angle = 0
                self.linetrack.p2(DraftVecUtils.scaleTo(self.point.sub(self.center),self.rad).add(self.center))
                self.ui.setRadiusValue(math.degrees(angle),unit="Angle")
                self.firstangle = angle
            else: # choose second angle
                currentrad = DraftVecUtils.dist(self.point,self.center)
                if currentrad != 0:
                    angle = DraftVecUtils.angle(plane.u, self.point.sub(self.center), plane.axis)
                else: angle = 0
                self.linetrack.p2(DraftVecUtils.scaleTo(self.point.sub(self.center),self.rad).add(self.center))
                self.updateAngle(angle)
                self.ui.setRadiusValue(math.degrees(self.angle),unit="Angle")
                self.arctrack.setApertureAngle(self.angle)

            redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.step == 0): # choose center
                        if not self.support:
                            getSupport(arg)
                            self.point,ctrlPoint,info = getPoint(self,arg)
                        if hasMod(arg,MODALT):
                            snapped=self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                            if snapped:
                                ob = self.doc.getObject(snapped['Object'])
                                num = int(snapped['Component'].lstrip('Edge'))-1
                                ed = ob.Shape.Edges[num]
                                self.tangents.append(ed)
                                if len(self.tangents) == 2:
                                    self.arctrack.on()
                                    self.ui.radiusUi()
                                    self.step = 1
                                    self.ui.setNextFocus()
                                    self.linetrack.on()
                                    FreeCAD.Console.PrintMessage(translate("draft", "Pick radius")+"\n")
                        else:
                            if len(self.tangents) == 1:
                                self.tanpoints.append(self.point)
                            else:
                                self.center = self.point
                                self.node = [self.point]
                                self.arctrack.setCenter(self.center)
                                self.linetrack.p1(self.center)
                                self.linetrack.p2(self.view.getPoint(arg["Position"][0],arg["Position"][1]))
                            self.arctrack.on()
                            self.ui.radiusUi()
                            self.step = 1
                            self.ui.setNextFocus()
                            self.linetrack.on()
                            FreeCAD.Console.PrintMessage(translate("draft", "Pick radius")+"\n")
                            if self.planetrack:
                                self.planetrack.set(self.point)
                    elif (self.step == 1): # choose radius
                        if self.closedCircle:
                            self.drawArc()
                        else:
                            self.ui.labelRadius.setText(translate("draft","Start angle"))
                            self.ui.radiusValue.setToolTip(translate("draft","Start angle"))
                            self.ui.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Angle).UserString)
                            self.linetrack.p1(self.center)
                            self.linetrack.on()
                            self.step = 2
                            FreeCAD.Console.PrintMessage(translate("draft", "Pick start angle")+"\n")
                    elif (self.step == 2): # choose first angle
                        self.ui.labelRadius.setText(translate("draft","Aperture angle"))
                        self.ui.radiusValue.setToolTip(translate("draft","Aperture angle"))
                        self.step = 3
                        # scale center->point vector for proper display
                        # u = DraftVecUtils.scaleTo(self.point.sub(self.center), self.rad) obsolete?
                        self.arctrack.setStartAngle(self.firstangle)
                        FreeCAD.Console.PrintMessage(translate("draft", "Pick aperture")+"\n")
                    else: # choose second angle
                        self.step = 4
                        self.drawArc()

    def drawArc(self):
        """actually draws the FreeCAD object"""
        rot,sup,pts,fil = self.getStrings()
        if self.closedCircle:
            try:
                FreeCADGui.addModule("Draft")
                if Draft.getParam("UsePartPrimitives",False):
                    # use primitive
                    self.commit(translate("draft","Create Circle"),
                                ['circle = FreeCAD.ActiveDocument.addObject("Part::Circle","Circle")',
                                 'circle.Radius = '+str(self.rad),
                                 'pl = FreeCAD.Placement()',
                                 'pl.Rotation.Q = '+rot,
                                 'pl.Base = '+DraftVecUtils.toString(self.center),
                                 'circle.Placement = pl',
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()'])
                else:
                    # building command string
                    FreeCADGui.addModule("Draft")
                    self.commit(translate("draft","Create Circle"),
                                ['pl=FreeCAD.Placement()',
                                 'pl.Rotation.Q='+rot,
                                 'pl.Base='+DraftVecUtils.toString(self.center),
                                 'circle = Draft.makeCircle(radius='+str(self.rad)+',placement=pl,face='+fil+',support='+sup+')',
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()'])
            except:
                print("Draft: error delaying commit")
        else:
            sta = math.degrees(self.firstangle)
            end = math.degrees(self.firstangle+self.angle)
            if end < sta: sta,end = end,sta
            while True:
                if sta > 360:
                    sta = sta - 360
                elif end > 360:
                    end = end - 360
                else:
                    break
            try:
                FreeCADGui.addModule("Draft")
                if Draft.getParam("UsePartPrimitives",False):
                    # use primitive
                    self.commit(translate("draft","Create Arc"),
                                ['circle = FreeCAD.ActiveDocument.addObject("Part::Circle","Circle")',
                                 'circle.Radius = '+str(self.rad),
                                 'circle.Angle0 = '+str(sta),
                                 'circle.Angle1 = '+str(end),
                                 'pl = FreeCAD.Placement()',
                                 'pl.Rotation.Q = '+rot,
                                 'pl.Base = '+DraftVecUtils.toString(self.center),
                                 'circle.Placement = pl',
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()'])
                else:
                    # building command string
                    self.commit(translate("draft","Create Arc"),
                                ['pl=FreeCAD.Placement()',
                                 'pl.Rotation.Q='+rot,
                                 'pl.Base='+DraftVecUtils.toString(self.center),
                                 'circle = Draft.makeCircle(radius='+str(self.rad)+',placement=pl,face='+fil+',startangle='+str(sta)+',endangle='+str(end)+',support='+sup+')',
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()'])
            except:
                    print("Draft: error delaying commit")
        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.setNextFocus()
        FreeCAD.Console.PrintMessage(translate("draft", "Pick radius")+"\n")

    def numericRadius(self,rad):
        """this function gets called by the toolbar when valid radius have been entered there"""
        if (self.step == 1):
            self.rad = rad
            if len(self.tangents) == 2:
                cir = DraftGeomUtils.circleFrom2tan1rad(self.tangents[0], self.tangents[1], rad)
                if self.center:
                    self.center = DraftGeomUtils.findClosestCircle(self.center,cir).Center
                else:
                    self.center = cir[-1].Center
            elif self.tangents and self.tanpoints:
                cir = DraftGeomUtils.circleFrom1tan1pt1rad(self.tangents[0],self.tanpoints[0],rad)
                if self.center:
                    self.center = DraftGeomUtils.findClosestCircle(self.center,cir).Center
                else:
                    self.center = cir[-1].Center
            if self.closedCircle:
                self.drawArc()
            else:
                self.step = 2
                self.arctrack.setCenter(self.center)
                self.ui.labelRadius.setText(translate("draft", "Start angle"))
                self.ui.radiusValue.setToolTip(translate("draft", "Start angle"))
                self.linetrack.p1(self.center)
                self.linetrack.on()
                self.ui.radiusValue.setText("")
                self.ui.radiusValue.setFocus()
                FreeCAD.Console.PrintMessage(translate("draft", "Pick start angle")+"\n")
        elif (self.step == 2):
            self.ui.labelRadius.setText(translate("draft", "Aperture angle"))
            self.ui.radiusValue.setToolTip(translate("draft", "Aperture angle"))
            self.firstangle = math.radians(rad)
            if DraftVecUtils.equals(plane.axis, Vector(1,0,0)): u = Vector(0,self.rad,0)
            else: u = DraftVecUtils.scaleTo(Vector(1,0,0).cross(plane.axis), self.rad)
            urotated = DraftVecUtils.rotate(u, math.radians(rad), plane.axis)
            self.arctrack.setStartAngle(self.firstangle)
            self.step = 3
            self.ui.radiusValue.setText("")
            self.ui.radiusValue.setFocus()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick aperture angle")+"\n")
        else:
            self.updateAngle(rad)
            self.angle = math.radians(rad)
            self.step = 4
            self.drawArc()


class Circle(Arc):
    """The Draft_Circle FreeCAD command definition"""

    def __init__(self):
        self.closedCircle=True
        self.featureName = "Circle"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Circle',
                'Accel' : "C, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Circle", "Circle"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Circle", "Creates a circle. CTRL to snap, ALT to select tangent objects")}


class Polygon(Creator):
    """the Draft_Polygon FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Polygon',
                'Accel' : "P, G",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Polygon", "Polygon"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Polygon", "Creates a regular polygon. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        name = translate("draft","Polygon")
        Creator.Activated(self,name)
        if self.ui:
            self.step = 0
            self.center = None
            self.rad = None
            self.tangents = []
            self.tanpoints = []
            self.ui.pointUi(name)
            self.ui.extUi()
            self.ui.numFaces.show()
            self.ui.numFacesLabel.show()
            self.altdown = False
            self.ui.sourceCmd = self
            self.arctrack = arcTracker()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            FreeCAD.Console.PrintMessage(translate("draft", "Pick center point")+"\n")

    def finish(self,closed=False,cont=False):
        """finishes the arc"""
        Creator.finish(self)
        if self.ui:
            self.arctrack.finalize()
            self.doc.recompute()
            if self.ui.continueMode:
                self.Activated()

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point,ctrlPoint,info = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            if self.center and DraftVecUtils.dist(self.point,self.center) > 0:
                viewdelta = DraftVecUtils.project(self.point.sub(self.center), plane.axis)
                if not DraftVecUtils.isNull(viewdelta):
                    self.point = self.point.add(viewdelta.negative())
            if (self.step == 0): # choose center
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.altdown = True
                        self.ui.switchUi(True)
                else:
                    if self.altdown:
                        self.altdown = False
                        self.ui.switchUi(False)
            else: # choose radius
                if len(self.tangents) == 2:
                    cir = DraftGeomUtils.circleFrom2tan1pt(self.tangents[0], self.tangents[1], self.point)
                    self.center = DraftGeomUtils.findClosestCircle(self.point,cir).Center
                    self.arctrack.setCenter(self.center)
                elif self.tangents and self.tanpoints:
                    cir = DraftGeomUtils.circleFrom1tan2pt(self.tangents[0], self.tanpoints[0], self.point)
                    self.center = DraftGeomUtils.findClosestCircle(self.point,cir).Center
                    self.arctrack.setCenter(self.center)
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.altdown = True
                    snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                    if snapped:
                        ob = self.doc.getObject(snapped['Object'])
                        num = int(snapped['Component'].lstrip('Edge'))-1
                        ed = ob.Shape.Edges[num]
                        if len(self.tangents) == 2:
                            cir = DraftGeomUtils.circleFrom3tan(self.tangents[0], self.tangents[1], ed)
                            cl = DraftGeomUtils.findClosestCircle(self.point,cir)
                            self.center = cl.Center
                            self.rad = cl.Radius
                            self.arctrack.setCenter(self.center)
                        else:
                            self.rad = self.center.add(DraftGeomUtils.findDistance(self.center,ed).sub(self.center)).Length
                    else:
                        self.rad = DraftVecUtils.dist(self.point,self.center)
                else:
                    if self.altdown:
                        self.altdown = False
                    self.rad = DraftVecUtils.dist(self.point,self.center)
                self.ui.setRadiusValue(self.rad,'Length')
                self.arctrack.setRadius(self.rad)

            redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.step == 0): # choose center
                        if (not self.node) and (not self.support):
                            getSupport(arg)
                            self.point,ctrlPoint,info = getPoint(self,arg)
                        if hasMod(arg,MODALT):
                            snapped=self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                            if snapped:
                                ob = self.doc.getObject(snapped['Object'])
                                num = int(snapped['Component'].lstrip('Edge'))-1
                                ed = ob.Shape.Edges[num]
                                self.tangents.append(ed)
                                if len(self.tangents) == 2:
                                    self.arctrack.on()
                                    self.ui.radiusUi()
                                    self.step = 1
                                    FreeCAD.Console.PrintMessage(translate("draft", "Pick radius")+"\n")
                        else:
                            if len(self.tangents) == 1:
                                self.tanpoints.append(self.point)
                            else:
                                self.center = self.point
                                self.node = [self.point]
                                self.arctrack.setCenter(self.center)
                            self.arctrack.on()
                            self.ui.radiusUi()
                            self.step = 1
                            FreeCAD.Console.PrintMessage(translate("draft", "Pick radius")+"\n")
                            if self.planetrack:
                                self.planetrack.set(self.point)
                    elif (self.step == 1): # choose radius
                        self.drawPolygon()

    def drawPolygon(self):
        """actually draws the FreeCAD object"""
        rot,sup,pts,fil = self.getStrings()
        FreeCADGui.addModule("Draft")
        if Draft.getParam("UsePartPrimitives",False):
            FreeCADGui.addModule("Part")
            self.commit(translate("draft","Create Polygon"),
                        ['pl=FreeCAD.Placement()',
                         'pl.Rotation.Q=' + rot,
                         'pl.Base=' + DraftVecUtils.toString(self.center),
                         'pol = FreeCAD.ActiveDocument.addObject("Part::RegularPolygon","RegularPolygon")',
                         'pol.Polygon = ' + str(self.ui.numFaces.value()),
                         'pol.Circumradius = ' + str(self.rad),
                         'pol.Placement = pl',
                         'Draft.autogroup(pol)',
                         'FreeCAD.ActiveDocument.recompute()'])
        else:
            # building command string
            self.commit(translate("draft","Create Polygon"),
                        ['pl=FreeCAD.Placement()',
                         'pl.Rotation.Q = ' + rot,
                         'pl.Base = ' + DraftVecUtils.toString(self.center),
                         'pol = Draft.makePolygon(' + str(self.ui.numFaces.value()) + ',radius=' + str(self.rad) + ',inscribed=True,placement=pl,face=' + fil + ',support=' + sup + ')',
                         'Draft.autogroup(pol)',
                         'FreeCAD.ActiveDocument.recompute()'])
        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.radiusValue.setFocus()
        FreeCAD.Console.PrintMessage(translate("draft", "Pick radius")+"\n")

    def numericRadius(self,rad):
        """this function gets called by the toolbar when valid radius have been entered there"""
        self.rad = rad
        if len(self.tangents) == 2:
            cir = DraftGeomUtils.circleFrom2tan1rad(self.tangents[0], self.tangents[1], rad)
            if self.center:
                self.center = DraftGeomUtils.findClosestCircle(self.center,cir).Center
            else:
                self.center = cir[-1].Center
        elif self.tangents and self.tanpoints:
            cir = DraftGeomUtils.circleFrom1tan1pt1rad(self.tangents[0],self.tanpoints[0],rad)
            if self.center:
                self.center = DraftGeomUtils.findClosestCircle(self.center,cir).Center
            else:
                self.center = cir[-1].Center
        self.drawPolygon()


class Ellipse(Creator):
    """the Draft_Ellipse FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Ellipse',
                'Accel' : "E, L",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Ellipse", "Ellipse"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Ellipse", "Creates an ellipse. CTRL to snap")}

    def Activated(self):
        name = translate("draft","Ellipse")
        Creator.Activated(self,name)
        if self.ui:
            self.refpoint = None
            self.ui.pointUi(name)
            self.ui.extUi()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.rect = rectangleTracker()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick first point")+"\n")

    def finish(self,closed=False,cont=False):
        """terminates the operation and closes the poly if asked"""
        Creator.finish(self)
        if self.ui:
            self.rect.off()
            self.rect.finalize()
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        """creates the final object in the current doc"""
        p1 = self.node[0]
        p3 = self.node[-1]
        diagonal = p3.sub(p1)
        halfdiag = Vector(diagonal).multiply(0.5)
        center = p1.add(halfdiag)
        p2 = p1.add(DraftVecUtils.project(diagonal, plane.v))
        p4 = p1.add(DraftVecUtils.project(diagonal, plane.u))
        r1 = (p4.sub(p1).Length)/2
        r2 = (p2.sub(p1).Length)/2
        try:
            # building command string
            rot,sup,pts,fil = self.getStrings()
            if r2 > r1:
                r1,r2 = r2,r1
                m = FreeCAD.Matrix()
                m.rotateZ(math.pi/2)
                rot1 = FreeCAD.Rotation()
                rot1.Q = eval(rot)
                rot2 = FreeCAD.Placement(m)
                rot2 = rot2.Rotation
                rot = str((rot1.multiply(rot2)).Q)
            FreeCADGui.addModule("Draft")
            if Draft.getParam("UsePartPrimitives",False):
                # Use Part Primitive
                self.commit(translate("draft","Create Ellipse"),
                            ['import Part',
                             'ellipse = FreeCAD.ActiveDocument.addObject("Part::Ellipse","Ellipse")',
                             'ellipse.MajorRadius = '+str(r1),
                             'ellipse.MinorRadius = '+str(r2),
                             'pl = FreeCAD.Placement()',
                             'pl.Rotation.Q='+rot,
                             'pl.Base = '+DraftVecUtils.toString(center),
                             'ellipse.Placement = pl',
                             'Draft.autogroup(ellipse)',
                             'FreeCAD.ActiveDocument.recompute()'])
            else:
                self.commit(translate("draft","Create Ellipse"),
                            ['pl = FreeCAD.Placement()',
                             'pl.Rotation.Q = '+rot,
                             'pl.Base = '+DraftVecUtils.toString(center),
                             'ellipse = Draft.makeEllipse('+str(r1)+','+str(r2)+',placement=pl,face='+fil+',support='+sup+')',
                             'Draft.autogroup(ellipse)',
                             'FreeCAD.ActiveDocument.recompute()'])
        except:
            print("Draft: Error: Unable to create object.")
        self.finish(cont=True)

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg,mobile=True,noTracker=True)
            self.rect.update(self.point)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish()
                else:
                    if (not self.node) and (not self.support):
                        getSupport(arg)
                        self.point,ctrlPoint,info = getPoint(self,arg,mobile=True,noTracker=True)
                    if self.point:
                        self.ui.redraw()
                        self.appendPoint(self.point)

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        self.appendPoint(self.point)

    def appendPoint(self,point):
        self.node.append(point)
        if (len(self.node) > 1):
            self.rect.update(point)
            self.createObject()
        else:
            FreeCAD.Console.PrintMessage(translate("draft", "Pick opposite point")+"\n")
            self.ui.setRelative()
            self.rect.setorigin(point)
            self.rect.on()
            if self.planetrack:
                self.planetrack.set(point)


class Text(Creator):
    """This class creates an annotation feature."""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Text',
                'Accel' : "T, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Text", "Text"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Text", "Creates an annotation. CTRL to snap")}

    def Activated(self):
        name = translate("draft","Text")
        Creator.Activated(self,name)
        if self.ui:
            self.dialog = None
            self.text = ''
            self.ui.sourceCmd = self
            self.ui.pointUi(name)
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.active = True
            self.ui.xValue.setFocus()
            self.ui.xValue.selectAll()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick location point")+"\n")
            FreeCADGui.draftToolBar.show()

    def finish(self,closed=False,cont=False):
        """terminates the operation"""
        Creator.finish(self)
        if self.ui:
            del self.dialog
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        """creates an object in the current doc"""
        tx = '['
        for l in self.text:
            if len(tx) > 1:
                tx += ','
            if sys.version_info.major < 3:
                l = unicode(l)
                tx += '"'+str(l.encode("utf8"))+'"'
            else:
                tx += '"'+l+'"' #Python3 no more unicode
        tx += ']'
        FreeCADGui.addModule("Draft")
        self.commit(translate("draft","Create Text"),
                    ['text = Draft.makeText('+tx+',point='+DraftVecUtils.toString(self.node[0])+')',
                    'Draft.autogroup(text)',
                    'FreeCAD.ActiveDocument.recompute()'])
        FreeCAD.ActiveDocument.recompute()

        self.finish(cont=True)

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            if self.active:
                self.point,ctrlPoint,info = getPoint(self,arg)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    self.active = False
                    FreeCADGui.Snapper.off()
                    self.node.append(self.point)
                    self.ui.textUi()
                    self.ui.textValue.setFocus()

    def numericInput(self,numx,numy,numz):
        '''this function gets called by the toolbar when valid
        x, y, and z have been entered there'''
        self.point = Vector(numx,numy,numz)
        self.node.append(self.point)
        self.ui.textUi()
        self.ui.textValue.setFocus()


class Dimension(Creator):
    """The Draft_Dimension FreeCAD command definition"""

    def __init__(self):
        self.max=2
        self.cont = None
        self.dir = None

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Dimension',
                'Accel' : "D, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Dimension", "Dimension"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Dimension", "Creates a dimension. CTRL to snap, SHIFT to constrain, ALT to select a segment")}

    def Activated(self):
        name = translate("draft","Dimension")
        if self.cont:
            self.finish()
        elif self.hasMeasures():
            Creator.Activated(self,name)
            self.dimtrack = dimTracker()
            self.arctrack = arcTracker()
            self.createOnMeasures()
            self.finish()
        else:
            Creator.Activated(self,name)
            if self.ui:
                self.ui.pointUi(name)
                self.ui.continueCmd.show()
                self.ui.selectButton.show()
                self.altdown = False
                self.call = self.view.addEventCallback("SoEvent",self.action)
                self.dimtrack = dimTracker()
                self.arctrack = arcTracker()
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
                FreeCAD.Console.PrintMessage(translate("draft", "Pick first point")+"\n")
                FreeCADGui.draftToolBar.show()

    def setFromSelection(self):
        """If we already have selected geometry, fill the nodes accordingly"""
        sel = FreeCADGui.Selection.getSelectionEx()
        import DraftGeomUtils
        if len(sel) == 1:
            if len(sel[0].SubElementNames) == 1:
                if "Edge" in sel[0].SubElementNames[0]:
                    edge = sel[0].SubObjects[0]
                    n = int(sel[0].SubElementNames[0].lstrip("Edge"))-1
                    self.indices.append(n)
                    if DraftGeomUtils.geomType(edge) == "Line":
                        self.node.extend([edge.Vertexes[0].Point,edge.Vertexes[1].Point])
                        v1 = None
                        v2 =None
                        for i,v in enumerate(sel[0].Object.Shape.Vertexes):
                            if v.Point == edge.Vertexes[0].Point:
                                v1 = i
                            if v.Point == edge.Vertexes[1].Point:
                                v2 = i
                        if (v1 != None) and (v2 != None):
                            self.link = [sel[0].Object,v1,v2]
                    elif DraftGeomUtils.geomType(edge) == "Circle":
                        self.node.extend([edge.Curve.Center,edge.Vertexes[0].Point])
                        self.edges = [edge]
                        self.arcmode = "diameter"
                        self.link = [sel[0].Object,n]

    def hasMeasures(self):
        """checks if only measurements objects are selected"""
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            return False
        for o in sel:
            if not o.isDerivedFrom("App::MeasureDistance"):
                return False
        return True

    def finish(self,closed=False):
        """terminates the operation"""
        self.cont = None
        self.dir = None
        Creator.finish(self)
        if self.ui:
            self.dimtrack.finalize()
            self.arctrack.finalize()

    def createOnMeasures(self):
        for o in FreeCADGui.Selection.getSelection():
            p1 = o.P1
            p2 = o.P2
            pt = o.ViewObject.RootNode.getChildren()[1].getChildren()[0].getChildren()[0].getChildren()[3]
            p3 = Vector(pt.point.getValues()[2].getValue())
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeDimension('+DraftVecUtils.toString(p1)+','+DraftVecUtils.toString(p2)+','+DraftVecUtils.toString(p3)+')',
                         'FreeCAD.ActiveDocument.removeObject("'+o.Name+'")',
                         'Draft.autogroup(dim)',
                         'FreeCAD.ActiveDocument.recompute()'])

    def createObject(self):
        """creates an object in the current doc"""
        FreeCADGui.addModule("Draft")
        if self.angledata:
            normal = "None"
            if len(self.edges) == 2:
                import DraftGeomUtils
                v1 = DraftGeomUtils.vec(self.edges[0])
                v2 = DraftGeomUtils.vec(self.edges[1])
                normal = DraftVecUtils.toString((v1.cross(v2)).normalize())
            self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeAngularDimension(center='+DraftVecUtils.toString(self.center)+',angles=['+str(self.angledata[0])+','+str(self.angledata[1])+'],p3='+DraftVecUtils.toString(self.node[-1])+',normal='+normal+')',
                        'Draft.autogroup(dim)',
                        'FreeCAD.ActiveDocument.recompute()'])
        elif self.link and (not self.arcmode):
            ops = []
            if self.force == 1:
                self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeDimension(FreeCAD.ActiveDocument.'+self.link[0].Name+','+str(self.link[1])+','+str(self.link[2])+','+DraftVecUtils.toString(self.node[2])+')','dim.Direction=FreeCAD.Vector(0,1,0)',
                        'Draft.autogroup(dim)',
                        'FreeCAD.ActiveDocument.recompute()'])
            elif self.force == 2:
                self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeDimension(FreeCAD.ActiveDocument.'+self.link[0].Name+','+str(self.link[1])+','+str(self.link[2])+','+DraftVecUtils.toString(self.node[2])+')','dim.Direction=FreeCAD.Vector(1,0,0)',
                        'Draft.autogroup(dim)',
                        'FreeCAD.ActiveDocument.recompute()'])
            else:
                self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeDimension(FreeCAD.ActiveDocument.'+self.link[0].Name+','+str(self.link[1])+','+str(self.link[2])+','+DraftVecUtils.toString(self.node[2])+')',
                        'Draft.autogroup(dim)',
                        'FreeCAD.ActiveDocument.recompute()'])
        elif self.arcmode:
            self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeDimension(FreeCAD.ActiveDocument.'+self.link[0].Name+','+str(self.link[1])+',"'+str(self.arcmode)+'",'+DraftVecUtils.toString(self.node[2])+')',
                        'Draft.autogroup(dim)',
                        'FreeCAD.ActiveDocument.recompute()'])
        else:
            self.commit(translate("draft","Create Dimension"),
                        ['dim = Draft.makeDimension('+DraftVecUtils.toString(self.node[0])+','+DraftVecUtils.toString(self.node[1])+','+DraftVecUtils.toString(self.node[2])+')',
                        'Draft.autogroup(dim)',
                        'FreeCAD.ActiveDocument.recompute()'])
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
        self.selectmode = not(self.selectmode)

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            import DraftGeomUtils
            shift = hasMod(arg,MODCONSTRAIN)
            if self.arcmode or self.point2:
                setMod(arg,MODCONSTRAIN,False)
            self.point,ctrlPoint,self.info = getPoint(self,arg,noTracker=(len(self.node)>0))
            if (hasMod(arg,MODALT) or self.selectmode) and (len(self.node)<3):
                self.dimtrack.off()
                if not self.altdown:
                    self.altdown = True
                    self.ui.switchUi(True)
                    if hasattr(FreeCADGui,"Snapper"):
                        FreeCADGui.Snapper.setSelectMode(True)
                snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                if snapped:
                    ob = self.doc.getObject(snapped['Object'])
                    if "Edge" in snapped['Component']:
                        num = int(snapped['Component'].lstrip('Edge'))-1
                        ed = ob.Shape.Edges[num]
                        v1 = ed.Vertexes[0].Point
                        v2 = ed.Vertexes[-1].Point
                        self.dimtrack.update([v1,v2,self.cont])
            else:
                if self.node and (len(self.edges) < 2):
                    self.dimtrack.on()
                if len(self.edges) == 2:
                    # angular dimension
                    self.dimtrack.off()
                    r = self.point.sub(self.center)
                    self.arctrack.setRadius(r.Length)
                    a = self.arctrack.getAngle(self.point)
                    pair = DraftGeomUtils.getBoundaryAngles(a,self.pts)
                    if not (pair[0] < a < pair[1]):
                        self.angledata = [4*math.pi-pair[0],2*math.pi-pair[1]]
                    else:
                        self.angledata = [2*math.pi-pair[0],2*math.pi-pair[1]]
                    self.arctrack.setStartAngle(self.angledata[0])
                    self.arctrack.setEndAngle(self.angledata[1])
                if self.altdown:
                    self.altdown = False
                    self.ui.switchUi(False)
                    if hasattr(FreeCADGui,"Snapper"):
                        FreeCADGui.Snapper.setSelectMode(False)
                if self.dir:
                    self.point = self.node[0].add(DraftVecUtils.project(self.point.sub(self.node[0]),self.dir))
                if len(self.node) == 2:
                    if self.arcmode and self.edges:
                        cen = self.edges[0].Curve.Center
                        rad = self.edges[0].Curve.Radius
                        baseray = self.point.sub(cen)
                        v2 = DraftVecUtils.scaleTo(baseray,rad)
                        v1 = v2.negative()
                        if shift:
                            self.node = [cen,cen.add(v2)]
                            self.arcmode = "radius"
                        else:
                            self.node = [cen.add(v1),cen.add(v2)]
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
                            a=abs(self.point.sub(self.node[0]).getAngle(plane.u))
                            if (a > math.pi/4) and (a <= 0.75*math.pi):
                                self.force = 1
                            else:
                                self.force = 2
                        if self.force == 1:
                            self.node[1] = Vector(self.node[0].x,self.node[1].y,self.node[0].z)
                        elif self.force == 2:
                            self.node[1] = Vector(self.node[1].x,self.node[0].y,self.node[0].z)
                else:
                    self.force = None
                    if self.point2 and (len(self.node) > 1):
                        self.node[1] = self.point2
                        self.point2 = None
                # update the dimline
                if self.node and (not self.arcmode):
                    self.dimtrack.update(self.node+[self.point]+[self.cont])
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                import DraftGeomUtils
                if self.point:
                    self.ui.redraw()
                    if (not self.node) and (not self.support):
                        getSupport(arg)
                    if (hasMod(arg,MODALT) or self.selectmode) and (len(self.node)<3):
                        #print("snapped: ",self.info)
                        if self.info:
                            ob = self.doc.getObject(self.info['Object'])
                            if 'Edge' in self.info['Component']:
                                num = int(self.info['Component'].lstrip('Edge'))-1
                                ed = ob.Shape.Edges[num]
                                v1 = ed.Vertexes[0].Point
                                v2 = ed.Vertexes[-1].Point
                                i1 = i2 = None
                                for i in range(len(ob.Shape.Vertexes)):
                                    if v1 == ob.Shape.Vertexes[i].Point:
                                        i1 = i
                                    if v2 == ob.Shape.Vertexes[i].Point:
                                        i2 = i
                                if (i1 != None) and (i2 != None):
                                    self.indices.append(num)
                                    if not self.edges:
                                        # nothing snapped yet, we treat it as normal edge-snapped dimension
                                        self.node = [v1,v2]
                                        self.link = [ob,i1,i2]
                                        self.edges.append(ed)
                                        if DraftGeomUtils.geomType(ed) == "Circle":
                                            # snapped edge is an arc
                                            self.arcmode = "diameter"
                                            self.link = [ob,num]
                                    else:
                                        # there is already a snapped edge, so we start angular dimension
                                        self.edges.append(ed)
                                        self.node.extend([v1,v2]) # self.node now has the 4 endpoints
                                        c = DraftGeomUtils.findIntersection(self.node[0],
                                                                   self.node[1],
                                                                   self.node[2],
                                                                   self.node[3],
                                                                   True,True)
                                        if c:
                                            #print("centers:",c)
                                            self.center = c[0]
                                            self.arctrack.setCenter(self.center)
                                            self.arctrack.on()
                                            for e in self.edges:
                                                for v in e.Vertexes:
                                                    self.pts.append(self.arctrack.getAngle(v.Point))
                                            self.link = [self.link[0],ob]
                                        else:
                                            FreeCAD.Console.PrintMessage(translate("draft", "Edges don't intersect!")+"\n")
                                            self.finish()
                                            return
                                self.dimtrack.on()
                    else:
                        self.node.append(self.point)
                    self.selectmode = False
                    #print("node",self.node)
                    self.dimtrack.update(self.node)
                    if (len(self.node) == 2):
                        self.point2 = self.node[1]
                    if (len(self.node) == 1):
                        self.dimtrack.on()
                        if self.planetrack:
                            self.planetrack.set(self.node[0])
                    elif (len(self.node) == 2) and self.cont:
                        self.node.append(self.cont)
                        self.createObject()
                        if not self.cont:
                            self.finish()
                    elif (len(self.node) == 3):
                        # for unlinked arc mode:
                        # if self.arcmode:
                        #        v = self.node[1].sub(self.node[0])
                        #        v.multiply(0.5)
                        #        cen = self.node[0].add(v)
                        #        self.node = [self.node[0],self.node[1],cen]
                        self.createObject()
                        if not self.cont:
                            self.finish()
                    elif self.angledata:
                        self.node.append(self.point)
                        self.createObject()
                        self.finish()

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        self.node.append(self.point)
        self.dimtrack.update(self.node)
        if (len(self.node) == 1):
            self.dimtrack.on()
        elif (len(self.node) == 3):
            self.createObject()
            if not self.cont:
                self.finish()

class ShapeString(Creator):
    """This class creates a shapestring feature."""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_ShapeString',
                'Accel' : "S, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ShapeString", "Shape from text..."),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ShapeString", "Creates text string in shapes.")}

    def Activated(self):
        name = translate("draft","ShapeString")
        Creator.Activated(self,name)
        self.creator = Creator
        if self.ui:
            self.ui.sourceCmd = self
            self.taskmode = Draft.getParam("UiMode",1)
            if self.taskmode:
                try:
                    del self.task
                except AttributeError:
                    pass
                self.task = DraftGui.ShapeStringTaskPanel()
                self.task.sourceCmd = self
                DraftGui.todo.delay(FreeCADGui.Control.showDialog,self.task)
            else:
                self.dialog = None
                self.text = ''
                self.ui.sourceCmd = self
                self.ui.pointUi(name)
                self.active = True
                self.call = self.view.addEventCallback("SoEvent",self.action)
                self.ssBase = None
                self.ui.xValue.setFocus()
                self.ui.xValue.selectAll()
                FreeCAD.Console.PrintMessage(translate("draft", "Pick ShapeString location point")+"\n")
                FreeCADGui.draftToolBar.show()

    def createObject(self):
        """creates object in the current doc"""
        #print("debug: D_T ShapeString.createObject type(self.SString): "  str(type(self.SString)))

        dquote = '"'
        if sys.version_info.major < 3: # Python3: no more unicode
            String  = 'u' + dquote + self.SString.encode('unicode_escape') + dquote
        else:
            String  = dquote + self.SString + dquote
        Size = str(self.SSSize)                              # numbers are ascii so this should always work
        Tracking = str(self.SSTrack)                         # numbers are ascii so this should always work
        FFile = dquote + self.FFile + dquote
#        print("debug: D_T ShapeString.createObject type(String): "  str(type(String)))
#        print("debug: D_T ShapeString.createObject type(FFile): "  str(type(FFile)))

        try:
            qr,sup,points,fil = self.getStrings()
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Create ShapeString"),
                        ['ss=Draft.makeShapeString(String='+String+',FontFile='+FFile+',Size='+Size+',Tracking='+Tracking+')',
                         'plm=FreeCAD.Placement()',
                         'plm.Base='+DraftVecUtils.toString(self.ssBase),
                         'plm.Rotation.Q='+qr,
                         'ss.Placement=plm',
                         'ss.Support='+sup,
                         'Draft.autogroup(ss)',
                         'FreeCAD.ActiveDocument.recompute()'])
        except Exception as e:
            FreeCAD.Console.PrintError("Draft_ShapeString: error delaying commit\n")
        self.finish()

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            if self.active:
                self.point,ctrlPoint,info = getPoint(self,arg,noTracker=True)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if not self.ssBase:
                    self.ssBase = self.point
                    self.active = False
                    FreeCADGui.Snapper.off()
                    self.ui.SSUi()

    def numericInput(self,numx,numy,numz):
        '''this function gets called by the toolbar when valid
        x, y, and z have been entered there'''
        self.ssBase = Vector(numx,numy,numz)
        self.ui.SSUi()                   #move on to next step in parameter entry

    def numericSSize(self,ssize):
        '''this function is called by the toolbar when valid size parameter
        has been entered. '''
        self.SSSize = ssize
        self.ui.STrackUi()

    def numericSTrack(self,strack):
        '''this function is called by the toolbar when valid size parameter
        has been entered. ?'''
        self.SSTrack = strack
        self.ui.SFileUi()

    def validSString(self,sstring):
        '''this function is called by the toolbar when a ?valid? string parameter
        has been entered.  '''
        self.SString = sstring
        self.ui.SSizeUi()

    def validFFile(self,FFile):
        '''this function is called by the toolbar when a ?valid? font file parameter
        has been entered. '''
        self.FFile = FFile
        # last step in ShapeString parm capture, create object
        self.createObject()

    def finish(self, finishbool=False):
        """terminates the operation"""
        Creator.finish(self)
        if self.ui:
#            del self.dialog                       # what does this do??
            if self.ui.continueMode:
                self.Activated()

#---------------------------------------------------------------------------
# Modifier functions
#---------------------------------------------------------------------------

class Modifier(DraftTool):
    """A generic Modifier Tool, used by modification tools such as move"""

    def __init__(self):
        DraftTool.__init__(self)
        self.copymode = False

class Move(Modifier):
    """The Draft_Move FreeCAD command definition"""

    def __init__(self):
        Modifier.__init__(self)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Move',
                'Accel' : "M, V",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Move", "Move"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Move", "Moves the selected objects between 2 points. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        self.name = translate("draft","Move", utf8_decode=True)
        Modifier.Activated(self, self.name,
                           is_subtool=isinstance(FreeCAD.activeDraftCommand, SubelementHighlight))
        if not self.ui:
            return
        self.ghosts = []
        self.get_object_selection()

    def get_object_selection(self):
        if FreeCADGui.Selection.getSelectionEx():
            return self.proceed()
        self.ui.selectUi()
        FreeCAD.Console.PrintMessage(translate("draft", "Select an object to move")+"\n")
        self.call = self.view.addEventCallback("SoEvent", selectObject)

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        self.selected_objects = FreeCADGui.Selection.getSelection()
        self.selected_objects = Draft.getGroupContents(self.selected_objects, addgroups=True, spaces=True, noarchchild=True)
        self.selected_subelements = FreeCADGui.Selection.getSelectionEx()
        self.ui.lineUi(self.name)
        self.ui.modUi()
        if self.copymode:
            self.ui.isCopy.setChecked(True)
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick start point")+"\n")

    def finish(self,closed=False,cont=False):
        for ghost in self.ghosts:
            ghost.finalize()
        if cont and self.ui:
            if self.ui.continueMode:
                todo.delayAfter(self.Activated,[])
        Modifier.finish(self)

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.handle_mouse_move_event(arg)
        elif arg["Type"] == "SoMouseButtonEvent" \
            and arg["State"] == "DOWN" \
            and arg["Button"] == "BUTTON1":
            self.handle_mouse_click_event(arg)

    def handle_mouse_move_event(self, arg):
        for ghost in self.ghosts:
            ghost.off()
        self.point, ctrlPoint, info = getPoint(self,arg)
        if (len(self.node) > 0):
            last = self.node[len(self.node)-1]
            self.vector = self.point.sub(last)
            for ghost in self.ghosts:
                ghost.move(self.vector)
                ghost.on()
        if self.extendedCopy:
            if not hasMod(arg,MODALT): self.finish()
        redraw3DView()

    def handle_mouse_click_event(self, arg):
        if not self.ghosts:
            self.set_ghosts()
        if not self.point:
            return
        self.ui.redraw()
        if self.node == []:
            self.node.append(self.point)
            self.ui.isRelative.show()
            for ghost in self.ghosts:
                ghost.on()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick end point")+"\n")
            if self.planetrack:
                self.planetrack.set(self.point)
        else:
            last = self.node[0]
            self.vector = self.point.sub(last)
            self.move()
            if hasMod(arg,MODALT):
                self.extendedCopy = True
            else:
                self.finish(cont=True)

    def set_ghosts(self):
        if self.ui.isSubelementMode.isChecked():
            return self.set_subelement_ghosts()
        self.ghosts = [ghostTracker(self.selected_objects)]

    def set_subelement_ghosts(self):
        import Part
        for object in self.selected_subelements:
            for subelement in object.SubObjects:
                if isinstance(subelement, Part.Vertex) \
                    or isinstance(subelement, Part.Edge):
                    self.ghosts.append(ghostTracker(subelement))

    def move(self):
        if self.ui.isSubelementMode.isChecked():
            self.move_subelements()
        else:
            self.move_object()

    def move_subelements(self):
        try:
            if self.ui.isCopy.isChecked():
                self.commit(translate("draft", "Copy"), self.build_copy_subelements_command())
            else:
                self.commit(translate("draft", "Move"), self.build_move_subelements_command())
        except:
            FreeCAD.Console.PrintError(translate("draft", "Some subelements could not be moved."))

    def build_copy_subelements_command(self):
        import Part
        command = []
        arguments = []
        for object in self.selected_subelements:
            for index, subelement in enumerate(object.SubObjects):
                if not isinstance(subelement, Part.Edge):
                    continue
                arguments.append('[FreeCAD.ActiveDocument.{}, {}, {}]'.format(
                    object.ObjectName,
                    int(object.SubElementNames[index][len("Edge"):])-1,
                    DraftVecUtils.toString(self.vector)))
        command.append('Draft.copyMovedEdges([{}])'.format(','.join(arguments)))
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def build_move_subelements_command(self):
        import Part
        command = []
        for object in self.selected_subelements:
            for index, subelement in enumerate(object.SubObjects):
                if isinstance(subelement, Part.Vertex):
                    command.append('Draft.moveVertex(FreeCAD.ActiveDocument.{}, {}, {})'.format(
                        object.ObjectName,
                        int(object.SubElementNames[index][len("Vertex"):])-1,
                        DraftVecUtils.toString(self.vector)
                        ))
                elif isinstance(subelement, Part.Edge):
                    command.append('Draft.moveEdge(FreeCAD.ActiveDocument.{}, {}, {})'.format(
                        object.ObjectName,
                        int(object.SubElementNames[index][len("Edge"):])-1,
                        DraftVecUtils.toString(self.vector)
                        ))
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def move_object(self):
        objects = '[' + ','.join(['FreeCAD.ActiveDocument.' + object.Name for object in self.selected_objects]) + ']'
        FreeCADGui.addModule("Draft")
        self.commit(translate("draft","Copy" if self.ui.isCopy.isChecked() else "Move"),
            ['Draft.move('+objects+','+DraftVecUtils.toString(self.vector)+',copy='+str(self.ui.isCopy.isChecked())+')', 'FreeCAD.ActiveDocument.recompute()'])

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        if not self.node:
            self.node.append(self.point)
            self.ui.isRelative.show()
            self.ui.isCopy.show()
            for ghost in self.ghosts:
                ghost.on()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick end point")+"\n")
        else:
            last = self.node[-1]
            self.vector = self.point.sub(last)
            self.move()
            self.finish()


class ApplyStyle(Modifier):
    """The Draft_ApplyStyle FreeCA command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Apply',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Apply Current Style"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Applies current line width and color to selected objects")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        Modifier.Activated(self)
        if self.ui:
            self.sel = FreeCADGui.Selection.getSelection()
            if (len(self.sel)>0):
                FreeCADGui.addModule("Draft")
                c = []
                for ob in self.sel:
                    if (ob.Type == "App::DocumentObjectGroup"):
                        c.extend(self.formatGroup(ob))
                    else:
                        c.append('Draft.formatObject(FreeCAD.ActiveDocument.'+ob.Name+')')
                self.commit(translate("draft","Change Style"),c)

    def formatGroup(self,grpob):
        FreeCADGui.addModule("Draft")
        c=[]
        for ob in grpob.Group:
            if (ob.Type == "App::DocumentObjectGroup"):
                c.extend(self.formatGroup(ob))
            else:
                c.append('Draft.formatObject(FreeCAD.ActiveDocument.'+ob.Name+')')

class Rotate(Modifier):
    """The Draft_Rotate FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'Accel' : "R, O",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Rotate", "Rotate"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Rotate", "Rotates the selected objects. CTRL to snap, SHIFT to constrain, ALT creates a copy")}

    def Activated(self):
        Modifier.Activated(self,"Rotate")
        if not self.ui:
            return
        self.ghosts = []
        self.arctrack = None
        self.get_object_selection()

    def get_object_selection(self):
        if FreeCADGui.Selection.getSelection():
            return self.proceed()
        self.ui.selectUi()
        FreeCAD.Console.PrintMessage(translate("draft", "Select an object to rotate")+"\n")
        self.call = self.view.addEventCallback("SoEvent", selectObject)

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selected_objects = FreeCADGui.Selection.getSelection()
        self.selected_objects = Draft.getGroupContents(self.selected_objects, addgroups=True, spaces=True, noarchchild=True)
        self.selected_subelements = FreeCADGui.Selection.getSelectionEx()
        self.step = 0
        self.center = None
        self.ui.rotateSetCenterUi()
        self.ui.modUi()
        self.ui.setTitle(translate("draft","Rotate"))
        self.arctrack = arcTracker()
        self.call = self.view.addEventCallback("SoEvent",self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick rotation center")+"\n")

    def action(self, arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.handle_mouse_move_event(arg)
        elif arg["Type"] == "SoMouseButtonEvent" \
            and arg["State"] == "DOWN" \
            and arg["Button"] == "BUTTON1":
            self.handle_mouse_click_event(arg)

    def handle_mouse_move_event(self, arg):
        for ghost in self.ghosts:
            ghost.off()
        self.point,ctrlPoint,info = getPoint(self,arg)
        # this is to make sure radius is what you see on screen
        if self.center and DraftVecUtils.dist(self.point,self.center):
            viewdelta = DraftVecUtils.project(self.point.sub(self.center), plane.axis)
            if not DraftVecUtils.isNull(viewdelta):
                self.point = self.point.add(viewdelta.negative())
        if self.extendedCopy:
            if not hasMod(arg,MODALT):
                self.step = 3
                self.finish()
        if (self.step == 0):
            pass
        elif (self.step == 1):
            currentrad = DraftVecUtils.dist(self.point,self.center)
            if (currentrad != 0):
                angle = DraftVecUtils.angle(plane.u, self.point.sub(self.center), plane.axis)
            else: angle = 0
            self.ui.setRadiusValue(math.degrees(angle),unit="Angle")
            self.firstangle = angle
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
        elif (self.step == 2):
            currentrad = DraftVecUtils.dist(self.point,self.center)
            if (currentrad != 0):
                angle = DraftVecUtils.angle(plane.u, self.point.sub(self.center), plane.axis)
            else: angle = 0
            if (angle < self.firstangle):
                sweep = (2*math.pi-self.firstangle)+angle
            else:
                sweep = angle - self.firstangle
            self.arctrack.setApertureAngle(sweep)
            for ghost in self.ghosts:
                ghost.rotate(plane.axis,sweep)
                ghost.on()
            self.ui.setRadiusValue(math.degrees(sweep), 'Angle')
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
        redraw3DView()

    def handle_mouse_click_event(self, arg):
        if not self.point:
            return
        if self.step == 0:
            self.set_center()
        elif self.step == 1:
            self.set_start_point()
        else:
            self.set_rotation_angle(arg)

    def set_center(self):
        if not self.ghosts:
            self.set_ghosts()
        self.center = self.point
        self.node = [self.point]
        self.ui.radiusUi()
        self.ui.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Angle).UserString)
        self.ui.hasFill.hide()
        self.ui.labelRadius.setText(translate("draft","Base angle"))
        self.ui.radiusValue.setToolTip(translate("draft","The base angle you wish to start the rotation from"))
        self.arctrack.setCenter(self.center)
        for ghost in self.ghosts:
            ghost.center(self.center)
        self.step = 1
        FreeCAD.Console.PrintMessage(translate("draft", "Pick base angle")+"\n")
        if self.planetrack:
            self.planetrack.set(self.point)

    def set_start_point(self):
        self.ui.labelRadius.setText(translate("draft","Rotation"))
        self.ui.radiusValue.setToolTip(translate("draft", "The amount of rotation you wish to perform. The final angle will be the base angle plus this amount."))
        self.rad = DraftVecUtils.dist(self.point,self.center)
        self.arctrack.on()
        self.arctrack.setStartPoint(self.point)
        for ghost in self.ghosts:
            ghost.on()
        self.step = 2
        FreeCAD.Console.PrintMessage(translate("draft", "Pick rotation angle")+"\n")

    def set_rotation_angle(self, arg):
        currentrad = DraftVecUtils.dist(self.point,self.center)
        angle = self.point.sub(self.center).getAngle(plane.u)
        if DraftVecUtils.project(self.point.sub(self.center), plane.v).getAngle(plane.v) > 1:
            angle = -angle
        if (angle < self.firstangle):
            self.angle = (2*math.pi-self.firstangle)+angle
        else:
            self.angle = angle - self.firstangle
        self.rotate(self.ui.isCopy.isChecked() or hasMod(arg,MODALT))
        if hasMod(arg,MODALT):
            self.extendedCopy = True
        else:
            self.finish(cont=True)

    def set_ghosts(self):
        if self.ui.isSubelementMode.isChecked():
            return self.set_subelement_ghosts()
        self.ghosts = [ghostTracker(self.selected_objects)]

    def set_subelement_ghosts(self):
        import Part
        for object in self.selected_subelements:
            for subelement in object.SubObjects:
                if isinstance(subelement, Part.Vertex) \
                    or isinstance(subelement, Part.Edge):
                    self.ghosts.append(ghostTracker(subelement))

    def finish(self, closed=False, cont=False):
        """finishes the arc"""
        if self.arctrack:
            self.arctrack.finalize()
        for ghost in self.ghosts:
            ghost.finalize()
        if cont and self.ui:
            if self.ui.continueMode:
                todo.delayAfter(self.Activated,[])
        Modifier.finish(self)
        if self.doc:
            self.doc.recompute()

    def rotate(self, is_copy=False):
        if self.ui.isSubelementMode.isChecked():
            self.rotate_subelements(is_copy)
        else:
            self.rotate_object(is_copy)

    def rotate_subelements(self, is_copy):
        try:
            if is_copy:
                self.commit(translate("draft", "Copy"), self.build_copy_subelements_command())
            else:
                self.commit(translate("draft", "Rotate"), self.build_rotate_subelements_command())
        except:
            FreeCAD.Console.PrintError(translate("draft", "Some subelements could not be moved."))

    def build_copy_subelements_command(self):
        import Part
        command = []
        arguments = []
        for object in self.selected_subelements:
            for index, subelement in enumerate(object.SubObjects):
                if not isinstance(subelement, Part.Edge):
                    continue
                arguments.append('[FreeCAD.ActiveDocument.{}, {}, {}, {}, {}]'.format(
                    object.ObjectName,
                    int(object.SubElementNames[index][len("Edge"):])-1,
                    math.degrees(self.angle),
                    DraftVecUtils.toString(self.center),
                    DraftVecUtils.toString(plane.axis)))
        command.append('Draft.copyRotatedEdges([{}])'.format(','.join(arguments)))
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def build_rotate_subelements_command(self):
        import Part
        command = []
        for object in self.selected_subelements:
            for index, subelement in enumerate(object.SubObjects):
                if isinstance(subelement, Part.Vertex):
                    command.append('Draft.rotateVertex(FreeCAD.ActiveDocument.{}, {}, {}, {}, {})'.format(
                        object.ObjectName,
                        int(object.SubElementNames[index][len("Vertex"):])-1,
                        math.degrees(self.angle),
                        DraftVecUtils.toString(self.center),
                        DraftVecUtils.toString(plane.axis)))
                elif isinstance(subelement, Part.Edge):
                    command.append('Draft.rotateEdge(FreeCAD.ActiveDocument.{}, {}, {}, {}, {})'.format(
                        object.ObjectName,
                        int(object.SubElementNames[index][len("Edge"):])-1,
                        math.degrees(self.angle),
                        DraftVecUtils.toString(self.center),
                        DraftVecUtils.toString(plane.axis)))
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def rotate_object(self, is_copy):
        objects = '[' + ','.join(['FreeCAD.ActiveDocument.' + object.Name for object in self.selected_objects]) + ']'
        FreeCADGui.addModule("Draft")
        self.commit(translate("draft","Copy" if is_copy else "Rotate"),
            ['Draft.rotate({},{},{},axis={},copy={})'.format(
                objects,
                math.degrees(self.angle),
                DraftVecUtils.toString(self.center),
                DraftVecUtils.toString(plane.axis),
                is_copy
            ),
            'FreeCAD.ActiveDocument.recompute()'])

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        for ghost in self.ghosts:
            ghost.center(self.center)
        self.ui.radiusUi()
        self.ui.hasFill.hide()
        self.ui.labelRadius.setText(translate("draft","Base angle"))
        self.ui.radiusValue.setToolTip(translate("draft","The base angle you wish to start the rotation from"))
        self.ui.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Angle).UserString)
        self.step = 1
        FreeCAD.Console.PrintMessage(translate("draft", "Pick base angle")+"\n")

    def numericRadius(self,rad):
        """this function gets called by the toolbar when valid radius have been entered there"""
        if (self.step == 1):
            self.ui.labelRadius.setText(translate("draft","Rotation"))
            self.ui.radiusValue.setToolTip(translate("draft","The amount of rotation you wish to perform. The final angle will be the base angle plus this amount."))
            self.ui.radiusValue.setText(FreeCAD.Units.Quantity(0,FreeCAD.Units.Angle).UserString)
            self.firstangle = math.radians(rad)
            self.arctrack.setStartAngle(self.firstangle)
            self.arctrack.on()
            for ghost in self.ghosts:
                ghost.on()
            self.step = 2
            FreeCAD.Console.PrintMessage(translate("draft", "Pick rotation angle")+"\n")
        else:
            self.angle = math.radians(rad)
            self.rotate(self.ui.isCopy.isChecked())
            self.finish(cont=True)


class Offset(Modifier):
    """The Draft_Offset FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Offset',
                'Accel' : "O, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Offset", "Offset"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Offset", "Offsets the active object. CTRL to snap, SHIFT to constrain, ALT to copy")}

    def Activated(self):
        self.running = False
        Modifier.Activated(self,"Offset")
        self.ghost = None
        self.linetrack = None
        self.arctrack = None
        if self.ui:
            if not FreeCADGui.Selection.getSelection():
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to offset")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            elif len(FreeCADGui.Selection.getSelection()) > 1:
                FreeCAD.Console.PrintWarning(translate("draft", "Offset only works on one object at a time")+"\n")
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = FreeCADGui.Selection.getSelection()[0]
        if not self.sel.isDerivedFrom("Part::Feature"):
            FreeCAD.Console.PrintWarning(translate("draft", "Cannot offset this object type")+"\n")
            self.finish()
        else:
            self.step = 0
            self.dvec = None
            self.npts = None
            self.constrainSeg = None
            self.ui.offsetUi()
            self.linetrack = lineTracker()
            self.faces = False
            self.shape = self.sel.Shape
            self.mode = None
            if Draft.getType(self.sel) in ["Circle","Arc"]:
                self.ghost = arcTracker()
                self.mode = "Circle"
                self.center = self.shape.Edges[0].Curve.Center
                self.ghost.setCenter(self.center)
                self.ghost.setStartAngle(math.radians(self.sel.FirstAngle))
                self.ghost.setEndAngle(math.radians(self.sel.LastAngle))
            elif Draft.getType(self.sel) == "BSpline":
                self.ghost = bsplineTracker(points=self.sel.Points)
                self.mode = "BSpline"
            elif Draft.getType(self.sel) == "BezCurve":
                FreeCAD.Console.PrintWarning(translate("draft", "Sorry, offset of Bezier curves is currently still not supported")+"\n")
                self.finish()
                return
            else:
                if len(self.sel.Shape.Edges) == 1:
                    import Part
                    if isinstance(self.sel.Shape.Edges[0].Curve,Part.Circle):
                        self.ghost = arcTracker()
                        self.mode = "Circle"
                        self.center = self.shape.Edges[0].Curve.Center
                        self.ghost.setCenter(self.center)
                        if len(self.sel.Shape.Vertexes) > 1:
                            self.ghost.setStartAngle(self.sel.Shape.Edges[0].FirstParameter)
                            self.ghost.setEndAngle(self.sel.Shape.Edges[0].LastParameter)
                if not self.ghost:
                    self.ghost = wireTracker(self.shape)
                    self.mode = "Wire"
            self.call = self.view.addEventCallback("SoEvent",self.action)
            FreeCAD.Console.PrintMessage(translate("draft", "Pick distance")+"\n")
            if self.planetrack:
                self.planetrack.set(self.shape.Vertexes[0].Point)
            self.running = True

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point,ctrlPoint,info = getPoint(self,arg)
            if hasMod(arg,MODCONSTRAIN) and self.constrainSeg:
                dist = DraftGeomUtils.findPerpendicular(self.point,self.shape,self.constrainSeg[1])
            else:
                dist = DraftGeomUtils.findPerpendicular(self.point,self.shape.Edges)
            if dist:
                self.ghost.on()
                if self.mode == "Wire":
                    d = dist[0].negative()
                    v1 = DraftGeomUtils.getTangent(self.shape.Edges[0],self.point)
                    v2 = DraftGeomUtils.getTangent(self.shape.Edges[dist[1]],self.point)
                    a = -DraftVecUtils.angle(v1,v2)
                    self.dvec = DraftVecUtils.rotate(d,a,plane.axis)
                    occmode = self.ui.occOffset.isChecked()
                    self.ghost.update(DraftGeomUtils.offsetWire(self.shape,self.dvec,occ=occmode),forceclosed=occmode)
                elif self.mode == "BSpline":
                    d = dist[0].negative()
                    e = self.shape.Edges[0]
                    basetan = DraftGeomUtils.getTangent(e,self.point)
                    self.npts = []
                    for p in self.sel.Points:
                        currtan = DraftGeomUtils.getTangent(e,p)
                        a = -DraftVecUtils.angle(currtan,basetan)
                        self.dvec = DraftVecUtils.rotate(d,a,plane.axis)
                        self.npts.append(p.add(self.dvec))
                    self.ghost.update(self.npts)
                elif self.mode == "Circle":
                    self.dvec = self.point.sub(self.center).Length
                    self.ghost.setRadius(self.dvec)
                self.constrainSeg = dist
                self.linetrack.on()
                self.linetrack.p1(self.point)
                self.linetrack.p2(self.point.add(dist[0]))
                self.ui.setRadiusValue(dist[0].Length,unit="Length")
            else:
                self.dvec = None
                self.ghost.off()
                self.constrainSeg = None
                self.linetrack.off()
                self.ui.radiusValue.setText("off")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
            redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                copymode = False
                occmode = self.ui.occOffset.isChecked()
                if hasMod(arg,MODALT) or self.ui.isCopy.isChecked(): copymode = True
                FreeCADGui.addModule("Draft")
                if self.npts:
                    print("offset:npts=",self.npts)
                    self.commit(translate("draft","Offset"),
                                ['Draft.offset(FreeCAD.ActiveDocument.'+self.sel.Name+','+DraftVecUtils.toString(self.npts)+',copy='+str(copymode)+')',
                                 'FreeCAD.ActiveDocument.recompute()'])
                elif self.dvec:
                    if isinstance(self.dvec,float):
                        d = str(self.dvec)
                    else:
                        d = DraftVecUtils.toString(self.dvec)
                    self.commit(translate("draft","Offset"),
                                ['Draft.offset(FreeCAD.ActiveDocument.'+self.sel.Name+','+d+',copy='+str(copymode)+',occ='+str(occmode)+')',
                                 'FreeCAD.ActiveDocument.recompute()'])
                if hasMod(arg,MODALT):
                    self.extendedCopy = True
                else:
                    self.finish()

    def finish(self,closed=False):
        if self.running:
            if self.linetrack:
                self.linetrack.finalize()
            if self.ghost:
                self.ghost.finalize()
        Modifier.finish(self)

    def numericRadius(self,rad):
        '''this function gets called by the toolbar when
        valid radius have been entered there'''
        #print("dvec:",self.dvec)
        #print("rad:",rad)
        if self.dvec:
            if isinstance(self.dvec,float):
                if self.mode == "Circle":
                    r1 = self.shape.Edges[0].Curve.Radius
                    r2 = self.ghost.getRadius()
                    if r2 >= r1:
                        rad = r1 + rad
                    else:
                        rad = r1 - rad
                    d = str(rad)
                else:
                    print("Draft.Offset error: Unhandled case")
            else:
                self.dvec.normalize()
                self.dvec.multiply(rad)
                d = DraftVecUtils.toString(self.dvec)
            copymode = False
            occmode = self.ui.occOffset.isChecked()
            if self.ui.isCopy.isChecked():
                copymode = True
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Offset"),
                        ['Draft.offset(FreeCAD.ActiveDocument.'+self.sel.Name+','+d+',copy='+str(copymode)+',occ='+str(occmode)+')',
                         'FreeCAD.ActiveDocument.recompute()'])
            self.finish()


class Stretch(Modifier):
    """The Draft_Stretch FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Stretch',
                'Accel' : "S, H",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Stretch", "Stretch"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Stretch", "Stretches the selected objects")}

    def Activated(self):
        Modifier.Activated(self,"Stretch")
        if self.ui:
            if not FreeCADGui.Selection.getSelection():
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to stretch")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        supported = ["Rectangle","Wire","BSpline","BezCurve","Sketch"]
        self.sel = []
        for obj in FreeCADGui.Selection.getSelection():
            if Draft.getType(obj) in supported:
                self.sel.append([obj,FreeCAD.Placement()])
            elif hasattr(obj,"Base"):
                if obj.Base:
                    if Draft.getType(obj.Base) in supported:
                        self.sel.append([obj.Base,obj.Placement])

                    elif Draft.getType(obj.Base) in ["Offset2D","Array"]:
                        base = None
                        if hasattr(obj.Base,"Source") and obj.Base.Source:
                            base = obj.Base.Source
                        elif hasattr(obj.Base,"Base") and obj.Base.Base:
                            base = obj.Base.Base
                        if base:
                            if Draft.getType(base) in supported:
                                self.sel.append([base,obj.Placement.multiply(obj.Base.Placement)])
            elif Draft.getType(obj) in ["Offset2D","Array"]:
                base = None
                if hasattr(obj,"Source") and obj.Source:
                    base = obj.Source
                elif hasattr(obj,"Base") and obj.Base:
                    base = obj.Base
                if base:
                    if Draft.getType(base) in supported:
                        self.sel.append([base,obj.Placement])
        if self.ui and self.sel:
            self.step = 1
            self.refpoint = None
            self.ui.pointUi("Stretch")
            self.ui.extUi()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.rectracker = rectangleTracker(dotted=True,scolor=(0.0,0.0,1.0),swidth=2)
            self.nodetracker = []
            self.displacement = None
            FreeCAD.Console.PrintMessage(translate("draft", "Pick first point of selection rectangle")+"\n")

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint,info = getPoint(self,arg) #,mobile=True) #,noTracker=(self.step < 3))
            if self.step == 2:
                self.rectracker.update(point)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    # clicked twice on the same point
                    self.finish()
                else:
                    point,ctrlPoint,info = getPoint(self,arg) #,mobile=True) #,noTracker=(self.step < 3))
                    self.addPoint(point)

    def addPoint(self,point):
        if self.step == 1:
            # first rctangle point
            FreeCAD.Console.PrintMessage(translate("draft", "Pick opposite point of selection rectangle")+"\n")
            self.ui.setRelative()
            self.rectracker.setorigin(point)
            self.rectracker.on()
            if self.planetrack:
                self.planetrack.set(point)
            self.step = 2
        elif self.step == 2:
            # second rectangle point
            FreeCAD.Console.PrintMessage(translate("draft", "Pick start point of displacement")+"\n")
            self.rectracker.off()
            nodes = []
            self.ops = []
            for sel in self.sel:
                o = sel[0]
                vispla = sel[1]
                tp = Draft.getType(o)
                if tp in ["Wire","BSpline","BezCurve"]:
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
                        self.ops.append([o,np])
                elif tp in ["Rectangle"]:
                    p1 = Vector(0,0,0)
                    p2 = Vector(o.Length.Value,0,0)
                    p3 = Vector(o.Length.Value,o.Height.Value,0)
                    p4 = Vector(0,o.Height.Value,0)
                    np = []
                    iso = False
                    for p in [p1,p2,p3,p4]:
                        p = o.Placement.multVec(p)
                        p = vispla.multVec(p)
                        isi = self.rectracker.isInside(p)
                        np.append(isi)
                        if isi:
                            iso = True
                            nodes.append(p)
                    if iso:
                        self.ops.append([o,np])
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
                        self.ops.append([o,np])
                else:
                    p = o.Placement.Base
                    p = vispla.multVec(p)
                    if self.rectracker.isInside(p):
                        self.ops.append([o])
                        nodes.append(p)
            for n in nodes:
                nt = editTracker(n,inactive=True)
                nt.on()
                self.nodetracker.append(nt)
            self.step = 3
        elif self.step == 3:
            # first point of displacement line
            FreeCAD.Console.PrintMessage(translate("draft", "Pick end point of displacement")+"\n")
            self.displacement = point
            #print "first point:",point
            self.node = [point]
            self.step = 4
        elif self.step == 4:
            #print "second point:",point
            self.displacement = point.sub(self.displacement)
            self.doStretch()
        if self.point:
            self.ui.redraw()

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        point = Vector(numx,numy,numz)
        self.addPoint(point)

    def finish(self,closed=False):
        if hasattr(self,"rectracker") and self.rectracker:
            self.rectracker.finalize()
        if hasattr(self,"nodetracker") and self.nodetracker:
            for n in self.nodetracker:
                n.finalize()
        Modifier.finish(self)

    def doStretch(self):
        """does the actual stretching"""
        commitops = []
        if self.displacement:
            if self.displacement.Length > 0:
                #print "displacement: ",self.displacement
                for ops in self.ops:
                    tp = Draft.getType(ops[0])
                    localdisp = ops[0].Placement.Rotation.inverted().multVec(self.displacement)
                    if tp in ["Wire","BSpline","BezCurve"]:
                        pts = []
                        for i in range(len(ops[1])):
                            if ops[1][i] == False:
                                pts.append(ops[0].Points[i])
                            else:
                                pts.append(ops[0].Points[i].add(localdisp))
                        pts = str(pts).replace("Vector","FreeCAD.Vector")
                        commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Points="+pts)
                    elif tp in ["Sketch"]:
                        baseverts = [ops[0].Shape.Vertexes[i].Point for i in range(len(ops[1])) if ops[1][i]]
                        for i in range(ops[0].GeometryCount):
                            j = 0
                            while True:
                                try:
                                    p = ops[0].getPoint(i,j)
                                except ValueError:
                                    break
                                else:
                                    p = ops[0].Placement.multVec(p)
                                    r = None
                                    for bv in baseverts:
                                        if DraftVecUtils.isNull(p.sub(bv)):
                                            commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".movePoint("+str(i)+","+str(j)+",FreeCAD."+str(localdisp)+",True)")
                                            r = bv
                                            break
                                    if r:
                                        baseverts.remove(r)
                                    j += 1
                    elif tp in ["Rectangle"]:
                        p1 = Vector(0,0,0)
                        p2 = Vector(ops[0].Length.Value,0,0)
                        p3 = Vector(ops[0].Length.Value,ops[0].Height.Value,0)
                        p4 = Vector(0,ops[0].Height.Value,0)
                        if ops[1] == [False,True,True,False]:
                            optype = 1
                        elif ops[1] == [False,False,True,True]:
                            optype = 2
                        elif ops[1] == [True,False,False,True]:
                            optype = 3
                        elif ops[1] == [True,True,False,False]:
                            optype = 4
                        else:
                            optype = 0
                        #print("length:",ops[0].Length,"height:",ops[0].Height," - ",ops[1]," - ",self.displacement)
                        done = False
                        if optype > 0:
                            v1 = ops[0].Placement.multVec(p2).sub(ops[0].Placement.multVec(p1))
                            a1 = round(self.displacement.getAngle(v1),4)
                            v2 = ops[0].Placement.multVec(p4).sub(ops[0].Placement.multVec(p1))
                            a2 = round(self.displacement.getAngle(v2),4)
                            # check if the displacement is along one of the rectangle directions
                            if a1 == 0:
                                if optype == 1:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Length="+str(d))
                                    done = True
                                elif optype == 3:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Length="+str(d))
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Placement.Base=FreeCAD."+str(ops[0].Placement.Base.add(self.displacement)))
                                    done = True
                            elif a1 == 3.1416:
                                if optype == 1:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Length="+str(d))
                                    done = True
                                elif optype == 3:
                                    if ops[0].Length.Value >= 0:
                                        d = ops[0].Length.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Length.Value - self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Length="+str(d))
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Placement.Base=FreeCAD."+str(ops[0].Placement.Base.add(self.displacement)))
                                    done = True
                            elif a2 == 0:
                                if optype == 2:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Height="+str(d))
                                    done = True
                                elif optype == 4:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Height="+str(d))
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Placement.Base=FreeCAD."+str(ops[0].Placement.Base.add(self.displacement)))
                                    done = True
                            elif a2 == 3.1416:
                                if optype == 2:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Height="+str(d))
                                    done = True
                                elif optype == 4:
                                    if ops[0].Height.Value >= 0:
                                        d = ops[0].Height.Value + self.displacement.Length
                                    else:
                                        d = ops[0].Height.Value - self.displacement.Length
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Height="+str(d))
                                    commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Placement.Base=FreeCAD."+str(ops[0].Placement.Base.add(self.displacement)))
                                    done = True
                        if not done:
                            # otherwise create a wire copy and stretch it instead
                            FreeCAD.Console.PrintMessage(translate("draft","Turning one Rectangle into a Wire")+"\n")
                            pts = []
                            opts = [p1,p2,p3,p4]
                            for i in range(4):
                                if ops[1][i] == False:
                                    pts.append(opts[i])
                                else:
                                    pts.append(opts[i].add(self.displacement))
                            pts = str(pts).replace("Vector","FreeCAD.Vector")
                            commitops.append("w = Draft.makeWire("+pts+",closed=True)")
                            commitops.append("Draft.formatObject(w,FreeCAD.ActiveDocument."+ops[0].Name+")")
                            commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".ViewObject.hide()")
                            for par in ops[0].InList:
                                if hasattr(par,"Base") and par.Base == ops[0]:
                                    commitops.append("FreeCAD.ActiveDocument."+par.Name+".Base = w")
                    else:
                        commitops.append("FreeCAD.ActiveDocument."+ops[0].Name+".Placement.Base=FreeCAD."+str(ops[0].Placement.Base.add(self.displacement)))
        if commitops:
            commitops.append("FreeCAD.ActiveDocument.recompute()")
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Stretch"),commitops)
        self.finish()

class Join(Modifier):
    '''The Draft_Join FreeCAD command definition.'''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Join',
                'Accel' : "J, O",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Join", "Join"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Join", "Joins two wires together")}

    def Activated(self):
        Modifier.Activated(self,"Join")
        if not self.ui:
            return
        if not FreeCADGui.Selection.getSelection():
            self.ui.selectUi()
            FreeCAD.Console.PrintMessage(translate("draft", "Select an object to join")+"\n")
            self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        if FreeCADGui.Selection.getSelection():
            print(FreeCADGui.Selection.getSelection())
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Join"),
                ['Draft.joinWires(FreeCADGui.Selection.getSelection())', 'FreeCAD.ActiveDocument.recompute()'])
        self.finish()

class Split(Modifier):
    '''The Draft_Split FreeCAD command definition.'''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Split',
                'Accel' : "S, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Split", "Split"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Split", "Splits a wire into two wires")}

    def Activated(self):
        Modifier.Activated(self,"Split")
        if not self.ui:
            return
        FreeCAD.Console.PrintMessage(translate("draft", "Select an object to split")+"\n")
        self.call = self.view.addEventCallback("SoEvent", self.action)

    def action(self, arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            getPoint(self, arg)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent" and arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
            self.point, ctrlPoint, info = getPoint(self, arg)
            if "Edge" in info["Component"]:
                return self.proceed(info)

    def proceed(self, info):
        Draft.split(FreeCAD.ActiveDocument.getObject(info["Object"]),
            self.point, int(info["Component"][4:]))
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.finish()

class Upgrade(Modifier):
    '''The Draft_Upgrade FreeCAD command definition.'''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Upgrade',
                'Accel' : "U, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Upgrade", "Upgrade"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Upgrade", "Joins the selected objects into one, or converts closed wires to filled faces, or unites faces")}

    def Activated(self):
        Modifier.Activated(self,"Upgrade")
        if self.ui:
            if not FreeCADGui.Selection.getSelection():
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to upgrade")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        if FreeCADGui.Selection.getSelection():
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Upgrade"),
                        ['Draft.upgrade(FreeCADGui.Selection.getSelection(),delete=True)',
                         'FreeCAD.ActiveDocument.recompute()'])
        self.finish()


class Downgrade(Modifier):
    '''The Draft_Downgrade FreeCAD command definition.'''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Downgrade',
                'Accel' : "D, N",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Downgrade", "Downgrade"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Downgrade", "Explodes the selected objects into simpler objects, or subtracts faces")}

    def Activated(self):
        Modifier.Activated(self,"Downgrade")
        if self.ui:
            if not FreeCADGui.Selection.getSelection():
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to upgrade")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        if FreeCADGui.Selection.getSelection():
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Downgrade"),
                        ['Draft.downgrade(FreeCADGui.Selection.getSelection(),delete=True)',
                         'FreeCAD.ActiveDocument.recompute()'])
        self.finish()

class Trimex(Modifier):
    """The Draft_Trimex FreeCAD command definition.
    This tool trims or extends lines, wires and arcs,
    or extrudes single faces. SHIFT constrains to the last point
    or extrudes in direction to the face normal."""

    def GetResources(self):
        return {'Pixmap' : 'Draft_Trimex',
                'Accel' : "T, R",
                'MenuText' : QtCore.QT_TRANSLATE_NOOP("Draft_Trimex", "Trimex"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Trimex", "Trims or extends the selected object, or extrudes single faces. CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts")}

    def Activated(self):
        Modifier.Activated(self,"Trimex")
        self.edges = []
        self.placement = None
        self.ghost = []
        self.linetrack = None
        self.color = None
        self.width = None
        if self.ui:
            if not FreeCADGui.Selection.getSelection():
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select object(s) to trim/extend")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 2:
            self.trimObjects(sel)
            self.finish()
            return
        self.obj = sel[0]
        self.ui.trimUi()
        self.linetrack = lineTracker()

        import DraftGeomUtils

        if not "Shape" in self.obj.PropertiesList: return
        if "Placement" in self.obj.PropertiesList:
            self.placement = self.obj.Placement
        if len(self.obj.Shape.Faces) == 1:
            # simple extrude mode, the object itself is extruded
            self.extrudeMode = True
            self.ghost = [ghostTracker([self.obj])]
            self.normal = self.obj.Shape.Faces[0].normalAt(.5,.5)
            for v in self.obj.Shape.Vertexes:
                self.ghost.append(lineTracker())
        elif len(self.obj.Shape.Faces) > 1:
            # face extrude mode, a new object is created
            ss =  FreeCADGui.Selection.getSelectionEx()[0]
            if len(ss.SubObjects) == 1:
                if ss.SubObjects[0].ShapeType == "Face":
                    self.obj = self.doc.addObject("Part::Feature","Face")
                    self.obj.Shape = ss.SubObjects[0]
                    self.extrudeMode = True
                    self.ghost = [ghostTracker([self.obj])]
                    self.normal = self.obj.Shape.Faces[0].normalAt(.5,.5)
                    for v in self.obj.Shape.Vertexes:
                        self.ghost.append(lineTracker())
        else:
            # normal wire trimex mode
            self.color = self.obj.ViewObject.LineColor
            self.width = self.obj.ViewObject.LineWidth
            #self.obj.ViewObject.Visibility = False
            self.obj.ViewObject.LineColor = (.5,.5,.5)
            self.obj.ViewObject.LineWidth = 1
            self.extrudeMode = False
            if self.obj.Shape.Wires:
                self.edges = self.obj.Shape.Wires[0].Edges
                self.edges = Part.__sortEdges__(self.edges)
            else:
                self.edges = self.obj.Shape.Edges
            self.ghost = []
            lc = self.color
            sc = (lc[0],lc[1],lc[2])
            sw = self.width
            for e in self.edges:
                if DraftGeomUtils.geomType(e) == "Line":
                    self.ghost.append(lineTracker(scolor=sc,swidth=sw))
                else:
                    self.ghost.append(arcTracker(scolor=sc,swidth=sw))
        if not self.ghost: self.finish()
        for g in self.ghost: g.on()
        self.activePoint = 0
        self.nodes = []
        self.shift = False
        self.alt = False
        self.force = None
        self.cv = None
        self.call = self.view.addEventCallback("SoEvent",self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick distance")+"\n")

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.shift = hasMod(arg,MODCONSTRAIN)
            self.alt = hasMod(arg,MODALT)
            self.ctrl = hasMod(arg,MODSNAP)
            if self.extrudeMode:
                arg["ShiftDown"] = False
            elif hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setSelectMode(not self.ctrl)
            wp = not(self.extrudeMode and self.shift)
            self.point,cp,info = getPoint(self,arg,workingplane=wp)
            if hasMod(arg,MODSNAP): self.snapped = None
            else: self.snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
            if self.extrudeMode:
                dist = self.extrude(self.shift)
            else:
                dist = self.redraw(self.point,self.snapped,self.shift,self.alt)
            self.ui.setRadiusValue(dist,unit="Length")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
            redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                self.shift = hasMod(arg,MODCONSTRAIN)
                self.alt = hasMod(arg,MODALT)
                if hasMod(arg,MODSNAP): self.snapped = None
                else: self.snapped = self.view.getObjectInfo((cursor[0],cursor[1]))
                self.trimObject()
                self.finish()

    def extrude(self,shift=False,real=False):
        """redraws the ghost in extrude mode"""
        self.newpoint = self.obj.Shape.Faces[0].CenterOfMass
        dvec = self.point.sub(self.newpoint)
        if not shift: delta = DraftVecUtils.project(dvec,self.normal)
        else: delta = dvec
        if self.force and delta.Length:
            ratio = self.force/delta.Length
            delta.multiply(ratio)
        if real: return delta
        self.ghost[0].trans.translation.setValue([delta.x,delta.y,delta.z])
        for i in range(1,len(self.ghost)):
            base = self.obj.Shape.Vertexes[i-1].Point
            self.ghost[i].p1(base)
            self.ghost[i].p2(base.add(delta))
        return delta.Length

    def redraw(self,point,snapped=None,shift=False,alt=False,real=None):
        """redraws the ghost"""

        # initializing
        reverse = False
        for g in self.ghost: g.off()
        if real: newedges = []

        import DraftGeomUtils

        # finding the active point
        vlist = []
        for e in self.edges: vlist.append(e.Vertexes[0].Point)
        vlist.append(self.edges[-1].Vertexes[-1].Point)
        if shift: npoint = self.activePoint
        else: npoint = DraftGeomUtils.findClosest(point,vlist)
        if npoint > len(self.edges)/2: reverse = True
        if alt: reverse = not reverse
        self.activePoint = npoint

        # sorting out directions
        if reverse and (npoint > 0): npoint = npoint-1
        if (npoint > len(self.edges)-1):
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
            if hasattr(snapped,"Shape"):
                pts = []
                for e in snapped.Shape.Edges:
                    int = DraftGeomUtils.findIntersection(edge,e,True,True)
                    if int: pts.extend(int)
                if pts:
                    point = pts[DraftGeomUtils.findClosest(point,pts)]

        # modifying active edge
        if DraftGeomUtils.geomType(edge) == "Line":
            ve = DraftGeomUtils.vec(edge)
            chord = v1.sub(point)
            n = ve.cross(chord)
            if n.Length == 0:
                self.newpoint = point
            else:
                perp = ve.cross(n)
                proj = DraftVecUtils.project(chord,perp)
                self.newpoint = Vector.add(point,proj)
            dist = v1.sub(self.newpoint).Length
            ghost.p1(self.newpoint)
            ghost.p2(v2)
            self.ui.labelRadius.setText(translate("draft","Distance"))
            self.ui.radiusValue.setToolTip(translate("draft", "The offset distance"))
            if real:
                if self.force:
                    ray = self.newpoint.sub(v1)
                    ray.multiply(self.force/ray.Length)
                    self.newpoint = Vector.add(v1,ray)
                newedges.append(Part.LineSegment(self.newpoint,v2).toShape())
        else:
            center = edge.Curve.Center
            rad = edge.Curve.Radius
            ang1 = DraftVecUtils.angle(v2.sub(center))
            ang2 = DraftVecUtils.angle(point.sub(center))
            self.newpoint=Vector.add(center,DraftVecUtils.rotate(Vector(rad,0,0),-ang2))
            self.ui.labelRadius.setText(translate("draft","Angle"))
            self.ui.radiusValue.setToolTip(translate("draft", "The offset angle"))
            dist = math.degrees(-ang2)
            # if ang1 > ang2: ang1,ang2 = ang2,ang1
            #print("last calculated:",math.degrees(-ang1),math.degrees(-ang2))
            ghost.setEndAngle(-ang2)
            ghost.setStartAngle(-ang1)
            ghost.setCenter(center)
            ghost.setRadius(rad)
            if real:
                if self.force:
                    angle = math.radians(self.force)
                    newray = DraftVecUtils.rotate(Vector(rad,0,0),-angle)
                    self.newpoint = Vector.add(center,newray)
                chord = self.newpoint.sub(v2)
                perp = chord.cross(Vector(0,0,1))
                scaledperp = DraftVecUtils.scaleTo(perp,rad)
                midpoint = Vector.add(center,scaledperp)
                newedges.append(Part.Arc(self.newpoint,midpoint,v2).toShape())
        ghost.on()

        # resetting the visible edges
        if not reverse:
            li = list(range(npoint+1,len(self.edges)))
        else:
            li = list(range(npoint-1,-1,-1))
        for i in li:
            edge = self.edges[i]
            ghost = self.ghost[i]
            if DraftGeomUtils.geomType(edge) == "Line":
                ghost.p1(edge.Vertexes[0].Point)
                ghost.p2(edge.Vertexes[-1].Point)
            else:
                ang1 = DraftVecUtils.angle(edge.Vertexes[0].Point.sub(center))
                ang2 = DraftVecUtils.angle(edge.Vertexes[-1].Point.sub(center))
                # if ang1 > ang2: ang1,ang2 = ang2,ang1
                ghost.setEndAngle(-ang2)
                ghost.setStartAngle(-ang1)
                ghost.setCenter(edge.Curve.Center)
                ghost.setRadius(edge.Curve.Radius)
            if real: newedges.append(edge)
            ghost.on()

        # finishing
        if real: return newedges
        else: return dist

    def trimObject(self):
        """trims the actual object"""
        if self.extrudeMode:
            delta = self.extrude(self.shift,real=True)
            #print("delta",delta)
            self.doc.openTransaction("Extrude")
            obj = Draft.extrude(self.obj,delta,solid=True)
            self.doc.commitTransaction()
            self.obj = obj
        else:
            edges = self.redraw(self.point,self.snapped,self.shift,self.alt,real=True)
            newshape = Part.Wire(edges)
            self.doc.openTransaction("Trim/extend")
            if Draft.getType(self.obj) in ["Wire","BSpline"]:
                p = []
                if self.placement:
                    invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement:
                        np = invpl.multVec(np)
                    p.append(np)
                self.obj.Points = p
            elif Draft.getType(self.obj) == "Part::Line":
                p = []
                if self.placement:
                    invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement:
                        np = invpl.multVec(np)
                    p.append(np)
                if ((p[0].x == self.obj.X1) and (p[0].y == self.obj.Y1) and (p[0].z == self.obj.Z1)):
                    self.obj.X2 = p[-1].x
                    self.obj.Y2 = p[-1].y
                    self.obj.Z2 = p[-1].z
                elif ((p[-1].x == self.obj.X1) and (p[-1].y == self.obj.Y1) and (p[-1].z == self.obj.Z1)):
                    self.obj.X2 = p[0].x
                    self.obj.Y2 = p[0].y
                    self.obj.Z2 = p[0].z
                elif ((p[0].x == self.obj.X2) and (p[0].y == self.obj.Y2) and (p[0].z == self.obj.Z2)):
                    self.obj.X1 = p[-1].x
                    self.obj.Y1 = p[-1].y
                    self.obj.Z1 = p[-1].z
                else:
                    self.obj.X1 = p[0].x
                    self.obj.Y1 = p[0].y
                    self.obj.Z1 = p[0].z
            elif Draft.getType(self.obj) == "Circle":
                angles = self.ghost[0].getAngles()
                #print("original",self.obj.FirstAngle," ",self.obj.LastAngle)
                #print("new",angles)
                if angles[0] > angles[1]: angles = (angles[1],angles[0])
                self.obj.FirstAngle = angles[0]
                self.obj.LastAngle = angles[1]
            else:
                self.obj.Shape = newshape
            self.doc.commitTransaction()
        self.doc.recompute()
        for g in self.ghost: g.off()

    def trimObjects(self,objectslist):
        """attempts to trim two objects together"""
        import Part
        wires = []
        for obj in objectslist:
            if not Draft.getType(obj) in ["Wire","Circle"]:
                FreeCAD.Console.PrintError(translate("draft","Unable to trim these objects, only Draft wires and arcs are supported")+"\n")
                return
            if len (obj.Shape.Wires) > 1:
                FreeCAD.Console.PrintError(translate("draft","Unable to trim these objects, too many wires")+"\n")
                return
            if len(obj.Shape.Wires) == 1:
                wires.append(obj.Shape.Wires[0])
            else:
                wires.append(Part.Wire(obj.Shape.Edges))
        ints = []
        edge1 = None
        edge2 = None
        for i1,e1 in enumerate(wires[0].Edges):
            for i2,e2 in enumerate(wires[1].Edges):
                i = DraftGeomUtils.findIntersection(e1,e2,dts=False)
                if len(i) == 1:
                    ints.append(i[0])
                    edge1 = i1
                    edge2 = i2
        if not ints:
            FreeCAD.Console.PrintErro(translate("draft","These objects don't intersect")+"\n")
            return
        if len(ints) != 1:
            FreeCAD.Console.PrintError(translate("draft","Too many intersection points")+"\n")
            return
        v11 = wires[0].Vertexes[0].Point
        v12 = wires[0].Vertexes[-1].Point
        v21 = wires[1].Vertexes[0].Point
        v22 = wires[1].Vertexes[-1].Point
        if DraftVecUtils.closest(ints[0],[v11,v12]) == 1:
            last1 = True
        else:
            last1 = False
        if DraftVecUtils.closest(ints[0],[v21,v22]) == 1:
            last2 = True
        else:
            last2 = False
        for i,obj in enumerate(objectslist):
            if i == 0:
                ed = edge1
                la = last1
            else:
                ed = edge2
                la = last2
            if Draft.getType(obj) == "Wire":
                if la:
                    pts = obj.Points[:ed+1] + ints
                else:
                    pts = ints + obj.Points[ed+1:]
                obj.Points = pts
            else:
                vec = ints[0].sub(obj.Placement.Base)
                vec = obj.Placement.inverse().Rotation.multVec(vec)
                ang = math.degrees(-DraftVecUtils.angle(vec,obj.Placement.Rotation.multVec(FreeCAD.Vector(1,0,0)),obj.Shape.Edges[0].Curve.Axis))
                if la:
                    obj.LastAngle = ang
                else:
                    obj.FirstAngle = ang
        self.doc.recompute()


    def finish(self,closed=False):
        Modifier.finish(self)
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
            Draft.select(self.obj)

    def numericRadius(self,dist):
        """this function gets called by the toolbar when valid distance have been entered there"""
        self.force = dist
        self.trimObject()
        self.finish()


class Scale(Modifier):
    '''The Draft_Scale FreeCAD command definition.
    This tool scales the selected objects from a base point.'''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Scale',
                'Accel' : "S, C",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Scale", "Scale"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Scale", "Scales the selected objects from a base point. CTRL to snap, SHIFT to constrain, ALT to copy")}

    def Activated(self):
        self.name = translate("draft","Scale", utf8_decode=True)
        Modifier.Activated(self,self.name)
        if not self.ui:
            return
        self.ghosts = []
        self.get_object_selection()

    def get_object_selection(self):
        if FreeCADGui.Selection.getSelection():
            return self.proceed()
        self.ui.selectUi()
        FreeCAD.Console.PrintMessage(translate("draft", "Select an object to scale")+"\n")
        self.call = self.view.addEventCallback("SoEvent",selectObject)

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        self.selected_objects = FreeCADGui.Selection.getSelection()
        self.selected_objects = Draft.getGroupContents(self.selected_objects)
        self.selected_subelements = FreeCADGui.Selection.getSelectionEx()
        self.refs = []
        self.ui.pointUi(self.name)
        self.ui.modUi()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.pickmode = False
        self.task = None
        self.call = self.view.addEventCallback("SoEvent", self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick base point")+"\n")

    def set_ghosts(self):
        if self.ui.isSubelementMode.isChecked():
            return self.set_subelement_ghosts()
        self.ghosts = [ghostTracker(self.selected_objects)]

    def set_subelement_ghosts(self):
        import Part
        for object in self.selected_subelements:
            for subelement in object.SubObjects:
                if isinstance(subelement, Part.Vertex) \
                    or isinstance(subelement, Part.Edge):
                    self.ghosts.append(ghostTracker(subelement))

    def pickRef(self):
        self.pickmode = True
        if self.node:
            self.node = self.node[:1] # remove previous picks
        FreeCAD.Console.PrintMessage(translate("draft", "Pick reference distance from base point")+"\n")
        self.call = self.view.addEventCallback("SoEvent",self.action)

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.handle_mouse_move_event(arg)
        elif arg["Type"] == "SoMouseButtonEvent" \
            and arg["State"] == "DOWN" \
            and (arg["Button"] == "BUTTON1") \
            and self.point:
            self.handle_mouse_click_event()

    def handle_mouse_move_event(self, arg):
        for ghost in self.ghosts:
            ghost.off()
        self.point, ctrlPoint, info = getPoint(self, arg, sym=True)

    def handle_mouse_click_event(self):
        if not self.ghosts:
            self.set_ghosts()
        self.numericInput(self.point.x, self.point.y, self.point.z)

    def scale(self):
        self.delta = Vector(self.task.xValue.value(), self.task.yValue.value(), self.task.zValue.value())
        self.center = self.node[0]
        if self.task.isSubelementMode.isChecked():
            self.scale_subelements()
        elif self.task.isClone.isChecked():
            self.scale_with_clone()
        else:
            self.scale_object()
        self.finish()

    def scale_subelements(self):
        try:
            if self.task.isCopy.isChecked():
                self.commit(translate("draft", "Copy"), self.build_copy_subelements_command())
            else:
                self.commit(translate("draft", "Scale"), self.build_scale_subelements_command())
        except:
            FreeCAD.Console.PrintError(translate("draft", "Some subelements could not be scaled."))

    def scale_with_clone(self):
        if self.task.relative.isChecked():
            self.delta = FreeCAD.DraftWorkingPlane.getGlobalCoords(self.delta)
        objects = '[' + ','.join(['FreeCAD.ActiveDocument.' + object.Name for object in self.selected_objects]) + ']'
        FreeCADGui.addModule("Draft")
        self.commit(translate("draft","Copy") if self.task.isCopy.isChecked() else translate("draft","Scale"),
                    ['clone = Draft.clone('+objects+',forcedraft=True)',
                     'clone.Scale = '+DraftVecUtils.toString(self.delta),
                     'FreeCAD.ActiveDocument.recompute()'])

    def build_copy_subelements_command(self):
        import Part
        command = []
        arguments = []
        for object in self.selected_subelements:
            for index, subelement in enumerate(object.SubObjects):
                if not isinstance(subelement, Part.Edge):
                    continue
                arguments.append('[FreeCAD.ActiveDocument.{}, {}, {}, {}]'.format(
                    object.ObjectName,
                    int(object.SubElementNames[index][len("Edge"):])-1,
                    DraftVecUtils.toString(self.delta),
                    DraftVecUtils.toString(self.center)))
        command.append('Draft.copyScaledEdges([{}])'.format(','.join(arguments)))
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command

    def build_scale_subelements_command(self):
        import Part
        command = []
        for object in self.selected_subelements:
            for index, subelement in enumerate(object.SubObjects):
                if isinstance(subelement, Part.Vertex):
                    command.append('Draft.scaleVertex(FreeCAD.ActiveDocument.{}, {}, {}, {})'.format(
                        object.ObjectName,
                        int(object.SubElementNames[index][len("Vertex"):])-1,
                        DraftVecUtils.toString(self.delta),
                        DraftVecUtils.toString(self.center)))
                elif isinstance(subelement, Part.Edge):
                    command.append('Draft.scaleEdge(FreeCAD.ActiveDocument.{}, {}, {}, {})'.format(
                        object.ObjectName,
                        int(object.SubElementNames[index][len("Edge"):])-1,
                        DraftVecUtils.toString(self.delta),
                        DraftVecUtils.toString(self.center)))
        command.append('FreeCAD.ActiveDocument.recompute()')
        return command
    
    def is_scalable(self,obj):
        t = Draft.getType(obj)
        if t in ["Rectangle","Wire","Annotation","BSpline"]:
            # TODO - support more types in Draft.scale
            return True
        else:
            return False

    def scale_object(self):
        if self.task.relative.isChecked():
            self.delta = FreeCAD.DraftWorkingPlane.getGlobalCoords(self.delta)
        goods = []
        bads = []
        for obj in self.selected_objects:
            if self.is_scalable(obj):
                goods.append(obj)
            else:
                bads.append(obj)
        if bads:
            if len(bads) == 1:
                m = translate("draft","Unable to scale object")+": "+bads[0].Label
            else:
                m = translate("draft","Unable to scale objects")+": "+",".join([o.Label for o in bads])
            m += " - "+translate("draft","This object type cannot be scaled directly. Please use the clone method.")+"\n"
            FreeCAD.Console.PrintError(m)
        if goods:
            objects = '[' + ','.join(['FreeCAD.ActiveDocument.' + obj.Name for obj in goods]) + ']'
            FreeCADGui.addModule("Draft")
            self.commit(translate("draft","Copy" if self.task.isCopy.isChecked() else "Scale"),
                        ['Draft.scale('+objects+',scale='+DraftVecUtils.toString(self.delta)+',center='+DraftVecUtils.toString(self.center)+',copy='+str(self.task.isCopy.isChecked())+')',
                         'FreeCAD.ActiveDocument.recompute()'])

    def scaleGhost(self,x,y,z,rel):
        delta = Vector(x,y,z)
        if rel:
            delta = FreeCAD.DraftWorkingPlane.getGlobalCoords(delta)
        for ghost in self.ghosts:
            ghost.scale(delta)
        # calculate a correction factor depending on the scaling center
        corr = Vector(self.node[0].x,self.node[0].y,self.node[0].z)
        corr.scale(delta.x,delta.y,delta.z)
        corr = (corr.sub(self.node[0])).negative()
        for ghost in self.ghosts:
            ghost.move(corr)
            ghost.on()

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when a valid base point has been entered"""
        self.point = Vector(numx,numy,numz)
        self.node.append(self.point)
        if not self.pickmode:
            if not self.ghosts:
                self.set_ghosts()
            self.ui.offUi()
            if self.call:
                self.view.removeEventCallback("SoEvent",self.call)
            self.task = DraftGui.ScaleTaskPanel()
            self.task.sourceCmd = self
            DraftGui.todo.delay(FreeCADGui.Control.showDialog,self.task)
            DraftGui.todo.delay(self.task.xValue.selectAll,None)
            DraftGui.todo.delay(self.task.xValue.setFocus,None)
            for ghost in self.ghosts:
                ghost.on()
        elif len(self.node) == 2:
            FreeCAD.Console.PrintMessage(translate("draft", "Pick new distance from base point")+"\n")
        elif len(self.node) == 3:
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.off()
            if self.call:
                self.view.removeEventCallback("SoEvent",self.call)
            d1 = (self.node[1].sub(self.node[0])).Length
            d2 = (self.node[2].sub(self.node[0])).Length
            #print d2,"/",d1,"=",d2/d1
            if hasattr(self,"task"):
                if self.task:
                    self.task.lock.setChecked(True)
                    self.task.setValue(d2/d1)

    def finish(self,closed=False,cont=False):
        Modifier.finish(self)
        for ghost in self.ghosts:
            ghost.finalize()

class ToggleConstructionMode():
    """The Draft_ToggleConstructionMode FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Construction',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode", "Toggle Construction Mode"),
                'Accel' : "C, M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode", "Toggles the Construction Mode for next objects.")}

    def Activated(self):
        FreeCADGui.draftToolBar.constrButton.toggle()


class ToggleContinueMode():
    """The Draft_ToggleContinueMode FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleContinueMode", "Toggle Continue Mode"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleContinueMode", "Toggles the Continue Mode for next commands.")}

    def Activated(self):
        FreeCADGui.draftToolBar.toggleContinue()


class Drawing(Modifier):
    """The Draft Drawing command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Drawing',
                'Accel' : "D, D",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Drawing", "Drawing"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Drawing", "Puts the selected objects on a Drawing sheet")}

    def Activated(self):
        Modifier.Activated(self,"Drawing")
        if not FreeCADGui.Selection.getSelection():
            self.ghost = None
            self.ui.selectUi()
            FreeCAD.Console.PrintMessage(translate("draft", "Select an object to project")+"\n")
            self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            self.page = self.createDefaultPage()
        else:
            self.page = None
            # if the user selected a page, put the objects on that page
            for obj in sel:
                if obj.isDerivedFrom("Drawing::FeaturePage"):
                    self.page = obj
                    break
            if not self.page:
                # no page selected, default to the first page in the document
                for obj in self.doc.Objects:
                    if obj.isDerivedFrom("Drawing::FeaturePage"):
                        self.page = obj
                        break
            if not self.page:
                # no page in the document, create a default page.
                self.page = self.createDefaultPage()
            otherProjection = None
            # if an existing projection is selected, reuse its projection properties
            for obj in sel:
                if obj.isDerivedFrom("Drawing::FeatureView"):
                    otherProjection = obj
                    break
            sel.reverse()
            for obj in sel:
                if ( obj.ViewObject.isVisible() and not obj.isDerivedFrom("Drawing::FeatureView")
                        and not obj.isDerivedFrom("Drawing::FeaturePage") ):
                    name = 'View'+obj.Name
                    # no reason to remove the old one...
                    #oldobj = self.page.getObject(name)
                    #if oldobj:
                    #    self.doc.removeObject(oldobj.Name)
                    Draft.makeDrawingView(obj,self.page,otherProjection=otherProjection)
            self.doc.recompute()

    def createDefaultPage(self):
        """created a default page"""
        template = Draft.getParam("template",FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg')
        page = self.doc.addObject('Drawing::FeaturePage','Page')
        page.ViewObject.HintOffsetX = 200
        page.ViewObject.HintOffsetY = 100
        page.ViewObject.HintScale = 20
        page.Template = template
        self.doc.recompute()
        return page


class ToggleDisplayMode():
    """The ToggleDisplayMode FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SwitchMode',
                'Accel' : "Shift+Space",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode", "Toggle display mode"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode", "Swaps display mode of selected objects between wireframe and flatlines")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        for obj in FreeCADGui.Selection.getSelection():
            if obj.ViewObject.DisplayMode == "Flat Lines":
                if "Wireframe" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Wireframe"
            elif obj.ViewObject.DisplayMode == "Wireframe":
                if "Flat Lines" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Flat Lines"

class SubelementHighlight(Modifier):
    """The Draft_SubelementHighlight FreeCAD command definition"""

    def __init__(self):
        self.is_running = False
        self.editable_objects = []
        self.original_view_settings = {}

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SubelementHighlight',
                'Accel' : "D, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_SubelementHighlight", "Subelement highlight"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_SubelementHighlight",
                                                    "Highlight the subelements "
                                                    "of the selected objects, "
                                                    "so that they can then be edited "
                                                    "with the move, rotate, and scale tools")}

    def Activated(self):
        if self.is_running:
            return self.finish()
        self.is_running = True
        Modifier.Activated(self, "SubelementHighlight")
        self.get_selection()

    def proceed(self):
        self.remove_view_callback()
        self.get_editable_objects_from_selection()
        if not self.editable_objects:
            return self.finish()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        self.highlight_editable_objects()

    def finish(self):
        Modifier.finish(self)
        self.remove_view_callback()
        self.restore_editable_objects_graphics()
        self.__init__()

    def action(self, event):
        if event["Type"] == "SoKeyboardEvent" and event["Key"] == "ESCAPE":
            self.finish()

    def get_selection(self):
        if not FreeCADGui.Selection.getSelection() and self.ui:
            FreeCAD.Console.PrintMessage(translate("draft", "Select an object to edit")+"\n")
            self.call = self.view.addEventCallback("SoEvent", selectObject)
        else:
            self.proceed()

    def remove_view_callback(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)

    def get_editable_objects_from_selection(self):
        for object in FreeCADGui.Selection.getSelection():
            if object.isDerivedFrom("Part::Part2DObject"):
                self.editable_objects.append(object)
            elif hasattr(object, "Base") and object.Base.isDerivedFrom("Part::Part2DObject"):
                self.editable_objects.append(object.Base)

    def highlight_editable_objects(self):
        for object in self.editable_objects:
            self.original_view_settings[object.Name] = {
                'Visibility': object.ViewObject.Visibility,
                'PointSize': object.ViewObject.PointSize,
                'PointColor': object.ViewObject.PointColor,
                'LineColor': object.ViewObject.LineColor
            }
            object.ViewObject.Visibility = True
            object.ViewObject.PointSize = 10
            object.ViewObject.PointColor = (1., 0., 0.)
            object.ViewObject.LineColor = (1., 0., 0.)
            xray = coin.SoAnnotation()
            xray.addChild(object.ViewObject.RootNode.getChild(2).getChild(0))
            xray.setName("xray")
            object.ViewObject.RootNode.addChild(xray)

    def restore_editable_objects_graphics(self):
        for object in self.editable_objects:
            try:
                for attribute, value in self.original_view_settings[object.Name].items():
                    view_object = object.ViewObject
                    setattr(view_object, attribute, value)
                    view_object.RootNode.removeChild(view_object.RootNode.getByName("xray"))
            except:
                # This can occur if objects have had graph changing operations
                pass

class AddToGroup():
    """The AddToGroup FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AddToGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddToGroup", "Move to group..."),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AddToGroup", "Moves the selected object(s) to an existing group")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        self.groups = ["Ungroup"]
        self.groups.extend(Draft.getGroupNames())
        self.labels = ["Ungroup"]
        for g in self.groups:
            o = FreeCAD.ActiveDocument.getObject(g)
            if o: self.labels.append(o.Label)
        self.ui = FreeCADGui.draftToolBar
        self.ui.sourceCmd = self
        self.ui.popupMenu(self.labels)

    def proceed(self,labelname):
        self.ui.sourceCmd = None
        if labelname == "Ungroup":
            for obj in FreeCADGui.Selection.getSelection():
                try:
                    Draft.ungroup(obj)
                except:
                    pass
        else:
            if labelname in self.labels:
                i = self.labels.index(labelname)
                g = FreeCAD.ActiveDocument.getObject(self.groups[i])
                for obj in FreeCADGui.Selection.getSelection():
                    try:
                        g.addObject(obj)
                    except:
                        pass


class AddPoint(Modifier):
    """The Draft_AddPoint FreeCAD command definition"""

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AddPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddPoint", "Add Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AddPoint", "Adds a point to an existing Wire or B-spline")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        selection = FreeCADGui.Selection.getSelection()
        if selection:
            if (Draft.getType(selection[0]) in ['Wire','BSpline']):
                FreeCADGui.runCommand("Draft_Edit")
                FreeCADGui.draftToolBar.vertUi(True)


class DelPoint(Modifier):
    """The Draft_DelPoint FreeCAD command definition"""

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_DelPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_DelPoint", "Remove Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_DelPoint", "Removes a point from an existing Wire or B-spline")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        selection = FreeCADGui.Selection.getSelection()
        if selection:
            if (Draft.getType(selection[0]) in ['Wire','BSpline']):
                FreeCADGui.runCommand("Draft_Edit")
                FreeCADGui.draftToolBar.vertUi(False)


class WireToBSpline(Modifier):
    """The Draft_Wire2BSpline FreeCAD command definition"""

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_WireToBSpline',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Wire to B-spline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Converts between Wire and B-spline")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        if self.running:
            self.finish()
        else:
            selection = FreeCADGui.Selection.getSelection()
            if selection:
                if (Draft.getType(selection[0]) in ['Wire','BSpline']):
                    Modifier.Activated(self,"Convert Curve Type")
                    if self.doc:
                        self.obj = FreeCADGui.Selection.getSelection()
                        if self.obj:
                            self.obj = self.obj[0]
                            self.pl = None
                            if "Placement" in self.obj.PropertiesList:
                                self.pl = self.obj.Placement
                            self.Points = self.obj.Points
                            self.closed = self.obj.Closed
                            n = None
                            if (Draft.getType(self.obj) == 'Wire'):
                                n = Draft.makeBSpline(self.Points, self.closed, self.pl)
                            elif (Draft.getType(self.obj) == 'BSpline'):
                                n = Draft.makeWire(self.Points, self.closed, self.pl)
                            if n:
                                Draft.formatObject(n,self.obj)
                                self.doc.recompute()
                        else:
                            self.finish()


class SelectGroup():
    """The SelectGroup FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SelectGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_SelectGroup", "Select group"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_SelectGroup", "Selects all objects with the same parents as this group")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        sellist = []
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1:
            if sel[0].isDerivedFrom("App::DocumentObjectGroup"):
                cts = Draft.getGroupContents(FreeCADGui.Selection.getSelection())
                for o in cts:
                    FreeCADGui.Selection.addSelection(o)
                return
        for ob in sel:
            for child in ob.OutList:
                FreeCADGui.Selection.addSelection(child)
            for parent in ob.InList:
                FreeCADGui.Selection.addSelection(parent)
                for child in parent.OutList:
                    FreeCADGui.Selection.addSelection(child)


class Shape2DView(Modifier):
    """The Shape2DView FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_2DShapeView',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Shape 2D view"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Creates Shape 2D views of selected objects")}

    def Activated(self):
        Modifier.Activated(self)
        if not FreeCADGui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select an object to project")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        faces = []
        objs = []
        vec = FreeCADGui.ActiveDocument.ActiveView.getViewDirection().negative()
        sel = FreeCADGui.Selection.getSelectionEx()
        for s in sel:
            objs.append(s.Object)
            for e in s.SubElementNames:
                if "Face" in e:
                    faces.append(int(e[4:])-1)
        #print(objs,faces)
        commitlist = []
        FreeCADGui.addModule("Draft")
        if (len(objs) == 1) and faces:
            commitlist.append("Draft.makeShape2DView(FreeCAD.ActiveDocument."+objs[0].Name+",FreeCAD.Vector"+str(tuple(vec))+",facenumbers="+str(faces)+")")
        else:
            for o in objs:
                commitlist.append("Draft.makeShape2DView(FreeCAD.ActiveDocument."+o.Name+",FreeCAD.Vector"+str(tuple(vec))+")")
        if commitlist:
            commitlist.append("FreeCAD.ActiveDocument.recompute()")
            self.commit(translate("draft","Create 2D view"),commitlist)
        self.finish()


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
    """The Shape2DView FreeCAD command definition"""

    def __init__(self,useLink=False):
        Modifier.__init__(self)
        self.useLink = useLink

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
                        ['obj = Draft.makeArray(FreeCAD.ActiveDocument.{},FreeCAD.Vector(1,0,0),FreeCAD.Vector(0,1,0),2,2,useLink={})'.format(obj.Name,self.useLink),
                         'Draft.autogroup(obj)',
                         'FreeCAD.ActiveDocument.recompute()'])
        self.finish()

class LinkArray(Array):
    "The Shape2DView FreeCAD command definition"

    def __init__(self):
        Array.__init__(self,True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_LinkArray',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_LinkArray", "LinkArray"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_LinkArray", "Creates a polar or rectangular link array from a selected object")}

class PathArray(Modifier):
    """The PathArray FreeCAD command definition"""

    def __init__(self,useLink=False):
        Modifier.__init__(self)
        self.useLink = useLink

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
            Draft.makePathArray(base,path,defCount,defXlate,defAlign,pathsubs,useLink=self.useLink)
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

class Point(Creator):
    """this class will create a vertex after the user clicks a point on the screen"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Point',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Point", "Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Point", "Creates a point object")}

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):
        Creator.Activated(self)
        self.view = Draft.get3DView()
        self.stack = []
        rot = self.view.getCameraNode().getField("orientation").getValue()
        upv = Vector(rot.multVec(coin.SbVec3f(0,1,0)).getValue())
        plane.setup(self.view.getViewDirection().negative(), Vector(0,0,0), upv)
        self.point = None
        if self.ui:
            self.ui.pointUi()
            self.ui.continueCmd.show()
        # adding 2 callback functions
        self.callbackClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.click)
        self.callbackMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.move)

    def move(self,event_cb):
        event = event_cb.getEvent()
        mousepos = event.getPosition().getValue()
        ctrl = event.wasCtrlDown()
        self.point = FreeCADGui.Snapper.snap(mousepos,active=ctrl)
        if self.ui:
            self.ui.displayPoint(self.point)

    def numericInput(self,numx,numy,numz):
        """called when a numeric value is entered on the toolbar"""
        self.point = FreeCAD.Vector(numx,numy,numz)
        self.click()

    def click(self,event_cb=None):
        if event_cb:
            event = event_cb.getEvent()
            if event.getState() != coin.SoMouseButtonEvent.DOWN:
                return
        if self.point:
            self.stack.append(self.point)
            if len(self.stack) == 1:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callbackClick)
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.callbackMove)
                commitlist = []
                if Draft.getParam("UsePartPrimitives",False):
                    # using
                    commitlist.append((translate("draft","Create Point"),
                                        ['point = FreeCAD.ActiveDocument.addObject("Part::Vertex","Point")',
                                         'point.X = '+str(self.stack[0][0]),
                                         'point.Y = '+str(self.stack[0][1]),
                                         'point.Z = '+str(self.stack[0][2]),
                                         'Draft.autogroup(point)',
                                         'FreeCAD.ActiveDocument.recompute()']))
                else:
                    # building command string
                    FreeCADGui.addModule("Draft")
                    commitlist.append((translate("draft","Create Point"),
                                        ['point = Draft.makePoint('+str(self.stack[0][0])+','+str(self.stack[0][1])+','+str(self.stack[0][2])+')',
                                         'Draft.autogroup(point)',
                                         'FreeCAD.ActiveDocument.recompute()']))
                todo.delayCommit(commitlist)
                FreeCADGui.Snapper.off()
            self.finish()

    def finish(self,cont=False):
        """terminates the operation and restarts if needed"""
        Creator.finish(self)
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

class ShowSnapBar():
    """The ShowSnapBar FreeCAD command definition"""

    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Show Snap Bar"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Shows Draft snap toolbar")}

    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()


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
            todo.delay(FreeCADGui.runCommand,"Draft_Move")


class ToggleGrid():
    """The Draft ToggleGrid command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Grid',
                'Accel' : "G,R",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleGrid", "Toggle Grid"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ToggleGrid", "Toggles the Draft grid on/off")}

    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.setTrackers()
            if FreeCADGui.Snapper.grid:
                if FreeCADGui.Snapper.grid.Visible:
                    FreeCADGui.Snapper.grid.off()
                    FreeCADGui.Snapper.forceGridOff=True
                else:
                    FreeCADGui.Snapper.grid.on()
                    FreeCADGui.Snapper.forceGridOff=False

class Heal():
    """The Draft Heal command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Heal',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Heal", "Heal"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Heal", "Heal faulty Draft objects saved from an earlier FreeCAD version")}

    def Activated(self):
        s = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Heal")
        if s:
            Draft.heal(s)
        else:
            Draft.heal()
        FreeCAD.ActiveDocument.commitTransaction()


class Draft_Facebinder(Creator):
    """The Draft Facebinder command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Facebinder',
                'Accel' : "F,F",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Facebinder", "Facebinder"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Facebinder", "Creates a facebinder object from selected face(s)")}

    def Activated(self):
        Creator.Activated(self)
        if not FreeCADGui.Selection.getSelection():
            if self.ui:
                self.ui.selectUi()
                FreeCAD.Console.PrintMessage(translate("draft", "Select face(s) on existing object(s)")+"\n")
                self.call = self.view.addEventCallback("SoEvent",selectObject)
        else:
            self.proceed()

    def proceed(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        if FreeCADGui.Selection.getSelection():
            FreeCAD.ActiveDocument.openTransaction("Facebinder")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("s = FreeCADGui.Selection.getSelectionEx()")
            FreeCADGui.doCommand("f = Draft.makeFacebinder(s)")
            FreeCADGui.doCommand('Draft.autogroup(f)')
            FreeCADGui.doCommand('FreeCAD.ActiveDocument.recompute()')
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        self.finish()

class Draft_FlipDimension():
    def GetResources(self):
        return {'Pixmap'  : 'Draft_FlipDimension',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_FlipDimension", "Flip Dimension"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_FlipDimension", "Flip the normal direction of a dimension")}

    def Activated(self):
        for o in FreeCADGui.Selection.getSelection():
            if Draft.getType(o) in ["Dimension","AngularDimension"]:
                FreeCAD.ActiveDocument.openTransaction("Flip dimension")
                FreeCADGui.doCommand("FreeCAD.ActiveDocument."+o.Name+".Normal = FreeCAD.ActiveDocument."+o.Name+".Normal.negative()")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()


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
        #self.ghost = ghostTracker(self.sel) TODO: solve this (see below)
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


class Draft_Slope():

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Slope',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Slope", "Set Slope"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Slope", "Sets the slope of a selected Line or Wire")}

    def Activated(self):
        if not FreeCADGui.Selection.getSelection():
            return
        for obj in FreeCADGui.Selection.getSelection():
            if Draft.getType(obj) != "Wire":
                FreeCAD.Console.PrintMessage(translate("draft", "This tool only works with Wires and Lines")+"\n")
                return
        w = QtGui.QWidget()
        w.setWindowTitle(translate("Draft","Slope"))
        layout = QtGui.QHBoxLayout(w)
        label = QtGui.QLabel(w)
        label.setText(translate("Draft", "Slope")+":")
        layout.addWidget(label)
        self.spinbox = QtGui.QDoubleSpinBox(w)
        self.spinbox.setMinimum(-9999.99)
        self.spinbox.setMaximum(9999.99)
        self.spinbox.setSingleStep(0.01)
        self.spinbox.setToolTip(translate("Draft", "Slope to give selected Wires/Lines: 0 = horizontal, 1 = 45deg up, -1 = 45deg down"))
        layout.addWidget(self.spinbox)
        taskwidget = QtGui.QWidget()
        taskwidget.form = w
        taskwidget.accept = self.accept
        FreeCADGui.Control.showDialog(taskwidget)

    def accept(self):
        if hasattr(self,"spinbox"):
            pc = self.spinbox.value()
            FreeCAD.ActiveDocument.openTransaction("Change slope")
            for obj in FreeCADGui.Selection.getSelection():
                if Draft.getType(obj) == "Wire":
                    if len(obj.Points) > 1:
                        lp = None
                        np = []
                        for p in obj.Points:
                            if not lp:
                                lp = p
                            else:
                                v = p.sub(lp)
                                z = pc*FreeCAD.Vector(v.x,v.y,0).Length
                                lp = FreeCAD.Vector(p.x,p.y,lp.z+z)
                            np.append(lp)
                        obj.Points = np
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()


class SetAutoGroup():
    """The SetAutoGroup FreeCAD command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AutoGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AutoGroup", "AutoGroup"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AutoGroup", "Select a group to automatically add all Draft & Arch objects to")}

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):
        if hasattr(FreeCADGui,"draftToolBar"):
            self.ui = FreeCADGui.draftToolBar
            s = FreeCADGui.Selection.getSelection()
            if len(s) == 1:
                if (Draft.getType(s[0]) == "Layer") or \
                ( FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").GetBool("AutogroupAddGroups",False) and \
                (s[0].isDerivedFrom("App::DocumentObjectGroup") or (Draft.getType(s[0]) in ["Site","Building","Floor","BuildingPart",]))):
                    self.ui.setAutoGroup(s[0].Name)
                    return
            self.groups = ["None"]
            gn = [o.Name for o in FreeCAD.ActiveDocument.Objects if Draft.getType(o) == "Layer"]
            if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").GetBool("AutogroupAddGroups",False):
                gn.extend(Draft.getGroupNames())
            if gn:
                self.groups.extend(gn)
                self.labels = [translate("draft","None")]
                self.icons = [self.ui.getIcon(":/icons/button_invalid.svg")]
                for g in gn:
                    o = FreeCAD.ActiveDocument.getObject(g)
                    if o:
                        self.labels.append(o.Label)
                        self.icons.append(o.ViewObject.Icon)
                self.labels.append(translate("draft","Add new Layer"))
                self.icons.append(self.ui.getIcon(":/icons/document-new.svg"))
                self.ui.sourceCmd = self
                from PySide import QtCore
                pos = self.ui.autoGroupButton.mapToGlobal(QtCore.QPoint(0,self.ui.autoGroupButton.geometry().height()))
                self.ui.popupMenu(self.labels,self.icons,pos)

    def proceed(self,labelname):
        self.ui.sourceCmd = None
        if labelname in self.labels:
            if labelname == self.labels[0]:
                self.ui.setAutoGroup(None)
            elif labelname == self.labels[-1]:
                FreeCADGui.runCommand("Draft_Layer")
            else:
                i = self.labels.index(labelname)
                self.ui.setAutoGroup(self.groups[i])



class Draft_Label(Creator):
    """The Draft_Label command definition"""

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Label',
                'Accel' : "D, L",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Label", "Label"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Label", "Creates a label, optionally attached to a selected object or element")}

    def Activated(self):
        self.name = translate("draft","Label", utf8_decode=True)
        Creator.Activated(self,self.name,noplanesetup=True)
        self.ghost = None
        self.labeltype = Draft.getParam("labeltype","Custom")
        self.sel = FreeCADGui.Selection.getSelectionEx()
        if self.sel:
            self.sel = self.sel[0]
        self.ui.labelUi(self.name,callback=self.setmode)
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.ghost = DraftTrackers.lineTracker()
        self.call = self.view.addEventCallback("SoEvent",self.action)
        FreeCAD.Console.PrintMessage(translate("draft", "Pick target point")+"\n")
        self.ui.isCopy.hide()

    def setmode(self,i):
        self.labeltype = ["Custom","Name","Label","Position","Length","Area","Volume","Tag","Material"][i]
        Draft.setParam("labeltype",self.labeltype)

    def finish(self,closed=False,cont=False):
        if self.ghost:
            self.ghost.finalize()
        Creator.finish(self)

    def create(self):
        if len(self.node) == 3:
            targetpoint = self.node[0]
            basepoint = self.node[2]
            v = self.node[2].sub(self.node[1])
            dist = v.Length
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                h = FreeCAD.DraftWorkingPlane.u
                n = FreeCAD.DraftWorkingPlane.axis
                r = FreeCAD.DraftWorkingPlane.getRotation().Rotation
            else:
                h = Vector(1,0,0)
                n = Vector(0,0,1)
                r = FreeCAD.Rotation()
            if abs(DraftVecUtils.angle(v,h,n)) <= math.pi/4:
                direction = "Horizontal"
                dist = -dist
            elif abs(DraftVecUtils.angle(v,h,n)) >= math.pi*3/4:
                direction = "Horizontal"
            elif DraftVecUtils.angle(v,h,n) > 0:
                direction = "Vertical"
            else:
                direction = "Vertical"
                dist = -dist
            tp = "targetpoint=FreeCAD."+str(targetpoint)+","
            sel = ""
            if self.sel:
                if self.sel.SubElementNames:
                    sub = "'"+self.sel.SubElementNames[0]+"'"
                else:
                    sub = "()"
                sel="target=(FreeCAD.ActiveDocument."+self.sel.Object.Name+","+sub+"),"
            pl = "placement=FreeCAD.Placement(FreeCAD."+str(basepoint)+",FreeCAD.Rotation"+str(r.Q)+")"
            FreeCAD.ActiveDocument.openTransaction("Create Label")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("l = Draft.makeLabel("+tp+sel+"direction='"+direction+"',distance="+str(dist)+",labeltype='"+self.labeltype+"',"+pl+")")
            FreeCAD.ActiveDocument.recompute()
            FreeCAD.ActiveDocument.commitTransaction()
        self.finish()

    def action(self,arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.affinity = None # don't keep affinity
            if len(self.node) == 2:
                setMod(arg,MODCONSTRAIN,True)
            self.point,ctrlPoint,info = getPoint(self,arg)
            redraw3DView()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    self.ui.redraw()
                    if not self.node:
                        # first click
                        self.node.append(self.point)
                        self.ui.isRelative.show()
                        FreeCAD.Console.PrintMessage(translate("draft", "Pick endpoint of leader line")+"\n")
                        if self.planetrack:
                            self.planetrack.set(self.point)
                    elif len(self.node) == 1:
                        # second click
                        self.node.append(self.point)
                        if self.ghost:
                            self.ghost.p1(self.node[0])
                            self.ghost.p2(self.node[1])
                            self.ghost.on()
                        FreeCAD.Console.PrintMessage(translate("draft", "Pick text position")+"\n")
                    else:
                        # third click
                        self.node.append(self.point)
                        self.create()

    def numericInput(self,numx,numy,numz):
        """this function gets called by the toolbar when valid x, y, and z have been entered there"""
        self.point = Vector(numx,numy,numz)
        if not self.node:
            # first click
            self.node.append(self.point)
            self.ui.isRelative.show()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick endpoint of leader line")+"\n")
            if self.planetrack:
                self.planetrack.set(self.point)
        elif len(self.node) == 1:
            # second click
            self.node.append(self.point)
            if self.ghost:
                self.ghost.p1(self.node[0])
                self.ghost.p2(self.node[1])
                self.ghost.on()
            FreeCAD.Console.PrintMessage(translate("draft", "Pick text position")+"\n")
        else:
            # third click
            self.node.append(self.point)
            self.create()


class Draft_AddConstruction():

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Construction',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddConstruction", "Add to Construction group"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_AddConstruction", "Adds the selected objects to the Construction group")}

    def Activated(self):
        import FreeCADGui
        if hasattr(FreeCADGui,"draftToolBar"):
            col = FreeCADGui.draftToolBar.getDefaultColor("constr")
            col = (float(col[0]),float(col[1]),float(col[2]),0.0)
            gname = Draft.getParam("constructiongroupname","Construction")
            grp = FreeCAD.ActiveDocument.getObject(gname)
            if not grp:
                grp = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",gname)
            for obj in FreeCADGui.Selection.getSelection():
                grp.addObject(obj)
                obrep = obj.ViewObject
                if "TextColor" in obrep.PropertiesList:
                    obrep.TextColor = col
                if "PointColor" in obrep.PropertiesList:
                    obrep.PointColor = col
                if "LineColor" in obrep.PropertiesList:
                    obrep.LineColor = col
                if "ShapeColor" in obrep.PropertiesList:
                    obrep.ShapeColor = col
                if hasattr(obrep,"Transparency"):
                    obrep.Transparency = 80


class Draft_Arc_3Points:


    def GetResources(self):

        return {'Pixmap'  : "Draft_Arc_3Points.svg",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Arc_3Points", "Arc 3 points"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Arc_3Points", "Creates an arc by 3 points"),
                'Accel'   : 'A,T'}

    def IsActive(self):

        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):

        import DraftTrackers
        self.points = []
        self.normal = None
        self.tracker = DraftTrackers.arcTracker()
        self.tracker.autoinvert = False
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            FreeCAD.DraftWorkingPlane.setup()
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.drawArc)

    def getPoint(self,point,info):
        if not point: # cancelled
            self.tracker.off()
            return
        if not(point in self.points): # avoid same point twice
            self.points.append(point)
        if len(self.points) < 3:
            if len(self.points) == 2:
                self.tracker.on()
            FreeCADGui.Snapper.getPoint(last=self.points[-1],callback=self.getPoint,movecallback=self.drawArc)
        else:
            import Part
            e = Part.Arc(self.points[0],self.points[1],self.points[2]).toShape()
            if Draft.getParam("UsePartPrimitives",False):
                o = FreeCAD.ActiveDocument.addObject("Part::Feature","Arc")
                o.Shape = e
            else:
                radius = e.Curve.Radius
                rot = FreeCAD.Rotation(e.Curve.XAxis,e.Curve.YAxis,e.Curve.Axis,"ZXY")
                placement = FreeCAD.Placement(e.Curve.Center,rot)
                start = e.FirstParameter
                end = e.LastParameter/math.pi*180
                c = Draft.makeCircle(radius,placement,startangle=start,endangle=end)
                Draft.autogroup(c)
            self.tracker.off()
            FreeCAD.ActiveDocument.recompute()

    def drawArc(self,point,info):
        if len(self.points) == 2:
            if point.sub(self.points[1]).Length > 0.001:
                self.tracker.setBy3Points(self.points[0],self.points[1],point)


#---------------------------------------------------------------------------
# Snap tools
#---------------------------------------------------------------------------

class Draft_Snap_Lock():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Lock',
                'Accel' : "Shift+S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Toggle On/Off"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Lock", "Activates/deactivates all snap tools at once")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"masterbutton"):
                FreeCADGui.Snapper.masterbutton.toggle()

class Draft_Snap_Midpoint():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Midpoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Midpoint"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Midpoint", "Snaps to midpoints of edges")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonmidpoint":
                        b.toggle()

class Draft_Snap_Perpendicular():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Perpendicular',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Perpendicular"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Perpendicular", "Snaps to perpendicular points on edges")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonperpendicular":
                        b.toggle()

class Draft_Snap_Grid():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Grid',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Grid"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Grid", "Snaps to grid points")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtongrid":
                        b.toggle()

class Draft_Snap_Intersection():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Intersection',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Intersection"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Intersection", "Snaps to edges intersections")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonintersection":
                        b.toggle()

class Draft_Snap_Parallel():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Parallel',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Parallel"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Parallel", "Snaps to parallel directions of edges")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonparallel":
                        b.toggle()

class Draft_Snap_Endpoint():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Endpoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Endpoint"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Endpoint", "Snaps to endpoints of edges")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonendpoint":
                        b.toggle()

class Draft_Snap_Angle():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Angle',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Angles"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Angle", "Snaps to 45 and 90 degrees points on arcs and circles")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonangle":
                        b.toggle()

class Draft_Snap_Center():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Center',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Center", "Center"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Center", "Snaps to center of circles and arcs")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtoncenter":
                        b.toggle()

class Draft_Snap_Extension():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Extension',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Extension"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Extension", "Snaps to extension of edges")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonextension":
                        b.toggle()

class Draft_Snap_Near():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Near',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Near", "Nearest"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Near", "Snaps to nearest point on edges")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonpassive":
                        b.toggle()

class Draft_Snap_Ortho():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Ortho',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Ortho"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Ortho", "Snaps to orthogonal and 45 degrees directions")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonortho":
                        b.toggle()

class Draft_Snap_Special():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Special',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Special", "Special"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Special", "Snaps to special locations of objects")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonspecial":
                        b.toggle()

class Draft_Snap_Dimensions():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_Dimensions',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Dimensions"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_Dimensions", "Shows temporary dimensions when snapping to Arch objects")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonDimensions":
                        b.toggle()

class Draft_Snap_WorkingPlane():
    def GetResources(self):
        return {'Pixmap'  : 'Snap_WorkingPlane',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Working Plane"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Snap_WorkingPlane", "Restricts the snapped point to the current working plane")}
    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if hasattr(FreeCADGui.Snapper,"toolbarButtons"):
                for b in FreeCADGui.Snapper.toolbarButtons:
                    if b.objectName() == "SnapButtonWorkingPlane":
                        b.toggle()

#---------------------------------------------------------------------------
# Adds the icons & commands to the FreeCAD command manager, and sets defaults
#---------------------------------------------------------------------------

# drawing commands

FreeCADGui.addCommand('Draft_Line',Line())
FreeCADGui.addCommand('Draft_Wire',Wire())
FreeCADGui.addCommand('Draft_Circle',Circle())
class CommandArcGroup:
    def GetCommands(self):
        return tuple(['Draft_Arc','Draft_Arc_3Points'])
    def GetResources(self):
        return { 'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ArcTools",'Arc tools'),
                 'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ArcTools",'Arc tools')
               }
    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
FreeCADGui.addCommand('Draft_Arc',Arc())
FreeCADGui.addCommand('Draft_Arc_3Points',Draft_Arc_3Points())
FreeCADGui.addCommand('Draft_ArcTools', CommandArcGroup())
FreeCADGui.addCommand('Draft_Text',Text())
FreeCADGui.addCommand('Draft_Rectangle',Rectangle())
FreeCADGui.addCommand('Draft_Dimension',Dimension())
FreeCADGui.addCommand('Draft_Polygon',Polygon())
FreeCADGui.addCommand('Draft_BSpline',BSpline())
class CommandBezierGroup:
    def GetCommands(self):
        return tuple(['Draft_CubicBezCurve', 'Draft_BezCurve'])
    def GetResources(self):
        return { 'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_BezierTools",'Bezier tools'),
                 'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_BezierTools",'Bezier tools')
               }
    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
FreeCADGui.addCommand('Draft_BezCurve',BezCurve())
FreeCADGui.addCommand('Draft_CubicBezCurve',CubicBezCurve())
FreeCADGui.addCommand('Draft_BezierTools', CommandBezierGroup())
FreeCADGui.addCommand('Draft_Point',Point())
FreeCADGui.addCommand('Draft_Ellipse',Ellipse())
FreeCADGui.addCommand('Draft_ShapeString',ShapeString())
FreeCADGui.addCommand('Draft_Facebinder',Draft_Facebinder())
FreeCADGui.addCommand('Draft_Label',Draft_Label())

# modification commands
FreeCADGui.addCommand('Draft_Move',Move())
FreeCADGui.addCommand('Draft_Rotate',Rotate())
FreeCADGui.addCommand('Draft_Offset',Offset())
FreeCADGui.addCommand('Draft_Join',Join())
FreeCADGui.addCommand('Draft_Split',Split())
FreeCADGui.addCommand('Draft_Upgrade',Upgrade())
FreeCADGui.addCommand('Draft_Downgrade',Downgrade())
FreeCADGui.addCommand('Draft_Trimex',Trimex())
FreeCADGui.addCommand('Draft_Scale',Scale())
FreeCADGui.addCommand('Draft_Drawing',Drawing())
FreeCADGui.addCommand('Draft_SubelementHighlight', SubelementHighlight())
FreeCADGui.addCommand('Draft_AddPoint',AddPoint())
FreeCADGui.addCommand('Draft_DelPoint',DelPoint())
FreeCADGui.addCommand('Draft_WireToBSpline',WireToBSpline())
FreeCADGui.addCommand('Draft_Draft2Sketch',Draft2Sketch())
FreeCADGui.addCommand('Draft_Array',Array())
FreeCADGui.addCommand('Draft_LinkArray',LinkArray())
FreeCADGui.addCommand('Draft_Clone',Draft_Clone())
FreeCADGui.addCommand('Draft_PathArray',PathArray())
FreeCADGui.addCommand('Draft_PathLinkArray',PathLinkArray())
FreeCADGui.addCommand('Draft_PointArray',PointArray())
FreeCADGui.addCommand('Draft_Heal',Heal())
FreeCADGui.addCommand('Draft_Mirror',Mirror())
FreeCADGui.addCommand('Draft_Slope',Draft_Slope())
FreeCADGui.addCommand('Draft_Stretch',Stretch())

# context commands
FreeCADGui.addCommand('Draft_FinishLine',FinishLine())
FreeCADGui.addCommand('Draft_CloseLine',CloseLine())
FreeCADGui.addCommand('Draft_UndoLine',UndoLine())
FreeCADGui.addCommand('Draft_ToggleConstructionMode',ToggleConstructionMode())
FreeCADGui.addCommand('Draft_ToggleContinueMode',ToggleContinueMode())
FreeCADGui.addCommand('Draft_ApplyStyle',ApplyStyle())
FreeCADGui.addCommand('Draft_ToggleDisplayMode',ToggleDisplayMode())
FreeCADGui.addCommand('Draft_AddToGroup',AddToGroup())
FreeCADGui.addCommand('Draft_SelectGroup',SelectGroup())
FreeCADGui.addCommand('Draft_Shape2DView',Shape2DView())
FreeCADGui.addCommand('Draft_ShowSnapBar',ShowSnapBar())
FreeCADGui.addCommand('Draft_ToggleGrid',ToggleGrid())
FreeCADGui.addCommand('Draft_FlipDimension',Draft_FlipDimension())
FreeCADGui.addCommand('Draft_AutoGroup',SetAutoGroup())
FreeCADGui.addCommand('Draft_AddConstruction',Draft_AddConstruction())

# snap commands
FreeCADGui.addCommand('Draft_Snap_Lock',Draft_Snap_Lock())
FreeCADGui.addCommand('Draft_Snap_Midpoint',Draft_Snap_Midpoint())
FreeCADGui.addCommand('Draft_Snap_Perpendicular',Draft_Snap_Perpendicular())
FreeCADGui.addCommand('Draft_Snap_Grid',Draft_Snap_Grid())
FreeCADGui.addCommand('Draft_Snap_Intersection',Draft_Snap_Intersection())
FreeCADGui.addCommand('Draft_Snap_Parallel',Draft_Snap_Parallel())
FreeCADGui.addCommand('Draft_Snap_Endpoint',Draft_Snap_Endpoint())
FreeCADGui.addCommand('Draft_Snap_Angle',Draft_Snap_Angle())
FreeCADGui.addCommand('Draft_Snap_Center',Draft_Snap_Center())
FreeCADGui.addCommand('Draft_Snap_Extension',Draft_Snap_Extension())
FreeCADGui.addCommand('Draft_Snap_Near',Draft_Snap_Near())
FreeCADGui.addCommand('Draft_Snap_Ortho',Draft_Snap_Ortho())
FreeCADGui.addCommand('Draft_Snap_Special',Draft_Snap_Special())
FreeCADGui.addCommand('Draft_Snap_Dimensions',Draft_Snap_Dimensions())
FreeCADGui.addCommand('Draft_Snap_WorkingPlane',Draft_Snap_WorkingPlane())

# a global place to look for active draft Command
FreeCAD.activeDraftCommand = None
