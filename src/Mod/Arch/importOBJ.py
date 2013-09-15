#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
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

import FreeCAD, DraftGeomUtils, Part, Draft
from DraftTools import translate

p = Draft.precision()

if open.__module__ == '__builtin__':
    pythonopen = open

def findVert(aVertex,aList):
    "finds aVertex in aList, returns index"
    for i in range(len(aList)):
        if ( round(aVertex.X,p) == round(aList[i].X,p) ):
            if ( round(aVertex.Y,p) == round(aList[i].Y,p) ):
                if ( round(aVertex.Z,p) == round(aList[i].Z,p) ):
                    return i

def getIndices(shape,offset):
    "returns a list with 2 lists: vertices and face indexes, offsetted with the given amount"
    vlist = []
    elist = []
    flist = []
    for v in shape.Vertexes:
        vlist.append(" "+str(round(v.X,p))+" "+str(round(v.Y,p))+" "+str(round(v.Z,p)))
    if not shape.Faces:
        for e in shape.Edges:
            if DraftGeomUtils.geomType(e) == "Line":
                ei = " " + str(findVert(e.Vertexes[0],shape.Vertexes) + offset)
                ei += " " + str(findVert(e.Vertexes[-1],shape.Vertexes) + offset)
                elist.append(ei)
    for f in shape.Faces:
        if len(f.Wires) > 1:
            # if we have holes, we triangulate
            tris = f.tessellate(1)
            for fdata in tris[1]:
                fi = ""
                for vi in fdata:
                    vdata = Part.Vertex(tris[0][vi])
                    fi += " " + str(findVert(vdata,shape.Vertexes) + offset)
                flist.append(fi)
        else:
            fi = ""
            # OCC vertices are unsorted. We need to sort in the right order...
            edges = DraftGeomUtils.sortEdges(f.Wire.Edges)
            #print edges
            for e in edges:
                #print e.Vertexes[0].Point,e.Vertexes[1].Point
                v = e.Vertexes[0]
                fi += " " + str(findVert(v,shape.Vertexes) + offset)
            flist.append(fi)
    return vlist,elist,flist

def export(exportList,filename):
    "called when freecad exports a file"
    outfile = pythonopen(filename,"wb")
    ver = FreeCAD.Version()
    outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
    outfile.write("# http://www.freecadweb.org\n")
    offset = 1
    for obj in exportList:
        if obj.isDerivedFrom("Part::Feature"):
            if obj.ViewObject.isVisible():
                vlist,elist,flist = getIndices(obj.Shape,offset)
                offset += len(vlist)
                outfile.write("o " + obj.Name + "\n")
                for v in vlist:
                    outfile.write("v" + v + "\n")
                for e in elist:
                    outfile.write("l" + e + "\n")
                for f in flist:
                    outfile.write("f" + f + "\n")
    outfile.close()
    FreeCAD.Console.PrintMessage(str(translate("Arch","successfully written "))+filename)
            
            
            
