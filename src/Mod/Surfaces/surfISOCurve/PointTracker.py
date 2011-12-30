# ##### BEGIN GPL LICENSE BLOCK #####
#
#  Author: Jose Luis Cercos Pita <jlcercos@gmail.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

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
from surfUtils import Paths
from surfUtils import Geometry
from surfISOCurve import Preview

class PointTracker:
    def __init__(self,view,task):
        self.view=view
        self.task=task
        task.tracker = self
        self.callback = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.mouseButton)
        self.callbackMove = self.view.addEventCallbackPivy(SoLocation2Event.getClassTypeId(),self.mouseMove)
    
    def mouseMove(self, event_cb):
        event = event_cb.getEvent()
        surf = self.task.surf
        if not surf:
            return
        # Get screen point
        screen = event.getPosition()
        ctrl   = event.wasCtrlDown()
        # Get snapped object if exist, else the screen point will used
        point = Gui.Snapper.snap(screen, ctrl)

    def mouseButton(self, event_cb):
        event = event_cb.getEvent()
        if event.getState() != coin.SoMouseButtonEvent.DOWN:
            return
        surf = self.task.surf
        if not surf:
            return
        # Get screen point
        screen = event.getPosition()
        ctrl   = event.wasCtrlDown()
        # Get snapped object if exist, else the screen point will used
        point = Gui.Snapper.snap(screen, ctrl)
        if not point:
            point = self.view.getPoint(screen[0],screen[1])
        # Get the nearest surface point parameter
        uv = surf.parameter(point)
        uv = [uv[0], uv[1]]
        uv[0] = uv[0] / surf.UKnotSequence[len(surf.UKnotSequence)-1]
        uv[0] = max(0, min(uv[0], 1))
        uv[1] = uv[1] / surf.VKnotSequence[len(surf.VKnotSequence)-1]
        uv[1] = max(0, min(uv[1], 1))
        # Set it into the task panel
        self.task.setUV(uv[self.task.dirId])

    def close(self):
        # Switch off snapping
        Gui.Snapper.off()
        # Remove callback (Program crash otherwise)
        self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callback)
        self.view.removeEventCallbackPivy(SoLocation2Event.getClassTypeId(),self.callbackMove)
