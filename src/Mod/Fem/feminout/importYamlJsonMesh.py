# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 - Johannes Hartung <j.hartung@gmx.net>             *
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

__title__ = "FreeCAD YAML and JSON mesh reader and writer"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"

## @package importYAMLJSONMesh
#  \ingroup FEM
#  \brief FreeCAD YAML and JSON Mesh reader and writer for FEM workbench

import json
import os

import FreeCAD
from . import importToolsFem

has_yaml = True
try:
    import yaml
except ImportError:
    FreeCAD.Console.PrintMessage(
        "No YAML available (import yaml failure), "
        "yaml import/export won't work\n"
    )
    has_yaml = False


# ****************************************************************************
# ********* generic FreeCAD import and export methods ************************
# names are fix given from FreeCAD, these methods are called from FreeCAD
# they are set in FEM modules Init.py

if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(
    filename
):
    '''called when freecad opens a file
    a FEM mesh object is created in a new document'''

    docname = os.path.splitext(os.path.basename(filename))[0]
    return insert(filename, docname)


def insert(
    filename,
    docname
):
    '''called when freecad wants to import a file"
    a FEM mesh object is created in a existing document'''

    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    import_yaml_json_mesh(filename)
    return doc


def export(objectslist, fileString):
    "called when freecad exports a file"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError(
            "This exporter can only "
            "export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return

    write(fileString, obj.FemMesh)


# ****************************************************************************
# ********* module specific methods ******************************************
# reader:
# - a method uses a FemMesh instance, creates the FEM mesh document object and
#     returns this object
# - a method read the data from file creates FemMesh instance out of the
#     FEM mesh dictionary. This instance is returned
# - a converts the raw read data into the FEM mesh dictionary which
#     can be used to create a FemMesh instance
#
#
# writer:
# - a method directly writes a FemMesh to the mesh file

# ********* reader ***********************************************************
def import_yaml_json_mesh(
    fileString
):
    """
    read a FemMesh from a yaml/json mesh file
    insert a FreeCAD FEM Mesh object in the ActiveDocument
    return the FEM mesh document object
    """

    mesh_name = os.path.basename(os.path.splitext(fileString)[0])

    femmesh = read(fileString)
    if femmesh:
        mesh_object = FreeCAD.ActiveDocument.addObject(
            'Fem::FemMeshObject',
            mesh_name
        )
        mesh_object.FemMesh = femmesh

    return mesh_object


def read(
    fileString
):
    '''read a FemMesh from a yaml/json mesh file and return the FemMesh
    '''
    # no document object is created, just the FemMesh is returned

    fileExtension = os.path.basename(os.path.splitext(fileString)[1])

    raw_mesh_data = {}
    if fileExtension.lower() == ".meshjson" or\
       fileExtension.lower() == ".json":
        fp = pyopen(fileString, "rt")
        raw_mesh_data = json.load(fp)
        fp.close()
    elif (
            fileExtension.lower() == ".meshyaml"
            or fileExtension.lower() == ".meshyml"
            or fileExtension.lower() == ".yaml"
            or fileExtension.lower() == ".yml"
    ) and has_yaml:
        fp = pyopen(fileString, "rt")
        raw_mesh_data = yaml.load(fp)
        fp.close()
    else:
        FreeCAD.Console.PrintError(
            "Unknown extension, "
            "please select other importer.\n")

    FreeCAD.Console.PrintMessage("Converting indices to integer numbers ...")
    mesh_data = convert_raw_data_to_mesh_data(raw_mesh_data)
    FreeCAD.Console.PrintMessage("OK\n")

    return importToolsFem.make_femmesh(mesh_data)


def convert_raw_data_to_mesh_data(
    raw_mesh_data
):
    """
    Converts raw dictionary data from JSON or YAML file to proper dict
    for importToolsFem.make_femmesh(mesh_data). This is necessary since
    JSON and YAML save dict keys as strings while make_femmesh expects
    integers.
    """

    mesh_data = {}
    for (type_key, type_dict) in raw_mesh_data.items():
        if type_key.lower() != "groups":
            mesh_data[type_key] = dict([
                (int(k), v) for (k, v) in type_dict.items()
            ])
    return mesh_data


# ********* writer ***********************************************************
def write(
    fileString,
    fem_mesh
):
    '''directly write a FemMesh to a yaml/json mesh file
    fem_mesh: a FemMesh'''

    mesh_data = importToolsFem.make_dict_from_femmesh(fem_mesh)

    if fileString != "":
        fileName, fileExtension = os.path.splitext(fileString)
        if fileExtension.lower() == ".json" \
                or fileExtension.lower() == ".meshjson":
            fp = pyopen(fileString, "wt")
            json.dump(mesh_data, fp, indent=4)
            fp.close()
        elif (
            fileExtension.lower() == ".meshyaml"
            or fileExtension.lower() == ".meshyml"
            or fileExtension.lower() == ".yaml"
            or fileExtension.lower() == ".yml"
        ) and has_yaml:
            fp = pyopen(fileString, "wt")
            yaml.safe_dump(mesh_data, fp)
            fp.close()
