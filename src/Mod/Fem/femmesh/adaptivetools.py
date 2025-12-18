# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD adaptive meshing helper functions"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package adaptivetools
#  \ingroup FEM
#  \brief Tools to help adaptive meshing based on previous results

import FreeCAD
from FreeCAD import Console
from os import path

def create_mesh_file(objname, data, folder_path):
    # writes a mesh file to the folder, that gmsh can import
    #
    # name = object name
    # data = vtkUnstructuredGrid
    # folder_path = the path where the file should be written to

    # gmsh can read vtk, but only without data elements attached.
    # So lets export the mesh only
    # (and in the older vtk file format)

    if not "BUILD_FEM_VTK_PYTHON" in FreeCAD.__cmake__:
        raise Exception("Need vtk python support")

    from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid
    from vtkmodules.vtkIOLegacy import vtkDataSetWriter

    grid = vtkUnstructuredGrid()
    grid.SetPoints(data.GetPoints())
    grid.SetCells(data.GetCellTypesArray(), data.GetCells())

    writer = vtkDataSetWriter()
    writer.SetFileName(path.join(folder_path, f"{objname}.vtk"))
    writer.SetFileVersion(vtkDataSetWriter.VTK_LEGACY_READER_VERSION_4_2)
    writer.SetInputData(grid)
    result = writer.Write()
    if result != 1:
        raise Exception("Failed to write result mesh to gmsh")


def create_element_file(objname, data, field, folder_path):
    # writes a element file in msh file format, to be appended to a msh file
    #
    # name = object name
    # data = vtkUnstructuredGrid
    # field = Field name of result in data
    # folder_path = the path where the file should be written to

    '''
    Format:

    $ElementData
    numStringTags(ASCII int)
    stringTag(string) ...
    numRealTags(ASCII int)
    realTag(ASCII double) ...
    numIntegerTags(ASCII int)
    integerTag(ASCII int) ...
    elementTag(int) value(double) ...
    ...
    $EndElementData
    '''

    if not "BUILD_FEM_VTK_PYTHON" in FreeCAD.__cmake__:
        raise Exception("Need vtk python support")

    def tuple_to_str(tuple):
        return ",".join(map(str, tuple))

    # add all fields to a single object elementdata file
    with open(path.join(folder_path, f"{objname}.elementdata"), "a") as f:

        pd = data.GetPointData()
        array = pd.GetAbstractArray(field)
        if not array:
            raise Exception("Field ist not available in data object")

        #f.write(f"{name} = {{ {tuple_to_str(array.GetTuple(0))}")

        components = array.GetNumberOfComponents()

        field_name = field.replace(" ", "_")
        f.write("$NodeData\n")
        f.write("1\n")                              # 1 string
        f.write(field_name + "\n")                  # field name
        f.write("1\n")                              # 1 int
        f.write("0.0\n")                            # time value (always 0 here)
        f.write("3\n")                              # 3 ints
        f.write("0\n")                              # time step (always 0 here)
        f.write(str(components)+"\n")               # components of data
        f.write(str(data.GetNumberOfPoints())+"\n") # number of datasets

        for i in range(0,array.GetNumberOfTuples()):
            s = str(i+1) + " " + " ".join(map(str, array.GetTuple(i))) + "\n"
            f.write(s)

        f.write("$EndNodeData\n")
        f.flush()

def generate_model_code(objname):

    code  = f"NewModel;\n"              # open model
    code += f"SetName '{objname}';\n"     # name model
    code += f"Merge '{objname}.msh';\n" # add mesh

    return code


def generate_view_code(objname, data, field, view_tag):
    # Returns the code required to use the mesh file and node data

    name = objname + "_"  + field.replace(" ", "_")


    code  = f"view_tag = gmsh.view.add('{field.replace(" ", "_")}', {view_tag});\n"   # open view
    code += f"Include {name}.geo;\n"                                # data array
    code += f"nodes[] = {{0:{data.GetNumberOfPoints()-1}}};\n"    # node array

    # add data to view
    code += f"gmsh.view.addHomogeneousModelData(view_tag, 0, {objname}, 'NodeData', nodes, {name}, 0, components);\n"

    return code


def generate_closeout_code():
    # make sure everything else is handled in annother model, to not interfere between mesh and geometry
    return f"NewModel;\n"

def write_result_settings(settings, geo, folder):

    # 1: get unique result objects
    settings_map = {}
    for setting in settings:
        # check if the same object is already in new result
        if not setting["name"] in settings_map:
            settings_map[setting["name"]] = []

        settings_map[setting["name"]].append(setting)


    # 2: iterate all unique objects and create the files and geo files
    code = ""
    for objname in settings_map:

        obj_settings = settings_map[objname]
        if not obj_settings:
            continue

        create_mesh_file(objname,
                         obj_settings[0]["data"],
                         folder)

        code += generate_model_code(objname)

        # for each field of that object create geo file and the correct code
        for obj_setting in obj_settings:

            create_element_file(objname,
                                obj_setting["data"],
                                obj_setting["field"],
                                folder)

            #code += generate_view_code(objname,
            #                          obj_setting["data"],
            #                          obj_setting["field"],
            #                          obj_setting["view_tag"])

    # we write the code only at the end into the geo file,
    # in case some exception was thrown during file creation
    code += generate_closeout_code()
    geo.write(code)







