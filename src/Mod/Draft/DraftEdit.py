# -*- coding: utf8 -*-

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

__title__= "FreeCAD Draft Edit Tool"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin, Carlo Pavan"
__url__ = "http://www.freecadweb.org"

import FreeCAD
import Draft
import math

from FreeCAD import Vector

# Do not import GUI-related modules if GUI is not there
if FreeCAD.GuiUp:
    import FreeCADGui
    import DraftTools
    from DraftTrackers import *
    from PySide import QtCore
    from PySide.QtCore import QT_TRANSLATE_NOOP
    from DraftTools import translate

class Edit():

    "The Draft_Edit FreeCAD command definition"

    def __init__(self):
        self.running = False
        self.trackers = []
        self.obj = None

        # event callbacks
        self.call = None
        self._keyPressedCB = None
        self._mouseMovedCB = None
        self._mousePressedCB = None

        self.selectstate = None
        self.originalDisplayMode = None
        self.originalPoints = None
        self.originalNodes = None

        # preview
        self.ghost = None

        # soraypick action things
        self.pick_radius = 30 # TODO: set pick radius according to user preferences

        #list of supported objects type
        self.supportedObjs = ["BezCurve","Wire","BSpline","Circle","Rectangle",
                            "Polygon","Dimension","Space","Structure","PanelCut",
                            "PanelSheet","Wall", "Window"]
        self.supportedPartObjs = ["Part", "Part::Line", "Part::Box"]

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Edit',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edits the active object")}

    #---------------------------------------------------------------------------
    # MAIN FUNCTIONS
    #---------------------------------------------------------------------------

    def Activated(self):
        if self.running: self.finish()
        DraftTools.Modifier.Activated(self,"Edit")
        if not FreeCAD.ActiveDocument: self.finish()

        self.ui = FreeCADGui.draftToolBar
        self.view = Draft.get3DView()

        if FreeCADGui.Selection.getSelection():
            self.proceed()
        else:
            self.ui.selectUi()
            FreeCAD.Console.PrintMessage(translate("draft", "Select a Draft object to edit")+"\n")
            self.register_selection_callback()

    def proceed(self):
        "this method defines editpoints and set the editTrackers"
        self.unregister_selection_callback()
        self.obj = self.getObjFromSelection()
        if not self.obj: return self.finish()          

        # store selectable state of the object TODO: Separate function
        if hasattr(self.obj.ViewObject,"Selectable"):
            self.selectstate = self.obj.ViewObject.Selectable
            self.obj.ViewObject.Selectable = False

        # start object editing
        FreeCADGui.Selection.clearSelection()
        self.editing = None
        self.editpoints = []
        self.arc3Pt = True # TODO: Find a more elegant way
        FreeCADGui.Snapper.setSelectMode(True)

        self.ui.editUi()

        self.setPlacement(self.obj)

        self.setEditPoints(self.obj)

        if self.editpoints: # set trackers and align plane
            self.setTrackers()
            # set plane tracker to match edited object
            FreeCAD.DraftWorkingPlane.save()
            self.alignWorkingPlane()
            self.editpoints = []
            self.register_editing_callbacks()#register action callback
        else:
            FreeCAD.Console.PrintWarning(translate("draft", "No edit point found for selected object")+"\n")
            self.finish()
            return

    def finish(self,closed=False):
        "terminates Edit Tool"
        self.unregister_selection_callback()
        self.unregister_editing_callbacks()
        FreeCADGui.Snapper.setSelectMode(False)
        self.finalizeGhost()
        if self.obj and closed:
            if "Closed" in self.obj.PropertiesList:
                if not self.obj.Closed:
                    self.obj.Closed = True
        if self.ui: self.removeTrackers()
        if self.obj:
            if hasattr(self.obj.ViewObject,"Selectable") and (self.selectstate != None):
                self.obj.ViewObject.Selectable = self.selectstate
        if Draft.getType(self.obj) == "Structure":
            if self.originalDisplayMode != None:
                self.obj.ViewObject.DisplayMode = self.originalDisplayMode
            if self.originalPoints != None:
                self.obj.ViewObject.NodeSize = self.originalPoints
            if self.originalNodes != None:
                self.obj.ViewObject.ShowNodes = self.originalNodes
        self.selectstate = None
        self.originalDisplayMode = None
        self.originalPoints = None
        self.originalNodes = None
        DraftTools.Modifier.finish(self)
        FreeCAD.DraftWorkingPlane.restore()
        if FreeCADGui.Snapper.grid: FreeCADGui.Snapper.grid.set()
        self.running = False
        # delay resetting edit mode otherwise it doesn't happen
        from PySide import QtCore
        QtCore.QTimer.singleShot(0,FreeCADGui.ActiveDocument.resetEdit)

    #---------------------------------------------------------------------------
    # SCENE EVENTS CALLBACKS
    #---------------------------------------------------------------------------

    def register_selection_callback(self):
        "register callback for selection when command is launched"
        self.unregister_selection_callback()
        self.call = self.view.addEventCallback("SoEvent",DraftTools.selectObject)

    def unregister_selection_callback(self):
        "remove callback for selection if it exhists"
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        self.call = None

    def register_editing_callbacks(self):
        "register callbacks to use during editing (former action function)"
        viewer = FreeCADGui.ActiveDocument.ActiveView.getViewer()
        self.render_manager = viewer.getSoRenderManager()
        view = FreeCADGui.ActiveDocument.ActiveView
        self._keyPressedCB = view.addEventCallbackPivy(
            coin.SoKeyboardEvent.getClassTypeId(), self.keyPressed)
        self._mouseMovedCB = view.addEventCallbackPivy(
            coin.SoLocation2Event.getClassTypeId(), self.mouseMoved)
        self._mousePressedCB = view.addEventCallbackPivy(
            coin.SoMouseButtonEvent.getClassTypeId(), self.mousePressed)
        #FreeCAD.Console.PrintMessage("Draft edit callbacks registered \n")

    def unregister_editing_callbacks(self):
        "remove callbacks used during editing if they exhist"
        view = FreeCADGui.ActiveDocument.ActiveView
        if self._keyPressedCB: 
            view.removeEventCallbackSWIG(coin.SoKeyboardEvent.getClassTypeId(), self._keyPressedCB)
            self._keyPressedCB = None
            #FreeCAD.Console.PrintMessage("Draft edit keyboard callback unregistered \n")
        if self._mouseMovedCB: 
            view.removeEventCallbackSWIG(coin.SoLocation2Event.getClassTypeId(), self._mouseMovedCB)
            self._mouseMovedCB = None
            #FreeCAD.Console.PrintMessage("Draft edit location callback unregistered \n")
        if self._mousePressedCB: 
            view.removeEventCallbackSWIG(coin.SoMouseButtonEvent.getClassTypeId(), self._mousePressedCB)
            self._mousePressedCB = None
            #FreeCAD.Console.PrintMessage("Draft edit mouse button callback unregistered \n")

    #---------------------------------------------------------------------------
    # SCENE EVENT HANDLERS
    #---------------------------------------------------------------------------

    def keyPressed(self, event_callback):
        "keyboard event handler"
        #TODO: Get the keys from preferences
        event = event_callback.getEvent()
        if event.getState() == coin.SoKeyboardEvent.DOWN:
            key = event.getKey()
            #FreeCAD.Console.PrintMessage("pressed key : "+str(key)+"\n")
            if key == 65307: # ESC
                if self.editing == None: self.finish()
                else:
                    self.finalizeGhost()
                    self.editpoints = []
                    self.setEditPoints(self.obj)
                    self.resetTrackers()
            if key == 97: # "a"
                self.finish()
            if key == 111: # "o"
                self.finish(closed=True)
            if key == 105: # "i"
                if Draft.getType(self.obj) == "Circle": self.arcInvert()

    def mousePressed(self, event_callback):
        "mouse button event handler, calls: startEditing, endEditing, addPoint, delPoint"
        event = event_callback.getEvent()
        if event.getTypeId().getName() != "SoMouseButtonEvent": return        
        if (event.getState() == coin.SoMouseButtonEvent.DOWN and 
            event.getButton() == event.BUTTON1):#left click
            if self.ui.addButton.isChecked():
                self.addPoint(event)
                return
            if self.ui.delButton.isChecked():
                self.delPoint(self.obj, event)
                return
            if Draft.getType(self.obj) == "BezCurve":
                pos = event.getPosition()      
                if self.ui.sharpButton.isChecked():
                    return self.smoothBezPoint(self.getEditNodeIndex(pos), 'Sharp')
                elif self.ui.tangentButton.isChecked():
                    return self.smoothBezPoint(self.getEditNodeIndex(pos), 'Tangent')
                elif self.ui.symmetricButton.isChecked():
                    return self.smoothBezPoint(self.getEditNodeIndex(pos), 'Symmetric')
            if self.editing == None:
                self.startEditing(event)
            else:
                self.endEditing()
    
    def mouseMoved(self, event_callback):
        "mouse moved event handler, update tracker position and update preview ghost"
        event = event_callback.getEvent()
        if self.editing != None:
            self.updateTrackerAndGhost(event)
        else:
            # TODO add preselection color change for trackers
            pass

    def startEditing(self, event):
        "start editing selected EditNode"
        pos = event.getPosition()
        ep = self.getEditNodeIndex(pos)
        if ep == None: 
            #FreeCAD.Console.PrintMessage("Node not found\n")
            return
        FreeCAD.Console.PrintMessage(str("Editing node: n° ")+str(ep)+"\n")
        self.ui.pointUi()
        self.ui.isRelative.show()
        self.editing = ep
        self.trackers[self.editing].off()
        self.finalizeGhost()
        self.ghost = self.initGhost(self.obj)
        self.node.append(self.trackers[self.editing].get())
        FreeCADGui.Snapper.setSelectMode(False)

    def updateTrackerAndGhost(self, event):
        "updates tracker position when editing and update ghost"
        pos = event.getPosition().getValue()
        orthoConstrain = False
        if event.wasShiftDown() == 1: orthoConstrain = True
        snappedPos = FreeCADGui.Snapper.snap((pos[0],pos[1]),self.node[-1], constrain=orthoConstrain)
        self.trackers[self.editing].set(snappedPos)
        self.ui.displayPoint(snappedPos,self.node[-1])
        if self.ghost: self.updateGhost(obj=self.obj,idx=self.editing,pt=snappedPos)

    def endEditing(self):
        "terminate editing and start object updating process"
        self.finalizeGhost()
        self.trackers[self.editing].on()
        #if hasattr(self.obj.ViewObject,"Selectable"):
        #    self.obj.ViewObject.Selectable = True
        FreeCADGui.Snapper.setSelectMode(True)
        self.numericInput(self.trackers[self.editing].get())
        DraftTools.redraw3DView()

    #---------------------------------------------------------------------------
    # UTILS
    #---------------------------------------------------------------------------

    def getObjFromSelection(self):
        "evaluate selection and returns a valid object to edit"
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintMessage(translate("draft", "Please select exactly one object")+"\n")
            return None
        if "Proxy" in selection[0].PropertiesList and hasattr(selection[0].Proxy,"Type"):
            if Draft.getType(selection[0]) in self.supportedObjs:
                return selection[0]
        else:
            try:
                if Draft.getType(selection[0]) in self.supportedPartObjs and selection[0].TypeId in self.supportedPartObjs:
                    return selection[0]
            except:
                pass
        FreeCAD.Console.PrintWarning(translate("draft", "This object is not editable")+"\n")
        return None

    def numericInput(self,v,numy=None,numz=None):
        '''this function gets called by the toolbar
        or by the mouse click and activate the update function'''
        if (numy != None):
            v = Vector(v,numy,numz)
        self.update(v)
        FreeCAD.ActiveDocument.recompute()
        self.editing = None
        self.ui.editUi(self.ui.lastMode)
        self.node = []

    def setPlacement(self,obj):
        "set self.pl and self.invpl to self.obj placement and inverse placement"
        if "Placement" in obj.PropertiesList:
            self.pl = obj.Placement
            self.invpl = self.pl.inverse()

    def alignWorkingPlane(self):
        "align working plane to self.obj"
        if "Shape" in self.obj.PropertiesList:
            if DraftTools.plane.weak:
                DraftTools.plane.alignToFace(self.obj.Shape)
        if self.planetrack:
            self.planetrack.set(self.editpoints[0])

    def getEditNodeIndex(self, pos):
        "get edit node index from given screen position"
        ep = self.sendRay(pos)
        return ep
    
    def sendRay(self, mouse_pos):
        "sends a ray trough the scene and return the nearest entity"
        ray_pick = coin.SoRayPickAction(self.render_manager.getViewportRegion())
        ray_pick.setPoint(coin.SbVec2s(*mouse_pos))
        ray_pick.setRadius(self.pick_radius)
        ray_pick.setPickAll(True)
        ray_pick.apply(self.render_manager.getSceneGraph())
        picked_point = ray_pick.getPickedPointList()
        return self.searchEditNode(picked_point)

    def searchEditNode(self, picked_point):
        "search edit node inside picked point list and retrurn node number"
        for point in picked_point:
            path = point.getPath()
            length = path.getLength()
            point = path.getNode(length - 2)
            import DraftTrackers
            if hasattr(point,"subElementName"):
                subElement = str(point.subElementName.getValue())
                if 'EditNode' in subElement:
                    ep = int(subElement[8:])
                    #doc = FreeCAD.getDocument(str(point.documentName.getValue()))
                    #self.obj = doc.getObject(str(point.objectName.getValue()))
                    return ep
                #FreeCAD.Console.PrintMessage(str(self.obj)+str(ep)+"\n")
        return None

    #---------------------------------------------------------------------------
    # EDIT TRACKERS functions
    #---------------------------------------------------------------------------

    def setTrackers(self):
        "set Edit Trackers for editpoints collected from self.obj"
        self.trackers = []
        if Draft.getType(self.obj) == "BezCurve":
            self.resetTrackersBezier()
        else:
            for ep in range(len(self.editpoints)):
                self.trackers.append(editTracker(pos=self.editpoints[ep],name=self.obj.Name,idx=ep))

    def resetTrackers(self):
        "reset Edit Trackers and set them again"
        self.removeTrackers()
        self.setTrackers()

    def removeTrackers(self):
        "reset Edit Trackers and set them again"
        if self.trackers:
            for t in self.trackers:
                t.finalize()
        self.trackers = []

    def hideTrackers(self):
        "hide Edit Trackers"
        for t in self.trackers:
            t.off()

    def showTrackers(self):
        "show Edit Trackers"
        for t in self.trackers:
            t.on()

    #---------------------------------------------------------------------------
    # PREVIEW
    #---------------------------------------------------------------------------

    def initGhost(self,obj):
        "initialize preview ghost"
        if Draft.getType(obj) == "Wire":
            return wireTracker(obj.Shape)
        elif Draft.getType(obj) == "BSpline":
            return bsplineTracker()
        elif Draft.getType(obj) == "BezCurve":
            return bezcurveTracker()
        elif Draft.getType(obj) == "Circle":
            return arcTracker()

    def updateGhost(self,obj,idx,pt):
        if Draft.getType(obj) in ["Wire"]:
            self.ghost.on()
            pointList = self.applyPlacement(obj.Points)
            pointList[idx] = pt
            if obj.Closed: pointList.append(pointList[0])
            self.ghost.updateFromPointlist(pointList)
        elif Draft.getType(obj) == "BSpline":
            self.ghost.on()
            pointList = self.applyPlacement(obj.Points)
            pointList[idx] = pt
            if obj.Closed: pointList.append(pointList[0])
            self.ghost.update(pointList)
        elif Draft.getType(obj) == "BezCurve":
            self.ghost.on()
            plist = self.applyPlacement(obj.Points)
            pointList = self.recomputePointsBezier(plist,idx,pt,obj.Degree,moveTrackers=True)
            self.ghost.update(pointList,obj.Degree)
        elif Draft.getType(obj) == "Circle":
            self.ghost.on()
            self.ghost.setCenter(obj.Placement.Base)
            self.ghost.setRadius(obj.Radius)
            if self.obj.FirstAngle == self.obj.LastAngle:#self.obj is a circle
                self.ghost.circle = True
                if self.editing == 0: self.ghost.setCenter(pt)
                elif self.editing == 1: 
                    radius = pt.sub(obj.Placement.Base).Length
                    self.ghost.setRadius(radius)
            else:
                if self.arc3Pt == False:
                    self.ghost.setStartAngle(math.radians(obj.FirstAngle))
                    self.ghost.setEndAngle(math.radians(obj.LastAngle))
                    if self.editing == 0:
                        self.ghost.setCenter(pt)
                    elif self.editing == 1:
                        self.ghost.setStartPoint(pt)
                    elif self.editing == 2:
                        self.ghost.setEndPoint(pt)
                    elif self.editing == 3:
                        self.ghost.setRadius(self.invpl.multVec(pt).Length)
                elif self.arc3Pt == True:
                    if self.editing == 0:#center point
                        import DraftVecUtils
                        p1 = self.invpl.multVec(self.obj.Shape.Vertexes[0].Point)
                        p2 = self.invpl.multVec(self.obj.Shape.Vertexes[1].Point)
                        p0 = DraftVecUtils.project(self.invpl.multVec(pt),self.invpl.multVec(self.getArcMid()))
                        self.ghost.autoinvert=False
                        self.ghost.setRadius(p1.sub(p0).Length)
                        self.ghost.setStartPoint(self.obj.Shape.Vertexes[1].Point)
                        self.ghost.setEndPoint(self.obj.Shape.Vertexes[0].Point)
                        self.ghost.setCenter(self.pl.multVec(p0))
                        return
                    else:
                        p1=self.obj.Shape.Vertexes[0].Point
                        p2=self.getArcMid()
                        p3=self.obj.Shape.Vertexes[1].Point
                        if self.editing == 1: p1=pt
                        elif self.editing == 3: p2=pt
                        elif self.editing == 2: p3=pt
                        self.ghost.setBy3Points(p1,p2,p3)
        DraftTools.redraw3DView()

    def applyPlacement(self,pointList):
        if self.pl:
            plist =[]
            for p in pointList:
                point = self.pl.multVec(p)
                plist.append(point)
            return plist
        else: return pointList

    def finalizeGhost(self):
        try:
            self.ghost.finalize()
            self.ghost = None
        except: return

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Add/Delete Vertexes
    #---------------------------------------------------------------------------

    def addPoint(self,event):
        "called by action, add point to obj and reset trackers"
        if not (Draft.getType(self.obj) in ["Wire","BSpline","BezCurve"]): return
        pos = event.getPosition()
        selobjs = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo((pos[0],pos[1]))
        if not selobjs: return
        for info in selobjs:
            if not info: return
            if self.obj.Name != info["Object"]: continue
            if Draft.getType(self.obj) == "Wire" and 'Edge' in info["Component"]:
                pt = FreeCAD.Vector(info["x"],info["y"],info["z"])
                self.addPointToWire(pt, int(info["Component"][4:]))
            elif Draft.getType(self.obj) in ["BSpline","BezCurve"]: #to fix double vertex created
                #pt = self.point
                if "x" in info:# prefer "real" 3D location over working-plane-driven one if possible
                    pt = FreeCAD.Vector(info["x"],info["y"],info["z"])
                else: continue
                self.addPointToCurve(pt,info)
        self.obj.recompute()
        self.editpoints = []
        self.setEditPoints(self.obj)
        self.resetTrackers()
        return


    def addPointToWire(self, newPoint, edgeIndex):
        newPoints = []
        hasAddedPoint = False
        for index, point in enumerate(self.obj.Points):
            if index == edgeIndex:
                hasAddedPoint = True
                newPoints.append(self.invpl.multVec(newPoint))
            newPoints.append(point)
        if not hasAddedPoint:
            newPoints.append(point)
        self.obj.Points = newPoints

    def addPointToCurve(self,point,info=None):
        import Part
        if not (Draft.getType(self.obj) in ["BSpline","BezCurve"]): return
        pts = self.obj.Points
        if Draft.getType(self.obj) == "BezCurve":
            if not info['Component'].startswith('Edge'):
                return # clicked control point
            edgeindex = int(info['Component'].lstrip('Edge'))-1
            wire=self.obj.Shape.Wires[0]
            bz=wire.Edges[edgeindex].Curve
            param=bz.parameter(point)
            seg1=wire.Edges[edgeindex].copy().Curve
            seg2=wire.Edges[edgeindex].copy().Curve
            seg1.segment(seg1.FirstParameter,param)
            seg2.segment(param,seg2.LastParameter)
            if edgeindex == len(wire.Edges):
                #we hit the last segment, we need to fix the degree
                degree=wire.Edges[0].Curve.Degree
                seg1.increase(degree)
                seg2.increase(degree)
            edges=wire.Edges[0:edgeindex]+[Part.Edge(seg1),Part.Edge(seg2)]\
                + wire.Edges[edgeindex+1:]
            pts = edges[0].Curve.getPoles()[0:1]
            for edge in edges:
                pts.extend(edge.Curve.getPoles()[1:])
            if self.obj.Closed:
                pts.pop()
            c=self.obj.Continuity
            # assume we have a tangent continuity for an arbitrarily split
            # segment, unless it's linear
            cont = 1 if (self.obj.Degree >= 2) else 0
            self.obj.Continuity = c[0:edgeindex]+[cont]+c[edgeindex:]
        else:
            if (Draft.getType(self.obj) in ["BSpline"]):
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
        self.obj.Points = pts

    def delPoint(self,obj,event):
        if not (Draft.getType(obj) in ["Wire","BSpline","BezCurve"]): return
        if len(obj.Points) <= 2: return FreeCAD.Console.PrintWarning(
            translate("draft", "Active object must have more than two points/nodes")+"\n")
        pos = event.getPosition()
        ep = self.getEditNodeIndex(pos)
        if ep == None: return FreeCAD.Console.PrintWarning(
            translate("draft", "Node not found\n"))
        pts = obj.Points
        pts.pop(ep)
        self.obj.Points = pts
        if Draft.getType(obj) =="BezCurve":
            self.obj.Proxy.resetcontinuity(obj)
        obj.recompute()
        
        # don't do tan/sym on DWire/BSpline!
        self.editpoints = []
        self.setEditPoints(self.obj)
        self.resetTrackers()

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : GENERAL
    #---------------------------------------------------------------------------

    def setEditPoints(self,obj):
        "calculate edit point position from given object and append to self.edipoints"

        objectType = Draft.getType(obj)

        if objectType == "Wall":
            # setWallPts can change self.obj to self.obj.Base, so better
            # to place it at the beginning
            self.setWallPts()
        if objectType in ["Wire","BSpline"]:
            self.ui.editUi("Wire")
            self.setWirePts(obj)
        elif objectType == "BezCurve":
            self.ui.editUi("BezCurve")
            self.setWirePts(obj)
        elif objectType == "Circle":
            self.setCirclePts()
        elif objectType == "Rectangle":
            self.setRectanglePts()
        elif objectType == "Polygon":
            self.setPolygonPts()
        elif objectType == "Dimension":
            self.setDimensionPts()
        elif objectType == "Window":
            self.setWindowPts()
        elif objectType == "Space":
            self.setSpacePts()
        elif objectType == "Structure":
            self.setStructurePts()
        elif objectType == "PanelCut":
            self.setPanelCutPts()
        elif objectType == "PanelSheet":
            self.setPanelSheetPts()
        elif objectType == "Part" and obj.TypeId == "Part::Box":
            self.setPartBoxPts()
        elif objectType == "Part::Line" and obj.TypeId == "Part::Line":
            self.setPartLinePts()

    def update(self,v):
        "apply the vector to the modified point and update self.obj"

        localVector = self.invpl.multVec(v) #subtract placement from edit vector, to be completed

        FreeCAD.ActiveDocument.openTransaction("Edit")

        if Draft.getType(self.obj) in ["Wire","BSpline"]: self.updateWire(localVector)
        elif Draft.getType(self.obj) == "BezCurve": self.updateWire(localVector)
        elif Draft.getType(self.obj) == "Circle": self.updateCircle(v)
        elif Draft.getType(self.obj) == "Rectangle": self.updateRectangle(v)
        elif Draft.getType(self.obj) == "Polygon": self.updatePolygon(v)
        elif Draft.getType(self.obj) == "Dimension": self.updateDimension(v)
        elif Draft.getType(self.obj) == "Wall": self.updateWall(v)
        elif Draft.getType(self.obj) == "Window": self.updateWindow(v)
        elif Draft.getType(self.obj) == "Space": self.updateSpace(v)
        elif Draft.getType(self.obj) == "Structure": self.updateStructure(v)
        elif Draft.getType(self.obj) == "PanelCut": self.updatePanelCut(v)
        elif Draft.getType(self.obj) == "PanelSheet": self.updatePanelSheet(v)
        elif Draft.getType(self.obj) == "Part::Line" and self.obj.TypeId == "Part::Line": self.updatePartLine(v)
        elif Draft.getType(self.obj) == "Part" and self.obj.TypeId == "Part::Box": self.updatePartBox(v)

        FreeCAD.ActiveDocument.commitTransaction()

        try:
            FreeCADGui.ActiveDocument.ActiveView.redraw()
        except AttributeError as err:
            pass

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Line/Wire/Bspline/Bezcurve
    #---------------------------------------------------------------------------

    def setWirePts(self,obj):
        for p in obj.Points:
            if self.pl: p = self.pl.multVec(p)
            self.editpoints.append(p)

    def updateWire(self,v):
        pts = self.obj.Points
        editPnt = v#self.invpl.multVec(v)
        # DNC: allows to close the curve by placing ends close to each other
        tol = 0.001
        if ( ( self.editing == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( self.editing == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
            self.obj.Closed = True
        # DNC: fix error message if edited point coincides with one of the existing points
        if ( editPnt in pts ) == True: # checks if point enter is equal to other, this could cause a OCC problem
            FreeCAD.Console.PrintMessage(translate("draft", "Is not possible to have two coincident points in this object, please try again.")+"\n")
            if Draft.getType(self.obj) in ["BezCurve"]: self.resetTrackers()
            else: self.trackers[self.editing].set(self.pl.multVec(self.obj.Points[self.editing]))
            return
        if Draft.getType(self.obj) in ["BezCurve"]:
            pts = self.recomputePointsBezier(pts,self.editing,v,self.obj.Degree,moveTrackers=False)
        # check that the new point lies on the plane of the wire
        import DraftGeomUtils, DraftVecUtils
        if self.obj.Closed:
            n = DraftGeomUtils.getNormal(self.obj.Shape)
            dv = editPnt.sub(pts[self.editing])
            rn = DraftVecUtils.project(dv,n)
            if dv.Length:
                editPnt = editPnt.add(rn.negative())
        pts[self.editing] = editPnt
        self.obj.Points = pts
        self.trackers[self.editing].set(self.pl.multVec(v))


    def recomputePointsBezier(self,pts,idx,v,degree,moveTrackers=True):
        "Point list, index of changed point, vector of new point, move trackers; return the new point list"

        editPnt = v#self.invpl.multVec(v)
        # DNC: allows to close the curve by placing ends close to each other
        tol = 0.001
        if ( ( idx == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( idx == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
            self.obj.Closed = True
        # DNC: fix error message if edited point coincides with one of the existing points
        #if ( editPnt in pts ) == False:
        knot = None
        ispole = idx % degree

        if ispole == 0: #knot
            if degree >=3:
                if idx >= 1: #move left pole
                    knotidx = idx if idx < len(pts) else 0
                    pts[idx-1] = pts[idx-1] + editPnt - pts[knotidx]
                    if moveTrackers: self.trackers[idx-1].set(pts[idx-1])
                if idx < len(pts)-1: #move right pole
                    pts[idx+1] = pts[idx+1] + editPnt - pts[idx]
                    if moveTrackers: self.trackers[idx+1].set(pts[idx+1])
                if idx == 0 and self.obj.Closed: # move last pole
                    pts[-1] = pts [-1] + editPnt -pts[idx]
                    if moveTrackers: self.trackers[-1].set(pts[-1])

        elif ispole == 1 and (idx >=2 or self.obj.Closed): #right pole
            knot = idx -1
            changep = idx -2 # -1 in case of closed curve

        elif ispole == degree-1 and idx <= len(pts)-3: #left pole
            knot = idx +1
            changep = idx +2

        elif ispole == degree-1 and self.obj.Closed and idx == len(pts)-1: #last pole
            knot = 0
            changep = 1

        if knot is not None: # we need to modify the opposite pole
            segment = int(knot / degree) -1
            cont=self.obj.Continuity[segment] if \
                len(self.obj.Continuity) > segment else 0
            if cont == 1: #tangent
                pts[changep] = self.obj.Proxy.modifytangentpole(\
                        pts[knot],editPnt,pts[changep])
                if moveTrackers: self.trackers[changep].set(pts[changep])
            elif cont ==2: #symmetric
                pts[changep] = self.obj.Proxy.modifysymmetricpole(\
                        pts[knot],editPnt)
                if moveTrackers: self.trackers[changep].set(pts[changep])
        pts[idx]=v

        return pts #returns the list of new points, taking into account knot continuity

    def resetTrackersBezier(self):
        #in future move tracker definition to DraftTrackers
        from pivy import coin
        knotmarkers = (coin.SoMarkerSet.DIAMOND_FILLED_9_9,#sharp
                coin.SoMarkerSet.SQUARE_FILLED_9_9,        #tangent
                coin.SoMarkerSet.HOURGLASS_FILLED_9_9)     #symmetric
        polemarker = coin.SoMarkerSet.CIRCLE_FILLED_9_9    #pole
        self.trackers=[]
        cont=self.obj.Continuity
        firstknotcont = cont[-1] if (self.obj.Closed and cont) else 0
        pointswithmarkers=[(self.obj.Shape.Edges[0].Curve.\
                getPole(1),knotmarkers[firstknotcont])]
        for edgeindex, edge in enumerate(self.obj.Shape.Edges):
            poles=edge.Curve.getPoles()
            pointswithmarkers.extend([(point,polemarker) for \
                    point in poles[1:-1]])
            if not self.obj.Closed or len(self.obj.Shape.Edges) > edgeindex +1:
                knotmarkeri=cont[edgeindex] if len(cont) > edgeindex else 0
                pointswithmarkers.append((poles[-1],knotmarkers[knotmarkeri]))
        for index,pwm in enumerate(pointswithmarkers):
            p,marker=pwm
            #if self.pl: p = self.pl.multVec(p)
            self.trackers.append(editTracker(p,self.obj.Name,\
                index,self.obj.ViewObject.LineColor,\
                marker=marker))

    def smoothBezPoint(self, point, style='Symmetric'):
        "called when changing the continuity of a knot"
        style2cont = {'Sharp':0,'Tangent':1,'Symmetric':2}
        if point == None: return
        if not (Draft.getType(self.obj) == "BezCurve"):return
        pts = self.obj.Points
        deg = self.obj.Degree
        if deg < 2: return
        if point % deg != 0: #point is a pole
            if deg >=3: #allow to select poles
                if (point % deg == 1) and (point > 2 or self.obj.Closed): #right pole
                    knot = point -1
                    keepp = point
                    changep = point -2
                elif point < len(pts) -3 and point % deg == deg -1: #left pole
                    knot = point +1
                    keepp = point
                    changep = point +2
                elif point == len(pts)-1 and self.obj.Closed: #last pole
                    # if the curve is closed the last pole has the last
                    # index in the points lists
                    knot = 0
                    keepp = point
                    changep = 1
                else:
                    FreeCAD.Console.PrintWarning(translate("draft", "Can't change Knot belonging to pole %d"%point)+"\n")
                    return
                if knot:
                    if style == 'Tangent':
                        pts[changep] = self.obj.Proxy.modifytangentpole(\
                                pts[knot],pts[keepp],pts[changep])
                    elif style == 'Symmetric':
                        pts[changep] = self.obj.Proxy.modifysymmetricpole(\
                                pts[knot],pts[keepp])
                    else: #sharp
                        pass #
            else:
                FreeCAD.Console.PrintWarning(translate("draft", "Selection is not a Knot\n"))
                return
        else: #point is a knot
            if style == 'Sharp':
                if self.obj.Closed and point == len(pts)-1:
                    knot = 0
                else:
                    knot = point
            elif style == 'Tangent' and point > 0 and point < len(pts)-1:
                prev, next = self.obj.Proxy.tangentpoles(pts[point],pts[point-1],pts[point+1])
                pts[point-1] = prev
                pts[point+1] = next
                knot = point #index for continuity
            elif style == 'Symmetric' and point > 0 and point < len(pts)-1:
                prev, next = self.obj.Proxy.symmetricpoles(pts[point],pts[point-1],pts[point+1])
                pts[point-1] = prev
                pts[point+1] = next
                knot = point #index for continuity
            elif self.obj.Closed and (style == 'Symmetric' or style == 'Tangent'):
                if style == 'Tangent':
                    pts[1],pts[-1] = self.obj.Proxy.tangentpoles(pts[0],pts[1],pts[-1])
                elif style == 'Symmetric':
                    pts[1],pts[-1] = self.obj.Proxy.symmetricpoles(pts[0],pts[1],pts[-1])
                knot = 0
            else:
                FreeCAD.Console.PrintWarning(translate("draft", "Endpoint of BezCurve can't be smoothed")+"\n")
                return
        segment = knot // deg #segment index
        newcont=self.obj.Continuity[:] #don't edit a property inplace !!!
        if not self.obj.Closed and (len(self.obj.Continuity) == segment -1 or \
                segment == 0) : pass # open curve
        elif len(self.obj.Continuity) >= segment or \
                self.obj.Closed and segment == 0 and \
                len(self.obj.Continuity) >1:
            newcont[segment-1] = style2cont.get(style)
        else: #should not happen
            FreeCAD.Console.PrintWarning('Continuity indexing error:'+\
                'point:%d deg:%d len(cont):%d' % (knot,deg,\
                len(self.obj.Continuity)))
        self.obj.Points = pts
        self.obj.Continuity=newcont
        self.resetTrackers()

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Rectangle
    #---------------------------------------------------------------------------

    def setRectanglePts(self):
        self.editpoints.append(self.obj.Placement.Base)
        self.editpoints.append(self.obj.Shape.Vertexes[2].Point)
        v = self.obj.Shape.Vertexes
        self.bx = v[1].Point.sub(v[0].Point)
        if self.obj.Length < 0:
            self.bx = self.bx.negative()
        self.by = v[2].Point.sub(v[1].Point)
        if self.obj.Height < 0:
            self.by = self.by.negative()

    def updateRectangle(self,v):
        import DraftVecUtils
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
                self.obj.recompute()
        self.trackers[0].set(self.obj.Placement.Base)
        self.trackers[1].set(self.obj.Shape.Vertexes[2].Point)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Ellipse (yet to be implemented)
    #---------------------------------------------------------------------------

    def setEllipsePts(self):
        return

    def updateEllipse(self,v):
        return

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Circle/Arc
    #---------------------------------------------------------------------------

    def setCirclePts(self):
        self.editpoints.append(self.obj.Placement.Base)
        if self.obj.FirstAngle == self.obj.LastAngle:#self.obj is a circle
            self.ui.editUi("Circle")
            self.editpoints.append(self.obj.Shape.Vertexes[0].Point)
        else:#self.obj is an arc
            self.ui.editUi("Arc")
            self.editpoints.append(self.obj.Shape.Vertexes[0].Point)#First endpoint
            self.editpoints.append(self.obj.Shape.Vertexes[1].Point)#Second endpoint
            self.editpoints.append(self.getArcMid())#Midpoint
        return

    def updateCirclePts(self,ep1=1,ep2=1,ep3=1,ep4=1):
        self.obj.recompute()
        if ep1 == 1:
            self.trackers[0].set(self.obj.Placement.Base)
        if ep2 == 1:
            self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        if ep3 == 1:
            self.trackers[2].set(self.obj.Shape.Vertexes[1].Point)
        if ep4 == 1:
            self.trackers[3].set(self.getArcMid())

    def updateCircle(self,v):
        delta = self.invpl.multVec(v)
        if self.obj.FirstAngle == self.obj.LastAngle:# object is a circle
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
                self.setPlacement(self.obj)
                self.updateCirclePts(0,1,0,0)
            if self.editing == 1:
                self.obj.Radius = delta.Length
                self.updateCirclePts(0,0,0,0)

        else:#self.obj is an arc

            if self.arc3Pt == False:#edit by center radius FirstAngle LastAngle
                #deltaX = v[0]-self.obj.Placement.Base[0]
                #deltaY = v[1]-self.obj.Placement.Base[1]
                dangle = math.degrees(math.atan2(delta[1],delta[0]))
                if self.editing == 0:
                    p = self.obj.Placement
                    p.move(delta)
                    self.obj.Placement = p
                    self.setPlacement(self.obj)
                    self.updateCirclePts(0,1,1,1)
                elif self.editing == 1:
                    self.obj.FirstAngle=dangle
                    self.obj.recompute()
                    self.updateCirclePts(0,1,0,1)
                elif self.editing == 2:
                    self.obj.LastAngle=dangle
                    self.obj.recompute()
                    self.updateCirclePts(0,0,1,1)
                elif self.editing == 3:
                    self.obj.Radius = delta.Length
                    self.obj.recompute()
                    self.updateCirclePts(0,1,1,1)

            elif self.arc3Pt == True:
                import Part
                if self.editing == 0:#center point
                    import DraftVecUtils
                    p1 = self.invpl.multVec(self.obj.Shape.Vertexes[0].Point)
                    p2 = self.invpl.multVec(self.obj.Shape.Vertexes[1].Point)
                    p0 = DraftVecUtils.project(delta,self.invpl.multVec(self.getArcMid()))
                    self.obj.Radius = p1.sub(p0).Length
                    self.obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                    self.obj.LastAngle = -math.degrees(DraftVecUtils.angle(p2.sub(p0)))
                    self.obj.Placement.Base = self.pl.multVec(p0)
                    self.setPlacement(self.obj)
                    self.updateCirclePts(1,0,0,1)
                    return
                else:
                    if self.editing == 1:#first point
                        p1=v
                        p2=self.getArcMid()
                        p3=self.obj.Shape.Vertexes[1].Point
                    elif self.editing == 3:#midpoint
                        p1=self.obj.Shape.Vertexes[0].Point
                        p2=v
                        p3=self.obj.Shape.Vertexes[1].Point
                    elif self.editing == 2:#second point
                        p1=self.obj.Shape.Vertexes[0].Point
                        p2=self.getArcMid()
                        p3=v
                    arc=Part.ArcOfCircle(p1,p2,p3)
                    e = arc.toShape()
                    self.obj.Placement.Base=arc.Center
                    self.setPlacement(self.obj)
                    self.obj.Radius = arc.Radius
                    delta = self.invpl.multVec(p1)
                    dangleF = math.degrees(math.atan2(delta[1],delta[0]))
                    delta = self.invpl.multVec(p3)
                    dangleL = math.degrees(math.atan2(delta[1],delta[0]))
                    self.obj.FirstAngle = dangleF
                    self.obj.LastAngle = dangleL
                    FreeCAD.Console.PrintMessage("Press I to invert the circle")
                    self.updateCirclePts()

    def getArcMid(self):#Returns object midpoint
        if Draft.getType(self.obj) == "Circle":
            if self.obj.LastAngle>self.obj.FirstAngle:
                midAngle=math.radians(self.obj.FirstAngle+(self.obj.LastAngle-self.obj.FirstAngle)/2)
            else:
                midAngle=math.radians(self.obj.FirstAngle+(self.obj.LastAngle-self.obj.FirstAngle)/2)+math.pi
            midRadX=self.obj.Radius*math.cos(midAngle)
            midRadY=self.obj.Radius*math.sin(midAngle)
            deltaMid=FreeCAD.Vector(midRadX,midRadY,0.0)
            midPoint=self.pl.multVec(deltaMid) # check this line
            return(midPoint)

    def arcInvert(self):
        self.obj.FirstAngle, self.obj.LastAngle = self.obj.LastAngle, self.obj.FirstAngle
        self.obj.recompute()
        self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        self.trackers[2].set(self.obj.Shape.Vertexes[1].Point)
        self.trackers[3].set(self.getArcMid())

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Polygon (maybe could also rotate the polygon)
    #---------------------------------------------------------------------------

    def setPolygonPts(self):
        self.editpoints.append(self.obj.Placement.Base)
        self.editpoints.append(self.obj.Shape.Vertexes[0].Point)

    def updatePolygon(self,v):
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
            self.obj.recompute()
        self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Dimension (point on dimension line is not clickable)
    #---------------------------------------------------------------------------

    def setDimensionPts(self):
        p = self.obj.ViewObject.Proxy.textpos.translation.getValue()
        self.editpoints.append(self.obj.Start)
        self.editpoints.append(self.obj.End)
        self.editpoints.append(self.obj.Dimline)
        self.editpoints.append(Vector(p[0],p[1],p[2]))

    def updateDimension(self,v):
        if self.editing == 0:
            self.obj.Start = v
        elif self.editing == 1:
            self.obj.End = v
        elif self.editing == 2:
            self.obj.Dimline = v
        elif self.editing == 3:
            self.obj.ViewObject.TextPosition = v

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : ARCH Wall, Windows, Structure, Panel, etc.
    #---------------------------------------------------------------------------

    #WALL-----------------------------------------------------------------------

    def setWallPts(self):
        # try to add here an editpoint based on wall height (maybe should be good to associate it with a circular tracker)
        if Draft.getType(self.obj.Base) in ["Wire","Circle","Rectangle",
                                        "Polygon"]:
            self.obj=self.obj.Base
            self.setPlacement(self.obj.Base)
        elif Draft.getType(self.obj.Base) == "Sketch":
            if self.obj.Base.GeometryCount == 1:
                self.editpoints.append(self.obj.Base.getPoint(0,1))
                self.editpoints.append(self.obj.Base.getPoint(0,2))
            self.setPlacement(self.obj.Base)
        else:
            FreeCAD.Console.PrintWarning(translate("draft","Wall base sketch is too complex to edit: it's suggested to edit directly the sketch")+"\n")

    def updateWall(self,v):
        # try to add here an editpoint based on wall height (maybe should be good to associate it with a circular tracker)
        if self.editing == 0:
            self.obj.Base.movePoint(0,1,v)
            self.obj.Base.recompute()
        if self.editing == 1:
            self.obj.Base.movePoint(0,2,v)
            self.obj.Base.recompute()
        self.obj.recompute()

    #WINDOW---------------------------------------------------------------------

    def setWindowPts(self):
        import DraftGeomUtils
        pos=self.obj.Base.Placement.Base
        h=float(self.obj.Height)+pos.z
        normal=self.obj.Normal
        angle=normal.getAngle(FreeCAD.Vector(1,0,0))
        self.editpoints.append(pos)
        self.editpoints.append(FreeCAD.Vector(pos.x+float(self.obj.Width)*math.cos(angle-math.pi/2),
                                                pos.y+float(self.obj.Width)*math.sin(angle-math.pi/2),
                                                pos.z))
        self.editpoints.append(FreeCAD.Vector(pos.x,pos.y,h))

    def updateWindow(self,v):
        pos=self.obj.Base.Placement.Base
        if self.editing == 0:
            self.obj.Base.Placement.Base=v
            self.obj.Base.recompute()
        if self.editing == 1:
            self.obj.Width = pos.sub(v).Length
            self.obj.Base.recompute()
        if self.editing == 2:
            self.obj.Height = pos.sub(v).Length
            self.obj.Base.recompute()
        for obj in self.obj.Hosts:
            obj.recompute()
        self.obj.recompute()

    #STRUCTURE-------------------------------------------------------------------

    def setStructurePts(self):
        if self.obj.Nodes:
            self.originalDisplayMode = self.obj.ViewObject.DisplayMode
            self.originalPoints = self.obj.ViewObject.NodeSize
            self.originalNodes = self.obj.ViewObject.ShowNodes
            self.obj.ViewObject.DisplayMode = "Wireframe"
            self.obj.ViewObject.NodeSize = 1
            self.obj.ViewObject.ShowNodes = True
            for p in self.obj.Nodes:
                if self.pl:
                    p = self.pl.multVec(p)
                self.editpoints.append(p)
        else: return

    def updateStructure(self,v):
        nodes = self.obj.Nodes
        nodes[self.editing] = self.invpl.multVec(v)
        self.obj.Nodes = nodes

    #SPACE----------------------------------------------------------------------

    def setSpacePts(self):
        try:
            self.editpoints.append(self.obj.ViewObject.Proxy.getTextPosition(self.obj.ViewObject))
        except:
            pass

    def updateSpace(self,v):
        if self.editing == 0:
            self.obj.ViewObject.TextPosition = v

    #PANELS---------------------------------------------------------------------

    def setPanelCutPts(self):
        if self.obj.TagPosition.Length == 0:
            pos = self.obj.Shape.BoundBox.Center
        else:
            pos = self.pl.multVec(self.obj.TagPosition)
        self.editpoints.append(pos)

    def updatePanelCut(self,v):
        if self.editing == 0:
            self.obj.TagPosition = self.invpl.multVec(v)

    def setPanelSheetPts(self):
        self.editpoints.append(self.pl.multVec(self.obj.TagPosition))
        for o in self.obj.Group:
            self.editpoints.append(self.pl.multVec(o.Placement.Base))

    def updatePanelSheet(self,v):
        if self.editing == 0:
            self.obj.TagPosition = self.invpl.multVec(v)
        else:
            self.obj.Group[self.editing-1].Placement.Base = self.invpl.multVec(v)
    
    # PART::LINE----------------------------------------------------------------

    def setPartLinePts(self):
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(self.obj.X1,self.obj.Y1,self.obj.Z1)))
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(self.obj.X2,self.obj.Y2,self.obj.Z2)))
    
    def updatePartLine(self,v):
        pt=self.invpl.multVec(v)
        if self.editing == 0:
            self.obj.X1 = pt.x
            self.obj.Y1 = pt.y
            self.obj.Z1 = pt.z
        elif self.editing == 1:
            self.obj.X2 = pt.x
            self.obj.Y2 = pt.y
            self.obj.Z2 = pt.z

    # PART::BOX-----------------------------------------------------------------

    def setPartBoxPts(self):
        self.editpoints.append(self.obj.Placement.Base)
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(self.obj.Length,0,0)))
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(0,self.obj.Width,0)))
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(0,0,self.obj.Height)))

    def updatePartBox(self,v):
        import DraftVecUtils
        delta = self.invpl.multVec(v)
        if self.editing == 0:
            self.obj.Placement.Base = v
            self.setPlacement(self.obj)
        elif self.editing == 1:
            xVector = DraftVecUtils.project(delta,FreeCAD.Vector(1,0,0))
            self.obj.Length = xVector.Length            
        elif self.editing == 2:
            xVector = DraftVecUtils.project(delta,FreeCAD.Vector(0,1,0))
            self.obj.Width = xVector.Length            
        elif self.editing == 3:
            xVector = DraftVecUtils.project(delta,FreeCAD.Vector(0,0,1))
            self.obj.Height = xVector.Length            
        self.trackers[0].set(self.obj.Placement.Base)
        self.trackers[1].set(self.pl.multVec(FreeCAD.Vector(self.obj.Length,0,0)))
        self.trackers[2].set(self.pl.multVec(FreeCAD.Vector(0,self.obj.Width,0)))
        self.trackers[3].set(self.pl.multVec(FreeCAD.Vector(0,0,self.obj.Height)))



if FreeCAD.GuiUp:
    # setup command
    FreeCADGui.addCommand('Draft_Edit', Edit())