# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>               *
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
"""Provides the TechDraw AxoLengthDimension GuiCommand."""

__title__ = "TechDrawTools.CommandAxoLengthDimension"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/02/01"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawTools

import TechDraw
from math import degrees

class CommandAxoLengthDimension:
    """Creates a 3D length dimension."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_AxoLengthDimension.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_AxoLengthDimension", "Axonometric length dimension"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_AxoLengthDimension", "Create an axonometric length dimension<br>\
                - select first edge to define direction and length of the dimension line<br>\
                - select second edge to define the direction of the extension lines<br>\
                - optional: select two more vertexes which define the measurement instead of the length<br>\
                  of the first selected edge")}

    def Activated(self):
        """Run the following code when the command is activated (button press)."""
        if  self.selectionTest():
            (edges,vertexes) = self.selectionTest()
            view = Gui.Selection.getSelection()[0]
            StartPt, EndPt = edges[1].Vertexes[0].Point, edges[1].Vertexes[1].Point
            extLineVec = EndPt.sub(StartPt)
            StartPt, EndPt = edges[0].Vertexes[0].Point, edges[0].Vertexes[1].Point
            dimLineVec = EndPt.sub(StartPt)
            xAxis = App.Vector(1,0,0)
            extAngle = degrees(extLineVec.getAngle(xAxis))
            lineAngle = degrees(dimLineVec.getAngle(xAxis))
            if extLineVec.y < 0.0:
                extAngle = 180-extAngle
            if dimLineVec.y < 0.0:
               lineAngle = 180-lineAngle
            if abs(extAngle-lineAngle)>0.1:
                distanceDim=TechDraw.makeDistanceDim(view,'Distance',vertexes[0].Point,vertexes[1].Point)
                distanceDim.AngleOverride = True
                distanceDim.LineAngle = lineAngle
                distanceDim.ExtensionAngle = extAngle
                distanceDim.X = (vertexes[0].Point.x+vertexes[1].Point.x)/2
                distanceDim.Y = (vertexes[0].Point.y+vertexes[1].Point.y)/2
                distanceDim.recompute()
                view.requestPaint()
            Gui.Selection.clearSelection()

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return TechDrawTools.TDToolsUtil.havePage() and TechDrawTools.TDToolsUtil.haveView()
        else:
            return False

    def selectionTest(self):
        '''test correct selection'''
        if not Gui.Selection.getSelection():
            return False
        view = Gui.Selection.getSelection()[0]

        if not Gui.Selection.getSelectionEx():
            return False
        objectList = Gui.Selection.getSelectionEx()[0].SubElementNames

        if not len(objectList)>=2:
            return False

        edges = []
        vertexes = []
        for objectString in objectList:
            if objectString[0:4] == 'Edge':
                edges.append(view.getEdgeBySelection(objectString))
            elif objectString[0:6] == 'Vertex':
                vertexes.append(view.getVertexBySelection(objectString))
        if not len(edges) >= 2:
            return False

        if len(vertexes)<2:
            vertexes = []
            vertexes.append(edges[0].Vertexes[0])
            vertexes.append(edges[0].Vertexes[1])
        return(edges,vertexes)


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_AxoLengthDimension', CommandAxoLengthDimension())

