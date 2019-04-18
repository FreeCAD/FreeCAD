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

import FreeCAD, DraftGeomUtils, Part, Draft, Arch, Mesh, os, sys
if FreeCAD.GuiUp:
    from DraftTools import translate
else:
    # \cond
    def translate(context,text):
        return text
    # \endcond

## @package importOBJ
#  \ingroup ARCH
#  \brief OBJ file format exporter
#
#  This module provides tools to import & export OBJ files.
#  It is an alternative to the standard Mesh OBJ exporter
#  and supports exporting faces with more than 3 vertices
#  and supports object colors / materials

p = Draft.precision()

if open.__module__ in ['__builtin__','io']:
    pythonopen = open

def decode(txt):

    if sys.version_info.major < 3:
        if isinstance(txt,unicode):
            return txt.encode("utf8")
    return txt

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
                        FreeCAD.Console.PrintWarning(translate("Arch","Found a shape containing curves, triangulating")+"\n")
                        break
            except: # unimplemented curve type
                curves = shape.tessellate(1)
                FreeCAD.Console.PrintWarning(translate("Arch","Found a shape containing curves, triangulating")+"\n")
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
    import codecs
    outfile = codecs.open(filename,"wb",encoding="utf8")
    ver = FreeCAD.Version()
    outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
    outfile.write("# http://www.freecadweb.org\n")
    offset = 1
    objectslist = Draft.getGroupContents(exportList,walls=True,addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist)
    filenamemtl = filename[:-4] + ".mtl"
    materials = []
    outfile.write("mtllib " + os.path.basename(filenamemtl) + "\n")
    for obj in objectslist:
        if obj.isDerivedFrom("Part::Feature"):
            hires = None
            if FreeCAD.GuiUp:
                visible = obj.ViewObject.isVisible()
                if obj.ViewObject.DisplayMode == "HiRes":
                    # check if high-resolution object is available
                    if hasattr(obj,"HiRes"):
                        if obj.HiRes:
                            if obj.HiRes.isDerivedFrom("Mesh::Feature"):
                                m = obj.HiRes.Mesh
                            else:
                                m = obj.HiRes.Shape
                            hires = m.copy()
                            hires.Placement = obj.Placement.multiply(m.Placement)
                    if not hires:
                        if hasattr(obj,"CloneOf"):
                            if obj.CloneOf:
                                if hasattr(obj.CloneOf,"HiRes"):
                                    if obj.CloneOf.HiRes:
                                        if obj.CloneOf.HiRes.isDerivedFrom("Mesh::Feature"):
                                            m = obj.CloneOf.HiRes.Mesh
                                        else:
                                            m = obj.CloneOf.HiRes.Shape
                                        hires = m.copy()
                                        hires.Placement = obj.Placement.multiply(obj.CloneOf.Placement).multiply(m.Placement)
            else:
                visible = True
            if visible:
                if hires:
                    vlist,elist,flist = getIndices(hires,offset)
                else:
                    vlist,elist,flist = getIndices(obj.Shape,offset)
                if vlist == None:
                    FreeCAD.Console.PrintError("Unable to export object "+obj.Label+". Skipping.\n")
                else:
                    offset += len(vlist)
                    outfile.write("o " + obj.Name + "\n")

                    # write material
                    m = False
                    if hasattr(obj,"Material"):
                        if obj.Material:
                            if hasattr(obj.Material,"Material"):
                                outfile.write("usemtl " + obj.Material.Name + "\n")
                                materials.append(obj.Material)
                                m = True
                    if not m:
                        if FreeCAD.GuiUp:
                            if hasattr(obj.ViewObject,"ShapeColor") and hasattr(obj.ViewObject,"Transparency"):
                                mn = Draft.getrgb(obj.ViewObject.ShapeColor)[1:]
                                outfile.write("usemtl color_" + mn + "\n")
                                materials.append(("color_" + mn,obj.ViewObject.ShapeColor,obj.ViewObject.Transparency))

                    # write geometry
                    for v in vlist:
                        outfile.write("v" + v + "\n")
                    for e in elist:
                        outfile.write("l" + e + "\n")
                    for f in flist:
                        outfile.write("f" + f + "\n")
    outfile.close()
    FreeCAD.Console.PrintMessage(translate("Arch","Successfully written") + " " + decode(filename) + "\n")
    if materials: 
        outfile = pythonopen(filenamemtl,"w")
        outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
        outfile.write("# http://www.freecadweb.org\n")
        kinds = {"AmbientColor":"Ka ","DiffuseColor":"Kd ","SpecularColor":"Ks ","EmissiveColor":"Ke ","Transparency":"d "}
        done = [] # store names to avoid duplicates
        for mat in materials:
            if isinstance(mat,tuple):
                if not mat[0] in done:
                    outfile.write("newmtl " + mat[0] + "\n")
                    outfile.write("Kd " + str(mat[1][0]) + " " + str(mat[1][1]) + " " + str(mat[1][2]) + "\n")
                    outfile.write("d " + str(mat[2]) + "\n")
                    done.append(mat[0])
            else:
                if not mat.Name in done:
                    outfile.write("newmtl " + mat.Name + "\n")
                    for prop in kinds:
                        if prop in mat.Material:
                            outfile.write(kinds[prop] + mat.Material[prop].strip("()").replace(',',' ') + "\n")
                    done.append(mat.Name)
        outfile.write("# Material Count: " + str(len(materials)))
        outfile.close()
        FreeCAD.Console.PrintMessage(translate("Arch","Successfully written") + ' ' + decode(filenamemtl) + "\n")


#def decode(name):
#    "decodes encoded strings"
#    try:
#        decodedName = (name.decode("utf8"))
#    except UnicodeDecodeError:
#        try:
#            decodedName = (name.decode("latin1"))
#        except UnicodeDecodeError:
#            FreeCAD.Console.PrintError(translate("Arch","Error: Couldn't determine character encoding"))
#            decodedName = name
#    return decodedName

def open(filename):
    "called when freecad wants to open a file"
    docname = (os.path.splitext(os.path.basename(filename))[0])
    doc = FreeCAD.newDocument(docname.encode("utf8"))
    doc.Label = docname
    return insert(filename,doc.Name)

def insert(filename,docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    with pythonopen(filename,"r") as infile:
        verts = []
        facets = []
        activeobject = None
        material = None
        colortable = {}
        for line in infile:
            line = line.strip()
            if line[:7] == "mtllib ":
                matlib = os.path.join(os.path.dirname(filename),line[7:])
                if os.path.exists(matlib):
                    with pythonopen(matlib,"r") as matfile:
                        mname = None
                        color = None
                        trans = None
                        for mline in matfile:
                            mline = mline.strip()
                            if mline[:7] == "newmtl ":
                                if mname and color:
                                    colortable[mname] = [color,trans]
                                color = None
                                trans = None
                                mname = mline[7:]
                            elif mline[:3] == "Kd ":
                                color = tuple([float(i) for i in mline[3:].split()])
                            elif mline[:2] == "d ":
                                trans = int(float(mline[2:])*100)
                        if mname and color:
                            colortable[mname] = [color,trans]
            elif line[:2] == "o ":
                if activeobject:
                    makeMesh(doc,activeobject,verts,facets,material,colortable)
                material = None
                facets = []
                activeobject = line[2:]
            elif line[:2] == "v ":
                verts.append([float(i) for i in line[2:].split()])
            elif line[:2] == "f ":
                fa = []
                for i in line[2:].split():
                    if "/" in i:
                        i = i.split("/")[0]
                    fa.append(int(i))
                facets.append(fa)
            elif line[:7] == "usemtl ":
                material = line[7:]
        if activeobject:
            makeMesh(doc,activeobject,verts,facets,material,colortable)
    FreeCAD.Console.PrintMessage(translate("Arch","Successfully imported") + ' ' + decode(filename) + "\n")
    return doc

def makeMesh(doc,activeobject,verts,facets,material,colortable):
    mfacets = []
    if facets:
        for facet in facets:
            if len(facet) > 3:
                vecs = [FreeCAD.Vector(*verts[i-1]) for i in facet]
                vecs.append(vecs[0])
                pol = Part.makePolygon(vecs)
                try:
                    face = Part.Face(pol)
                except Part.OCCError:
                    print("Skipping non-planar polygon:",vecs)
                else:
                    tris = face.tessellate(1)
                    for tri in tris[1]:
                        mfacets.append([tris[0][i] for i in tri])
            else:
                mfacets.append([verts[i-1] for i in facet])
    if mfacets:
        mobj = doc.addObject("Mesh::Feature",activeobject)
        mobj.Mesh = Mesh.Mesh(mfacets)
        if material and FreeCAD.GuiUp:
            if material in colortable:
                mobj.ViewObject.ShapeColor = colortable[material][0]
                if colortable[material][1] != None:
                    mobj.ViewObject.Transparency = colortable[material][1]
