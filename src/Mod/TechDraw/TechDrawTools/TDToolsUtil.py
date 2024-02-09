# ***************************************************************************
# *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
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
"""Provides utility functions for TD Tools."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtGui
from PySide.QtGui import QMessageBox

def havePage():
    objs = App.ActiveDocument.Objects
    for o in objs:
        if o.isDerivedFrom("TechDraw::DrawPage"):
            return True
    return False

def haveView():
    objs = App.ActiveDocument.Objects
    for o in objs:
        if o.isDerivedFrom("TechDraw::DrawView"):
            return True
    return False

def displayMessage(title,message):
    '''
    displayMessage('Title','Messagetext')
    '''
    msgBox = QtGui.QMessageBox()
    msgBox.setText(message)
    msgBox.setWindowTitle(title)
    msgBox.exec_()


def getSelView(nSel=0):
    '''
    view = getSelView()
    nSel=0 ... number of selected view, 0 = first selected
    Return selected view, otherwise return False
    '''
    if not Gui.Selection.getSelection():
        displayMessage('TechDraw_Utils','No view selected')
    else:
        view = Gui.Selection.getSelection()[nSel]
        return view

def getSelVertexes(nVertex=1, nSel=0):
    '''
    vertexes = getSelVertexes(nVertex)
    nVertex=1 ... min. number of selected vertexes
    nSel=0 ... number of selected view, 0 = first selected
    Return a list of selected vertexes if at least nVertex vertexes are selected, otherwise return False
    '''
    if getSelView(nSel):
        view = getSelView(nSel)
    else:
        return False
    if not Gui.Selection.getSelectionEx():
        displayMessage('TechDraw_Utils',
                        QT_TRANSLATE_NOOP('TechDraw_Utils','No vertex selected'))
        return False
    objectList = Gui.Selection.getSelectionEx()[nSel].SubElementNames

    vertexes = []
    for objectString in objectList:
        if objectString[0:6] == 'Vertex':
            vertexes.append(view.getVertexBySelection(objectString))

    if (len(vertexes) < nVertex):
        displayMessage('TechDraw_Utils',
                        QT_TRANSLATE_NOOP('TechDraw_Utils','Select at least ')+
                        str(nVertex)+
                        QT_TRANSLATE_NOOP('TechDraw_Utils',' vertexes'))
        return False
    else:
        return vertexes

def getSelEdges(nEdge=1, nSel=0):
    '''
    edges = getSelEdges(nEdge)
    nEdge=1 ... min. number of selected edges
    nSel=0 ... number of selected view, 0 = first selected
    Return a list of selected edges if at least nedge edges are selected, otherwise return False
    '''
    if getSelView(nSel):
        view = getSelView(nSel)
    else:
        return False
    if not Gui.Selection.getSelectionEx():
        displayMessage('TechDraw_Utils',
                        QT_TRANSLATE_NOOP('TechDraw_Utils','No edge selected'))
        return False
    objectList = Gui.Selection.getSelectionEx()[nSel].SubElementNames

    edges = []
    for objectString in objectList:
        if objectString[0:4] == 'Edge':
            edges.append(view.getEdgeBySelection(objectString))

    if (len(edges) < nEdge):
        displayMessage('TechDraw_Utils',
                        QT_TRANSLATE_NOOP('TechDraw_Utils','Select at least ')+
                        str(nEdge)+
                        QT_TRANSLATE_NOOP('TechDraw_Utils',' edges'))
        return False
    else:
        return edges

def getCoordinateVectors(view):
    '''
    (px,py,pz) = getCoordinateVectors(view)
    view ... selected view
    (px,py,pz) ... returned tuple of vectors (App.Vector)
    calculate projected vectors of x-, y- and z-axis
    '''
    diagonal = view.Direction
    xDirection = view.XDirection
    origin = App.Vector()
    xAxisProj = App.Vector(1,0,0).projectToPlane(origin,diagonal)
    yAxisProj = App.Vector(0,1,0).projectToPlane(origin,diagonal)
    zAxisProj = App.Vector(0,0,1).projectToPlane(origin,diagonal)
    # create wire polygon and roll back in 3D
    wire3D = Part.makePolygon([xAxisProj,yAxisProj,zAxisProj,xDirection,xAxisProj])
    rotation3D = App.Rotation(diagonal,App.Vector(0,0,1))
    wire3D.Placement.Rotation = rotation3D
    px = wire3D.Edges[0].Vertexes[0].Point
    py = wire3D.Edges[1].Vertexes[0].Point
    pz = wire3D.Edges[2].Vertexes[0].Point
    pDir = wire3D.Edges[3].Vertexes[0].Point
    pDir.z = 0.0
    # rotate inside drawing plane
    wire2D = Part.makePolygon([px,py,pz,px])
    rotation2D = App.Rotation(pDir,App.Vector(1,0,0))
    wire2D.Placement.Rotation = rotation2D
    px = wire2D.Edges[0].Vertexes[0].Point
    py = wire2D.Edges[1].Vertexes[0].Point
    pz = wire2D.Edges[2].Vertexes[0].Point
    return (px,py,pz)
