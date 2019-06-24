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

__title__="FreeCAD Draft Edit Tool"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin"
__url__ = "http://www.freecadweb.org"

import FreeCAD, FreeCADGui, Draft, DraftTools, math

from FreeCAD import Vector
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
        self.call = None
        self.selectstate = None
        self.originalDisplayMode = None
        self.originalPoints = None
        self.originalNodes = None
        self.ghost = None
        self.supportedObjs = ["BezCurve","Wire","BSpline","Circle","Rectangle",
                            "Polygon","Dimension","Space","Structure","PanelCut",
                            "PanelSheet","Wall", "Window"]

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Edit',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edits the active object")}

    def Activated(self):
        if self.running:
            self.finish()
        DraftTools.Modifier.Activated(self,"Edit")

        self.ui = FreeCADGui.draftToolBar
        self.view = Draft.get3DView()

        if FreeCADGui.Selection.getSelection():
            self.proceed()
        else:    
            self.ui.selectUi()
            FreeCAD.Console.PrintMessage(translate("draft", "Select a Draft object to edit")+"\n")
            if self.call:
                self.view.removeEventCallback("SoEvent",self.call)
            self.call = self.view.addEventCallback("SoEvent",DraftTools.selectObject)

    def proceed(self):
        "this method defines editpoints and set the editTrackers"

        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
            self.call = None

        self.parseSelection()

        if not self.obj:
            self.finish()

        if not FreeCAD.ActiveDocument:
            self.finish()

        # store selectable state of the object
        if hasattr(self.obj.ViewObject,"Selectable"):
            self.selectstate = self.obj.ViewObject.Selectable
            self.obj.ViewObject.Selectable = False

        # start object editing

        FreeCADGui.Selection.clearSelection()
        self.editing = None
        self.editpoints = []
        self.pl = None
        self.arc3Pt = False
        FreeCADGui.Snapper.setSelectMode(True)
        
        self.ui.editUi()

        self.getPlacement(self.obj)                

        self.setEditPoints(self.obj)

        if self.editpoints: # set trackers and align plane 
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.setTrackers()
            # set plane tracker to match edited object
            FreeCAD.DraftWorkingPlane.save()
            self.alignWorkingPlane()
            self.editpoints = []
        else:
            FreeCAD.Console.PrintWarning(translate("draft", "No edit point found for selected object")+"\n")
            self.finish()
    
    def parseSelection(self):
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintMessage(translate("draft", "Please select only one object")+"\n")
            self.finish()
        try:
            if "Proxy" in selection[0].PropertiesList and hasattr(selection[0].Proxy,"Type"):
                if Draft.getType(selection[0]) in self.supportedObjs:
                    self.obj = selection[0]
        except:    
            FreeCAD.Console.PrintWarning(translate("draft", "This object is not editable")+"\n")
            self.finish()
    
    def getPlacement(self,obj):
        if "Placement" in obj.PropertiesList:
            self.pl = obj.Placement
            self.invpl = self.pl.inverse()

    def alignWorkingPlane(self):
        if "Shape" in self.obj.PropertiesList:
            if DraftTools.plane.weak:
                DraftTools.plane.alignToFace(self.obj.Shape)
        if self.planetrack:
            self.planetrack.set(self.editpoints[0])

    def setTrackers(self):
        "set Edit Trackers for editpoints collected from self.obj"
        self.trackers = []
        if Draft.getType(self.obj) == "BezCurve":
            self.resetTrackersBezier()
        else:
            for ep in range(len(self.editpoints)):
                self.trackers.append(editTracker(self.editpoints[ep],self.obj.Name,ep))

    def resetTrackers(self):
        "reset Edit Trackers and set them again"
        self.removeTrackers()
        self.trackers = []
        self.setTrackers()
    
    def removeTrackers(self):
        "reset Edit Trackers and set them again"
        for t in self.trackers:
            t.finalize()     

    def hideTrackers(self):
        "hide Edit Trackers"
        for t in self.trackers:
            t.off()

    def showTrackers(self):
        "show Edit Trackers"
        for t in self.trackers:
            t.on()

    def action(self,arg):
        "scene event handler"
        
        if arg["Type"] == "SoKeyboardEvent" and arg["State"] == "DOWN":
            if arg["Key"] == "ESCAPE":
                self.finish()
            elif arg["Key"] == "a":
                self.finish()
            elif arg["Key"] == "o":
                self.finish(closed=True)            
            elif arg["Key"] == "i":
                if Draft.getType(self.obj) == "Circle": self.arcInvert()               
                    
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.point,ctrlPoint,info = DraftTools.getPoint(self,arg)# causes problems when mouseover bezcurves
            if self.editing != None:
                self.trackers[self.editing].set(self.point)
                #FreeCAD.Console.PrintMessage(self.ghost)
                self.updateGhost(obj=self.obj,idx=self.editing,pt=self.point)

            if hasattr(self.obj.ViewObject,"Selectable"):
                if self.ui.addButton.isChecked():
                    self.obj.ViewObject.Selectable = True
                else:
                    self.obj.ViewObject.Selectable = False
            DraftTools.redraw3DView()
            
        elif arg["Type"] == "SoMouseButtonEvent" and arg["State"] == "DOWN":
            
            if arg["Button"] == "BUTTON1":
                self.ui.redraw()
                
                if self.editing == None:
                    # USECASE: User click on one of the editpoints or another object
                    ep = None
                    selobjs = self.getSelection()
                    if selobjs == None: return
                    
                    if self.ui.addButton.isChecked():# still quite raw
                        # USECASE: User add a new point to the object
                        for info in selobjs:
                            if Draft.getType(self.obj) == "Wire" and 'Edge' in info["Component"]:
                                pt = FreeCAD.Vector(info["x"],info["y"],info["z"])
                                self.addPointToWire(pt, int(info["Component"][4:]))
                            elif self.point:
                                pt = self.point
                                if "x" in info:# prefer "real" 3D location over working-plane-driven one if possible
                                    pt = FreeCAD.Vector(info["x"],info["y"],info["z"])
                                self.addPointToCurve(pt,info)
                        self.removeTrackers() 
                        self.editpoints = []
                        self.setEditPoints(self.obj)
                        self.resetTrackers()
                        return
                    
                    ep = self.lookForClickedNode(selobjs,tolerance=20)
                    if ep == None: return

                    if self.ui.delButton.isChecked(): # still quite raw
                        # USECASE: User delete a point of the object
                        self.delPoint(ep)
                        # don't do tan/sym on DWire/BSpline!
                        self.removeTrackers()
                        self.editpoints = []
                        self.setEditPoints(self.obj)
                        self.resetTrackers()
                        return

                    if Draft.getType(self.obj) == "BezCurve":
                        # USECASE: User change the continuity of a Bezcurve point
                        if self.ui.sharpButton.isChecked():
                            return self.smoothBezPoint(ep, 'Sharp')
                        elif self.ui.tangentButton.isChecked():
                            return self.smoothBezPoint(ep, 'Tangent')
                        elif self.ui.symmetricButton.isChecked():
                            return self.smoothBezPoint(ep, 'Symmetric')

                    self.ui.pointUi()
                    self.ui.isRelative.show()
                    self.editing = ep
                    self.trackers[self.editing].off()
                    self.finalizeGhost()
                    self.ghost = self.initGhost(self.obj)
                    '''if hasattr(self.obj.ViewObject,"Selectable"):
                        self.obj.ViewObject.Selectable = False'''
                    self.node.append(self.trackers[self.editing].get())
                    FreeCADGui.Snapper.setSelectMode(False)

                else: #if self.editing != None:
                    # USECASE: Destination point of editing is clicked
                    self.finalizeGhost()
                    self.trackers[self.editing].on()
                    #if hasattr(self.obj.ViewObject,"Selectable"):
                    #    self.obj.ViewObject.Selectable = True
                    FreeCADGui.Snapper.setSelectMode(True)
                    self.numericInput(self.trackers[self.editing].get())
    
    def getSelection(self):
        p = FreeCADGui.ActiveDocument.ActiveView.getCursorPos()
        selobjs = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo(p)
        if not selobjs:
            selobjs = [FreeCADGui.ActiveDocument.ActiveView.getObjectInfo(p)]
        if not selobjs or selobjs == [None]:
            return None
        else: return selobjs

    def lookForClickedNode(self,selobjs,tolerance=20):
        for info in selobjs:
            #if info["Object"] == self.obj.Name:
            #    return
            if ('EditNode' in info["Component"]):#True as a result of getObjectInfo
                ep = int(info["Component"][8:])
            elif ('Vertex' in info["Component"]):# if vertex is clicked, the edit point is selected only if (distance < tolerance)
                p = FreeCAD.Vector(info["x"],info["y"],info["z"])
                for i,t in enumerate(self.trackers):
                    if (t.get().sub(p)).Length <= 0.01:
                        ep = i
                        break
            elif ('Edge' in info["Component"]) or ('Face' in info["Component"]) : # if edge is clicked, the nearest edit point is selected, then tolerance is verified
                p = FreeCAD.Vector(info["x"],info["y"],info["z"])
                d = 1000000.0
                for i,t in enumerate(self.trackers):
                    if (t.get().sub(p)).Length < d:
                        d = (t.get().sub(p)).Length
                        ep = i
                if d > tolerance:# should find a way to link the tolerance to zoom
                    return
        return ep


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

    def finish(self,closed=False):
        "terminates Edit Tool"
        FreeCADGui.Snapper.setSelectMode(False)
        if self.obj and closed:
            if "Closed" in self.obj.PropertiesList:
                if not self.obj.Closed:
                    self.obj.Closed = True
        if self.ui:
            if self.trackers:
                for t in self.trackers:
                    t.finalize()
        self.finalizeGhost()
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
        if FreeCADGui.Snapper.grid:
            FreeCADGui.Snapper.grid.set()
        self.running = False
        # following line causes crash
        # FreeCADGui.ActiveDocument.resetEdit()

    #---------------------------------------------------------------------------
    # PREVIEW
    #---------------------------------------------------------------------------
    
    def initGhost(self,obj):
        if Draft.getType(obj) == "Wire": 
            return wireTracker(obj.Shape)
        elif Draft.getType(obj) == "BSpline":
            return bsplineTracker()
        elif Draft.getType(obj) == "BezCurve":
            return bezcurveTracker()
            

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
        FreeCAD.ActiveDocument.recompute()
        self.resetTrackers()

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
        FreeCAD.ActiveDocument.recompute()
        self.resetTrackers()

    def delPoint(self,point):
        if not (Draft.getType(self.obj) in ["Wire","BSpline","BezCurve"]): return
        if len(self.obj.Points) <= 2:
            FreeCAD.Console.PrintWarning(translate("draft", "Active object must have more than two points/nodes")+"\n")
        else:
            pts = self.obj.Points
            pts.pop(point)
            self.obj.Points = pts
            if Draft.getType(self.obj) =="BezCurve":
                self.obj.Proxy.resetcontinuity(self.obj)
            FreeCAD.ActiveDocument.recompute()
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
        if ( editPnt in pts ) == False: # checks if point enter is equal to other, this could cause a OCC problem
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
        if ( editPnt in pts ) == False:
            if Draft.getType(self.obj) in ["BezCurve"]:
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

        else:
            self.finish()

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

    def smoothBezPoint(self,point, style='Symmetric'):
        "called when changing the continuity of a knot"
        style2cont = {'Sharp':0,'Tangent':1,'Symmetric':2}
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
        delta = v.sub(self.obj.Placement.Base)
        if self.obj.FirstAngle == self.obj.LastAngle:# object is a circle
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
                self.updateCirclePts(0,1,0,0)
            if self.editing == 1:
                self.obj.Radius = delta.Length
                self.updateCirclePts(0,0,0,0)
                
        else:#self.obj is an arc
            
            if self.arc3Pt == True:#edit by center radius FirstAngle LastAngle
                deltaX = v[0]-self.obj.Placement.Base[0]
                deltaY = v[1]-self.obj.Placement.Base[1]
                dangle = math.degrees(math.atan2(deltaY,deltaX))
                if self.editing == 0:
                    p = self.obj.Placement
                    p.move(delta)
                    self.obj.Placement = p
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
                    
            elif self.arc3Pt == False:
                import Part
                if self.editing == 0:#center point
                    import DraftVecUtils
                    p1 = self.obj.Shape.Vertexes[0].Point
                    p2 = self.obj.Shape.Vertexes[1].Point
                    p0 = DraftVecUtils.project(delta,self.getArcMid().sub(self.obj.Placement.Base))
                    p0 = p0.add(self.obj.Placement.Base)
                    self.obj.Placement.Base = p0
                    self.obj.Radius = p1.sub(p0).Length
                    self.obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                    self.obj.LastAngle = -math.degrees(DraftVecUtils.angle(p2.sub(p0)))
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
                    arc=Part.ArcOfCircle(p1,p2,p3)#object is a support, do i have to delete it someway after?
                    self.obj.Placement.Base=arc.Center
                    self.obj.Radius = arc.Radius
                    deltaX = p1[0]-self.obj.Placement.Base[0]
                    deltaY = p1[1]-self.obj.Placement.Base[1]
                    dangleF = math.degrees(math.atan2(deltaY,deltaX))
                    deltaX = p3[0]-self.obj.Placement.Base[0]
                    deltaY = p3[1]-self.obj.Placement.Base[1]
                    dangleL = math.degrees(math.atan2(deltaY,deltaX))
                    self.obj.FirstAngle = dangleF
                    self.obj.LastAngle = dangleL                    
                    self.updateCirclePts()

    def getArcMid(self):#Returns object midpoint
        if Draft.getType(self.obj) == "Circle":
            if self.obj.LastAngle>self.obj.FirstAngle:
                midAngle=math.radians(self.obj.FirstAngle+(self.obj.LastAngle-self.obj.FirstAngle)/2)
            else:
                midAngle=math.radians(self.obj.FirstAngle+(self.obj.LastAngle-self.obj.FirstAngle)/2)+math.pi
            midRadX=self.obj.Radius*math.cos(midAngle)
            midRadY=self.obj.Radius*math.sin(midAngle)
            deltaMid=FreeCAD.Vector(midRadX,midRadY,0)
            midPoint=(self.obj.Placement.Base+deltaMid)
            return(midPoint)
        elif Draft.getType(self.obj) == "Wall":
            if self.obj.Base.GeometryCount == 1:
                print("wall edit mode: get midpoint")
        else:
            print("Failed to get object midpoint during Editing")
            
    def arcInvert(self):
        FA=self.obj.FirstAngle
        self.obj.FirstAngle=self.obj.LastAngle
        self.obj.LastAngle=FA
        self.obj.recompute()
        self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        self.trackers[2].set(self.obj.Shape.Vertexes[1].Point)
        self.trackers[3].set(self.getArcMid())

    #---------------------------------------------------------------------------
    # EDIT OBJECT TOOLS : Polygon (maybe could also rotate the poligon)
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
            self.getPlacement(self.obj.Base)
        elif Draft.getType(self.obj.Base) == "Sketch":
            if self.obj.Base.GeometryCount == 1:
                self.editpoints.append(self.obj.Base.getPoint(0,1))
                self.editpoints.append(self.obj.Base.getPoint(0,2))
            self.getPlacement(self.obj.Base)
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