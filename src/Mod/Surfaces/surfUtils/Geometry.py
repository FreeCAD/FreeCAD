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
# FreeCAD
import FreeCAD, FreeCADGui
from FreeCAD import Base
from FreeCAD import Part
# FreeCAD ship
from surfUtils import Math

def getSelectedObjs():
    """ Returns the selected objects list
    @return Selected objects list
    """
    return FreeCADGui.Selection.getSelection()

def getSelectedObj():
    """ Returns the first element of the selected objects list
    @return Selected object. None if errors happens
    """
    objs = FreeCADGui.Selection.getSelection()
    if not objs:
        return None
    if not len(objs):
        return None
    obj = objs[0]
    return FreeCAD.ActiveDocument.getObject(obj.Name)

def getEdges(objs=None):
    """ Returns object edges (list of them)
    @param objs Object to get the faces, none if selected
    object may used.
    @return Selected edges. None if errors happens
    """
    edges = []
    if not objs:
        objs = FreeCADGui.Selection.getSelection()
    if not objs:
        return None
    for i in range(0, len(objs)):
        obj = objs[i]
        if obj.isDerivedFrom('Part::Feature'):
            # get shape
            shape = obj.Shape
            if not shape:
                return None
            obj = shape
        if not obj.isDerivedFrom('Part::TopoShape'):
            return None
        objEdges = obj.Edges
        if not objEdges:
            continue
        for j in range(0, len(objEdges)):
            edges.append(objEdges[j])
    return edges

def getFaces(obj=None):
    """ Returns object faces (list of them)
    @param obj Object to get the faces, none if selected
    object may used.
    @return Selected faces. None if errors happens
    """
    if not obj:
        obj = getSelectedObj()
    if not obj:
        return None
    if obj.isDerivedFrom('Part::Feature'):
        # get shape
        shape = obj.Shape
        if not shape:
            return None
        obj = shape
    if not obj.isDerivedFrom('Part::TopoShape'):
        return None
    # get face
    faces = obj.Faces
    if not faces:
        return None
    if not len(faces):
        return None
    return faces

def getSelectedSurface(obj=None):
    """ Returns object surface (the first of the list)
    @param obj Object to get the surface, none if selected
    object may used.
    @return Selected surface. None if errors happens
    """
    faces = getFaces(obj)
    if not faces:
        return None
    obj = faces[0]
    # get surface
    surf = obj.Surface
    if not surf:
        return None
    return surf

def getBorders(objs=None):
    """ Returns the borders of all selected objects as edge array
    @param objs Objects to get the edges, none if selected objects
    may used.
    @return Borders
    """
    edges = []
    if not objs:
        objs = FreeCADGui.Selection.getSelection()
    if not objs:
        return None
    if len(objs) < 1:
        return None
    for i in range(0, len(objs)):
        faces = getFaces(objs[i])
        if not faces:
            continue
        for j in range(0, len(faces)):
            edgList = faces[j].Edges
            for k in range(0, len(edgList)):
                edges.append(edgList[k])
    return edges
        

def lineFaceSection(line,surface):
    """ Returns the point of section of a line with a face
    @param line Line object, that can be a curve.
    @param surface Surface object (must be a Part::Shape)
    @return Section points array, [] if line don't cut surface
    """
    # Get initial data
    result = []
    vertexes = line.Vertexes
    nVertex = len(vertexes)
    # Perform the cut
    section = line.cut(surface)
    # Filter all old points
    points = section.Vertexes
    nPoint = len(points)
    if nPoint <= nVertex:
        # Any valid point
        result
    for i in range(0,nPoint):
        disp = len(result)
        flag = 0
        if not Math.isAprox(points[i].X,vertexes[i-disp].X,0.0001):
            flag = flag+1
        if not Math.isAprox(points[i].Y,vertexes[i-disp].Y,0.0001):
            flag = flag+1
        if not Math.isAprox(points[i].Z,vertexes[i-disp].Z,0.0001):
            flag = flag+1
        if flag > 0:
            result.append(points[i])
    return result

