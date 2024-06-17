# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>                                *
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
"""
Provides the TechDraw PositionSectionView GuiCommand.
00.01 2021/03/17 C++ Basic version
00.02 2023/12/21 Option to select an edge and it's corresponding vertex
"""

__title__ = "TechDrawTools.CommandPositionSectionView"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.02"
__date__ = "2023/12/21"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawTools.TDToolsUtil as Utils

class CommandPositionSectionView:
    """Orthogonally align a section view with its source view."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'TechDraw_ExtensionPositionSectionView.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_PositionSectionView", "Position Section View"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_PositionSectionView", 
                  "Orthogonally align a section view with its source view:<br>\
                - Select a single section view<br>\
                - Click this tool<br>\
                - optional: select one edge in the section view and it's corresponding vertex in the base view<br>\
                  Click this tool")}

    def Activated(self):
        """Run the following code when the command is activated (button pressed)."""
        selection = Gui.Selection.getSelectionEx()
        if not selection:
            return
        if len(selection) == 1:
            if Utils.getSelView():
                sectionView = Utils.getSelView()
                if sectionView.TypeId == 'TechDraw::DrawViewSection':
                    baseView = sectionView.BaseView
                    if baseView.TypeId == "TechDraw::DrawProjGroupItem":
                        baseView = baseView.InList[0]
                    basePoint = App.Vector(baseView.X,baseView.Y,0.0)
                    sectionPoint = App.Vector(sectionView.X,sectionView.Y,0.0)
                    moveVector = sectionPoint.sub(basePoint)
                    if abs(moveVector.x) > abs(moveVector.y):
                        moveVector.x = 0.0
                    else:
                        moveVector.y = 0.0
                else:
                    return

        elif len(selection) == 2:
            if Utils.getSelEdges() and Utils.getSelVertexes(1,1):
                sectionEdge = Utils.getSelEdges()
                sectionDir = sectionEdge[0].Curve.Direction
                sectionSel = sectionEdge[0].Vertexes
                baseSel = Utils.getSelVertexes(1,1)
                sectionView = Utils.getSelView(0)
                baseView = Utils.getSelView(1)
                if baseView.TypeId == "TechDraw::DrawProjGroupItem":
                    baseView = baseView.InList[0]

                basePoint = baseSel[0].Point
                sectionPoint = sectionSel[0].Point
                Scale = baseView.Scale
                centerBase = App.Vector(baseView.X,baseView.Y,0)
                basePoint = centerBase+basePoint*Scale
                centerSection = App.Vector(sectionView.X,sectionView.Y,0)
                sectionPoint = centerSection+sectionPoint*Scale
                sectionPoint = self.getTrianglePoint(sectionPoint,sectionDir,basePoint)
                moveVector = sectionPoint.sub(basePoint)
            else:
                return

        sectionView.X = sectionView.X.Value-moveVector.x
        sectionView.Y = sectionView.Y.Value-moveVector.y

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return Utils.havePage() and Utils.haveView()
        else:
            return False

    def getTrianglePoint(self,p1,dir,p2):
        '''
        Get third point of a perpendicular triangle
        p1, p2 ...vertexes of hypothenusis, dir ...direction of one kathete, p3 ...3rd vertex
        '''
        a = -dir.y
        b = dir.x
        c1 = p1.x * a + p1.y * b
        c2 = -p2.x * b + p2.y * a
        ab = a * a + b * b
        x = (c1 * a - c2 * b) / ab
        y = (c2 * a + c1 * b) / ab
        return App.Vector(x,y,0.0)

# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_ExtensionPositionSectionView', CommandPositionSectionView())

