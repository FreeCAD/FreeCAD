#***************************************************************************
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

import os
import codecs
import ntpath
from builtins import open as pyopen

import FreeCAD
import Arch
import Draft
import DraftGeomUtils
import Mesh
import MeshPart
import Part
from draftutils import params

if FreeCAD.GuiUp:
    from draftutils.translate import translate
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

def findVert(aVertex,aList):
    "finds aVertex in aList, returns index"
    p = Draft.precision()
    for i in range(len(aList)):
        if round(aVertex.X,p) == round(aList[i].X,p):
            if round(aVertex.Y,p) == round(aList[i].Y,p):
                if round(aVertex.Z,p) == round(aList[i].Z,p):
                    return i
    return None

def getIndices(obj,shape,offsetv,offsetvn):
    "returns a list with 2 lists: vertices and face indexes, offset with the given amount"
    p = Draft.precision()
    vlist = []
    vnlist = []
    elist = []
    flist = []
    hascurve = False
    mesh = None

    if isinstance(shape,Part.Shape):
        for e in shape.Edges:
            try:
                if not isinstance(e.Curve,Part.Line):
                    hascurve = True
            except Exception: # unimplemented curve type
                hascurve = True
            if hascurve:
                tol = params.get_param("MaxDeviationExport",path="Mod/Mesh")
                mesh = Mesh.Mesh()
                mesh.addFacets(shape.getFaces(tol))
                FreeCAD.Console.PrintWarning(translate("Arch","Found a shape containing curves, triangulating")+"\n")
                break
    elif isinstance(shape,Mesh.Mesh):
        mesh = shape

    if mesh:
        for v in mesh.Topology[0]:
            vlist.append(" "+str(round(v[0],p))+" "+str(round(v[1],p))+" "+str(round(v[2],p)))

        for vn in mesh.Facets:
            vnlist.append(" "+str(round(vn.Normal[0],p))+" "+str(round(vn.Normal[1],p))+" "+str(round(vn.Normal[2],p)))

        for i, vn in enumerate(mesh.Topology[1]):
            flist.append(" "+str(vn[0]+offsetv)+"//"+str(i+offsetvn)+" "+str(vn[1]+offsetv)+"//"+str(i+offsetvn)+" "+str(vn[2]+offsetv)+"//"+str(i+offsetvn)+" ")
    else:
        for v in shape.Vertexes:
            vlist.append(" "+str(round(v.X,p))+" "+str(round(v.Y,p))+" "+str(round(v.Z,p)))
        if not shape.Faces:
            for e in shape.Edges:
                if DraftGeomUtils.geomType(e) == "Line":
                    ei = " " + str(findVert(e.Vertexes[0],shape.Vertexes) + offsetv)
                    ei += " " + str(findVert(e.Vertexes[-1],shape.Vertexes) + offsetv)
                    elist.append(ei)
        for f in shape.Faces:
            if len(f.Wires) > 1:
                # if we have holes, we triangulate
                tris = f.tessellate(1)
                for fdata in tris[1]:
                    fi = ""
                    for vi in fdata:
                        vdata = Part.Vertex(tris[0][vi])
                        fi += " " + str(findVert(vdata,shape.Vertexes) + offsetv)
                    flist.append(fi)
            else:
                fi = ""
                edges = f.OuterWire.OrderedEdges
                # Avoid flipped normals:
                if f.Orientation == "Reversed":
                    edges.reverse()
                for e in edges:
                    v = e.Vertexes[0 if e.Orientation == "Forward" else 1]
                    ind = findVert(v,shape.Vertexes)
                    if ind is None:
                        return None,None,None,None
                    fi += " " + str(ind + offsetv)
                flist.append(fi)

    return vlist,vnlist,elist,flist


def export(exportList,filename,colors=None):

    """export(exportList,filename,colors=None):
    Called when freecad exports a file. exportList is a list
    of objects, filename is the .obj file to export (a .mtl
    file with same name will also be created together), and
    optionally colors can be a dict containing ["objectName:colorTuple"]
    pairs for use in non-GUI mode."""

    outfile = codecs.open(filename,"wb",encoding="utf8")
    ver = FreeCAD.Version()
    outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
    outfile.write("# https://www.freecad.org\n")
    offsetv = 1
    offsetvn = 1
    objectslist = Draft.get_group_contents(exportList, walls=True,
                                           addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist, strict=True)
    filenamemtl = filename[:-4] + ".mtl"
    materials = []
    outfile.write("mtllib " + os.path.basename(filenamemtl) + "\n")

    for obj in objectslist:
        if FreeCAD.GuiUp:
            visible = obj.Visibility
        else:
            visible = True
        if not visible:
            continue

        if not (obj.isDerivedFrom("Part::Feature")
                or obj.isDerivedFrom("Mesh::Feature")
                or obj.isDerivedFrom("App::Link")):
            continue

        if obj.isDerivedFrom("App::Link"):
            # Get global placement from a Link:
            obj_place = obj.Placement
            parents = obj.Parents
            if parents:
                doc = obj.Document
                parents = [parents[0][0]] + [doc.getObject(name) for name in parents[0][1].split(".")[:-2]]
                parents.reverse()  # child-parent-grandparent order.
                for parent in parents:
                    obj_place = parent.Placement.multiply(obj_place)
        else:
            obj_place = obj.getGlobalPlacement()

        hires = None
        if FreeCAD.GuiUp:
            if obj.ViewObject.DisplayMode == "HiRes":
                # check if high-resolution object is available
                if hasattr(obj,"HiRes") and obj.HiRes:
                    if obj.HiRes.isDerivedFrom("Mesh::Feature"):
                        hires = obj.HiRes.Mesh.copy()
                    else:
                        hires = obj.HiRes.Shape.copy(False, True)
                    hires.Placement = obj_place.multiply(hires.Placement)
                if not hires \
                        and hasattr(obj,"CloneOf") \
                        and obj.CloneOf \
                        and hasattr(obj.CloneOf,"HiRes") \
                        and obj.CloneOf.HiRes:
                    if obj.CloneOf.HiRes.isDerivedFrom("Mesh::Feature"):
                        hires = obj.CloneOf.HiRes.Mesh.copy()
                    else:
                        hires = obj.CloneOf.HiRes.Shape.copy(False, True)
                    hires.Placement = obj_place.multiply(obj.CloneOf.Placement).multiply(hires.Placement)

        if hires:
            vlist,vnlist,elist,flist = getIndices(obj,hires,offsetv,offsetvn)
        elif hasattr(obj,"Shape") and obj.Shape:
            shape = obj.Shape.copy(False, True)
            shape.Placement = obj_place
            vlist,vnlist,elist,flist = getIndices(obj,shape,offsetv,offsetvn)
        elif hasattr(obj,"Mesh") and obj.Mesh:
            mesh = obj.Mesh.copy()
            mesh.Placement = obj_place
            vlist,vnlist, elist,flist = getIndices(obj,mesh,offsetv,offsetvn)

        if vlist is None:
            FreeCAD.Console.PrintError("Unable to export object "+obj.Label+". Skipping.\n")
        else:
            offsetv += len(vlist)
            offsetvn += len(vnlist)
            outfile.write("o " + obj.Label + "\n")

            # write material
            m = False
            if hasattr(obj,"Material"):
                if obj.Material:
                    if hasattr(obj.Material,"Material"):
                        outfile.write("usemtl " + obj.Material.Name + "\n")
                        materials.append(obj.Material)
                        m = True
            if not m:
                if colors:
                    if obj.Name in colors:
                        color = colors[obj.Name]
                        if color:
                            if isinstance(color[0],tuple):
                                # this is a diffusecolor. For now, use the first color - #TODO: Support per-face colors
                                color = color[0]
                            #print("found color for obj",obj.Name,":",color)
                            mn = Draft.getrgb(color,testbw=False)[1:]
                            outfile.write("usemtl color_" + mn + "\n")
                            materials.append(("color_" + mn,color,0))
                elif FreeCAD.GuiUp:
                    if hasattr(obj.ViewObject,"ShapeAppearance") and hasattr(obj.ViewObject,"Transparency"):
                        mn = Draft.getrgb(obj.ViewObject.ShapeColor,testbw=False)[1:]
                        outfile.write("usemtl color_" + mn + "\n")
                        materials.append(("color_" + mn,obj.ViewObject.ShapeColor,obj.ViewObject.Transparency))

            # write geometry
            for v in vlist:
                outfile.write("v" + v + "\n")
            for vn in vnlist:
                outfile.write("vn" + vn + "\n")
            for e in elist:
                outfile.write("l" + e + "\n")
            for f in flist:
                outfile.write("f" + f + "\n")

    outfile.close()
    FreeCAD.Console.PrintMessage(translate("Arch","Successfully written") + " " + filename + "\n")
    if materials:
        outfile = pyopen(filenamemtl,"w")
        outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
        outfile.write("# https://www.freecad.org\n")
        kinds = {"AmbientColor":"Ka ","DiffuseColor":"Kd ","SpecularColor":"Ks ","EmissiveColor":"Ke ","Transparency":"Tr ","Dissolve":"d "}
        done = [] # store names to avoid duplicates
        for mat in materials:
            if isinstance(mat,tuple):
                if not mat[0] in done:
                    outfile.write("newmtl " + mat[0] + "\n")
                    outfile.write("Kd " + str(mat[1][0]) + " " + str(mat[1][1]) + " " + str(mat[1][2]) + "\n")
                    outfile.write("Tr " + str(mat[2]/100) + "\n")
                    outfile.write("d " + str(1-mat[2]/100) + "\n")
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
        FreeCAD.Console.PrintMessage(translate("Arch","Successfully written") + ' ' + filenamemtl + "\n")


def open(filename):
    "called when freecad wants to open a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    return insert(filename,doc.Name)

def insert(filename,docname):

    meshName = ntpath.basename(filename)
    for i in meshName.split():
        if "." in i:
            i = i.split(".")[0]
    meshName = i
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    with pyopen(filename,"r",encoding="utf8") as infile:
        verts = []
        facets = []
        activeobject = None
        material = None
        colortable = {}
        content_array = []
        for line in infile:
            content_array.append(line)
    activeobjectExists = False
    for line in content_array:
        line = line.strip()
        if line[:2] == "o ":
            activeobjectExists = True
    if not activeobjectExists:
        activeobject = meshName
    for line in content_array:
        line = line.strip()
        if line[:7] == "mtllib ":
            matlib = os.path.join(os.path.dirname(filename),line[7:])
            if os.path.exists(matlib):
                with pyopen(matlib,"r") as matfile:
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
                            trans = int((1-float(mline[2:]))*100)
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
    FreeCAD.Console.PrintMessage(translate("Arch","Successfully imported") + ' ' + filename + "\n")
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
        mobj.Label = activeobject
        mobj.Mesh = Mesh.Mesh(mfacets)
        if material and FreeCAD.GuiUp:
            if material in colortable:
                mobj.ViewObject.ShapeColor = colortable[material][0]
                if colortable[material][1] is not None:
                    mobj.ViewObject.Transparency = colortable[material][1]
