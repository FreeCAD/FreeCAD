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
from DraftGui import translate

def QT_TRANSLATE_NOOP(ctx,txt): return txt



class Draft_SelectPlane:

    """The Draft_SelectPlane FreeCAD command definition"""

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

        if FreeCAD.activeDraftCommand:
            FreeCAD.activeDraftCommand.finish()
        FreeCAD.activeDraftCommand = self
        self.view = Draft.get3DView()
        self.ui = FreeCADGui.draftToolBar
        self.featureName = "SelectPlane"
        self.ui.sourceCmd = self
        self.ui.setTitle(translate("draft","Set Working Plane"))
        self.ui.show()
        FreeCAD.DraftWorkingPlane.setup()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.setTrackers()
        self.offset = 0
        if self.handle():
            return
        self.ui.selectPlaneUi()
        FreeCAD.Console.PrintMessage(translate("draft", "Pick a face to define the drawing plane")+"\n")
        if FreeCAD.DraftWorkingPlane.alignToSelection(self.offset):
            FreeCADGui.Selection.clearSelection()
            self.display(FreeCAD.DraftWorkingPlane.axis)
            self.finish()
        else:
            self.call = self.view.addEventCallback("SoEvent", self.action)

    def finish(self,close=False):
        
        FreeCAD.activeDraftCommand = None
        if self.ui:
            self.ui.offUi()
            self.ui.sourceCmd = None
        FreeCAD.DraftWorkingPlane.restore()
        if hasattr(FreeCADGui,"Snapper"):
            FreeCADGui.Snapper.off()
        if self.call:
            try:
                self.view.removeEventCallback("SoEvent",self.call)
            except RuntimeError:
                # the view has been deleted already
                pass
            self.call = None

    def action(self, arg):
        
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        if arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                # coin detection happens before the selection got a chance of being updated, so we must delay
                DraftGui.todo.delay(self.checkSelection,None)

    def checkSelection(self):
        
        if self.handle():
            self.finish()

    def handle(self):
        
        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) == 1:
            sel = sel[0]
            self.ui = FreeCADGui.draftToolBar
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
                self.ui.wplabel.setText(sel.Object.Label)
                self.ui.wplabel.setToolTip(translate("draft", "Current working plane")+": "+self.ui.wplabel.text())
                return True
            elif Draft.getType(sel.Object) == "SectionPlane":
                FreeCAD.DraftWorkingPlane.setFromPlacement(sel.Object.Placement,rebase=True)
                FreeCAD.DraftWorkingPlane.weak = False
                self.display(FreeCAD.DraftWorkingPlane.axis)
                self.ui.wplabel.setText(sel.Object.Label)
                self.ui.wplabel.setToolTip(translate("draft", "Current working plane")+": "+self.ui.wplabel.text())
                return True
            elif sel.HasSubObjects:
                if len(sel.SubElementNames) == 1:
                    if "Face" in sel.SubElementNames[0]:
                        FreeCAD.DraftWorkingPlane.alignToFace(sel.SubObjects[0], self.offset)
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
                                                                 self.offset)
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
            elif sel.Object.isDerivedFrom("Part::Feature"):
                if sel.Object.Shape:
                    if len(sel.Object.Shape.Faces) == 1:
                        FreeCAD.DraftWorkingPlane.alignToFace(sel.Object.Shape.Faces[0], self.offset)
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
                                                         self.offset)
                self.display(FreeCAD.DraftWorkingPlane.axis)
                return True
        return False

    def getCenterPoint(self,x,y,z):
        
        if not self.ui.isCenterPlane:
            return "0,0,0"
        v = FreeCAD.Vector(x,y,z)
        cam1 = FreeCAD.Vector(FreeCADGui.ActiveDocument.ActiveView.getCameraNode().position.getValue().getValue())
        cam2 = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
        vcam1 = DraftVecUtils.project(cam1,v)
        a = vcam1.getAngle(cam2)
        if a < 0.0001:
            return "0,0,0"
        d = vcam1.Length
        L = d/math.cos(a)
        vcam2 = DraftVecUtils.scaleTo(cam2,L)
        cp = cam1.add(vcam2)
        return str(cp.x)+","+str(cp.y)+","+str(cp.z)

    def selectHandler(self, arg):
        
        try:
            self.offset = float(self.ui.offset)
        except:
            self.offset = 0
        if arg == "XY":
            FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.alignToPointAndAxis(FreeCAD.Vector("+self.getCenterPoint(0,0,1)+"), FreeCAD.Vector(0,0,1), "+str(self.offset)+")")
            self.display('Top')
            self.finish()
        elif arg == "XZ":
            FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.alignToPointAndAxis(FreeCAD.Vector("+self.getCenterPoint(0,-1,0)+"), FreeCAD.Vector(0,-1,0), "+str(self.offset)+")")
            self.display('Front')
            self.finish()
        elif arg == "YZ":
            FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.alignToPointAndAxis(FreeCAD.Vector("+self.getCenterPoint(1,0,0)+"), FreeCAD.Vector(1,0,0), "+str(self.offset)+")")
            self.display('Side')
            self.finish()
        elif arg == "currentView":
            d = self.view.getViewDirection().negative()
            FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.alignToPointAndAxis(FreeCAD.Vector("+self.getCenterPoint(d.x,d.y,d.z)+"), FreeCAD.Vector("+str(d.x)+","+str(d.y)+","+str(d.z)+"), "+str(self.offset)+")")
            self.display(d)
            self.finish()
        elif arg == "reset":
            FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.reset()")
            self.display('Auto')
            self.finish()
        elif arg == "alignToWP":
            c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            r = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
            c.orientation.setValue(r)
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
        self.ui.wplabel.setToolTip(translate("draft", "Current working plane:")+self.ui.wplabel.text())
        FreeCADGui.doCommandGui("FreeCADGui.Snapper.setGrid()")



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
