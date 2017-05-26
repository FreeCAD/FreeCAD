# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Johannes Hartung <j.hartung@gmx.net>             *
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
from __future__ import print_function

__title__ = "FreeCAD Fenics mesh reader and writer"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"

## @package importFenicsMesh
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh reader and writer for FEM workbench

import FreeCAD
import importToolsFem
import os

import readFenicsXML
import writeFenicsXML
import writeFenicsXDMF


# Template copied from importZ88Mesh.py. Thanks Bernd!
########## generic FreeCAD import and export methods ##########
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    import_fenics_mesh(filename)


def export(objectslist, fileString):
    "called when freecad exports a file"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError("This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return

    if fileString != "":
        fileName, fileExtension = os.path.splitext(fileString)
        if fileExtension.lower() == '.xml':
            writeFenicsXML.write_fenics_mesh_xml(obj, fileString)
        elif fileExtension.lower() == '.xdmf':
            writeFenicsXDMF.write_fenics_mesh_xdmf(obj, fileString)

    # write_fenics_mesh(obj, filename)


def import_fenics_mesh(filename, analysis=None):
    '''insert a FreeCAD FEM Mesh object in the ActiveDocument
    '''
    mesh_data = readFenicsXML.read_fenics_mesh_xml(filename)
    # xdmf not operational

    mesh_name = os.path.basename(os.path.splitext(filename)[0])
    femmesh = importToolsFem.make_femmesh(mesh_data)
    if femmesh:
        mesh_object = FreeCAD.ActiveDocument.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = femmesh
