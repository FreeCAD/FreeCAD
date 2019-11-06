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
__url__ = "https://www.freecadweb.org"

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

    COLORS = {
        "default": FreeCADGui.draftToolBar.getDefaultColor("snap"),
        "black":  (0., 0., 0.),
        "white":  (1., 1., 1.),
        "grey":   (.5, .5, .5),
        "red":    (1., 0., 0.),
        "green":  (0., 1., 0.),
        "blue":   (0., 0., 1.),
        "yellow": (1., 1., 0.),
        "cyan":   (0., 1., 1.),
        "magenta":(1., 0., 1.)
    }


class Edit():

    "The Draft_Edit FreeCAD command definition"

    def __init__(self):
        self.running = False
        self.trackers = {'object':[]}
        self.overNode = None # preselected node with mouseover
        self.obj = None
        self.editing = None

        # event callbacks
        self.call = None
        self._keyPressedCB = None
        self._mouseMovedCB = None
        self._mousePressedCB = None

        self.selectstate = None
        self.originalDisplayMode = None
        self.originalPoints = None
        self.originalNodes = None

        # settings
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.maxObjects = param.GetInt("DraftEditMaxObjects", 5)
        self.pick_radius = param.GetInt("DraftEditPickRadius", 20)

        # preview
        self.ghost = None

        #list of supported Draft and Arch objects
        self.supportedObjs = ["BezCurve","Wire","BSpline","Circle","Rectangle",
                            "Polygon","Dimension","Space","Structure","PanelCut",
                            "PanelSheet","Wall", "Window"]
        #list of supported Part objects (they don't have a proxy)
        #TODO: Add support for "Part::Circle" "Part::RegularPolygon" "Part::Plane" "Part::Ellipse" "Part::Vertex" "Part::Spiral"
        self.supportedPartObjs = ["Sketch", "Sketcher::SketchObject", \
                                "Part", "Part::Line", "Part::Box"]

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
        self.objs = self.getObjsFromSelection()
        if not self.objs: return self.finish() 

        # Save selectstate and turn selectable false.
        # Object can remain selectable commenting following lines, 
        # but snap only process the first edge of self.obj TODO: fix it
        #self.saveSelectState(self.obj)        
        #self.setSelectState(self.obj, False)

        # start object editing
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Snapper.setSelectMode(True)
        
        self.arc3Pt = True # TODO: Find a more elegant way
        self.ui.editUi()

        for obj in self.objs:
            self.setEditPoints(obj)

        self.register_editing_callbacks()#register action callback
        # set plane tracker to match edited object
        #FreeCAD.DraftWorkingPlane.save()
        #self.alignWorkingPlane()


    def finish(self,closed=False):
        "terminates Edit Tool"
        self.unregister_selection_callback()
        self.unregister_editing_callbacks()
        self.editing = None
        self.finalizeGhost()
        FreeCADGui.Snapper.setSelectMode(False)
        if self.obj and closed:
            if "Closed" in self.obj.PropertiesList:
                if not self.obj.Closed:
                    self.obj.Closed = True
        if self.ui: self.removeTrackers()
        self.restoreSelectState(self.obj)
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
        if self._keyPressedCB is None:
            self._keyPressedCB = view.addEventCallbackPivy(
            coin.SoKeyboardEvent.getClassTypeId(), self.keyPressed)
        if self._mouseMovedCB is None:
            self._mouseMovedCB = view.addEventCallbackPivy(
            coin.SoLocation2Event.getClassTypeId(), self.mouseMoved)
        if self._mousePressedCB is None:
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
                self.finish()
            if key == 97: # "a"
                self.finish()
            if key == 111: # "o"
                self.finish(closed=True)
            if key == 105: # "i"
                if Draft.getType(self.obj) == "Circle": self.arcInvert()

    def mousePressed(self, event_callback):
        "mouse button event handler, calls: startEditing, endEditing, addPoint, delPoint"
        event = event_callback.getEvent()
        if (event.getState() == coin.SoMouseButtonEvent.DOWN and 
            event.getButton() == event.BUTTON1):#left click
            if self.ui.addButton.isChecked():
                self.addPoint(event)
                return
            if self.ui.delButton.isChecked():
                self.delPoint(event)
                return
            if Draft.getType(self.obj) == "BezCurve" and (self.ui.sharpButton.isChecked()
                                    or self.ui.tangentButton.isChecked() or
                                    self.ui.symmetricButton.isChecked()):
                pos = event.getPosition()
                node = self.getEditNode(pos)
                ep = self.getEditNodeIndex(node)
                if ep is None: return
                doc = FreeCAD.getDocument(str(node.documentName.getValue()))
                self.obj = doc.getObject(str(node.objectName.getValue()))
                if self.obj is None: return
                if self.ui.sharpButton.isChecked():
                    return self.smoothBezPoint(ep, 'Sharp')
                elif self.ui.tangentButton.isChecked():
                    return self.smoothBezPoint(ep, 'Tangent')
                elif self.ui.symmetricButton.isChecked():
                    return self.smoothBezPoint(ep, 'Symmetric')
            if self.editing is None:
                self.startEditing(event)
            else:
                self.endEditing()
    
    def mouseMoved(self, event_callback):
        "mouse moved event handler, update tracker position and update preview ghost"
        event = event_callback.getEvent()
        if self.editing != None:
            self.updateTrackerAndGhost(event)
        else:
            # look for a node in mouse position and highlight it
            pos = event.getPosition()
            node = self.getEditNode(pos)
            ep = self.getEditNodeIndex(node)
            if ep != None: 
                if self.overNode != None: self.overNode.setColor(COLORS["default"])
                self.trackers[str(node.objectName.getValue())][ep].setColor(COLORS["red"])
                self.overNode = self.trackers[str(node.objectName.getValue())][ep]
            else:
                if self.overNode != None:
                    self.overNode.setColor(COLORS["default"])
                    self.overNode = None

    def startEditing(self, event):
        "start editing selected EditNode"
        pos = event.getPosition()
        node = self.getEditNode(pos)
        ep = self.getEditNodeIndex(node)
        if ep is None: return

        doc = FreeCAD.getDocument(str(node.documentName.getValue()))
        self.obj = doc.getObject(str(node.objectName.getValue()))
        if self.obj is None: return
        self.setPlacement(self.obj)

        FreeCAD.Console.PrintMessage(str(self.obj.Name)+str(": editing node: n° ")+str(ep)+"\n")

        self.ui.lineUi()
        self.ui.isRelative.show()
        self.editing = ep
        self.trackers[self.obj.Name][self.editing].off()
        self.finalizeGhost()
        self.ghost = self.initGhost(self.obj)
        self.node.append(self.trackers[self.obj.Name][self.editing].get())
        FreeCADGui.Snapper.setSelectMode(False)
        self.hideTrackers()

    def updateTrackerAndGhost(self, event):
        "updates tracker position when editing and update ghost"
        pos = event.getPosition().getValue()
        orthoConstrain = False
        if event.wasShiftDown() == 1: orthoConstrain = True
        snappedPos = FreeCADGui.Snapper.snap((pos[0],pos[1]),self.node[-1], constrain=orthoConstrain)
        self.trackers[self.obj.Name][self.editing].set(snappedPos)
        self.ui.displayPoint(snappedPos,self.node[-1])
        if self.ghost: self.updateGhost(obj=self.obj,idx=self.editing,pt=snappedPos)

    def endEditing(self):
        "terminate editing and start object updating process"
        self.finalizeGhost()
        self.trackers[self.obj.Name][self.editing].on()
        FreeCADGui.Snapper.setSelectMode(True)
        self.numericInput(self.trackers[self.obj.Name][self.editing].get())
        self.showTrackers()
        DraftTools.redraw3DView()

    #---------------------------------------------------------------------------
    # UTILS
    #---------------------------------------------------------------------------

    def getObjsFromSelection(self):
        "evaluate selection and returns a valid object to edit"
        selection = FreeCADGui.Selection.getSelection()
        self.objs = []
        if len(selection) > self.maxObjects:
            FreeCAD.Console.PrintMessage(translate("draft", 
                "Too many objects selected, max number set to: "+
                str(self.maxObjects)+"\n"))
            return None
        for obj in selection:
            if "Proxy" in selection[0].PropertiesList and hasattr(selection[0].Proxy,"Type"):
                if Draft.getType(obj) in self.supportedObjs:
                    self.objs.append(obj)
                    continue
            else:
                try:
                    if Draft.getType(obj) in self.supportedPartObjs and obj.TypeId in self.supportedPartObjs:
                        self.objs.append(obj)
                        continue
                except:
                    pass
            FreeCAD.Console.PrintWarning(translate("draft", str(obj.Name)+": this object is not editable")+"\n")
        return self.objs

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

    def setSelectState(self, obj, selState = False):
        if hasattr(obj.ViewObject,"Selectable"):
            obj.ViewObject.Selectable = selState

    def saveSelectState(self, obj):
        if hasattr(obj.ViewObject,"Selectable"):
            self.selectstate = obj.ViewObject.Selectable

    def restoreSelectState(self,obj):
        if obj:
            if hasattr(obj.ViewObject,"Selectable") and (self.selectstate != None):
                obj.ViewObject.Selectable = self.selectstate

    def setPlacement(self,obj):
        "set self.pl and self.invpl to self.obj placement and inverse placement"
        if not obj: return
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

    def getEditNode(self, pos):
        "get edit node from given screen position"
        node = self.sendRay(pos)
        return node
    
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
            #import DraftTrackers
            if hasattr(point,"subElementName") and 'EditNode' in str(point.subElementName.getValue()):
                return point
        return None

    def getEditNodeIndex(self, point):
        "get edit node index from given screen position"
        if point:
            subElement = str(point.subElementName.getValue())
            ep = int(subElement[8:])
            return ep
        else:
            return None


    #---------------------------------------------------------------------------
    # EDIT TRACKERS functions
    #---------------------------------------------------------------------------

    def setTrackers(self, obj):
        "set Edit Trackers for editpoints collected from self.obj"
        self.trackers[obj.Name] = []
        if Draft.getType(obj) == "BezCurve":
            self.resetTrackersBezier(obj)
        else:
            if obj.Name in self.trackers:
                self.removeTrackers(obj)
            for ep in range(len(self.editpoints)):
                self.trackers[obj.Name].append(editTracker(pos=self.editpoints[ep],name=obj.Name,idx=ep))

    def resetTrackers(self):
        "reset Edit Trackers and set them again"
        self.removeTrackers(self.obj)
        self.setTrackers(self.obj)

    def removeTrackers(self, obj = None):
        "reset Edit Trackers and set them again"
        if obj:
            if obj.Name in self.trackers:
                for t in self.trackers[obj.Name]:
                    t.finalize()
            self.trackers[obj.Name] = []
        else:
            for key in self.trackers.keys():
                for t in self.trackers[key]:
                    t.finalize()
            self.trackers = {'object':[]}


    def hideTrackers(self):
        "hide Edit Trackers"
        for t in self.trackers[self.obj.Name]:
            t.off()

    def showTrackers(self):
        "show Edit Trackers"
        for t in self.trackers[self.obj.Name]:
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
                        p0 = DraftVecUtils.project(self.invpl.multVec(pt),self.invpl.multVec(self.getArcMid(self.obj)))
                        self.ghost.autoinvert=False
                        self.ghost.setRadius(p1.sub(p0).Length)
                        self.ghost.setStartPoint(self.obj.Shape.Vertexes[1].Point)
                        self.ghost.setEndPoint(self.obj.Shape.Vertexes[0].Point)
                        self.ghost.setCenter(self.pl.multVec(p0))
                        return
                    else:
                        p1=self.obj.Shape.Vertexes[0].Point
                        p2=self.getArcMid(self.obj)
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
        pos = event.getPosition()
        #self.setSelectState(self.obj, True)
        selobjs = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo((pos[0],pos[1]))
        if not selobjs: return
        for info in selobjs:
            if not info: return
            for o in self.objs:
                if o.Name != info["Object"]: continue
                self.obj = o
                break
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
        self.removeTrackers(self.obj)
        self.setEditPoints(self.obj)
        #self.setSelectState(self.obj, False)
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

    def delPoint(self,event):
        pos = event.getPosition()
        node = self.getEditNode(pos)
        ep = self.getEditNodeIndex(node)

        if ep is None: return FreeCAD.Console.PrintWarning(
            translate("draft", "Node not found\n"))

        doc = FreeCAD.getDocument(str(node.documentName.getValue()))
        self.obj = doc.getObject(str(node.objectName.getValue()))
        if self.obj is None: return
        if not (Draft.getType(self.obj) in ["Wire","BSpline","BezCurve"]): return
        if len(self.obj.Points) <= 2: return FreeCAD.Console.PrintWarning(
            translate("draft", "Active object must have more than two points/nodes")+"\n")

        pts = self.obj.Points
        pts.pop(ep)
        self.obj.Points = pts
        if Draft.getType(self.obj) =="BezCurve":
            self.obj.Proxy.resetcontinuity(self.obj)
        self.obj.recompute()
        
        # don't do tan/sym on DWire/BSpline!
        self.removeTrackers(self.obj)
        self.setEditPoints(self.obj)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : GENERAL
    #---------------------------------------------------------------------------

    def setEditPoints(self,obj):
        "append given object's editpoints to self.edipoints and set EditTrackers"
        self.editpoints = []
        self.setPlacement(obj)

        objectType = Draft.getType(obj)

        if objectType in ["Wire","BSpline"]:
            self.ui.editUi("Wire")
            self.setWirePts(obj)
        elif objectType == "BezCurve":
            self.ui.editUi("BezCurve")
            self.resetTrackersBezier(obj)
            self.editpoints = []
            return
            #self.setWirePts(obj)
        elif objectType == "Circle":
            self.setCirclePts(obj)
        elif objectType == "Rectangle":
            self.setRectanglePts(obj)
        elif objectType == "Polygon":
            self.setPolygonPts(obj)
        elif objectType == "Dimension":
            self.setDimensionPts(obj)
        elif objectType == "Wall":
            self.setWallPts(obj)            
        elif objectType == "Window":
            self.setWindowPts(obj)
        elif objectType == "Space":
            self.setSpacePts(obj)
        elif objectType == "Structure":
            self.setStructurePts(obj)
        elif objectType == "PanelCut":
            self.setPanelCutPts(obj)
        elif objectType == "PanelSheet":
            self.setPanelSheetPts(obj)
        elif objectType == "Part" and obj.TypeId == "Part::Box":
            self.setPartBoxPts(obj)
        elif objectType == "Part::Line" and obj.TypeId == "Part::Line":
            self.setPartLinePts(obj)
        elif objectType == "Sketch":
            self.setSketchPts(obj)

        if self.editpoints: # set trackers and align plane
            self.setTrackers(obj)
            self.editpoints = []
        else:
            FreeCAD.Console.PrintWarning(translate("draft", "No edit point found for selected object")+"\n")
            # do not finish if some trackers are still present
            if self.trackers == {'object':[]}: self.finish()
            return



    def update(self,v):
        "apply the vector to the modified point and update self.obj"

        FreeCAD.ActiveDocument.openTransaction("Edit")

        if Draft.getType(self.obj) in ["Wire","BSpline"]: self.updateWire(v)
        elif Draft.getType(self.obj) == "BezCurve": self.updateWire(v)
        elif Draft.getType(self.obj) == "Circle": self.updateCircle(v)
        elif Draft.getType(self.obj) == "Rectangle": self.updateRectangle(v)
        elif Draft.getType(self.obj) == "Polygon": self.updatePolygon(v)
        elif Draft.getType(self.obj) == "Dimension": self.updateDimension(v)
        elif Draft.getType(self.obj) == "Sketch": self.updateSketch(v)
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
        editPnt = self.invpl.multVec(v)
        # DNC: allows to close the curve by placing ends close to each other
        tol = 0.001
        if ( ( self.editing == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( self.editing == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
            self.obj.Closed = True
        # DNC: fix error message if edited point coincides with one of the existing points
        if ( editPnt in pts ) == True: # checks if point enter is equal to other, this could cause a OCC problem
            FreeCAD.Console.PrintMessage(translate("draft", "Is not possible to have two coincident points in this object, please try again.")+"\n")
            if Draft.getType(self.obj) in ["BezCurve"]: self.resetTrackers()
            else: self.trackers[self.obj.Name][self.editing].set(self.pl.multVec(self.obj.Points[self.editing]))
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
        self.trackers[self.obj.Name][self.editing].set(v)


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
                    if moveTrackers: self.trackers[self.obj.Name][idx-1].set(pts[idx-1])
                if idx < len(pts)-1: #move right pole
                    pts[idx+1] = pts[idx+1] + editPnt - pts[idx]
                    if moveTrackers: self.trackers[self.obj.Name][idx+1].set(pts[idx+1])
                if idx == 0 and self.obj.Closed: # move last pole
                    pts[-1] = pts [-1] + editPnt -pts[idx]
                    if moveTrackers: self.trackers[self.obj.Name][-1].set(pts[-1])

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
                if moveTrackers: self.trackers[self.obj.Name][changep].set(pts[changep])
            elif cont ==2: #symmetric
                pts[changep] = self.obj.Proxy.modifysymmetricpole(\
                        pts[knot],editPnt)
                if moveTrackers: self.trackers[self.obj.Name][changep].set(pts[changep])
        pts[idx]=v

        return pts #returns the list of new points, taking into account knot continuity

    def resetTrackersBezier(self, obj):
        #in future move tracker definition to DraftTrackers
        from pivy import coin
        knotmarkers = (coin.SoMarkerSet.DIAMOND_FILLED_9_9,#sharp
                coin.SoMarkerSet.SQUARE_FILLED_9_9,        #tangent
                coin.SoMarkerSet.HOURGLASS_FILLED_9_9)     #symmetric
        polemarker = coin.SoMarkerSet.CIRCLE_FILLED_9_9    #pole
        self.trackers[obj.Name]=[]
        cont=obj.Continuity
        firstknotcont = cont[-1] if (obj.Closed and cont) else 0
        pointswithmarkers=[(obj.Shape.Edges[0].Curve.\
                getPole(1),knotmarkers[firstknotcont])]
        for edgeindex, edge in enumerate(obj.Shape.Edges):
            poles=edge.Curve.getPoles()
            pointswithmarkers.extend([(point,polemarker) for \
                    point in poles[1:-1]])
            if not obj.Closed or len(obj.Shape.Edges) > edgeindex +1:
                knotmarkeri=cont[edgeindex] if len(cont) > edgeindex else 0
                pointswithmarkers.append((poles[-1],knotmarkers[knotmarkeri]))
        for index,pwm in enumerate(pointswithmarkers):
            p,marker=pwm
            #if self.pl: p = self.pl.multVec(p)
            self.trackers[obj.Name].append(editTracker(p,obj.Name,\
                index,obj.ViewObject.LineColor,\
                marker=marker))

    def smoothBezPoint(self, point, style='Symmetric'):
        "called when changing the continuity of a knot"
        style2cont = {'Sharp':0,'Tangent':1,'Symmetric':2}
        if point is None: return
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

    def setRectanglePts(self, obj):
        self.editpoints.append(obj.Placement.Base)
        self.editpoints.append(obj.Shape.Vertexes[2].Point)
        v = obj.Shape.Vertexes
        self.bx = v[1].Point.sub(v[0].Point)
        if obj.Length < 0:
            self.bx = self.bx.negative()
        self.by = v[2].Point.sub(v[1].Point)
        if obj.Height < 0:
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
        self.trackers[self.obj.Name][0].set(self.obj.Placement.Base)
        self.trackers[self.obj.Name][1].set(self.obj.Shape.Vertexes[2].Point)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Ellipse (# TODO: yet to be implemented)
    #---------------------------------------------------------------------------

    def setEllipsePts(self):
        return

    def updateEllipse(self,v):
        return

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Circle/Arc
    #---------------------------------------------------------------------------

    def setCirclePts(self, obj):
        self.editpoints.append(obj.Placement.Base)
        if obj.FirstAngle == obj.LastAngle:#self.obj is a circle
            self.ui.editUi("Circle")
            self.editpoints.append(obj.Shape.Vertexes[0].Point)
        else:#self.obj is an arc
            self.ui.editUi("Arc")
            self.editpoints.append(obj.Shape.Vertexes[0].Point)#First endpoint
            self.editpoints.append(obj.Shape.Vertexes[1].Point)#Second endpoint
            self.editpoints.append(self.getArcMid(obj))#Midpoint
        return

    def updateCirclePts(self,ep1=1,ep2=1,ep3=1,ep4=1):
        self.obj.recompute()
        if ep1 == 1:
            self.trackers[self.obj.Name][0].set(self.obj.Placement.Base)
        if ep2 == 1:
            self.trackers[self.obj.Name][1].set(self.obj.Shape.Vertexes[0].Point)
        if ep3 == 1:
            self.trackers[self.obj.Name][2].set(self.obj.Shape.Vertexes[1].Point)
        if ep4 == 1:
            self.trackers[self.obj.Name][3].set(self.getArcMid(self.obj))

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
                    p0 = DraftVecUtils.project(delta,self.invpl.multVec(self.getArcMid(self.obj)))
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
                        p2=self.getArcMid(self.obj)
                        p3=self.obj.Shape.Vertexes[1].Point
                    elif self.editing == 3:#midpoint
                        p1=self.obj.Shape.Vertexes[0].Point
                        p2=v
                        p3=self.obj.Shape.Vertexes[1].Point
                    elif self.editing == 2:#second point
                        p1=self.obj.Shape.Vertexes[0].Point
                        p2=self.getArcMid(self.obj)
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
                    FreeCAD.Console.PrintMessage("Press I to invert the circle\n")
                    self.updateCirclePts()

    def getArcMid(self, obj):#Returns object midpoint
        if Draft.getType(obj) == "Circle":
            if obj.LastAngle>obj.FirstAngle:
                midAngle=math.radians(obj.FirstAngle+(obj.LastAngle-obj.FirstAngle)/2)
            else:
                midAngle=math.radians(obj.FirstAngle+(obj.LastAngle-obj.FirstAngle)/2)+math.pi
            midRadX=obj.Radius*math.cos(midAngle)
            midRadY=obj.Radius*math.sin(midAngle)
            deltaMid=FreeCAD.Vector(midRadX,midRadY,0.0)
            midPoint=self.pl.multVec(deltaMid) # check this line
            return(midPoint)

    def arcInvert(self):
        self.obj.FirstAngle, self.obj.LastAngle = self.obj.LastAngle, self.obj.FirstAngle
        self.obj.recompute()
        self.trackers[self.obj.Name][1].set(self.obj.Shape.Vertexes[0].Point)
        self.trackers[self.obj.Name][2].set(self.obj.Shape.Vertexes[1].Point)
        self.trackers[self.obj.Name][3].set(self.getArcMid(self.obj))

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Polygon (maybe could also rotate the polygon)
    #---------------------------------------------------------------------------

    def setPolygonPts(self, obj):
        self.editpoints.append(obj.Placement.Base)
        self.editpoints.append(obj.Shape.Vertexes[0].Point)

    def updatePolygon(self,v):
        delta = v.sub(self.obj.Placement.Base)
        if self.editing == 0:
            p = self.obj.Placement
            p.move(delta)
            self.obj.Placement = p
            self.trackers[self.obj.Name][0].set(self.obj.Placement.Base)
        elif self.editing == 1:
            if self.obj.DrawMode == 'inscribed':
                self.obj.Radius = delta.Length
            else:
                halfangle = ((math.pi*2)/self.obj.FacesNumber)/2
                rad = math.cos(halfangle)*delta.Length
                self.obj.Radius = rad
            self.obj.recompute()
        self.trackers[self.obj.Name][1].set(self.obj.Shape.Vertexes[0].Point)

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Dimension (point on dimension line is not clickable)
    #---------------------------------------------------------------------------

    def setDimensionPts(self, obj):
        p = obj.ViewObject.Proxy.textpos.translation.getValue()
        self.editpoints.append(obj.Start)
        self.editpoints.append(obj.End)
        self.editpoints.append(obj.Dimline)
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

    # SKETCH: just if it's composed by a single segment-------------------------

    def setSketchPts(self, obj):
        if obj.GeometryCount == 1:
            self.editpoints.append(self.pl.multVec(obj.getPoint(0,1)))
            self.editpoints.append(self.pl.multVec(obj.getPoint(0,2)))
        else:
            FreeCAD.Console.PrintWarning(translate("draft","Sketch is too complex\
                 to edit: it is suggested to use sketcher editor")+"\n")

    def updateSketch(self,v):
        if self.editing == 0:
            self.obj.movePoint(0,1,self.invpl.multVec(v))
            self.obj.recompute()
        if self.editing == 1:
            self.obj.movePoint(0,2,self.invpl.multVec(v))
            self.obj.recompute()


    #WALL-----------------------------------------------------------------------

    def setWallPts(self, obj):
        # try to add here an editpoint based on wall height (maybe should be good to associate it with a circular tracker)
        if Draft.getType(obj.Base) in ["Wire","Circle","Rectangle",
                                        "Polygon", "Sketch"]:
            self.setEditPoints(obj.Base)
            pl=obj.Base.Placement.multiply(obj.Placement)
        self.editpoints.append(pl.multVec(FreeCAD.Vector(0,0,obj.Height)))

    def updateWall(self,v):
        import DraftVecUtils
        if Draft.getType(self.obj.Base) in ["Wire","Circle","Rectangle",
                                        "Polygon", "Sketch"]:
            invpl=self.obj.Base.Placement.multiply(self.obj.Placement).inverse()
        else:
            invpl=self.invpl
        if self.editing == 0:
            delta= invpl.multVec(v)
            vz=DraftVecUtils.project(delta,FreeCAD.Vector(0,0,1))
            self.obj.Height=vz.Length
            self.obj.recompute()


    #WINDOW---------------------------------------------------------------------

    def setWindowPts(self, obj):
        import DraftGeomUtils
        pos=obj.Base.Placement.Base
        h=float(obj.Height)+pos.z
        normal=obj.Normal
        angle=normal.getAngle(FreeCAD.Vector(1,0,0))
        self.editpoints.append(pos)
        self.editpoints.append(FreeCAD.Vector(pos.x+float(obj.Width)*math.cos(angle-math.pi/2),
                                                pos.y+float(obj.Width)*math.sin(angle-math.pi/2),
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

    def setStructurePts(self, obj):
        if obj.Nodes:
            self.originalDisplayMode = obj.ViewObject.DisplayMode
            self.originalPoints = obj.ViewObject.NodeSize
            self.originalNodes = obj.ViewObject.ShowNodes
            self.obj.ViewObject.DisplayMode = "Wireframe"
            self.obj.ViewObject.NodeSize = 1
            #   self.obj.ViewObject.ShowNodes = True
            for p in obj.Nodes:
                if self.pl:
                    p = self.pl.multVec(p)
                self.editpoints.append(p)
        else: return

    def updateStructure(self,v):
        nodes = self.obj.Nodes
        nodes[self.editing] = self.invpl.multVec(v)
        self.obj.Nodes = nodes

    #SPACE----------------------------------------------------------------------

    def setSpacePts(self, obj):
        try:
            self.editpoints.append(obj.ViewObject.Proxy.getTextPosition(obj.ViewObject))
        except:
            pass

    def updateSpace(self,v):
        if self.editing == 0:
            self.obj.ViewObject.TextPosition = v

    #PANELS---------------------------------------------------------------------

    def setPanelCutPts(self, obj):
        if self.obj.TagPosition.Length == 0:
            pos = obj.Shape.BoundBox.Center
        else:
            pos = self.pl.multVec(obj.TagPosition)
        self.editpoints.append(pos)

    def updatePanelCut(self,v):
        if self.editing == 0:
            self.obj.TagPosition = self.invpl.multVec(v)

    def setPanelSheetPts(self, obj):
        self.editpoints.append(self.pl.multVec(obj.TagPosition))
        for o in obj.Group:
            self.editpoints.append(self.pl.multVec(o.Placement.Base))

    def updatePanelSheet(self,v):
        if self.editing == 0:
            self.obj.TagPosition = self.invpl.multVec(v)
        else:
            self.obj.Group[self.editing-1].Placement.Base = self.invpl.multVec(v)
    
    # PART::LINE----------------------------------------------------------------

    def setPartLinePts(self, obj):
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(obj.X1,obj.Y1,obj.Z1)))
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(obj.X2,obj.Y2,obj.Z2)))
    
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

    def setPartBoxPts(self, obj):
        self.editpoints.append(obj.Placement.Base)
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(obj.Length,0,0)))
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(0,obj.Width,0)))
        self.editpoints.append(self.pl.multVec(FreeCAD.Vector(0,0,obj.Height)))

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
        self.trackers[self.obj.Name][0].set(self.obj.Placement.Base)
        self.trackers[self.obj.Name][1].set(self.pl.multVec(FreeCAD.Vector(self.obj.Length,0,0)))
        self.trackers[self.obj.Name][2].set(self.pl.multVec(FreeCAD.Vector(0,self.obj.Width,0)))
        self.trackers[self.obj.Name][3].set(self.pl.multVec(FreeCAD.Vector(0,0,self.obj.Height)))



if FreeCAD.GuiUp:
    # setup command
    FreeCADGui.addCommand('Draft_Edit', Edit())
