#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
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

import math
# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
import Draft
from FreeCAD import Vector
# Qt library
from PyQt4 import QtGui,QtCore
# Pivy
import pivy
from pivy import coin
from pivy.coin import *
# Module
from surfUtils import Paths, Geometry, Math

class PointTracker:
    def __init__(self,view,task):
        self.view=view
        self.task=task
        task.tracker = self
        self.callback = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.mouseButton)
        self.callbackMove = self.view.addEventCallbackPivy(SoLocation2Event.getClassTypeId(),self.mouseMove)
        # Drag and drop functionality
        self.isMouseDown = False
        self.isDrag      = False
        self.screen      = [0,0]
        self.ctrl        = False
        self.point       = App.Base.Vector(0.0,0.0,0.0)
        self.threshold   = 16.0
        self.sel         = []
    
    def mouseMove(self, event_cb):
        event = event_cb.getEvent()
        # Get screen point
        screen = event.getPosition()
        ctrl   = event.wasCtrlDown()
        # Special because when new selection is performed, mouse button up event is not generated
        if self.isMouseDown:
            if self.hasSelectionChange():
                if self.isDrag:
                    # Get 3D point (snapped)
                    point = Gui.Snapper.snap(screen, ctrl)
                    if not point:
                        point = self.view.getPoint(screen[0],screen[1])
                    # Translate points
                    self.movePoints(point - self.point)
                self.isMouseDown=False
                Gui.Snapper.off()
                return
        # Take a look if we are on drag operation
        if not self.isOnDrag(screen):
            return
        # Get 3D point (snapped)
        point = Gui.Snapper.snap(screen, ctrl)
        if not point:
            point = self.view.getPoint(screen[0],screen[1])
        # Translate points
        self.movePoints(point - self.point)
        
    def mouseButton(self, event_cb):
        event = event_cb.getEvent()
        # Get screen point
        screen = event.getPosition()
        ctrl   = event.wasCtrlDown()
        if event.getState() == coin.SoMouseButtonEvent.DOWN:
            self.isMouseDown = True
            self.screen      = screen
            self.ctrl        = ctrl
            self.sel         = Gui.Selection.getSelectionEx()[:]
            return
        if event.getState() == coin.SoMouseButtonEvent.UP:
            self.isMouseDown = False
            Gui.Snapper.off()
            # Get 3D point (snapped)
            point = Gui.Snapper.snap(screen, ctrl)
            if not point:
                point = self.view.getPoint(screen[0],screen[1])
            # Translate points
            self.movePoints(point - self.point)
            return

    def isOnDrag(self, screen):
        """ Get if we are involved into a drag process.
        @param Mouse screen position
        @return True if drag and drop process is registered.
        """
        # Specific cases
        if not self.isMouseDown:
            self.isDrag = False
            return False
        if self.isDrag:
            return True
        # Study if valid object has been selected
        flag = False
        objs = Gui.Selection.getSelection()
        for i in range(0,len(objs)):
            obj = objs[i]
            props = obj.PropertiesList
            try:
                props.index("ValidCtrlPoints")
            except ValueError:
                continue
            if not obj.ValidCtrlPoints:
                continue
            flag = True
            break
        if not flag:
            return False
        # Study if we are starting to drag
        dX = float(self.screen[0] - screen[0])
        dY = float(self.screen[1] - screen[1])
        if self.threshold*self.threshold < dX*dX + dY*dY:
            self.isDrag = True
            self.point  = Gui.Snapper.snap(self.screen, self.ctrl)
            Gui.Snapper.off()
            return True

    def hasSelectionChange(self):
        """ Gets if selected object has been changed
        @return True if selection has been changed
        """
        sel = Gui.Selection.getSelectionEx()[:]
        if(len(sel) != len(self.sel)):
            return True
        for i in range(0,len(sel)):
            selobj1 = sel[i]
            selobj2 = self.sel[i]
            if selobj1.ObjectName != selobj2.ObjectName:
                return True
            sub1 = selobj1.SubElementNames
            sub2 = selobj2.SubElementNames
            if len(sub1) != len(sub2):
                return True
            for j in range(0,len(sub1)):
                if sub1[j] != sub2[j]:
                    return True
        return False

    def movePoints(self,vec):
        """ Move selected control points.
        @param vec Translation vector (App.Base.Vector type expected).
        @param perform True if objects must be completely renewed, building new edges and modifying surface.
        """
        # Get selection objects
        sel = Gui.Selection.getSelectionEx()[:]
        for i in range(0,len(sel)):
            selobj = sel[i]
            # Ensure that is a control points selection
            obj =  App.ActiveDocument.getObject(selobj.ObjectName)
            props = obj.PropertiesList
            try:
                props.index("ValidCtrlPoints")
            except ValueError:
                continue
            if not obj.ValidCtrlPoints:
                continue
            # Get selected vertexes
            subs  = selobj.SubObjects
            selList = []
            for j in range(0,len(subs)):
                sub = subs[j]
                for k in range(0,len(sub.Vertexes)):
                    selList.append(sub.Vertexes[0])
            # Translate selected vertexes
            nU = obj.nU
            nV = obj.nV
            n = nU*nV
            vList = obj.Shape.Vertexes[0:n]
            for i in range(0,n):
                for j in range(0,len(selList)):
                    if Math.isSameVertex(vList[i],selList[j]):
                        dX = vList[i].X - self.point.x
                        dY = vList[i].Y - self.point.y
                        dZ = vList[i].Z - self.point.z
                        vList[i].translate(vec - App.Base.Vector(dX,dY,dZ))
                        break
            # Call to modify the object
            obj.Proxy.movedVertexes(obj,vList)

    def close(self):
        # Switch off snapping
        Gui.Snapper.off()
        # Remove callback (Program crash otherwise)
        self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callback)
        self.view.removeEventCallbackPivy(SoLocation2Event.getClassTypeId(),self.callbackMove)
