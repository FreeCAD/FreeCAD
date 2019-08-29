#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 Yorik van Havre <yorik@uncreated.net>              *  
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

from __future__ import print_function
import os,FreeCAD,Mesh

__title__="FreeCAD 3DS importer"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

DEBUG = True

## @package import3DS
#  \ingroup ARCH
#  \brief 3DS file format importer
#
#  This module provides tools to import 3DS files.

def check3DS():
    "checks if collada if available"
    global dom3ds
    dom3ds = None
    try:
        from Dice3DS import dom3ds
    except ImportError:
        FreeCAD.Console.PrintError("Dice3DS not found, 3DS support is disabled.\n")
        return False
    else:
        return True
        

def open(filename):
    "called when freecad wants to open a file"
    if not check3DS(): 
        return
    docname = (os.path.splitext(os.path.basename(filename))[0]).encode("utf8")
    doc = FreeCAD.newDocument(docname)
    doc.Label = decode(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def insert(filename,docname):
    "called when freecad wants to import a file"
    if not check3DS(): 
        return
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
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
            FreeCAD.Console.PrintError(translate("Arch","Error: Couldn't determine character encoding"))
            decodedName = name
    return decodedName


def read(filename):
    dom = dom3ds.read_3ds_file(filename,tight=False)

    for j,d_nobj in enumerate(dom.mdata.objects):
        if type(d_nobj.obj) != dom3ds.N_TRI_OBJECT:
            continue
        verts = []
        if d_nobj.obj.points:
            for d_point in d_nobj.obj.points.array:
                verts.append([d_point[0],d_point[1],d_point[2]])
            meshdata = []
            for d_face in d_nobj.obj.faces.array:
                meshdata.append([verts[int(d_face[i])] for i in range(3)])
            m = [tuple(r) for r in d_nobj.obj.matrix.array]
            m = m[0] + m[1] + m[2] + m[3]
            placement = FreeCAD.Placement(FreeCAD.Matrix(*m))
            mesh = Mesh.Mesh(meshdata)
            obj = FreeCAD.ActiveDocument.addObject("Mesh::Feature","Mesh")
            obj.Mesh = mesh
            obj.Placement = placement
        else:
            print("Skipping object without vertices array: ",d_nobj.obj)

