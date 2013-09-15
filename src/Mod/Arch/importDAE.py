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

import FreeCAD, Mesh, os, numpy
from DraftTools import translate

__title__="FreeCAD Collada importer"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

DEBUG = True

def checkCollada():
    "checks if collada if available"
    global collada
    COLLADA = None
    try:
        import collada
    except:
        FreeCAD.Console.PrintError(str(translate("Arch","pycollada not found, collada support is disabled.\n")))
        return False
    else:
        return True
    
def open(filename):
    "called when freecad wants to open a file"
    if not checkCollada(): 
        return
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = decode(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc

def insert(filename,docname):
    "called when freecad wants to import a file"
    if not checkCollada(): 
        return
    try:
        doc = FreeCAD.getDocument(docname)
    except:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc

def decode(name):
    "decodes encoded strings"
    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
            FreeCAD.Console.PrintError(str(translate("Arch","Error: Couldn't determine character encoding")))
            decodedName = name
    return decodedName

def read(filename):
    global col
    col = collada.Collada(filename, ignore=[collada.DaeUnsupportedError])
    for geom in col.scene.objects('geometry'):
    #for geom in col.geometries:
        for prim in geom.primitives():
        #for prim in geom.primitives:
            print prim, dir(prim)
            meshdata = []
            if hasattr(prim,"triangles"):
                tset = prim.triangles()
            elif hasattr(prim,"triangleset"):
                tset = prim.triangleset()
            for tri in tset:
                face = []
                for v in tri.vertices:
                    face.append([v[0],v[1],v[2]])
                meshdata.append(face)
            print meshdata
            newmesh = Mesh.Mesh(meshdata)
            print newmesh
            obj = FreeCAD.ActiveDocument.addObject("Mesh::Feature","Mesh")
            obj.Mesh = newmesh

def export(exportList,filename):
    "called when freecad exports a file"
    if not checkCollada(): return
    colmesh = collada.Collada()
    colmesh.assetInfo.upaxis = collada.asset.UP_AXIS.Z_UP
    effect = collada.material.Effect("effect0", [], "phong", diffuse=(.7,.7,.7), specular=(1,1,1))
    mat = collada.material.Material("material0", "mymaterial", effect)
    colmesh.effects.append(effect)
    colmesh.materials.append(mat)
    objind = 0
    scenenodes = []
    for obj in exportList:
        if obj.isDerivedFrom("Part::Feature"):
            print "exporting object ",obj.Name, obj.Shape
            m = obj.Shape.tessellate(1)
            vindex = []
            nindex = []
            findex = []
            # vertex indices
            for v in m[0]:
                vindex.extend([v.x,v.y,v.z])
            # normals
            for f in obj.Shape.Faces:
                n = f.normalAt(0,0)
                for i in range(len(f.tessellate(1)[1])):
                    nindex.extend([n.x,n.y,n.z])
            # face indices
            for i in range(len(m[1])):
                f = m[1][i]
                findex.extend([f[0],i,f[1],i,f[2],i])
        elif obj.isDerivedFrom("Mesh::Feature"):
            print "exporting object ",obj.Name, obj.Mesh
            m = obj.Mesh
            vindex = []
            nindex = []
            findex = []
            # vertex indices
            for v in m.Topology[0]:
                vindex.extend([v.x,v.y,v.z])
            # normals
            for f in m.Facets:
                n = f.Normal
                nindex.extend([n.x,n.y,n.z])
            # face indices
            for i in range(len(m.Topology[1])):
                f = m.Topology[1][i]
                findex.extend([f[0],i,f[1],i,f[2],i])
        print len(vindex), " vert indices, ", len(nindex), " norm indices, ", len(findex), " face indices."
        vert_src = collada.source.FloatSource("cubeverts-array"+str(objind), numpy.array(vindex), ('X', 'Y', 'Z'))
        normal_src = collada.source.FloatSource("cubenormals-array"+str(objind), numpy.array(nindex), ('X', 'Y', 'Z'))
        geom = collada.geometry.Geometry(colmesh, "geometry"+str(objind), obj.Name, [vert_src, normal_src])
        input_list = collada.source.InputList()
        input_list.addInput(0, 'VERTEX', "#cubeverts-array"+str(objind))
        input_list.addInput(1, 'NORMAL', "#cubenormals-array"+str(objind))
        triset = geom.createTriangleSet(numpy.array(findex), input_list, "materialref")
        geom.primitives.append(triset)
        colmesh.geometries.append(geom)
        matnode = collada.scene.MaterialNode("materialref", mat, inputs=[])
        geomnode = collada.scene.GeometryNode(geom, [matnode])
        node = collada.scene.Node("node"+str(objind), children=[geomnode])
        scenenodes.append(node)
        objind += 1
    myscene = collada.scene.Scene("myscene", scenenodes)
    colmesh.scenes.append(myscene)
    colmesh.scene = myscene
    colmesh.write(filename)
    FreeCAD.Console.PrintMessage(str(translate("Arch","file %s successfully created.")) % filename)
