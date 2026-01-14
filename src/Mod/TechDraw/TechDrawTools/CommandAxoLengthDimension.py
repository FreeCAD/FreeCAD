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
Provides the TechDraw AxoLengthDimension GuiCommand.
00.01 2023/02/01 Basic version
00.02 2023/12/07 Calculate real 3D values if parallel to coordinate axis
"""

__title__ = "TechDrawTools.CommandAxoLengthDimension"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.02"
__date__ = "2023/12/07"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawTools.TDToolsUtil as Utils

import TechDraw
import math

# A hack to deal with garbage in the low bits causing wrong decisions. dimensionLineAngle is in radians.  If 
# dimensionLineAngle is 90deg +/- AngularTolerance, we treat it as a vertical dimension line.  This affects which
# side of the dimension line is used for the dimension text.
def makePlumb(dimensionLineAngle):
    HalfPi = math.pi / 2.0
    
    AngularTolerance = 0.01   # This value is a guess.  It works for the file in issue #16172, but
                              # seems small (~0.5deg).  

    if math.isclose(dimensionLineAngle, HalfPi, abs_tol=AngularTolerance):
        return HalfPi
    elif math.isclose(dimensionLineAngle, -HalfPi, abs_tol=AngularTolerance):
        return -HalfPie
 
    return dimensionLineAngle
    

class CommandAxoLengthDimension:
    """Creates a 3D length dimension."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_AxoLengthDimension.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_AxoLengthDimension", "Axonometric Length Dimension"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_AxoLengthDimension", "Creates a length dimension in with "
                            "axonometric view, using selected edges or vertex pairs to define direction and measurement")}

    def Activated(self):
        """Run the following code when the command is activated (button press)."""

        App.ActiveDocument.openTransaction("Create axonometric length dimension")
        vertexes = []
        edges = []

        if not Utils.getSelEdges(2):
            return
            
        edges = Utils.getSelEdges(2)
        vertexes = Utils.getSelVertexes(0)

        vertNames = list()
        edgeNames = list()
        if len(vertexes)<2:
            vertexes.append(edges[0].Vertexes[0])
            vertexes.append(edges[0].Vertexes[1])
            edgeNames = Utils.getSelEdgeNames(2)
        else:
            vertNames = Utils.getSelVertexNames(2)

        view = Utils.getSelView()
        page = view.findParentPage()
        scale = view.getScale()

        StartPt, EndPt = edges[1].Vertexes[0].Point, edges[1].Vertexes[1].Point
        extLineVec = EndPt.sub(StartPt)
        StartPt, EndPt = edges[0].Vertexes[0].Point, edges[0].Vertexes[1].Point
        dimLineVec = EndPt.sub(StartPt)

        xAxis = App.Vector(1,0,0)
        extAngle = math.degrees(extLineVec.getAngle(xAxis))
        lineAngle = math.degrees(makePlumb(dimLineVec.getAngle(xAxis)))

        if extLineVec.y < 0.0:
            extAngle = 180-extAngle
        if dimLineVec.y < 0.0:
            lineAngle = 180-lineAngle

        if abs(extAngle-lineAngle)>0.1:
            # re issue: https://github.com/FreeCAD/FreeCAD/issues/13677
            # Instead of using makeDistanceDim (which is meant for extent dimensions), we use the 
            # same steps as in CommandCreateDims.cpp to create a regular length dimension.  This avoids
            # the creation of CosmeticVertex objects to serve as dimension points.  These CosmeticVertex
            # objects are never deleted, but are no longer used once their dimension is deleted. 
            # distanceDim=TechDraw.makeDistanceDim(view,'Distance',vertexes[0].Point*scale,vertexes[1].Point*scale)
            distanceDim = view.Document.addObject("TechDraw::DrawViewDimension", "Dimension")
            distanceDim.Type = "Distance"
            distanceDim.MeasureType = "Projected"       #should this not be True?
            self.setReferences(distanceDim, view, edgeNames, vertNames)
            page.addView(distanceDim)

            distanceDim.AngleOverride = True
            distanceDim.LineAngle = lineAngle
            distanceDim.ExtensionAngle = extAngle

            distanceDim.recompute()     # ensure linearPoints has been set

            # as in CmdCreateDims::positionDimText:
            linearPoints = distanceDim.getLinearPoints()
            mid = (linearPoints[0] + linearPoints[1]) / 2
            distanceDim.X = mid.x
            distanceDim.Y = -mid.y

            (px,py,pz) = Utils.getCoordinateVectors(view)
            arrowTips = distanceDim.getArrowPositions()
            value2D = (arrowTips[1].sub(arrowTips[0])).Length
            value3D = 1.0
            if px.isParallel(dimLineVec,0.1):
                value3D = value2D/px.Length
            elif py.isParallel(dimLineVec,0.1):
                value3D = value2D/py.Length
            elif pz.isParallel(dimLineVec,0.1):
                value3D = value2D/pz.Length
            if value3D != 1.0:
                fomatted3DValue = self._formatValueToSpec(value3D,distanceDim.FormatSpec)
                distanceDim.Arbitrary = True
                distanceDim.Label = distanceDim.Label.replace('Dimension','Dimension3D')
                distanceDim.FormatSpec = fomatted3DValue

            distanceDim.recompute()
            view.requestPaint()

        Gui.Selection.clearSelection()
        App.ActiveDocument.commitTransaction()
        view.touch()	# make view claim its new child

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return Utils.havePage() and Utils.haveView()
        else:
            return False

    def _formatValueToSpec(self, value, formatSpec):
        '''Calculate value using "%.nf" or "%.nw" formatSpec'''
        formatSpec = '{'+formatSpec+'}'
        formatSpec = formatSpec.replace('%',':')
        if formatSpec.find('w') > 0:
            formatSpec = formatSpec.replace('w','f')
            numDig = formatSpec.find(":.")
            if numDig != -1:
                numDig = numDig+2
                charList = list(formatSpec)
                digits = int(charList[numDig])
                value = round(value,digits)
                strValue = formatSpec.format(value)
                strValueList = list(strValue)
                while strValueList[-1] == '0':
                    strValueList.pop()
                if strValueList[-1] == '.':
                    strValueList.pop()
                return ''.join(strValueList)
        else:
            return formatSpec.format(value)

    def setReferences(self, dimension, view, edgeNameList, vertexNameList):
        references = list()
        if vertexNameList:
            for vert in vertexNameList:
                references.append((view, vert))
        else:
            references.append((view, edgeNameList[0]))

        dimension.References2D = references

#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand('TechDraw_AxoLengthDimension', CommandAxoLengthDimension())
