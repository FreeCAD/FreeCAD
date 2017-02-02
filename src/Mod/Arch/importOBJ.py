#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *  
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

import FreeCAD, DraftGeomUtils, Part, Draft, Arch, Mesh
if FreeCAD.GuiUp:
    from DraftTools import translate
else:
    # \cond
    def translate(context,text):
        return text
    # \endcond

## @package importOBJ
#  \ingroup ARCH
#  \brief OBJ file format importer and exporter
#
#  This module provides tools to import and export OBJ files.
#  It is an alternative tothe standard Mesh OBJ importer/exporter
#  and supports exporting faces with more than 3 vertices.

p = Draft.precision()

if open.__module__ in ['__builtin__','io']:
    pythonopen = open

def findVert(aVertex,aList):
    "finds aVertex in aList, returns index"
    for i in range(len(aList)):
        if ( round(aVertex.X,p) == round(aList[i].X,p) ):
            if ( round(aVertex.Y,p) == round(aList[i].Y,p) ):
                if ( round(aVertex.Z,p) == round(aList[i].Z,p) ):
                    return i
    return None

def getIndices(shape,offset):
    "returns a list with 2 lists: vertices and face indexes, offsetted with the given amount"
    vlist = []
    elist = []
    flist = []
    curves = None
    if isinstance(shape,Part.Shape):
        for e in shape.Edges:
            try:
                if not isinstance(e.Curve,Part.LineSegment):
                    if not curves:
                        curves = shape.tessellate(1)
                        FreeCAD.Console.PrintWarning(translate("Arch","Found a shape containing curves, triangulating\n").decode('utf8'))
                        break
            except: # unimplemented curve type
                curves = shape.tessellate(1)
                FreeCAD.Console.PrintWarning(translate("Arch","Found a shape containing curves, triangulating\n").decode('utf8'))
                break
    elif isinstance(shape,Mesh.Mesh):
        curves = shape.Topology
    if curves:
        for v in curves[0]:
            vlist.append(" "+str(round(v.x,p))+" "+str(round(v.y,p))+" "+str(round(v.z,p)))
        for f in curves[1]:
            fi = ""
            for vi in f:
                fi += " " + str(vi + offset)
            flist.append(fi)
    else:
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
                for e in f.OuterWire.OrderedEdges:
                    #print(e.Vertexes[0].Point,e.Vertexes[1].Point)
                    v = e.Vertexes[0]
                    ind = findVert(v,shape.Vertexes)
                    if ind == None:
                        return None,None,None
                    fi += " " + str(ind + offset)
                flist.append(fi)
    return vlist,elist,flist

def export(exportList,filename):
    "called when freecad exports a file"
    outfile = pythonopen(filename,"wb")
    ver = FreeCAD.Version()
    outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
    outfile.write("# http://www.freecadweb.org\n")
    offset = 1
    objectslist = Draft.getGroupContents(exportList,walls=True,addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist)
    for obj in objectslist:
        if obj.isDerivedFrom("Part::Feature"):
            mesh = None
            if FreeCAD.GuiUp:
                visible = obj.ViewObject.isVisible()
                if obj.ViewObject.DisplayMode == "Mesh":
                    if hasattr(obj,"Mesh"):
                        if obj.Mesh:
                            mesh = obj.Mesh.Mesh.copy()
                            mesh.Placement = obj.Placement.multiply(obj.Mesh.Mesh.Placement)
                    if not mesh:
                        if hasattr(obj,"CloneOf"):
                            if obj.CloneOf:
                                if hasattr(obj.CloneOf,"Mesh"):
                                    if obj.CloneOf.Mesh:
                                        mesh = obj.CloneOf.Mesh.Mesh.copy()
                                        mesh.Placement = obj.Placement.multiply(obj.CloneOf.Placement).multiply(obj.CloneOf.Mesh.Mesh.Placement)
            else:
                visible = True
            if visible:
                if mesh:
                    vlist,elist,flist = getIndices(mesh,offset)
                else:
                    vlist,elist,flist = getIndices(obj.Shape,offset)
                if vlist == None:
                    FreeCAD.Console.PrintError("Unable to export object "+obj.Label+". Skipping.\n")
                else:
                    offset += len(vlist)
                    outfile.write("o " + obj.Name + "\n")
                    for v in vlist:
                        outfile.write("v" + v + "\n")
                    for e in elist:
                        outfile.write("l" + e + "\n")
                    for f in flist:
                        outfile.write("f" + f + "\n")
    outfile.close()
    FreeCAD.Console.PrintMessage(translate("Arch","successfully written ").decode('utf8')+filename+"\n")
            
            
            
