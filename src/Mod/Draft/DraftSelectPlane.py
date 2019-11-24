# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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

__title__="FreeCAD Draft Workbench GUI Tools - Working plane-related tools"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin"
__url__ = "https://www.freecadweb.org"


import FreeCAD
import FreeCADGui
import math
import Draft
import DraftVecUtils
from DraftGui import todo, translate

def QT_TRANSLATE_NOOP(ctx,txt): return txt



class Draft_SelectPlane:

    """The Draft_SelectPlane FreeCAD command definition"""

    def __init__(self):

        self.ac = "FreeCAD.DraftWorkingPlane.alignToPointAndAxis"
        self.param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.states = []

    def GetResources(self):

        return {'Pixmap'  : 'Draft_SelectPlane',
                'Accel' : "W, P",
                'MenuText': QT_TRANSLATE_NOOP("Draft_SelectPlane", "SelectPlane"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select a working plane for geometry creation")}

    def IsActive(self):

        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):

        # reset variables
        self.view = Draft.get3DView()
        self.wpButton = FreeCADGui.draftToolBar.wplabel
        FreeCAD.DraftWorkingPlane.setup()
        
        # write current WP if states are empty
        if not self.states:
            p = FreeCAD.DraftWorkingPlane
            self.states.append([p.u, p.v, p.axis, p.position])

        m = translate("draft", "Pick a face, 3 vertices or a WP Proxy to define the drawing plane")
        FreeCAD.Console.PrintMessage(m+"\n")

        from PySide import QtCore,QtGui

        # create UI panel
        FreeCADGui.Control.closeDialog()
        self.taskd = SelectPlane_TaskPanel()

        # fill values
        self.taskd.form.checkCenter.setChecked(self.param.GetBool("CenterPlaneOnView",False))
        q = FreeCAD.Units.Quantity(self.param.GetFloat("gridSpacing",1.0),FreeCAD.Units.Length)
        self.taskd.form.fieldGridSpacing.setText(q.UserString)
        self.taskd.form.fieldGridMainLine.setValue(self.param.GetInt("gridEvery",10))
        self.taskd.form.fieldSnapRadius.setValue(self.param.GetInt("snapRange",8))

        # set icons
        self.taskd.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"))
        self.taskd.form.buttonTop.setIcon(QtGui.QIcon(":/icons/view-top.svg"))
        self.taskd.form.buttonFront.setIcon(QtGui.QIcon(":/icons/view-front.svg"))
        self.taskd.form.buttonSide.setIcon(QtGui.QIcon(":/icons/view-right.svg"))
        self.taskd.form.buttonAlign.setIcon(QtGui.QIcon(":/icons/view-isometric.svg"))
        self.taskd.form.buttonAuto.setIcon(QtGui.QIcon(":/icons/view-axonometric.svg"))
        self.taskd.form.buttonMove.setIcon(QtGui.QIcon(":/icons/Draft_Move.svg"))
        self.taskd.form.buttonCenter.setIcon(QtGui.QIcon(":/icons/view-fullscreen.svg"))
        self.taskd.form.buttonPrevious.setIcon(QtGui.QIcon(":/icons/edit-undo.svg"))

        # connect slots
        self.taskd.form.buttonTop.clicked.connect(self.onClickTop)
        self.taskd.form.buttonFront.clicked.connect(self.onClickFront)
        self.taskd.form.buttonSide.clicked.connect(self.onClickSide)
        self.taskd.form.buttonAlign.clicked.connect(self.onClickAlign)
        self.taskd.form.buttonAuto.clicked.connect(self.onClickAuto)
        self.taskd.form.buttonMove.clicked.connect(self.onClickMove)
        self.taskd.form.buttonCenter.clicked.connect(self.onClickCenter)
        self.taskd.form.buttonPrevious.clicked.connect(self.onClickPrevious)
        self.taskd.form.fieldGridSpacing.textEdited.connect(self.onSetGridSize)
        self.taskd.form.fieldGridMainLine.valueChanged.connect(self.onSetMainline)
        self.taskd.form.fieldSnapRadius.valueChanged.connect(self.onSetSnapRadius)

        # try to find a WP from the current selection
        if self.handle():
            return

        # try other method
        if FreeCAD.DraftWorkingPlane.alignToSelection():
            FreeCADGui.Selection.clearSelection()
            self.display(FreeCAD.DraftWorkingPlane.axis)
            self.finish()
            return
        
        # rock 'n roll!
        FreeCADGui.Control.showDialog(self.taskd)
        self.call = self.view.addEventCallback("SoEvent", self.action)

    def finish(self,close=False):

        # store values
        self.param.SetBool("CenterPlaneOnView",self.taskd.form.checkCenter.isChecked())

        # terminate coin callbacks
        if self.call:
            try:
                self.view.removeEventCallback("SoEvent",self.call)
            except RuntimeError:
                # the view has been deleted already
                pass
            self.call = None

        # reset everything else
        FreeCADGui.Control.closeDialog()
        FreeCAD.DraftWorkingPlane.restore()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):

        self.finish()
        return True

    def action(self, arg):

        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        if arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                # coin detection happens before the selection got a chance of being updated, so we must delay
                todo.delay(self.checkSelection,None)

    def checkSelection(self):

        if self.handle():
            self.finish()

    def handle(self):
        
        """tries to build a WP. Returns True if successful"""

        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) == 1:
            sel = sel[0]
            if Draft.getType(sel.Object) == "Axis":
                FreeCAD.DraftWorkingPlane.alignToEdges(sel.Object.Shape.Edges)
                self.display(FreeCAD.DraftWorkingPlane.axis)
                return True
            elif Draft.getType(sel.Object) in ["WorkingPlaneProxy","BuildingPart"]:
                FreeCAD.DraftWorkingPlane.setFromPlacement(sel.Object.Placement,rebase=True)
                FreeCAD.DraftWorkingPlane.weak = False
                if hasattr(sel.Object.ViewObject,"AutoWorkingPlane"):
                    if sel.Object.ViewObject.AutoWorkingPlane:
                        FreeCAD.DraftWorkingPlane.weak = True
                if hasattr(sel.Object.ViewObject,"CutView") and hasattr(sel.Object.ViewObject,"AutoCutView"):
                    if sel.Object.ViewObject.AutoCutView:
                        sel.Object.ViewObject.CutView = True
                if hasattr(sel.Object.ViewObject,"RestoreView"):
                    if sel.Object.ViewObject.RestoreView:
                        if hasattr(sel.Object.ViewObject,"ViewData"):
                            if len(sel.Object.ViewObject.ViewData) >= 12:
                                d = sel.Object.ViewObject.ViewData
                                camtype = "orthographic"
                                if len(sel.Object.ViewObject.ViewData) == 13:
                                    if d[12] == 1:
                                        camtype = "perspective"
                                c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
                                from pivy import coin
                                if isinstance(c,coin.SoOrthographicCamera):
                                    if camtype == "perspective":
                                        FreeCADGui.ActiveDocument.ActiveView.setCameraType("Perspective")
                                elif isinstance(c,coin.SoPerspectiveCamera):
                                    if camtype == "orthographic":
                                        FreeCADGui.ActiveDocument.ActiveView.setCameraType("Orthographic")
                                c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
                                c.position.setValue([d[0],d[1],d[2]])
                                c.orientation.setValue([d[3],d[4],d[5],d[6]])
                                c.nearDistance.setValue(d[7])
                                c.farDistance.setValue(d[8])
                                c.aspectRatio.setValue(d[9])
                                c.focalDistance.setValue(d[10])
                                if camtype == "orthographic":
                                    c.height.setValue(d[11])
                                else:
                                    c.heightAngle.setValue(d[11])
                if hasattr(sel.Object.ViewObject,"RestoreState"):
                    if sel.Object.ViewObject.RestoreState:
                        if hasattr(sel.Object.ViewObject,"VisibilityMap"):
                            if sel.Object.ViewObject.VisibilityMap:
                                for k,v in sel.Object.ViewObject.VisibilityMap.items():
                                    o = FreeCADGui.ActiveDocument.getObject(k)
                                    if o:
                                        if o.Visibility != (v == "True"):
                                            FreeCADGui.doCommand("FreeCADGui.ActiveDocument.getObject(\""+k+"\").Visibility = "+v)
                self.display(FreeCAD.DraftWorkingPlane.axis)
                self.wpButton.setText(sel.Object.Label)
                self.wpButton.setToolTip(translate("draft", "Current working plane")+": "+self.wpButton.text())
                return True
            elif Draft.getType(sel.Object) == "SectionPlane":
                FreeCAD.DraftWorkingPlane.setFromPlacement(sel.Object.Placement,rebase=True)
                FreeCAD.DraftWorkingPlane.weak = False
                self.display(FreeCAD.DraftWorkingPlane.axis)
                self.wpButton.setText(sel.Object.Label)
                self.wpButton.setToolTip(translate("draft", "Current working plane")+": "+self.wpButton.text())
                return True
            elif sel.HasSubObjects:
                if len(sel.SubElementNames) == 1:
                    if "Face" in sel.SubElementNames[0]:
                        FreeCAD.DraftWorkingPlane.alignToFace(sel.SubObjects[0], self.getOffset())
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
                    elif sel.SubElementNames[0] == "Plane":
                        FreeCAD.DraftWorkingPlane.setFromPlacement(sel.Object.Placement,rebase=True)
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
                elif len(sel.SubElementNames) == 3:
                    if ("Vertex" in sel.SubElementNames[0]) \
                    and ("Vertex" in sel.SubElementNames[1]) \
                    and ("Vertex" in sel.SubElementNames[2]):
                        FreeCAD.DraftWorkingPlane.alignTo3Points(sel.SubObjects[0].Point,
                                                                 sel.SubObjects[1].Point,
                                                                 sel.SubObjects[2].Point,
                                                                 self.getOffset())
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
            elif sel.Object.isDerivedFrom("Part::Feature"):
                if sel.Object.Shape:
                    if len(sel.Object.Shape.Faces) == 1:
                        FreeCAD.DraftWorkingPlane.alignToFace(sel.Object.Shape.Faces[0], self.getOffset())
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
        elif sel:
            subs = []
            import Part
            for s in sel:
                for so in s.SubObjects:
                    if isinstance(so,Part.Vertex):
                        subs.append(so)
            if len(subs) == 3:
                FreeCAD.DraftWorkingPlane.alignTo3Points(subs[0].Point,
                                                         subs[1].Point,
                                                         subs[2].Point,
                                                         self.getOffset())
                self.display(FreeCAD.DraftWorkingPlane.axis)
                return True
        return False

    def getCenterPoint(self,x,y,z):

        if not self.taskd.form.checkCenter.isChecked():
            return FreeCAD.Vector()
        v = FreeCAD.Vector(x,y,z)
        cam1 = FreeCAD.Vector(FreeCADGui.ActiveDocument.ActiveView.getCameraNode().position.getValue().getValue())
        cam2 = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
        vcam1 = DraftVecUtils.project(cam1,v)
        a = vcam1.getAngle(cam2)
        if a < 0.0001:
            return FreeCAD.Vector()
        d = vcam1.Length
        L = d/math.cos(a)
        vcam2 = DraftVecUtils.scaleTo(cam2,L)
        cp = cam1.add(vcam2)
        return cp

    def tostr(self,v):
        
        """makes a string from a vector or tuple"""

        return "FreeCAD.Vector("+str(v[0])+","+str(v[1])+","+str(v[2])+")"

    def getOffset(self):

        """returns the offset value as a float in mm"""

        try:
            o = float(self.taskd.form.fieldOffset.text())
        except:
            o = FreeCAD.Units.Quantity(self.taskd.form.fieldOffset.text())
            o = o.Value
        return o

    def onClickTop(self):

        o = str(self.getOffset())
        FreeCADGui.doCommandGui(self.ac+"("+self.tostr(self.getCenterPoint(0,0,1))+","+self.tostr((0,0,1))+","+o+")")
        self.display('Top')
        self.finish()

    def onClickFront(self):

        o = str(self.getOffset())
        FreeCADGui.doCommandGui(self.ac+"("+self.tostr(self.getCenterPoint(0,-1,0))+","+self.tostr((0,-1,0))+","+o+")")
        self.display('Front')
        self.finish()

    def onClickSide(self):

        o = str(self.getOffset())
        FreeCADGui.doCommandGui(self.ac+"("+self.tostr(self.getCenterPoint(1,0,0))+","+self.tostr((1,0,0))+","+o+")")
        self.display('Side')
        self.finish()

    def onClickAlign(self):

        FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.setup(force=True)")
        d = self.view.getViewDirection().negative()
        self.display(d)
        self.finish()

    def onClickAuto(self):

        FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.reset()")
        self.display('Auto')
        self.finish()

    def onClickMove(self):

        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            verts = []
            import Part
            for s in sel:
                for so in s.SubObjects:
                    if isinstance(so,Part.Vertex):
                        verts.append(so)
            if len(verts) == 1:
                target = verts[0].Point
                FreeCAD.DraftWorkingPlane.position = target
                self.display(target)
                self.finish()
        else:
            # move the WP to the center of the current view
            c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            p = FreeCAD.Vector(c.position.getValue().getValue())
            d = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
            pp = FreeCAD.DraftWorkingPlane.projectPoint(p,d)
            FreeCAD.DraftWorkingPlane.position = pp
            self.display(pp)
            self.finish()

    def onClickCenter(self):

        c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
        r = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
        c.orientation.setValue(r)
        # calculate delta
        p = FreeCAD.Vector(c.position.getValue().getValue())
        pp = FreeCAD.DraftWorkingPlane.projectPoint(p)
        delta = pp.negative() # to bring it above the (0,0) point
        np = p.add(delta)
        c.position.setValue(tuple(np))
        self.finish()

    def onClickPrevious(self):
        
        p = FreeCAD.DraftWorkingPlane
        if len(self.states) > 1:
            self.states.pop() # discard the last one
            s = self.states[-1]
            p.u = s[0]
            p.v = s[1]
            p.axis = s[2]
            p.position = s[3]
            FreeCADGui.Snapper.setGrid()
            self.finish()

    def onSetGridSize(self,text):

        try:
            q = FreeCAD.Units.Quantity(text)
        except:
            pass
        else:
            self.param.SetFloat("gridSpacing",q.Value)
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()

    def onSetMainline(self,i):

        if i > 1:
            self.param.SetInt("gridEvery",i)
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()

    def onSetSnapRadius(self,i):

        self.param.SetInt("snapRange",i)
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.showradius()

    def display(self,arg):

        """sets the text of the WP button"""

        o = self.getOffset()
        if o:
            if o > 0:
                suffix = ' +O'
            else:
                suffix = ' -O'
        else:
            suffix = ''
        vdir = FreeCAD.DraftWorkingPlane.axis
        vdir = '('+str(vdir.x)[:4]+','+str(vdir.y)[:4]+','+str(vdir.z)[:4]+')'
        vdir = " "+translate("draft","Dir")+": "+vdir
        if type(arg).__name__  == 'str':
            self.wpButton.setText(arg+suffix)
            if o != 0:
                o = " "+translate("draft","Offset")+": "+str(o)
            else:
                o = ""
            self.wpButton.setToolTip(translate("draft", "Current working plane")+": "+self.wpButton.text()+o+vdir)
        elif type(arg).__name__ == 'Vector':
            plv = '('+str(arg.x)[:6]+','+str(arg.y)[:6]+','+str(arg.z)[:6]+')'
            self.wpButton.setText(translate("draft","Custom"))
            self.wpButton.setToolTip(translate("draft", "Current working plane")+": "+plv+vdir)
        p = FreeCAD.DraftWorkingPlane
        self.states.append([p.u, p.v, p.axis, p.position])
        FreeCADGui.doCommandGui("FreeCADGui.Snapper.setGrid()")



class SelectPlane_TaskPanel:

    '''The editmode TaskPanel for Arch Material objects'''

    def __init__(self):

        import Draft_rc
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/TaskSelectPlane.ui")

    def getStandardButtons(self):

        return 2097152 #int(QtGui.QDialogButtonBox.Close)



class Draft_SetWorkingPlaneProxy():

    """The Draft_SetWorkingPlaneProxy FreeCAD command definition"""

    def GetResources(self):

        return {'Pixmap'  : 'Draft_SelectPlane',
                'MenuText': QT_TRANSLATE_NOOP("Draft_SetWorkingPlaneProxy", "Create Working Plane Proxy"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_SetWorkingPlaneProxy", "Creates a proxy object from the current working plane")}

    def IsActive(self):

        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):

        if hasattr(FreeCAD,"DraftWorkingPlane"):
            FreeCAD.ActiveDocument.openTransaction("Create WP proxy")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.makeWorkingPlaneProxy(FreeCAD.DraftWorkingPlane.getPlacement())")
            FreeCAD.ActiveDocument.recompute()
            FreeCAD.ActiveDocument.commitTransaction()



FreeCADGui.addCommand('Draft_SelectPlane',Draft_SelectPlane())
FreeCADGui.addCommand('Draft_SetWorkingPlaneProxy',Draft_SetWorkingPlaneProxy())
