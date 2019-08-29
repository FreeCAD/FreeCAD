#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Sebastian Hoogen <github@sebastianhoogen.de>       *
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

__title__="FreeCAD OpenSCAD Workbench - 2D helper functions"
__author__ = "Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

'''
This Script includes python functions to find out the most basic shape type
in a compound and to change the color of shapes according to their shape type
'''

import FreeCAD
def shapedict(shapelst):
    return dict([(shape.hashCode(),shape) for shape in shapelst])

def shapeset(shapelst):
    return set([shape.hashCode() for shape in shapelst])

def mostbasiccompound(comp):
    '''searches for the most basic shape in a Compound'''
    solids=shapeset(comp.Solids)
    shells=shapeset(comp.Shells)
    faces=shapeset(comp.Faces)
    wires=shapeset(comp.Wires)
    edges=shapeset(comp.Edges)
    vertexes=shapeset(comp.Vertexes)
    #FreeCAD.Console.PrintMessage('%s\n' % (str((len(solids),len(shells),len(faces),len(wires),len(edges),len(vertexes)))))
    for shape in comp.Solids:
        shells -= shapeset(shape.Shells)
        faces -= shapeset(shape.Faces)
        wires -= shapeset(shape.Wires)
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Shells:
        faces -= shapeset(shape.Faces)
        wires -= shapeset(shape.Wires)
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Faces:
        wires -= shapeset(shape.Wires)
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Wires:
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Edges:
        vertexes -= shapeset(shape.Vertexes)
    #FreeCAD.Console.PrintMessage('%s\n' % (str((len(solids),len(shells),len(faces),len(wires),len(edges),len(vertexes)))))
    #return len(solids),len(shells),len(faces),len(wires),len(edges),len(vertexes)
    if vertexes:
        return "Vertex"
    elif edges:
        return "Edge"
    elif wires:
        return "Wire"
    elif faces:
        return "Face"
    elif shells:
        return "Shell"
    elif solids:
        return "Solid"

def colorcodeshapes(objs):
    shapecolors={
        "Compound":(0.3,0.3,0.4),
        "CompSolid":(0.1,0.5,0.0),
        "Solid":(0.0,0.8,0.0),
        "Shell":(0.8,0.0,0.0),
        "Face":(0.6,0.6,0.0),
        "Wire":(0.1,0.1,0.1),
        "Edge":(1.0,1.0,1.0),
        "Vertex":(8.0,8.0,8.0),
        "Shape":(0.0,0.0,1.0),
        None:(0.0,0.0,0.0)}

    for obj in objs:
        if hasattr(obj,'Shape'):
            try:
                if obj.Shape.isNull():
                    continue
                if not obj.Shape.isValid():
                        color=(1.0,0.4,0.4)
                else:
                    st=obj.Shape.ShapeType
                    if st in ["Compound","CompSolid"]:
                        st = mostbasiccompound(obj.Shape)
                    color=shapecolors[st]
                obj.ViewObject.ShapeColor = color
            except:
                raise

#colorcodeshapes(App.ActiveDocument.Objects)
