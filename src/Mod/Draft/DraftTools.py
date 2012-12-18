#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *  
#*   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *  
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
__url__ = "http://free-cad.sourceforge.net"

#---------------------------------------------------------------------------
# Generic stuff
#---------------------------------------------------------------------------

import os, FreeCAD, FreeCADGui, WorkingPlane, math, re, importSVG, Draft, Draft_rc, DraftVecUtils
from FreeCAD import Vector
from DraftGui import todo,QtCore,QtGui
from DraftSnap import *
from DraftTrackers import *
from pivy import coin

#---------------------------------------------------------------------------
# Preflight stuff
#---------------------------------------------------------------------------

# update the translation engine
FreeCADGui.updateLocale()

# loads the fill patterns
FreeCAD.svgpatterns = importSVG.getContents(Draft_rc.qt_resource_data,'pattern',True)
altpat = Draft.getParam("patternFile")
if os.path.isdir(altpat):
    for f in os.listdir(altpat):
        if f[-4:].upper() == ".SVG":
            p = importSVG.getContents(altpat+os.sep+f,'pattern')
            if p:
                FreeCAD.svgpatterns.update(p)

# sets the default working plane
plane = WorkingPlane.plane()
FreeCAD.DraftWorkingPlane = plane
defaultWP = Draft.getParam("defaultWP")
if defaultWP == 1: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,0,1), 0)
elif defaultWP == 2: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,1,0), 0)
elif defaultWP == 3: plane.alignToPointAndAxis(Vector(0,0,0), Vector(1,0,0), 0)

# last snapped objects, for quick intersection calculation
lastObj = [0,0]

# set modifier keys
MODS = ["shift","ctrl","alt"]
MODCONSTRAIN = MODS[Draft.getParam("modconstrain")]
MODSNAP = MODS[Draft.getParam("modsnap")]
MODALT = MODS[Draft.getParam("modalt")]

# sets defaults on first load

if not FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod").HasGroup("Draft"):
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    p.SetBool("copymode",1)
    p.SetBool("alwaysSnap",1)
    p.SetBool("showSnapBar",1)
    p.SetUnsigned("constructioncolor",746455039)
    p.SetFloat("textheight",0.2)
    p.SetInt("precision",4)
    p.SetInt("gridEvery",10)
    p.SetFloat("gridSpacing",1.0)
    p.SetInt("UiMode",1)

#---------------------------------------------------------------------------
# General functions
#---------------------------------------------------------------------------


def translate(context,text):
    "convenience function for Qt translator"
    return QtGui.QApplication.translate(context, text, None, QtGui.QApplication.UnicodeUTF8).toUtf8()

def msg(text=None,mode=None):
    "prints the given message on the FreeCAD status bar"
    if not text: FreeCAD.Console.PrintMessage("")
    else:
        if mode == 'warning':
            FreeCAD.Console.PrintWarning(text)
        elif mode == 'error':
            FreeCAD.Console.PrintError(text)
        else:
            FreeCAD.Console.PrintMessage(text)

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

def getPoint(target,args,mobile=False,sym=False,workingplane=True):
    '''
    Function used by the Draft Tools.
    returns a constrained 3d point and its original point.
    if mobile=True, the constraining occurs from the location of
    mouse cursor when Shift is pressed, otherwise from last entered
    point. If sym=True, x and y values stay always equal. If workingplane=False,
    the point wont be projected on the Working Plane.
    '''
    
    ui = FreeCADGui.draftToolBar
    view = Draft.get3DView()

    # get point
    if target.node:
        last = target.node[-1]
    else:
        last = None
    amod = hasMod(args,MODSNAP)
    cmod = hasMod(args,MODCONSTRAIN)
    point = FreeCADGui.Snapper.snap(args["Position"],lastpoint=last,active=amod,constrain=cmod)
    info = FreeCADGui.Snapper.snapInfo
    ctrlPoint = Vector(point)
    mask = FreeCADGui.Snapper.affinity
    if target.node:
        if target.featureName == "Rectangle":
            ui.displayPoint(point, target.node[0], plane=plane, mask=mask)
        else:
            ui.displayPoint(point, target.node[-1], plane=plane, mask=mask)
    else: ui.displayPoint(point, plane=plane, mask=mask)
    return point,ctrlPoint,info

def getSupport(args=None):
    "returns the supporting object and sets the working plane"
    if not args:
        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) == 1:
            sel = sel[0]
            if sel.HasSubObjects:
                if len(sel.SubElementNames) == 1:
                    if "Face" in sel.SubElementNames[0]:
                            plane.alignToFace(sel.SubObjects[0])
                            return sel.Object
        return None
        
    snapped = Draft.get3DView().getObjectInfo((args["Position"][0],args["Position"][1]))
    if not snapped: return None
    obj = None
    plane.save()
    try:
        obj = FreeCAD.ActiveDocument.getObject(snapped['Object'])
        shape = obj.Shape
        component = getattr(shape,snapped["Component"])
        if plane.alignToFace(component, 0) \
                or plane.alignToCurve(component, 0):
            self.display(plane.axis)
    except:
        pass
    return obj

def hasMod(args,mod):
    "checks if args has a specific modifier"
    if mod == "shift":
        return args["ShiftDown"]
    elif mod == "ctrl":
        return args["CtrlDown"]
    elif mod == "alt":
        return args["AltDown"]

def setMod(args,mod,state):
    "sets a specific modifier state in args"
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
    "The base class of all Draft Tools"
    
    def __init__(self):
        self.commitList = []

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self,name="None"):
        if FreeCAD.activeDraftCommand:
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
        self.ui.sourceCmd = self
        self.ui.setTitle(name)
        self.ui.show()
        rot = self.view.getCameraNode().getField("orientation").getValue()
        upv = Vector(rot.multVec(coin.SbVec3f(0,1,0)).getValue())
        plane.setup(DraftVecUtils.neg(self.view.getViewDirection()), Vector(0,0,0), upv)
        self.node = []
        self.pos = []
        self.constrain = None
        self.obj = None
        self.extendedCopy = False
        self.ui.setTitle(name)
        self.featureName = name
        self.planetrack = None
        if Draft.getParam("showPlaneTracker"):
            self.planetrack = PlaneTracker()
		
    def finish(self):
        self.node = []
        FreeCAD.activeDraftCommand = None
        if self.ui:
            self.ui.offUi()
            self.ui.sourceCmd = None
        msg("")
        if self.planetrack:
            self.planetrack.finalize()
        if self.support:
            plane.restore()
        FreeCADGui.Snapper.off()
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
            self.call = None
        if self.commitList:
            todo.delayCommit(self.commitList)
        self.commitList = []

    def commit(self,name,func):
        "stores actions to be committed to the FreeCAD document"
        # print "committing"
        self.commitList.append((name,func))

    def getStrings(self):
        "returns a couple of useful strings fro building python commands"

        # current plane rotation
        p = plane.getRotation()
        qr = p.Rotation.Q
        qr = '('+str(qr[0])+','+str(qr[1])+','+str(qr[2])+','+str(qr[3])+')'

        # support object
        if self.support:
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
# Helper tools
#---------------------------------------------------------------------------    

class SelectPlane(DraftTool):
    "The Draft_SelectPlane FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SelectPlane',
                'Accel' : "W, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_SelectPlane", "SelectPlane"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select a working plane for geometry creation")}
        
    def Activated(self):
        DraftTool.Activated(self)
        self.offset = 0
        if self.doc:
            sel = FreeCADGui.Selection.getSelectionEx()
            if len(sel) == 1:
                sel = sel[0]
                self.ui = FreeCADGui.draftToolBar
                if Draft.getType(sel.Object) == "Axis":
                    plane.alignToEdges(sel.Object.Shape.Edges)
                    self.display(plane.axis)
                    self.finish()
                    return
                elif sel.HasSubObjects:
                    if len(sel.SubElementNames) == 1:
                        if "Face" in sel.SubElementNames[0]:
                            plane.alignToFace(sel.SubObjects[0], self.offset)
                            self.display(plane.axis)
                            self.finish()
                            return
            self.ui.selectPlaneUi()
            msg(translate("draft", "Pick a face to define the drawing plane\n"))
            if plane.alignToSelection(self.offset):
                FreeCADGui.Selection.clearSelection()
                self.display(plane.axis)
                self.finish()
            else:
                self.call = self.view.addEventCallback("SoEvent", self.action)

    def action(self, arg):
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        if arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                doc = FreeCADGui.ActiveDocument
                info = Draft.get3DView().getObjectInfo((cursor[0],cursor[1]))
                if info:
                    try:
                        shape = doc.getObject(info["Object"]).Object.Shape
                        component = getattr(shape, info["Component"])
                        if plane.alignToFace(component, self.offset) \
                                or plane.alignToCurve(component, self.offset):
                            self.display(plane.axis)
                            self.finish()
                    except:
                        pass

    def selectHandler(self, arg):
        try:
            self.offset = float(self.ui.offsetValue.text())
        except:
            self.offset = 0
        if arg == "XY":
            plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,0,1), self.offset)
            self.display('top')
            self.finish()
        elif arg == "XZ":
            plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,-1,0), self.offset)
            self.display('front')
            self.finish()
        elif arg == "YZ":
            plane.alignToPointAndAxis(Vector(0,0,0), Vector(1,0,0), self.offset)
            self.display('side')
            self.finish()
        elif arg == "currentView":
            viewDirection = DraftVecUtils.neg(self.view.getViewDirection())
            plane.alignToPointAndAxis(Vector(0,0,0), viewDirection, self.offset)
            self.display(viewDirection)
            self.finish()
        elif arg == "reset":
            plane.reset()
            self.display('None')
            self.finish()

    def offsetHandler(self, arg):
        self.offset = arg

    def display(self,arg):
        if self.offset:
            if self.offset > 0: suffix = ' + '+str(self.offset)
            else: suffix = ' - '+str(self.offset)
        else: suffix = ''
        if type(arg).__name__  == 'str':
            self.ui.wplabel.setText(arg+suffix)
        elif type(arg).__name__ == 'Vector':
            plv = 'd('+str(arg.x)+','+str(arg.y)+','+str(arg.z)+')'
            self.ui.wplabel.setText(plv+suffix)
        FreeCADGui.Snapper.setGrid()

#---------------------------------------------------------------------------
# Geometry constructors
#---------------------------------------------------------------------------
            
class Creator(DraftTool):
    "A generic Draft Creator Tool used by creation tools such as line or arc"
    
    def __init__(self):
        DraftTool.__init__(self)

    def Activated(self,name="None"):
        DraftTool.Activated(self)
        self.support = getSupport()
            
class Line(Creator):
    "The Line FreeCAD command definition"

    def __init__(self, wiremode=False):
        Creator.__init__(self)
        self.isWire = wiremode

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Line',
                'Accel' : "L,I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Line", "Line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Line", "Creates a 2-point line. CTRL to snap, SHIFT to constrain")}

    def Activated(self,name=str(translate("draft","Line"))):
        Creator.Activated(self,name)
        if self.doc:
            self.obj = None
            if self.isWire:
                self.ui.wireUi(name)
            else:
                self.ui.lineUi(name)
            self.obj=self.doc.addObject("Part::Feature",self.featureName)
            # self.obj.ViewObject.Selectable = False
            Draft.formatObject(self.obj)
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick first point:\n"))

    def finish(self,closed=False,cont=False):
        "terminates the operation and closes the poly if asked"
        if self.obj:
            # remove temporary object, if any
            old = self.obj.Name
            todo.delay(self.doc.removeObject,old)
        self.obj = None
        if (len(self.node) > 1):
            # building command string
            rot,sup,pts,fil = self.getStrings()
            self.commit(translate("draft","Create DWire"),
                        ['import Draft',
                         'points='+pts,
                         'Draft.makeWire(points,closed='+str(closed)+',face='+fil+',support='+sup+')'])
        Creator.finish(self)
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            # key detection
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            # mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg)
        elif arg["Type"] == "SoMouseButtonEvent":
            # mouse button detection
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish(False,cont=True)
                else:
                    if (not self.node) and (not self.support):
                        self.support = getSupport(arg)
                    if self.point:
                        self.pos = arg["Position"]
                        self.node.append(self.point)
                        self.drawSegment(self.point)
                        if (not self.isWire and len(self.node) == 2):
                            self.finish(False,cont=True)
                        if (len(self.node) > 2):
                            if ((self.point-self.node[0]).Length < Draft.tolerance()):
                                self.undolast()
                                self.finish(True,cont=True)
                                msg(translate("draft", "DWire has been closed\n"))

    def undolast(self):
        "undoes last line segment"
        if (len(self.node) > 1):
            self.node.pop()
            last = self.node[len(self.node)-1]
            if self.obj.Shape.Edges:
                edges = self.obj.Shape.Edges
                if len(edges) > 1:
                    edges.pop()
                    newshape = Part.Wire(edges)
                    self.obj.Shape = newshape
                else:
                    self.obj.ViewObject.hide()
                # DNC: report on removal
                msg(translate("draft", "Last point has been removed\n"))

    def drawSegment(self,point):
        "draws a new segment"
        if (len(self.node) == 1):
            msg(translate("draft", "Pick next point:\n"))
            if self.planetrack:
                self.planetrack.set(self.node[0])
        elif (len(self.node) == 2):
            last = self.node[len(self.node)-2]
            newseg = Part.Line(last,point).toShape()
            self.obj.Shape = newseg
            self.obj.ViewObject.Visibility = True
            if self.isWire:
                msg(translate("draft", "Pick next point, or (F)inish or (C)lose:\n"))
        else:
            currentshape = self.obj.Shape.copy()
            last = self.node[len(self.node)-2]
            newseg = Part.Line(last,point).toShape()
            newshape=currentshape.fuse(newseg)
            self.obj.Shape = newshape
            msg(translate("draft", "Pick next point, or (F)inish or (C)lose:\n"))

    def wipe(self):
        "removes all previous segments and starts from last point"
        if len(self.node) > 1:
            # self.obj.Shape.nullify() - for some reason this fails
            self.obj.ViewObject.Visibility = False
            self.node = [self.node[-1]]
            if self.planetrack:
                self.planetrack.set(self.node[0])
            msg(translate("draft", "Pick next point:\n"))
                        
    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.point = Vector(numx,numy,numz)
        self.node.append(self.point)
        self.drawSegment(self.point)
        if (not self.isWire and len(self.node) == 2):
            self.finish(False,cont=True)
        self.ui.setNextFocus()
            
class Wire(Line):
    "a FreeCAD command for creating a wire"
    def __init__(self):
        Line.__init__(self,wiremode=True)
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Wire',
                'Accel' : "W, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Wire", "DWire"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Wire", "Creates a multiple-point DraftWire (DWire). CTRL to snap, SHIFT to constrain")}
    def Activated(self):
        Line.Activated(self,name=str(translate("draft","DWire")))

    
class BSpline(Line):
    "a FreeCAD command for creating a b-spline"
    
    def __init__(self):
        Line.__init__(self,wiremode=True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_BSpline',
                'Accel' : "B, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_BSpline", "B-Spline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_BSpline", "Creates a multiple-point b-spline. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Line.Activated(self,name=str(translate("draft","BSpline")))
        if self.doc:
            self.bsplinetrack = bsplineTracker()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg)
            self.bsplinetrack.update(self.node + [self.point])
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish(False,cont=True)
                else:
                    if (not self.node) and (not self.support):
                        self.support = getSupport(arg)
                    if self.point:
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
                                msg(translate("draft", "Spline has been closed\n"))

    def undolast(self):
        "undoes last line segment"
        if (len(self.node) > 1):
            self.node.pop()
            self.bsplinetrack.update(self.node)
            spline = Part.BSplineCurve()
            spline.interpolate(self.node, False)
            self.obj.Shape = spline.toShape()
            msg(translate("draft", "Last point has been removed\n"))

    def drawUpdate(self,point):
        if (len(self.node) == 1):
            self.bsplinetrack.on()
            if self.planetrack:
                self.planetrack.set(self.node[0])
            msg(translate("draft", "Pick next point:\n"))
        else:
            spline = Part.BSplineCurve()
            spline.interpolate(self.node, False)
            self.obj.Shape = spline.toShape()
            msg(translate("draft", "Pick next point, or (F)inish or (C)lose:\n"))
	
    def finish(self,closed=False,cont=False):
        "terminates the operation and closes the poly if asked"
        if self.ui:
			self.bsplinetrack.finalize()
        if not Draft.getParam("UiMode"):
            FreeCADGui.Control.closeDialog()
        if (len(self.node) > 1):
            old = self.obj.Name
            todo.delay(self.doc.removeObject,old)
            try:
                # building command string
                rot,sup,pts,fil = self.getStrings()
                self.commit(translate("draft","Create BSpline"),
                            ['import Draft',
                             'points='+pts,
                             'Draft.makeBSpline(points,closed='+str(closed)+',face='+fil+',support='+sup+')'])
            except:
                print "Draft: error delaying commit"
        Creator.finish(self)
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

                
class FinishLine:
    "a FreeCAD command to finish any running Line drawing operation"
    
    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.finish(False)
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Finish',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_FinishLine", "Finish line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_FinishLine", "Finishes a line without closing it")}
    def IsActive(self):
        if FreeCAD.activeDraftCommand:
            if FreeCAD.activeDraftCommand.featureName == "Line":
                return True
        return False

    
class CloseLine:
    "a FreeCAD command to close any running Line drawing operation"
    
    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.finish(True)
                
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Lock',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_CloseLine", "Close Line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_CloseLine", "Closes the line being drawn")}
    
    def IsActive(self):
        if FreeCAD.activeDraftCommand:
            if FreeCAD.activeDraftCommand.featureName == "Line":
                return True
        return False


class UndoLine:
    "a FreeCAD command to undo last drawn segment of a line"
    
    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.undolast()
                
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_UndoLine", "Undo last segment"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_UndoLine", "Undoes the last drawn segment of the line being drawn")}
    
    def IsActive(self):
        if FreeCAD.activeDraftCommand:
            if FreeCAD.activeDraftCommand.featureName == "Line":
                return True
        return False

    
class Rectangle(Creator):
    "the Draft_Rectangle FreeCAD command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rectangle',
                'Accel' : "R, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Rectangle", "Rectangle"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Rectangle", "Creates a 2-point rectangle. CTRL to snap")}

    def Activated(self):
        name = str(translate("draft","Rectangle"))
        Creator.Activated(self,name)
        if self.ui:
            self.refpoint = None
            self.ui.pointUi(name)
            self.ui.extUi()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.rect = rectangleTracker()
            msg(translate("draft", "Pick first point:\n"))

    def finish(self,closed=False,cont=False):
        "terminates the operation and closes the poly if asked"
        Creator.finish(self) 
        if self.ui:
            self.rect.off()
            self.rect.finalize()
        if self.ui:
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        "creates the final object in the current doc"
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
            self.commit(translate("draft","Create Rectangle"),
                        ['import Draft',
                         'pl=FreeCAD.Placement()',
                         'pl.Rotation.Q='+rot,
                         'pl.Base='+DraftVecUtils.toString(p1),
                         'Draft.makeRectangle(length='+str(length)+',height='+str(height)+',placement=pl,face='+fil+',support='+sup+')'])
        except:
            print "Draft: error delaying commit"
        self.finish(cont=True)

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg,mobile=True)
            self.rect.update(self.point)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish()
                else:
                    if (not self.node) and (not self.support):
                        self.support = getSupport(arg)
                    if self.point:
                        self.appendPoint(self.point)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.point = Vector(numx,numy,numz)
        self.appendPoint(self.point)

    def appendPoint(self,point):
        self.node.append(point)
        if (len(self.node) > 1):
            self.rect.update(point)
            self.createObject()
        else:
            msg(translate("draft", "Pick opposite point:\n"))
            self.ui.setRelative()
            self.rect.setorigin(point)
            self.rect.on()
            if self.planetrack:
                self.planetrack.set(point)


class Arc(Creator):
    "the Draft_Arc FreeCAD command definition"
        
    def __init__(self):
        self.closedCircle=False
        self.featureName = "Arc"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Arc',
                'Accel' : "A, R",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Arc", "Arc"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Arc", "Creates an arc. CTRL to snap, SHIFT to constrain")}

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
            msg(translate("draft", "Pick center point:\n"))

    def finish(self,closed=False,cont=False):
        "finishes the arc"
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
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point,ctrlPoint,info = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            if self.center and DraftVecUtils.dist(self.point,self.center) > 0:
                viewdelta = DraftVecUtils.project(self.point.sub(self.center), plane.axis)
                if not DraftVecUtils.isNull(viewdelta):
                    self.point = self.point.add(DraftVecUtils.neg(viewdelta))
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
                self.ui.setRadiusValue(self.rad)
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
                self.ui.setRadiusValue(math.degrees(angle))
                self.firstangle = angle
            else: # choose second angle
                currentrad = DraftVecUtils.dist(self.point,self.center)
                if currentrad != 0:
                    angle = DraftVecUtils.angle(plane.u, self.point.sub(self.center), plane.axis)
                else: angle = 0
                self.linetrack.p2(DraftVecUtils.scaleTo(self.point.sub(self.center),self.rad).add(self.center))
                self.ui.setRadiusValue(math.degrees(angle))
                self.updateAngle(angle)
                self.arctrack.setApertureAngle(self.angle)

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.step == 0): # choose center
                        if not self.support:
                            self.support = getSupport(arg)
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
                                    self.linetrack.on()
                                    msg(translate("draft", "Pick radius:\n"))
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
                            self.linetrack.on()
                            msg(translate("draft", "Pick radius:\n"))
                            if self.planetrack:
                                self.planetrack.set(self.point)                        
                    elif (self.step == 1): # choose radius
                        if self.closedCircle:
                            self.drawArc()
                        else: 
                            self.ui.labelRadius.setText("Start angle")
                            self.linetrack.p1(self.center)
                            self.linetrack.on()
                            self.step = 2
                            msg(translate("draft", "Pick start angle:\n"))
                    elif (self.step == 2): # choose first angle
                        self.ui.labelRadius.setText("Aperture")
                        self.step = 3
                        # scale center->point vector for proper display
                        # u = DraftVecUtils.scaleTo(self.point.sub(self.center), self.rad) obsolete?
                        self.arctrack.setStartAngle(self.firstangle)
                        msg(translate("draft", "Pick aperture:\n"))
                    else: # choose second angle
                        self.step = 4
                        self.drawArc()

    def drawArc(self):
        "actually draws the FreeCAD object"
        rot,sup,pts,fil = self.getStrings()
        if self.closedCircle:
            try:
                # building command string
                self.commit(translate("draft","Create Circle"),
                            ['import Draft',
                             'pl=FreeCAD.Placement()',
                             'pl.Rotation.Q='+rot,
                             'pl.Base='+DraftVecUtils.toString(self.center),
                             'Draft.makeCircle(radius='+str(self.rad)+',placement=pl,face='+fil+',support='+sup+')'])
            except:
                print "Draft: error delaying commit"
        else:
            sta = math.degrees(self.firstangle)
            end = math.degrees(self.firstangle+self.angle)
            if end < sta: sta,end = end,sta
            try:
                # building command string
                self.commit(translate("draft","Create Arc"),
                            ['import Draft',
                             'pl=FreeCAD.Placement()',
                             'pl.Rotation.Q='+rot,
                             'pl.Base='+DraftVecUtils.toString(self.center),
                             'Draft.makeCircle(radius='+str(self.rad)+',placement=pl,face='+fil+',startangle='+str(sta)+',endangle='+str(end)+',support='+sup+')'])
            except:
                    print "Draft: error delaying commit"
        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.setNextFocus()
        msg(translate("draft", "Pick radius:\n"))
		
    def numericRadius(self,rad):
        "this function gets called by the toolbar when valid radius have been entered there"
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
                self.ui.labelRadius.setText(str(translate("draft", "Start Angle")))
                self.linetrack.p1(self.center)
                self.linetrack.on()
                self.ui.radiusValue.setText("")
                self.ui.radiusValue.setFocus()
                msg(translate("draft", "Pick start angle:\n"))
        elif (self.step == 2):
            self.ui.labelRadius.setText(str(translate("draft", "Aperture")))
            self.firstangle = math.radians(rad)
            if DraftVecUtils.equals(plane.axis, Vector(1,0,0)): u = Vector(0,self.rad,0)
            else: u = DraftVecUtils.scaleTo(Vector(1,0,0).cross(plane.axis), self.rad)
            urotated = DraftVecUtils.rotate(u, math.radians(rad), plane.axis)
            self.arctrack.setStartAngle(self.firstangle)
            self.step = 3
            self.ui.radiusValue.setText("")
            self.ui.radiusValue.setFocus()
            msg(translate("draft", "Aperture angle:\n"))
        else:
            self.updateAngle(rad)
            self.angle = math.radians(rad)
            self.step = 4
            self.drawArc()

            
class Circle(Arc):
    "The Draft_Circle FreeCAD command definition"
        
    def __init__(self):
        self.closedCircle=True
        self.featureName = "Circle"
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Circle',
                'Accel' : "C, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Circle", "Circle"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Circle", "Creates a circle. CTRL to snap, ALT to select tangent objects")}


class Polygon(Creator):
    "the Draft_Polygon FreeCAD command definition"
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Polygon',
                'Accel' : "P, G",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Polygon", "Polygon"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Polygon", "Creates a regular polygon. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        name = str(translate("draft","Polygon"))
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
            self.altdown = False
            self.ui.sourceCmd = self
            self.arctrack = arcTracker()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick center point:\n"))

    def finish(self,closed=False,cont=False):
        "finishes the arc"
        Creator.finish(self)
        if self.ui:
            self.arctrack.finalize()
            self.doc.recompute()
            if self.ui.continueMode:
                self.Activated()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.point,ctrlPoint,info = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            if self.center and DraftVecUtils.dist(self.point,self.center) > 0:
                viewdelta = DraftVecUtils.project(self.point.sub(self.center), plane.axis)
                if not DraftVecUtils.isNull(viewdelta):
                    self.point = self.point.add(DraftVecUtils.neg(viewdelta))
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
                self.ui.setRadiusValue(self.rad)
                self.arctrack.setRadius(self.rad)

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.step == 0): # choose center
                        if (not self.node) and (not self.support):
                            self.support = getSupport(arg)
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
                                    msg(translate("draft", "Pick radius:\n"))
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
                            msg(translate("draft", "Pick radius:\n"))
                            if self.planetrack:
                                self.planetrack.set(self.point)
                    elif (self.step == 1): # choose radius
                        self.drawPolygon()

    def drawPolygon(self):
        "actually draws the FreeCAD object"
        rot,sup,pts,fil = self.getStrings()        
        # building command string
        self.commit(translate("draft","Create Polygon"),
                    ['import Draft',
                     'pl=FreeCAD.Placement()',
                     'pl.Rotation.Q='+rot,
                     'pl.Base='+DraftVecUtils.toString(self.center),
                     'Draft.makePolygon('+str(self.ui.numFaces.value())+',radius='+str(self.rad)+',inscribed=True,placement=pl,face='+fil+',support='+sup+')'])
        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.radiusValue.setFocus()
        msg(translate("draft", "Pick radius:\n"))
		
    def numericRadius(self,rad):
        "this function gets called by the toolbar when valid radius have been entered there"
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

        
class Text(Creator):
    "This class creates an annotation feature."

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Text',
                'Accel' : "T, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Text", "Text"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Text", "Creates an annotation. CTRL to snap")}

    def Activated(self):
        name = str(translate("draft","Text"))
        Creator.Activated(self,name)
        if self.ui:
            self.dialog = None
            self.text = ''
            self.ui.sourceCmd = self
            self.ui.pointUi(name)
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.ui.xValue.setFocus()
            self.ui.xValue.selectAll()
            msg(translate("draft", "Pick location point:\n"))
            FreeCADGui.draftToolBar.show()

    def finish(self,closed=False,cont=False):
        "terminates the operation"
        Creator.finish(self)
        if self.ui:
            del self.dialog
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        "creates an object in the current doc"
        tx = '['
        for l in self.text:
            if len(tx) > 1:
                tx += ','
            tx += '"'+str(l)+'"'
        tx += ']'
        self.commit(translate("draft","Create Text"),
                    ['import Draft',
                     'Draft.makeText('+tx+',point='+DraftVecUtils.toString(self.node[0])+')'])

        self.finish(cont=True)

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = getPoint(self,arg)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
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
    "The Draft_Dimension FreeCAD command definition"
        
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
        name = str(translate("draft","Dimension"))
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
                msg(translate("draft", "Pick first point:\n"))
                FreeCADGui.draftToolBar.show()

    def hasMeasures(self):
        "checks if only measurements objects are selected"
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            return False
        for o in sel:
            if not o.isDerivedFrom("App::MeasureDistance"):
                return False
        return True

    def finish(self,closed=False):
        "terminates the operation"
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
            self.commit(translate("draft","Create Dimension"),
                        ['import Draft',
                         'Draft.makeDimension('+DraftVecUtils.toString(p1)+','+DraftVecUtils.toString(p2)+','+DraftVecUtils.toString(p3)+')',
                         'FreeCAD.ActiveDocument.removeObject("'+o.Name+'")'])

    def createObject(self):
        "creates an object in the current doc"
        if self.angledata:
            self.commit(translate("draft","Create Dimension"),
                        ['import Draft',
                         'Draft.makeAngularDimension(center='+DraftVecUtils.toString(self.center)+',angles=['+str(self.angledata[0])+','+str(self.angledata[1])+'],p3='+DraftVecUtils.toString(self.node[-1])+')'])
        elif self.link and (not self.arcmode):
            self.commit(translate("draft","Create Dimension"),
                        ['import Draft',
                         'Draft.makeDimension(FreeCAD.ActiveDocument.'+self.link[0].Name+','+str(self.link[1])+','+str(self.link[2])+','+DraftVecUtils.toString(self.node[2])+')'])
        elif self.arcmode:
            self.commit(translate("draft","Create Dimension"),
                        ['import Draft',
                         'Draft.makeDimension(FreeCAD.ActiveDocument.'+self.link[0].Name+','+str(self.link[1])+',"'+str(self.arcmode)+'",'+DraftVecUtils.toString(self.node[2])+')'])                      
        else:
            self.commit(translate("draft","Create Dimension"),
                        ['import Draft',
                         'Draft.makeDimension('+DraftVecUtils.toString(self.node[0])+','+DraftVecUtils.toString(self.node[1])+','+DraftVecUtils.toString(self.node[2])+')'])
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

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            shift = hasMod(arg,MODCONSTRAIN)
            self.point,ctrlPoint,self.info = getPoint(self,arg)
            if self.arcmode or self.point2:
                setMod(arg,MODCONSTRAIN,False)
            if hasMod(arg,MODALT) and (len(self.node)<3):
                self.dimtrack.off()
                if not self.altdown:
                    self.altdown = True
                    self.ui.switchUi(True)
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
                if self.dir:
                    self.point = self.node[0].add(DraftVecUtils.project(self.point.sub(self.node[0]),self.dir))
                if len(self.node) == 2:
                    if self.arcmode and self.edges:
                        cen = self.edges[0].Curve.Center
                        rad = self.edges[0].Curve.Radius
                        baseray = self.point.sub(cen)
                        v2 = DraftVecUtils.scaleTo(baseray,rad)
                        v1 = DraftVecUtils.neg(v2)
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
                    if self.point2:
                        self.node[1] = self.point2
                        self.point2 = None
                # update the dimline
                if self.node and (not self.arcmode):
                    self.dimtrack.update(self.node+[self.point]+[self.cont])
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (not self.node) and (not self.support):
                        self.support = getSupport(arg)
                    if hasMod(arg,MODALT) and (len(self.node)<3):
                        print "snapped: ",self.info
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
                                        if isinstance(ed.Curve,Part.Circle):
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
                                            self.center = c[0]
                                            self.arctrack.setCenter(self.center)
                                            self.arctrack.on()
                                            for e in self.edges:
                                                for v in e.Vertexes:
                                                    self.pts.append(self.arctrack.getAngle(v.Point))
                                            self.link = [self.link[0],ob]
                                        else:
                                            msg(translate("draft", "Edges don't intersect!\n"))
                                            self.finish()                                
                                self.dimtrack.on()
                    else:
                        self.node.append(self.point)
                    #print "node",self.node
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
                        if not self.cont: self.finish()
                    elif (len(self.node) == 3):
                        # for unlinked arc mode:
                        # if self.arcmode:
                        #        v = self.node[1].sub(self.node[0])
                        #        v = DraftVecUtils.scale(v,0.5)
                        #        cen = self.node[0].add(v)
                        #        self.node = [self.node[0],self.node[1],cen]
                        self.createObject()
                        if not self.cont: self.finish()
                    elif self.angledata:
                        self.node.append(self.point)
                        self.createObject()
                        self.finish()

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.point = Vector(numx,numy,numz)
        self.node.append(self.point)
        self.dimtrack.update(self.node)
        if (len(self.node) == 1):
            self.dimtrack.on()
        elif (len(self.node) == 3):
            self.createObject()
            if not self.cont: self.finish()


#---------------------------------------------------------------------------
# Modifier functions
#---------------------------------------------------------------------------

class Modifier(DraftTool):
    "A generic Modifier Tool, used by modification tools such as move"

    def __init__(self):
        DraftTool.__init__(self)
            
class Move(Modifier):
    "The Draft_Move FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Move',
                'Accel' : "M, V",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Move", "Move"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Move", "Moves the selected objects between 2 points. CTRL to snap, SHIFT to constrain, ALT to copy")}
    
    def Activated(self):
        self.name = str(translate("draft","Move"))
        Modifier.Activated(self,self.name)
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to move\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        self.sel = Draft.getGroupContents(self.sel)
        self.ui.pointUi(self.name)
        self.ui.modUi()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.ghost = ghostTracker(self.sel)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick start point:\n"))

    def finish(self,closed=False,cont=False):
        if self.ghost:
            self.ghost.finalize()
        Modifier.finish(self)
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def move(self,delta,copy=False):
        "moving the real shapes"
        sel = '['
        for o in self.sel:
            if len(sel) > 1:
                sel += ','
            sel += 'FreeCAD.ActiveDocument.'+o.Name
        sel += ']'
        if copy:
            self.commit(translate("draft","Copy"),
                        ['import Draft',
                         'Draft.move('+sel+','+DraftVecUtils.toString(delta)+',copy='+str(copy)+')'])
        else:
            self.commit(translate("draft","Move"),
                        ['import Draft',
                         'Draft.move('+sel+','+DraftVecUtils.toString(delta)+',copy='+str(copy)+')'])
        self.doc.recompute()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            if self.ghost:
                self.ghost.off()
            self.point,ctrlPoint,info = getPoint(self,arg)
            if (len(self.node) > 0):
                last = self.node[len(self.node)-1]
                delta = self.point.sub(last)
                if self.ghost:
                    self.ghost.move(delta)
                    self.ghost.on()
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.node == []):
                        self.node.append(self.point)
                        self.ui.isRelative.show()
                        if self.ghost:
                            self.ghost.on()
                        msg(translate("draft", "Pick end point:\n"))
                        if self.planetrack:
                            self.planetrack.set(self.point)
                    else:
                        last = self.node[0]
                        if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                            self.move(self.point.sub(last),True)
                        else:
                            self.move(self.point.sub(last))
                        if hasMod(arg,MODALT):
                            self.extendedCopy = True
                        else:
                            self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.point = Vector(numx,numy,numz)
        if not self.node:
            self.node.append(self.point)
            self.ui.isRelative.show()
            self.ui.isCopy.show()
            self.ghost.on()
            msg(translate("draft", "Pick end point:\n"))
        else:
            last = self.node[-1]
            if self.ui.isCopy.isChecked():
                self.move(self.point.sub(last),True)
            else:
                self.move(self.point.sub(last))
            self.finish()

			
class ApplyStyle(Modifier):
    "The Draft_ApplyStyle FreeCA command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Apply',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Apply Current Style"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Applies current line width and color to selected objects")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False

    def Activated(self):
        Modifier.Activated(self)
        if self.ui:
            self.sel = Draft.getSelection()
            if (len(self.sel)>0):
                c = ['import Draft']
                for ob in self.sel:
                    if (ob.Type == "App::DocumentObjectGroup"):
                        c.extend(self.formatGroup(ob))
                    else:
                        c.append('Draft.formatObject(FreeCAD.ActiveDocument.'+ob.Name+')')
                self.commit(translate("draft","Change Style"),c)

    def formatGroup(self,grpob):
        c=[]
        for ob in grpob.Group:
            if (ob.Type == "App::DocumentObjectGroup"):
                c.extend(self.formatGroup(ob))
            else:
                c.append('Draft.formatObject(FreeCAD.ActiveDocument.'+ob.Name+')') 
			
class Rotate(Modifier):
    "The Draft_Rotate FreeCAD command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'Accel' : "R, O",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Rotate", "Rotate"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Rotate", "Rotates the selected objects. CTRL to snap, SHIFT to constrain, ALT creates a copy")}

    def Activated(self):
        Modifier.Activated(self,"Rotate")
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.arctrack = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to rotate\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        self.sel = Draft.getGroupContents(self.sel)
        self.step = 0
        self.center = None
        self.ui.arcUi()
        self.ui.isCopy.show()
        self.ui.setTitle("Rotate")
        self.arctrack = arcTracker()
        self.ghost = ghostTracker(self.sel)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick rotation center:\n"))
				
    def finish(self,closed=False,cont=False):
        "finishes the arc"
        Modifier.finish(self)
        if self.arctrack:
            self.arctrack.finalize()
        if self.ghost:
            self.ghost.finalize()
        if self.doc:
            self.doc.recompute()
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def rot (self,angle,copy=False):
        "rotating the real shapes"
        sel = '['
        for o in self.sel:
            if len(sel) > 1:
                sel += ','
            sel += 'FreeCAD.ActiveDocument.'+o.Name
        sel += ']'
        if copy:
            self.commit(translate("draft","Copy"),
                        ['import Draft',
                         'Draft.rotate('+sel+','+str(math.degrees(angle))+','+DraftVecUtils.toString(self.center)+',axis='+DraftVecUtils.toString(plane.axis)+',copy='+str(copy)+')'])
        else:
            self.commit(translate("draft","Rotate"),
                        ['import Draft',
                         'Draft.rotate('+sel+','+str(math.degrees(angle))+','+DraftVecUtils.toString(self.center)+',axis='+DraftVecUtils.toString(plane.axis)+',copy='+str(copy)+')'])

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            if self.ghost:
                self.ghost.off()
            self.point,ctrlPoint,info = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            if self.center and DraftVecUtils.dist(self.point,self.center):
                viewdelta = DraftVecUtils.project(self.point.sub(self.center), plane.axis)
                if not DraftVecUtils.isNull(viewdelta):
                    self.point = self.point.add(DraftVecUtils.neg(viewdelta))
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
                self.ui.radiusValue.setText("%.2f" % math.degrees(angle))
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
                if self.ghost:
                    self.ghost.rotate(plane.axis,sweep)
                    self.ghost.on()
                self.ui.radiusValue.setText("%.2f" % math.degrees(sweep))
                self.ui.radiusValue.setFocus()
                self.ui.radiusValue.selectAll()
                
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.step == 0):
                        self.center = self.point
                        self.node = [self.point]
                        self.ui.radiusUi()
                        self.ui.hasFill.hide()
                        self.ui.labelRadius.setText("Base angle")
                        self.arctrack.setCenter(self.center)
                        if self.ghost:
                            self.ghost.center(self.center)
                        self.step = 1
                        msg(translate("draft", "Pick base angle:\n"))
                        if self.planetrack:
                            self.planetrack.set(self.point)
                    elif (self.step == 1):
                        self.ui.labelRadius.setText("Rotation")
                        self.rad = DraftVecUtils.dist(self.point,self.center)
                        self.arctrack.on()
                        self.arctrack.setStartPoint(self.point)
                        if self.ghost:
                            self.ghost.on()
                        self.step = 2
                        msg(translate("draft", "Pick rotation angle:\n"))
                    else:
                        currentrad = DraftVecUtils.dist(self.point,self.center)
                        angle = self.point.sub(self.center).getAngle(plane.u)
                        if DraftVecUtils.project(self.point.sub(self.center), plane.v).getAngle(plane.v) > 1:
                            angle = -angle
                        if (angle < self.firstangle): 
                            sweep = (2*math.pi-self.firstangle)+angle
                        else:
                            sweep = angle - self.firstangle
                        if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                            self.rot(sweep,True)
                        else:
                            self.rot(sweep)
                        if hasMod(arg,MODALT):
                            self.extendedCopy = True
                        else:
                            self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        if self.ghost:
            self.ghost.center(self.center)
        self.ui.radiusUi()
        self.ui.hasFill.hide()
        self.ui.labelRadius.setText("Base angle")
        self.step = 1
        msg(translate("draft", "Pick base angle:\n"))

    def numericRadius(self,rad):
        "this function gets called by the toolbar when valid radius have been entered there"
        if (self.step == 1):
            self.ui.labelRadius.setText("Rotation")
            self.firstangle = math.radians(rad)
            self.arctrack.setStartAngle(self.firstangle)
            self.arctrack.on()
            if self.ghost:
                self.ghost.on()
            self.step = 2
            msg(translate("draft", "Pick rotation angle:\n"))
        else:
            self.rot(math.radians(rad),self.ui.isCopy.isChecked())
            self.finish(cont=True)


class Offset(Modifier):
    "The Draft_Offset FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Offset',
                'Accel' : "O, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Offset", "Offset"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Offset", "Offsets the active object. CTRL to snap, SHIFT to constrain, ALT to copy")}

    def Activated(self):
        self.running = False
        Modifier.Activated(self,"Offset")
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.linetrack = None
                self.arctrack = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to offset\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            elif len(Draft.getSelection()) > 1:
                msg(translate("draft", "Offset only works on one object at a time\n"),"warning")
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()[0]
        if not self.sel.isDerivedFrom("Part::Feature"):
            msg(translate("draft", "Cannot offset this object type\n"),"warning")
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
            else:
                self.ghost = wireTracker(self.shape)
                self.mode = "Wire"
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick distance:\n"))
            if self.planetrack:
                self.planetrack.set(self.shape.Vertexes[0].Point)
            self.running = True

    def action(self,arg):
        "scene event handler"
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
                    d = DraftVecUtils.neg(dist[0])
                    v1 = DraftGeomUtils.getTangent(self.shape.Edges[0],self.point)
                    v2 = DraftGeomUtils.getTangent(self.shape.Edges[dist[1]],self.point)
                    a = -DraftVecUtils.angle(v1,v2)
                    self.dvec = DraftVecUtils.rotate(d,a,plane.axis)
                    occmode = self.ui.occOffset.isChecked()
                    self.ghost.update(DraftGeomUtils.offsetWire(self.shape,self.dvec,occ=occmode),forceclosed=occmode)
                elif self.mode == "BSpline":
                    d = DraftVecUtils.neg(dist[0])
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
                self.ui.radiusValue.setText("%.2f" % dist[0].Length)
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
                                
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                copymode = False
                occmode = self.ui.occOffset.isChecked()
                if hasMod(arg,MODALT) or self.ui.isCopy.isChecked(): copymode = True
                if self.npts:
                    print "offset:npts=",self.npts
                    self.commit(translate("draft","Offset"),
                                ['import Draft',
                                 'Draft.offset(FreeCAD.ActiveDocument.'+self.sel.Name+','+DraftVecUtils.toString(self.ntps)+',copy='+str(copymode)+')'])
                elif self.dvec:
                    if isinstance(self.dvec,float):
                        d = str(self.dvec)
                    else:
                        d = DraftVecUtils.toString(self.dvec)
                    self.commit(translate("draft","Offset"),
                                ['import Draft',
                                 'Draft.offset(FreeCAD.ActiveDocument.'+self.sel.Name+','+d+',copy='+str(copymode)+',occ='+str(occmode)+')'])
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
        if self.dvec:
            self.dvec.normalize()
            self.dvec.multiply(rad)
            copymode = False
            occmode = self.ui.occOffset.isChecked()
            if self.ui.isCopy.isChecked(): copymode = True
            if isinstance(self.dvec,float):
                d = str(self.dvec)
            else:
                d = DraftVecUtils.toString(self.dvec)
            self.commit(translate("draft","Offset"),
                        ['import Draft',
                         'Draft.offset(FreeCAD.ActiveDocument.'+self.sel.Name+','+d+',copy='+str(copymode)+',occ='+str(occmode)+')'])
            self.finish()

            
class Upgrade(Modifier):
    '''The Draft_Upgrade FreeCAD command definition.
    This class upgrades selected objects in different ways,
    following this list (in order):
    - if there are more than one faces, the faces are merged (union)
    - if there is only one face, nothing is done
    - if there are closed wires, they are transformed in a face
    - otherwise join all edges into a wire (closed if applicable)
    - if nothing of the above is possible, a Compound is created
    '''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Upgrade',
                'Accel' : "U, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Upgrade", "Upgrade"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Upgrade", "Joins the selected objects into one, or converts closed wires to filled faces, or unite faces")}

    def Activated(self):
        Modifier.Activated(self,"Upgrade")
        if self.ui:
            if not Draft.getSelection():
                self.ui.selectUi()
                msg(translate("draft", "Select an object to upgrade\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def compound(self):
        # shapeslist = []
        # for ob in self.sel: shapeslist.append(ob.Shape)
        # newob = self.doc.addObject("Part::Feature","Compound")
        # newob.Shape = Part.makeCompound(shapeslist)
        newob = Draft.makeBlock(self.sel)
        self.nodelete = True
        return newob
		
    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        newob = None
        self.nodelete = False
        edges = []
        wires = []
        openwires = []
        faces = []
        groups = []
        curves = []
        facewires = []
        
        # determining what we have in our selection
        for ob in self.sel:
            if ob.Type == "App::DocumentObjectGroup":
                groups.append(ob)
            else:
                if ob.Shape.ShapeType == 'Edge': openwires.append(ob.Shape)
                for f in ob.Shape.Faces:
                    faces.append(f)
                    facewires.extend(f.Wires)
                for w in ob.Shape.Wires:
                    if w.isClosed():
                        wires.append(w)
                    else:
                        openwires.append(w)
                for e in ob.Shape.Edges:
                    if not isinstance(e.Curve,Part.Line):
                        curves.append(e)
                lastob = ob
        # print "objects:",self.sel," edges:",edges," wires:",wires," openwires:",openwires," faces:",faces
        # print "groups:",groups," curves:",curves," facewires:",facewires
                
        # applying transformation
        self.doc.openTransaction("Upgrade")
        
        if groups:
            # if we have a group: turn each closed wire inside into a face
            msg(translate("draft", "Found groups: closing each open object inside\n"))
            for grp in groups: 
                for ob in grp.Group:
                    if not ob.Shape.Faces:
                        for w in ob.Shape.Wires:
                            newob = Draft.makeWire(w,closed=w.isClosed())
                            self.sel.append(ob)
                            grp.addObject(newob)
                            
        elif faces and (len(wires)+len(openwires)==len(facewires)):
            # we have only faces here, no lone edges
            
            if (len(self.sel) == 1) and (len(faces) > 1):
                # we have a shell: we try to make a solid
                sol = Part.makeSolid(self.sel[0].Shape)
                if sol.isClosed():
                    msg(translate("draft", "Found 1 solidificable object: solidifying it\n"))
                    newob = self.doc.addObject("Part::Feature","Solid")
                    newob.Shape = sol
                    Draft.formatObject(newob,lastob)
                        
            elif (len(self.sel) == 2) and (not curves):
                # we have exactly 2 objects: we fuse them
                msg(translate("draft", "Found 2 objects: fusing them\n"))
                newob = Draft.fuse(self.sel[0],self.sel[1])
                self.nodelete = True
                                
            elif (len(self.sel) > 2) and (len(faces) > 6):
                # we have many separate faces: we try to make a shell
                sh = Part.makeShell(faces)
                newob = self.doc.addObject("Part::Feature","Shell")
                newob.Shape = sh
                Draft.formatObject(newob,lastob)
                
            elif (len(self.sel) > 2) or (len(faces) > 1):
                # more than 2 objects or faces: we try the draft way: make one face out of them
                u = faces.pop(0)
                for f in faces:
                    u = u.fuse(f)
                if DraftGeomUtils.isCoplanar(faces):
                    if self.sel[0].ViewObject.DisplayMode == "Wireframe":
                        f = False
                    else:
                        f = True
                    u = DraftGeomUtils.concatenate(u)
                    if not curves:
                        # several coplanar and non-curved faces: they can becoem a Draft wire
                        msg(translate("draft", "Found several objects or faces: making a parametric face\n"))
                        newob = Draft.makeWire(u.Wires[0],closed=True,face=f)
                        Draft.formatObject(newob,lastob)
                    else:
                        # if not possible, we do a non-parametric union
                        msg(translate("draft", "Found objects containing curves: fusing them\n"))
                        newob = self.doc.addObject("Part::Feature","Union")
                        newob.Shape = u
                        Draft.formatObject(newob,lastob)
                else:
                    # if not possible, we do a non-parametric union
                    msg(translate("draft", "Found several objects: fusing them\n"))
                    # if we have a solid, make sure we really return a solid
                    if (len(u.Faces) > 1) and u.isClosed():
                        u = Part.makeSolid(u)
                    newob = self.doc.addObject("Part::Feature","Union")
                    newob.Shape = u
                    Draft.formatObject(newob,lastob)
            elif len(self.sel) == 1:
                # only one object: if not parametric, we "draftify" it
                self.nodelete = True
                if (not curves) and (Draft.getType(self.sel[0]) == "Part"):
                    msg(translate("draft", "Found 1 non-parametric objects: draftifying it\n"))
                    Draft.draftify(self.sel[0])
                else:
                    msg(translate("draft", "No upgrade available for this object\n"))
                    self.doc.abortTransaction()
                    return
            else:
                msg(translate("draft", "Couldn't upgrade these objects\n"))
                self.doc.abortTransaction()
                return
                                        
        elif wires and (not faces) and (not openwires):
            # we have only wires, no faces
            
            if (len(self.sel) == 1) and self.sel[0].isDerivedFrom("Sketcher::SketchObject") and (not curves):
                # we have a sketch
                msg(translate("draft", "Found 1 closed sketch object: making a face from it\n"))
                newob = Draft.makeWire(self.sel[0].Shape,closed=True)
                newob.Base = self.sel[0]
                self.sel[0].ViewObject.Visibility = False
                self.nodelete = True
                
            else:
                # only closed wires
                for w in wires:
                    if DraftGeomUtils.isPlanar(w):
                        f = Part.Face(w)
                        faces.append(f)
                    else:
                        msg(translate("draft", "One wire is not planar, upgrade not done\n"))
                        self.nodelete = True
                for f in faces:
                    # if there are curved segments, we do a non-parametric face
                    msg(translate("draft", "Found a closed wire: making a face\n"))
                    newob = self.doc.addObject("Part::Feature","Face")
                    newob.Shape = f
                    Draft.formatObject(newob,lastob)
                    newob.ViewObject.DisplayMode = "Flat Lines"
                        
        elif (len(openwires) == 1) and (not faces) and (not wires):
            # special case, we have only one open wire. We close it, unless it has only 1 edge!"
            p0 = openwires[0].Vertexes[0].Point
            p1 = openwires[0].Vertexes[-1].Point
            edges = openwires[0].Edges
            if len(edges) > 1:
                edges.append(Part.Line(p1,p0).toShape())
            w = Part.Wire(DraftGeomUtils.sortEdges(edges))
            if len(edges) == 1:
                if len(w.Vertexes) == 2:
                    msg(translate("draft", "Found 1 open edge: making a line\n"))
                    newob = Draft.makeWire(w,closed=False)
                elif len(w.Vertexes) == 1:
                    msg(translate("draft", "Found 1 circular edge: making a circle\n"))
                    c = w.Edges[0].Curve.Center
                    r = w.Edges[0].Curve.Radius
                    p = FreeCAD.Placement()
                    p.move(c)
                    newob = Draft.makeCircle(r,p)
            else:
                msg(translate("draft", "Found 1 open wire: closing it\n"))
                if not curves:
                    newob = Draft.makeWire(w,closed=True)
                else:
                    # if not possible, we do a non-parametric union
                    newob = self.doc.addObject("Part::Feature","Wire")
                    newob.Shape = w
                    Draft.formatObject(newob,lastob)

        elif openwires and (not wires) and (not faces):
            # only open wires and edges: we try to join their edges
            for ob in self.sel:
                for e in ob.Shape.Edges:
                    edges.append(e)
            newob = None
            nedges = DraftGeomUtils.sortEdges(edges[:])
            # for e in nedges: print "debug: ",e.Curve,e.Vertexes[0].Point,e.Vertexes[-1].Point
            w = Part.Wire(nedges)
            if len(w.Edges) == len(edges):
                msg(translate("draft", "Found several edges: wiring them\n"))
                newob = self.doc.addObject("Part::Feature","Wire")
                newob.Shape = w
                Draft.formatObject(newob,lastob)
            if not newob:
                print "no new object found"
                msg(translate("draft", "Found several non-connected edges: making compound\n"))
                newob = self.compound()
                Draft.formatObject(newob,lastob)
        else:
            # all other cases
            msg(translate("draft", "Found several non-treatable objects: making compound\n"))
            newob = self.compound()
            Draft.formatObject(newob,lastob)
            
        if not self.nodelete:
            # deleting original objects, if needed
            for ob in self.sel:
                if not ob.Type == "App::DocumentObjectGroup":
                    self.doc.removeObject(ob.Name)
                    
        self.doc.commitTransaction()
        if newob: Draft.select(newob)
        Modifier.finish(self)

        
class Downgrade(Modifier):
    '''
    The Draft_Downgrade FreeCAD command definition.
    This class downgrades selected objects in different ways,
    following this list (in order):
    - if there are more than one faces, the subsequent
    faces are subtracted from the first one
    - if there is only one face, it gets converted to a wire
    - otherwise wires are exploded into single edges
    '''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Downgrade',
                'Accel' : "D, N",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Downgrade", "Downgrade"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Downgrade", "Explodes the selected objects into simpler objects, or subtract faces")}

    def Activated(self):
        Modifier.Activated(self,"Downgrade")
        if self.ui:
            if not Draft.getSelection():
                self.ui.selectUi()
                msg(translate("draft", "Select an object to upgrade\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        self.sel = Draft.getSelection()
        edges = []
        faces = []

        # scanning objects
        for ob in self.sel:
            for f in ob.Shape.Faces:
                faces.append(f)
        for ob in self.sel:
            for e in ob.Shape.Edges:
                edges.append(e)
            lastob = ob
                        
        # applying transformation
        self.doc.openTransaction("Downgrade")
        
        if (len(self.sel) == 1) and (Draft.getType(self.sel[0]) == "Block"):
            # we have a block, we explode it
            pl = self.sel[0].Placement
            newob = []
            for ob in self.sel[0].Components:
                ob.ViewObject.Visibility = True
                ob.Placement = ob.Placement.multiply(pl)
                newob.append(ob)
            self.doc.removeObject(self.sel[0].Name)
            
        elif (len(self.sel) == 1) and (self.sel[0].isDerivedFrom("Part::Feature")) and ("Base" in self.sel[0].PropertiesList):
            # special case, we have one parametric object: we "de-parametrize" it
            msg(translate("draft", "Found 1 parametric object: breaking its dependencies\n"))
            newob = Draft.shapify(self.sel[0])
            
        elif len(self.sel) == 2:
            # we have only 2 objects: cut 2nd from 1st
            msg(translate("draft", "Found 2 objects: subtracting them\n"))
            newob = Draft.cut(self.sel[0],self.sel[1])
            
        elif (len(faces) > 1):
            
            if len(self.sel) == 1:
                # one object with several faces: split it
                for f in faces:
                    msg(translate("draft", "Found several faces: splitting them\n"))
                    newob = self.doc.addObject("Part::Feature","Face")
                    newob.Shape = f
                    Draft.formatObject(newob,self.sel[0])
                self.doc.removeObject(ob.Name)
                
            else:
                # several objects: remove all the faces from the first one
                msg(translate("draft", "Found several objects: subtracting them from the first one\n"))
                u = faces.pop(0)
                for f in faces:
                    u = u.cut(f)
                newob = self.doc.addObject("Part::Feature","Subtraction")
                newob.Shape = u
                for ob in self.sel:
                    Draft.formatObject(newob,ob)
                    self.doc.removeObject(ob.Name)
                    
        elif (len(faces) > 0):
            # only one face: we extract its wires
            msg(translate("draft", "Found 1 face: extracting its wires\n"))
            for w in faces[0].Wires:
                newob = self.doc.addObject("Part::Feature","Wire")
                newob.Shape = w
                Draft.formatObject(newob,lastob)
            for ob in self.sel:
                self.doc.removeObject(ob.Name)
                
        else:
            # no faces: split wire into single edges
            onlyedges = True
            for ob in self.sel:
                if ob.Shape.ShapeType != "Edge":
                    onlyedges = False
            if onlyedges:
                msg(translate("draft", "No more downgrade possible\n"))
                self.doc.abortTransaction()
                return
            msg(translate("draft", "Found only wires: extracting their edges\n"))
            for ob in self.sel:
                for e in edges:
                    newob = self.doc.addObject("Part::Feature","Edge")
                    newob.Shape = e
                    Draft.formatObject(newob,ob)
                self.doc.removeObject(ob.Name)
        self.doc.commitTransaction()
        Draft.select(newob)
        Modifier.finish(self)


class Trimex(Modifier):
    ''' The Draft_Trimex FreeCAD command definition.
    This tool trims or extends lines, wires and arcs,
    or extrudes single faces. SHIFT constrains to the last point
    or extrudes in direction to the face normal.'''

    def GetResources(self):
        return {'Pixmap' : 'Draft_Trimex',
                'Accel' : "T, R",
                'MenuText' : QtCore.QT_TRANSLATE_NOOP("Draft_Trimex", "Trimex"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Trimex", "Trims or extends the selected object, or extrudes single faces. CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts")}

    def Activated(self):
        Modifier.Activated(self,"Trimex")
        self.edges = []
        self.placement = None
        self.ghost = None
        self.linetrack = None
        if self.ui:
            if not Draft.getSelection():
                self.ui.selectUi()
                msg(translate("draft", "Select an object to trim/extend\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.obj = Draft.getSelection()[0]
        self.ui.trimUi()
        self.linetrack = lineTracker()

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
            self.obj.ViewObject.Visibility = False
            self.extrudeMode = False
            if self.obj.Shape.Wires:
                self.edges = self.obj.Shape.Wires[0].Edges
                self.edges = DraftGeomUtils.sortEdges(self.edges)
            else:
                self.edges = self.obj.Shape.Edges	
            self.ghost = []
            lc = self.obj.ViewObject.LineColor
            sc = (lc[0],lc[1],lc[2])
            sw = self.obj.ViewObject.LineWidth
            for e in self.edges:
                if isinstance(e.Curve,Part.Line):
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
        msg(translate("draft", "Pick distance:\n"))

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.shift = hasMod(arg,MODCONSTRAIN)
            self.alt = hasMod(arg,MODALT)
            if self.extrudeMode:
                arg["ShiftDown"] = False
            wp = not(self.extrudeMode and self.shift)
            self.point,cp,info = getPoint(self,arg,workingplane=wp)
            if hasMod(arg,MODSNAP): self.snapped = None
            else: self.snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
            if self.extrudeMode:
                dist = self.extrude(self.shift)
            else:
                dist = self.redraw(self.point,self.snapped,self.shift,self.alt)
            self.ui.radiusValue.setText("%.2f" % dist)
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
			
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
        "redraws the ghost in extrude mode"
        self.newpoint = self.obj.Shape.Faces[0].CenterOfMass
        dvec = self.point.sub(self.newpoint)
        if not shift: delta = DraftVecUtils.project(dvec,self.normal)
        else: delta = dvec
        if self.force:
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
        "redraws the ghost"
        
        # initializing
        reverse = False
        for g in self.ghost: g.off()
        if real: newedges = []
        
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
            pts = []
            for e in snapped.Shape.Edges:
                int = DraftGeomUtils.findIntersection(edge,e,True,True)
                if int: pts.extend(int)
            if pts:
                point = pts[DraftGeomUtils.findClosest(point,pts)]

        # modifying active edge
        if isinstance(edge.Curve,Part.Line):
            perp = DraftGeomUtils.vec(edge).cross(Vector(0,0,1))
            chord = v1.sub(point)
            proj = DraftVecUtils.project(chord,perp)
            self.newpoint = Vector.add(point,proj)
            dist = v1.sub(self.newpoint).Length
            ghost.p1(self.newpoint)
            ghost.p2(v2)
            self.ui.labelRadius.setText("Distance")
            if real:
                if self.force:
                    ray = self.newpoint.sub(v1)
                    ray = DraftVecUtils.scale(ray,self.force/ray.Length)
                    self.newpoint = Vector.add(v1,ray)
                newedges.append(Part.Line(self.newpoint,v2).toShape())
        else:
            center = edge.Curve.Center
            rad = edge.Curve.Radius
            ang1 = DraftVecUtils.angle(v2.sub(center))
            ang2 = DraftVecUtils.angle(point.sub(center))
            self.newpoint=Vector.add(center,DraftVecUtils.rotate(Vector(rad,0,0),-ang2))
            self.ui.labelRadius.setText("Angle")
            dist = math.degrees(-ang2)
            # if ang1 > ang2: ang1,ang2 = ang2,ang1
            print "last calculated:",math.degrees(-ang1),math.degrees(-ang2)
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
        if not reverse: list = range(npoint+1,len(self.edges))
        else: list = range(npoint-1,-1,-1)
        for i in list:
            edge = self.edges[i]
            ghost = self.ghost[i]
            if isinstance(edge.Curve,Part.Line):
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
        "trims the actual object"
        if self.extrudeMode:
            delta = self.extrude(self.shift,real=True)
            print "delta",delta
            self.doc.openTransaction("Extrude")
            obj = Draft.extrude(self.obj,delta)
            self.doc.commitTransaction()
            self.obj = obj
        else:
            edges = self.redraw(self.point,self.snapped,self.shift,self.alt,real=True)
            newshape = Part.Wire(edges)
            self.doc.openTransaction("Trim/extend")
            if Draft.getType(self.obj) in ["Wire","BSpline"]:
                p = []
                if self.placement: invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement: np = invpl.multVec(np)
                    p.append(np)
                self.obj.Points = p
            elif Draft.getType(self.obj) == "Circle":
                angles = self.ghost[0].getAngles()
                print "original",self.obj.FirstAngle," ",self.obj.LastAngle
                print "new",angles
                if angles[0] > angles[1]: angles = (angles[1],angles[0])
                self.obj.FirstAngle = angles[0]
                self.obj.LastAngle = angles[1]
            else:
                self.obj.Shape = newshape
            self.doc.commitTransaction()
        for g in self.ghost: g.off()

    def finish(self,closed=False):		
        Modifier.finish(self)
        self.force = None
        if self.ui:
            if self.linetrack:
                self.linetrack.finalize()
            if self.ghost:
                for g in self.ghost:
                    g.finalize()
            self.obj.ViewObject.Visibility = True
            Draft.select(self.obj)

    def numericRadius(self,dist):
        "this function gets called by the toolbar when valid distance have been entered there"
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
        self.name = str(translate("draft","Scale"))
        Modifier.Activated(self,self.name)
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to scale\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        self.sel = Draft.getGroupContents(self.sel)
        self.ui.pointUi(self.name)
        self.ui.modUi()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.ghost = ghostTracker(self.sel)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick base point:\n"))

    def finish(self,closed=False,cont=False):
        Modifier.finish(self)
        if self.ghost:
            self.ghost.finalize()
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def scale(self,delta,copy=False):
        "moving the real shapes"
        sel = '['
        for o in self.sel:
            if len(sel) > 1:
                sel += ','
            sel += 'FreeCAD.ActiveDocument.'+o.Name
        sel += ']'
        if copy:
            self.commit(translate("draft","Copy"),
                        ['import Draft',
                         'Draft.scale('+sel+',delta='+DraftVecUtils.toString(delta)+',center='+DraftVecUtils.toString(self.node[0])+',copy='+str(copy)+')'])
        else:
            self.commit(translate("draft","Scale"),
                        ['import Draft',
                         'Draft.scale('+sel+',delta='+DraftVecUtils.toString(delta)+',center='+DraftVecUtils.toString(self.node[0])+',copy='+str(copy)+')'])                     

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            if self.ghost:
                self.ghost.off()
            self.point,ctrlPoint,info = getPoint(self,arg,sym=True)
            if (len(self.node) > 0):
                last = self.node[len(self.node)-1]
                delta = self.point.sub(last)
                if self.ghost:
                    self.ghost.scale(delta)
                    # calculate a correction factor depending on the scaling center
                    corr = Vector(self.node[0].x,self.node[0].y,self.node[0].z)
                    corr.scale(delta.x,delta.y,delta.z)
                    corr = DraftVecUtils.neg(corr.sub(self.node[0]))
                    self.ghost.move(corr)
                    self.ghost.on()
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.point:
                    if (self.node == []):
                        self.node.append(self.point)
                        self.ui.isRelative.show()
                        self.ui.isCopy.show()
                        if self.ghost:
                            self.ghost.on()
                        msg(translate("draft", "Pick scale factor:\n"))
                    else:
                        last = self.node[0]
                        if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                            self.scale(self.point.sub(last),True)
                        else:
                            self.scale(self.point.sub(last))
                        if hasMod(arg,MODALT):
                            self.extendedCopy = True
                        else:
                            self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.point = Vector(numx,numy,numz)
        if not self.node:
            self.node.append(self.point)
            self.ui.isRelative.show()
            self.ui.isCopy.show()
            if self.ghost:
                self.ghost.on()
            msg(translate("draft", "Pick scale factor:\n"))
        else:
            last = self.node[-1]
            if self.ui.isCopy.isChecked():
                self.scale(self.point.sub(last),True)
            else:
                self.scale(self.point.sub(last))
            self.finish(cont=True)

            
class ToggleConstructionMode():
    "The Draft_ToggleConstructionMode FreeCAD command definition"

    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode", "Toggle construcion Mode"),
                'Accel' : "C, M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode", "Toggles the Construction Mode for next objects.")}
    
    def Activated(self):
        FreeCADGui.draftToolBar.constrButton.toggle()

        
class ToggleContinueMode():
    "The Draft_ToggleContinueMode FreeCAD command definition"

    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleContinueMode", "Toggle continue Mode"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleContinueMode", "Toggles the Continue Mode for next commands.")}

    def Activated(self):
        FreeCADGui.draftToolBar.continueCmd.toggle()

        
class Drawing(Modifier):
    "The Draft Drawing command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Drawing',
                'Accel' : "D, D",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Drawing", "Drawing"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Drawing", "Puts the selected objects on a Drawing sheet.")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False

    def Activated(self):
        Modifier.Activated(self,"Drawing")
        sel = Draft.getSelection()
        if not sel:
            self.page = self.createDefaultPage()
        else:
            self.page = None
            for obj in sel:
                if obj.isDerivedFrom("Drawing::FeaturePage"):
                    self.page = obj
                    sel.pop(sel.index(obj))
            if not self.page:
                for obj in self.doc.Objects:
                    if obj.isDerivedFrom("Drawing::FeaturePage"):
                        self.page = obj
            if not self.page:
                self.page = self.createDefaultPage()
            sel.reverse() 
            for obj in sel:
                if obj.ViewObject.isVisible():
                    name = 'View'+obj.Name
                    oldobj = self.page.getObject(name)
                    if oldobj: self.doc.removeObject(oldobj.Name)
                    Draft.makeDrawingView(obj,self.page)
            self.doc.recompute()

    def createDefaultPage(self):
        "created a default page"
        template = Draft.getParam("template")
        if not template:
            template = FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg'
        page = self.doc.addObject('Drawing::FeaturePage','Page')
        page.ViewObject.HintOffsetX = 200
        page.ViewObject.HintOffsetY = 100
        page.ViewObject.HintScale = 20
        page.Template = template
        self.doc.recompute()
        return page

    
class ToggleDisplayMode():
    "The ToggleDisplayMode FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SwitchMode',
                'Accel' : "Shift+Space",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode", "Toggle display mode"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode", "Swaps display mode of selected objects between wireframe and flatlines")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        for obj in Draft.getSelection():
            if obj.ViewObject.DisplayMode == "Flat Lines":
                if "Wireframe" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Wireframe"
            elif obj.ViewObject.DisplayMode == "Wireframe":
                if "Flat Lines" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Flat Lines"


class Edit(Modifier):
    "The Draft_Edit FreeCAD command definition"

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Edit',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edits the active object")}

    def IsActive(self):
        if Draft.getSelection():
            self.selection = Draft.getSelection()
            if "Proxy" in self.selection[0].PropertiesList:
                if hasattr(self.selection[0].Proxy,"Type"):
                    return True
        return False

    def Activated(self):
        if self.running:
            self.finish()
        else:
            Modifier.Activated(self,"Edit")
            self.ui.editUi()
            if self.doc:
                self.obj = Draft.getSelection()
                if self.obj:
                    self.obj = self.obj[0]
                    # store selectable state of the object
                    if hasattr(self.obj.ViewObject,"Selectable"):
                        self.selectstate = self.obj.ViewObject.Selectable
                        self.obj.ViewObject.Selectable = False
                    if Draft.getType(self.obj) in ["Wire","BSpline"]:
                        self.ui.setEditButtons(True)
                    else:
                        self.ui.setEditButtons(False)
                    self.editing = None
                    self.editpoints = []
                    self.pl = None
                    if "Placement" in self.obj.PropertiesList:
                        self.pl = self.obj.Placement
                        self.invpl = self.pl.inverse()
                    if Draft.getType(self.obj) in ["Wire","BSpline"]:
                        for p in self.obj.Points:
                            if self.pl: p = self.pl.multVec(p)
                            self.editpoints.append(p)
                    elif Draft.getType(self.obj) == "Circle":
                        self.editpoints.append(self.obj.Placement.Base)
                        if self.obj.FirstAngle == self.obj.LastAngle:
                            self.editpoints.append(self.obj.Shape.Vertexes[0].Point)
                    elif Draft.getType(self.obj) == "Rectangle":
                        self.editpoints.append(self.obj.Placement.Base)
                        self.editpoints.append(self.obj.Shape.Vertexes[2].Point)
                        v = self.obj.Shape.Vertexes
                        self.bx = v[1].Point.sub(v[0].Point)
                        if self.obj.Length < 0: self.bx = DraftVecUtils.neg(self.bx)
                        self.by = v[2].Point.sub(v[1].Point)
                        if self.obj.Height < 0: self.by = DraftVecUtils.neg(self.by)
                    elif Draft.getType(self.obj) == "Polygon":
                        self.editpoints.append(self.obj.Placement.Base)
                        self.editpoints.append(self.obj.Shape.Vertexes[0].Point)
                    elif Draft.getType(self.obj) == "Dimension":
                        p = self.obj.ViewObject.Proxy.textpos.translation.getValue()
                        self.editpoints.append(self.obj.Start)
                        self.editpoints.append(self.obj.End)
                        self.editpoints.append(self.obj.Dimline)
                        self.editpoints.append(Vector(p[0],p[1],p[2]))
                    self.trackers = []
                    if self.editpoints:
                        for ep in range(len(self.editpoints)):
                            self.trackers.append(editTracker(self.editpoints[ep],self.obj.Name,
                                                             ep,self.obj.ViewObject.LineColor))
                        self.call = self.view.addEventCallback("SoEvent",self.action)
                        self.running = True
                        plane.save()
                        if "Shape" in self.obj.PropertiesList:
                            plane.alignToFace(self.obj.Shape)
                        if self.planetrack:
                            self.planetrack.set(self.editpoints[0])
                    else:
                        msg(translate("draft", "This object type is not editable\n"),'warning')
                        self.finish()
                else:
                    self.finish()

    def finish(self,closed=False):
        "terminates the operation"
        if closed:
            if "Closed" in self.obj.PropertiesList:
                if not self.obj.Closed:
                    self.obj.Closed = True
        if self.ui:
            if self.trackers:
                for t in self.trackers:
                    t.finalize()
        if hasattr(self.obj.ViewObject,"Selectable"):
            self.obj.ViewObject.Selectable = self.selectstate
        Modifier.finish(self)
        plane.restore()
        self.running = False
        FreeCADGui.ActiveDocument.resetEdit()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
            elif arg["Key"] == "f":
                self.finish()
            elif arg["Key"] == "c":
                self.finish(closed=True)
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            if self.editing != None:
                self.point,ctrlPoint,info = getPoint(self,arg)
                self.trackers[self.editing].set(self.point)
                self.update(self.trackers[self.editing].get())
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.editing == None:
                    sel = FreeCADGui.Selection.getSelectionEx()
                    if sel:
                        sel = sel[0]
                        if sel.ObjectName == self.obj.Name:
                            if self.ui.addButton.isChecked():
                                if self.point:
                                    self.pos = arg["Position"]
                                    self.addPoint(self.point)
                            elif self.ui.delButton.isChecked():
                                if 'EditNode' in sel.SubElementNames[0]:
                                    self.delPoint(int(sel.SubElementNames[0][8:]))
                            elif 'EditNode' in sel.SubElementNames[0]:
                                self.ui.pointUi()
                                self.ui.isRelative.show()
                                self.editing = int(sel.SubElementNames[0][8:])
                                self.trackers[self.editing].off()
                                if hasattr(self.obj.ViewObject,"Selectable"):
                                    self.obj.ViewObject.Selectable = False
                                if "Points" in self.obj.PropertiesList:
                                    self.node.append(self.obj.Points[self.editing])
                else:
                    self.trackers[self.editing].on()
                    if hasattr(self.obj.ViewObject,"Selectable"):
                        self.obj.ViewObject.Selectable = True
                    self.numericInput(self.trackers[self.editing].get())

    def update(self,v):
        if Draft.getType(self.obj) in ["Wire","BSpline"]:
            pts = self.obj.Points
            editPnt = self.invpl.multVec(v)
            # DNC: allows to close the curve by placing ends close to each other
            tol = 0.001
            if ( ( self.editing == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( self.editing == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
                self.obj.Closed = True
            # DNC: fix error message if edited point coinsides with one of the existing points
            if ( editPnt in pts ) == False:
                pts[self.editing] = editPnt
                self.obj.Points = pts
                self.trackers[self.editing].set(v)
        elif Draft.getType(self.obj) == "Circle":
            delta = v.sub(self.obj.Placement.Base)
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
                self.trackers[0].set(self.obj.Placement.Base)
            elif self.editing == 1:
                self.obj.Radius = delta.Length
            self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        elif Draft.getType(self.obj) == "Rectangle":
            delta = v.sub(self.obj.Placement.Base)
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
            elif self.editing == 1:
                diag = v.sub(self.obj.Placement.Base)
                nx = DraftVecUtils.project(diag,self.bx)
                ny = DraftVecUtils.project(diag,self.by)
                ax = nx.Length
                ay = ny.Length
                if ax and ay:
                    if abs(nx.getAngle(self.bx)) > 0.1:
                        ax = -ax
                    if abs(ny.getAngle(self.by)) > 0.1:
                        ay = -ay
                    self.obj.Length = ax
                    self.obj.Height = ay
            self.trackers[0].set(self.obj.Placement.Base)
            self.trackers[1].set(self.obj.Shape.Vertexes[2].Point)
        elif Draft.getType(self.obj) == "Polygon":
            delta = v.sub(self.obj.Placement.Base)
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
                self.trackers[0].set(self.obj.Placement.Base)
            elif self.editing == 1:
                if self.obj.DrawMode == 'inscribed':
                    self.obj.Radius = delta.Length
                else:
                    halfangle = ((math.pi*2)/self.obj.FacesNumber)/2
                    rad = math.cos(halfangle)*delta.Length
                    self.obj.Radius = rad
            self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        elif Draft.getType(self.obj) == "Dimension":
            if self.editing == 0:
                self.obj.Start = v
            elif self.editing == 1:
                self.obj.End = v
            elif self.editing == 2:
                self.obj.Dimline = v
            elif self.editing == 3:
                self.obj.ViewObject.TextPosition = v        

    def numericInput(self,v,numy=None,numz=None):
        '''this function gets called by the toolbar
        when valid x, y, and z have been entered there'''
        if (numy != None):
            v = Vector(v,numy,numz)
        self.doc.openTransaction("Edit "+self.obj.Name)
        self.update(v)
        self.doc.commitTransaction()
        self.editing = None
        self.ui.editUi()
        self.node = []
       
    def addPoint(self,point):
        if not (Draft.getType(self.obj) in ["Wire","BSpline"]): return
        pts = self.obj.Points
        if ( Draft.getType(self.obj) == "Wire" ):
            if (self.obj.Closed == True):
                # DNC: work around.... seems there is a
                # bug in approximate method for closed wires...
                edges = self.obj.Shape.Wires[0].Edges
                e1 = edges[-1] # last edge
                v1 = e1.Vertexes[0].Point
                v2 = e1.Vertexes[1].Point
                v2.multiply(0.9999)
                edges[-1] = Part.makeLine(v1,v2)
                edges.reverse()
                wire = Part.Wire(edges)
                curve = wire.approximate(0.0001,0.0001,100,25)
            else:
                # DNC: this version is much more reliable near sharp edges!
                curve = self.obj.Shape.Wires[0].approximate(0.0001,0.0001,100,25)
        elif ( Draft.getType(self.obj) == "BSpline" ):
            if (self.obj.Closed == True):
                curve = self.obj.Shape.Edges[0].Curve
            else:
                curve = self.obj.Shape.Curve
        uNewPoint = curve.parameter(point)
        uPoints = []
        for p in self.obj.Points:
            uPoints.append(curve.parameter(p))
        for i in range(len(uPoints)-1):
            if ( uNewPoint > uPoints[i] ) and ( uNewPoint < uPoints[i+1] ):
                pts.insert(i+1, self.invpl.multVec(point))
                break
        # DNC: fix: add points to last segment if curve is closed 
        if ( self.obj.Closed ) and ( uNewPoint > uPoints[-1] ) :
            pts.append(self.invpl.multVec(point))
        self.doc.openTransaction("Edit "+self.obj.Name)
        self.obj.Points = pts
        self.doc.commitTransaction()
        self.resetTrackers()
        
    def delPoint(self,point):
        if not (Draft.getType(self.obj) in ["Wire","BSpline"]): return
        if len(self.obj.Points) <= 2:
            msg(translate("draft", "Active object must have more than two points/nodes\n"),'warning')
        else: 
            pts = self.obj.Points
            pts.pop(point)
            self.doc.openTransaction("Edit "+self.obj.Name)
            self.obj.Points = pts
            self.doc.commitTransaction()
            self.resetTrackers()

    def resetTrackers(self):
        for t in self.trackers:
            t.finalize()
        self.trackers = []
        for ep in range(len(self.obj.Points)):
            objPoints = self.obj.Points[ep]
            if self.pl: objPoints = self.pl.multVec(objPoints)
            self.trackers.append(editTracker(objPoints,self.obj.Name,ep,self.obj.ViewObject.LineColor))

            
class AddToGroup():
    "The AddToGroup FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AddToGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddToGroup", "Add to group..."),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AddToGroup", "Adds the selected object(s) to an existing group")}

    def IsActive(self):
        if Draft.getSelection():
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
            for obj in Draft.getSelection():
                try:
                    Draft.ungroup(obj)
                except:
                    pass
        else:
            if labelname in self.labels:
                i = self.labels.index(labelname)
                g = FreeCAD.ActiveDocument.getObject(self.groups[i])
                for obj in Draft.getSelection():
                    try:
                        g.addObject(obj)
                    except:
                        pass

                    
class AddPoint(Modifier):
    "The Draft_AddPoint FreeCAD command definition"

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AddPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddPoint", "Add Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AddPoint", "Adds a point to an existing wire/bspline")}

    def IsActive(self):
        self.selection = Draft.getSelection()
        if (Draft.getType(self.selection[0]) in ['Wire','BSpline']):
            return True
        else:
            return False

    def Activated(self):
        FreeCADGui.draftToolBar.vertUi(True)
        FreeCADGui.runCommand("Draft_Edit")

        
class DelPoint(Modifier):
    "The Draft_DelPoint FreeCAD command definition"

    def __init__(self):
        self.running = False
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_DelPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_DelPoint", "Remove Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_DelPoint", "Removes a point from an existing wire or bspline")}

    def IsActive(self):
        self.selection = Draft.getSelection()
        if (Draft.getType(self.selection[0]) in ['Wire','BSpline']):
            return True
        else:
            return False

    def Activated(self):
        FreeCADGui.draftToolBar.vertUi(False)
        FreeCADGui.runCommand("Draft_Edit")

        
class WireToBSpline(Modifier):
    "The Draft_Wire2BSpline FreeCAD command definition"

    def __init__(self):
        self.running = False
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_WireToBSpline',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Wire to BSpline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Converts between Wire and BSpline")}

    def IsActive(self):
        self.selection = Draft.getSelection()
        if (Draft.getType(self.selection[0]) in ['Wire','BSpline']):
            return True
        else:
            return False

    def Activated(self):
        if self.running:
            self.finish()
        else:
            Modifier.Activated(self,"Convert Curve Type")
            if self.doc:
                self.obj = Draft.getSelection()
                if self.obj:
                    self.obj = self.obj[0]
                    self.pl = None
                    if "Placement" in self.obj.PropertiesList:
                        self.pl = self.obj.Placement
                    self.Points = self.obj.Points
                    self.closed = self.obj.Closed
                    n = None
                    if (Draft.getType(self.selection[0]) == 'Wire'):
                        n = Draft.makeBSpline(self.Points, self.closed, self.pl)
                    elif (Draft.getType(self.selection[0]) == 'BSpline'):
                        n = Draft.makeWire(self.Points, self.closed, self.pl)
                    if n:
                        Draft.formatObject(n,self.selection[0])
                else:
                    self.finish()
	def finish(self):
		Modifier.finish(self)


class SelectGroup():
    "The SelectGroup FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SelectGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_SelectGroup", "Select group"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_SelectGroup", "Selects all objects with the same parents as this group")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sellist = []
        sel = Draft.getSelection()
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


class Shape2DView():
    "The Shape2DView FreeCAD command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Draft_2DShapeView',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Shape 2D view"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Creates Shape 2D views of selected objects")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        faces = []
        objs = []
        sel = FreeCADGui.Selection.getSelectionEx()
        for s in sel:
            objs.append(s.Object)
            for e in s.SubElementNames:
                if "Face" in e:
                    faces.append(int(e[4:])-1)
        print objs,faces
        if len(objs) == 1:
            if faces:
                Draft.makeShape2DView(objs[0],facenumbers=faces)
                return
        for o in objs:
            Draft.makeShape2DView(o)

class Draft2Sketch():
    "The Draft2Sketch FreeCAD command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Draft2Sketch',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Draft2Sketch", "Draft to Sketch"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Convert bidirectionally between Draft and Sketch objects")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sel = Draft.getSelection()
        allSketches = True
        allDraft = True
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
            FreeCAD.ActiveDocument.openTransaction("Convert to Sketch")
            Draft.makeSketch(sel,autoconstraints=True)
            FreeCAD.ActiveDocument.commitTransaction()
        elif allSketches:
            FreeCAD.ActiveDocument.openTransaction("Convert to Draft")
            Draft.draftify(sel,makeblock=True)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.openTransaction("Convert")
            for obj in sel:
                if obj.isDerivedFrom("Sketcher::SketchObject"):
                    Draft.draftify(obj)
                elif obj.isDerivedFrom("Part::Part2DObjectPython"):
                    Draft.makeSketch(obj,autoconstraints=True)
                elif obj.isDerivedFrom("Part::Feature"):
                    if (len(obj.Shape.Wires) == 1) or (len(obj.Shape.Edges) == 1):
                        Draft.makeSketch(obj,autoconstraints=False)
            FreeCAD.ActiveDocument.commitTransaction()

                 
class Array():
    "The Shape2DView FreeCAD command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Array',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Array", "Array"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Array", "Creates a polar or rectangular array from a selected object")}

    def IsActive(self):
        if len(Draft.getSelection()) == 1:
            return True
        else:
            return False
        
    def Activated(self):
        obj = Draft.getSelection()[0]
        FreeCAD.ActiveDocument.openTransaction("Array")
        Draft.makeArray(obj,Vector(1,0,0),Vector(0,1,0),2,2)
        FreeCAD.ActiveDocument.commitTransaction()

class Point:
    "this class will create a vertex after the user clicks a point on the screen"

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
        self.view = Draft.get3DView()
        self.stack = []
        rot = self.view.getCameraNode().getField("orientation").getValue()
        upv = Vector(rot.multVec(coin.SbVec3f(0,1,0)).getValue())
        plane.setup(DraftVecUtils.neg(self.view.getViewDirection()), Vector(0,0,0), upv)
        self.point = None
        # adding 2 callback functions
        self.callbackClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.click)
        self.callbackMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.move)

    def move(self,event_cb):
        event = event_cb.getEvent()
        mousepos = event.getPosition().getValue()
        ctrl = event.wasCtrlDown()
        self.point = FreeCADGui.Snapper.snap(mousepos,active=ctrl)
        
    def click(self,event_cb):
        event = event_cb.getEvent()
        if event.getState() == coin.SoMouseButtonEvent.DOWN:
            if self.point:
                self.stack.append(self.point)
                if len(self.stack) == 1:
                    self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callbackClick)
                    self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.callbackMove)
                    FreeCAD.ActiveDocument.openTransaction("Create Point")
                    Draft.makePoint((self.stack[0][0]),(self.stack[0][1]),self.stack[0][2])
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCADGui.Snapper.off()

class ToggleSnap():
    "The ToggleSnap FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Snap_Lock',
                'Accel' : "Shift+S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleSnap", "Toggle snap"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ToggleSnap", "Toggles Draft snap on or off")}

    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.toggle()

class ShowSnapBar():
    "The ShowSnapBar FreeCAD command definition"

    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Show Snap Bar"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ShowSnapBar", "Shows Draft snap toolbar")}

    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.show()


class Draft_Clone():
    "The Draft Clone command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Clone',
                'Accel' : "C,L",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Clone", "Clone"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Clone", "Clones the selected object(s)")}

    def Activated(self):
        if FreeCADGui.Selection.getSelection():
            FreeCAD.ActiveDocument.openTransaction("Clone")
            for obj in FreeCADGui.Selection.getSelection():
                Draft.clone(obj)
            FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False


class ToggleGrid():
    "The Draft ToggleGrid command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Snap_Grid',
                'Accel' : "G,R",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleGrid", "Toggle Grid"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ToggleGrid", "Toggles the Draft gid on/off")}

    def Activated(self):
        if hasattr(FreeCADGui,"Snapper"):
            if FreeCADGui.Snapper.grid:
                if FreeCADGui.Snapper.grid.Visible:
                    FreeCADGui.Snapper.grid.off()
                    FreeCADGui.Snapper.forceGridOff=True
                else:
                    FreeCADGui.Snapper.grid.on()
                    FreeCADGui.Snapper.forceGridOff=False
            else:
                FreeCADGui.Snapper.show()
                    
#---------------------------------------------------------------------------
# Adds the icons & commands to the FreeCAD command manager, and sets defaults
#---------------------------------------------------------------------------
		
# drawing commands
FreeCADGui.addCommand('Draft_SelectPlane',SelectPlane())
FreeCADGui.addCommand('Draft_Line',Line())
FreeCADGui.addCommand('Draft_Wire',Wire())
FreeCADGui.addCommand('Draft_Circle',Circle())
FreeCADGui.addCommand('Draft_Arc',Arc())
FreeCADGui.addCommand('Draft_Text',Text())
FreeCADGui.addCommand('Draft_Rectangle',Rectangle())
FreeCADGui.addCommand('Draft_Dimension',Dimension())
FreeCADGui.addCommand('Draft_Polygon',Polygon())
FreeCADGui.addCommand('Draft_BSpline',BSpline())
FreeCADGui.addCommand('Draft_Point',Point())

# modification commands
FreeCADGui.addCommand('Draft_Move',Move())
FreeCADGui.addCommand('Draft_Rotate',Rotate())
FreeCADGui.addCommand('Draft_Offset',Offset())
FreeCADGui.addCommand('Draft_Upgrade',Upgrade())
FreeCADGui.addCommand('Draft_Downgrade',Downgrade())
FreeCADGui.addCommand('Draft_Trimex',Trimex())
FreeCADGui.addCommand('Draft_Scale',Scale())
FreeCADGui.addCommand('Draft_Drawing',Drawing())
FreeCADGui.addCommand('Draft_Edit',Edit())
FreeCADGui.addCommand('Draft_AddPoint',AddPoint())
FreeCADGui.addCommand('Draft_DelPoint',DelPoint())
FreeCADGui.addCommand('Draft_WireToBSpline',WireToBSpline())
FreeCADGui.addCommand('Draft_Draft2Sketch',Draft2Sketch())
FreeCADGui.addCommand('Draft_Array',Array())
FreeCADGui.addCommand('Draft_Clone',Draft_Clone())

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
FreeCADGui.addCommand('Draft_ToggleSnap',ToggleSnap())
FreeCADGui.addCommand('Draft_ShowSnapBar',ShowSnapBar())
FreeCADGui.addCommand('Draft_ToggleGrid',ToggleGrid())

# a global place to look for active draft Command
FreeCAD.activeDraftCommand = None


