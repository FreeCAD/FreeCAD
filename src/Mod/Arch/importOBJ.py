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

import FreeCAD, DraftGeomUtils

if open.__module__ == '__builtin__':
    pythonopen = open

def findVert(aVertex,aList):
    "finds aVertex in aList, returns index"
    for i in range(len(aList)):
        if (aVertex.X == aList[i].X) and (aVertex.Y == aList[i].Y) and (aVertex.Z == aList[i].Z):
            return i

def getIndices(shape,offset):
    "returns a list with 2 lists: vertices and face indexes, offsetted with the given amount"
    vlist = []
    flist = []
    for v in shape.Vertexes:
        vlist.append(" "+str(round(v.X,4))+" "+str(round(v.Z,4))+" "+str(round(v.Y,4)))
    for f in shape.Faces:
        fi = ""
        # OCC vertices are unsorted. We need to sort in the right order...
        edges = DraftGeomUtils.sortEdges(f.Wire.Edges)
        print edges
        for e in edges:
            print e.Vertexes[0].Point,e.Vertexes[1].Point
            v = e.Vertexes[0]
            fi+=" "+str(findVert(v,shape.Vertexes)+offset)
        flist.append(fi)
    return vlist,flist

def export(exportList,filename):
    "called when freecad exports a file"
    outfile = pythonopen(filename,"wb")
    ver = FreeCAD.Version()
    outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
    outfile.write("# http://free-cad.sf.net\n")
    offset = 1
    for obj in exportList:
        if obj.isDerivedFrom("Part::Feature"):
            vlist,flist = getIndices(obj.Shape,offset)
            offset += len(vlist)
            outfile.write("o " + obj.Name + "\n")
            for v in vlist:
                outfile.write("v" + v + "\n")
            for f in flist:
                outfile.write("f" + f + "\n")
    outfile.close()
    FreeCAD.Console.PrintMessage("successfully written "+filename)
            
            
            
