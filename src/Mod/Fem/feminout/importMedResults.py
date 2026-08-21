# ***************************************************************************
# *   Copyright (c) 2024 Tim Swait <timswait@gmail.com>                     *
# *   Copyright (c) 2024 Julian Todd <julian@goatchurch.org.uk >            *
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

__title__ = "Import of .med file containing mesh and results field data"
__author__ = "Tim Swait, Julian Todd"
__url__ = "https://www.freecad.org"

## @package importMedResults
#  \ingroup FEM
#  \brief FreeCAD med file results reader


import FreeCAD
import numpy as np

# we need to import FreeCAD before the non FreeCAD library because of the print
try:
    import sys

    param_group = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/CodeAster")
    mcPath = param_group.GetString("medcouplingPath")
    sys.path.append(mcPath)
    import medcoupling as mc
except Exception:
    FreeCAD.Console.PrintError("Module medcoupling not found. Cannot load med file.\n")

from vtkmodules.util import numpy_support as vtk_np
from vtkmodules.vtkCommonDataModel import vtkCellArray, vtkUnstructuredGrid
from vtkmodules.vtkCommonCore import vtkPoints

from . import importToolsFem


def read_med_meshVTK(medfile):
    """
    Function to read geometry from a med file to create an FEM result mesh
    (So far only reads tria3 meshes)
    """

    FreeCAD.Console.PrintMessage(
        f"FEM: Results file found, reading codeaster results from: {medfile}\n"
    )
    m = mc.ReadMeshFromFile(medfile)
    grid = vtkUnstructuredGrid()
    nodes = []
    for nidinu in m.getNodeIdsInUse()[0]:
        nodes.append(m.getCoordinatesOfNode(nidinu[0]))
    nodes = np.array(nodes)
    connectivity = []
    offsets = [0]
    types = []
    for e in range(m.getNumberOfCells()):
        if m.getTypeOfCell(e) == 3:
            types.append(vtk_np.vtkConstants.VTK_TRIANGLE)
        else:
            FreeCAD.Console.PrintError("Only tria3 elements supported at present")
        nid = m.getNodeIdsOfCell(e)
        connectivity += nid
        offsets.append(offsets[-1] + len(nid))
    connectivity = np.array(connectivity)
    offsets = np.array(offsets)

    vtk_connectivity = vtk_np.numpy_to_vtkIdTypeArray(connectivity, deep=True)
    vtk_offsets = vtk_np.numpy_to_vtkIdTypeArray(offsets, deep=True)

    cell_array = vtkCellArray()
    cell_array.SetData(vtk_offsets, vtk_connectivity)

    points = vtkPoints()
    points.SetData(vtk_np.numpy_to_vtk(nodes))
    grid.SetPoints(points)
    grid.SetCells(types, cell_array)
    return grid


def read_med_mesh(medfile):
    """
    Function to read geometry from a med file to create an FEM result mesh
    (So far only reads tria3 meshes)
    """

    FreeCAD.Console.PrintMessage(
        f"FEM: Results file found, reading codeaster results from: {medfile}\n"
    )
    m = mc.ReadMeshFromFile(medfile)
    nodes = {}
    for nidinu in m.getNodeIdsInUse()[0]:
        nodes[nidinu[0] + 1] = m.getCoordinatesOfNode(
            nidinu[0]
        )  # NOTE: FreeCAD mesh node naming starts at 1, MED starts at 0.
    elements_tria3 = {}
    for e in range(m.getNumberOfCells()):
        assert m.getTypeOfCell(e) == 3, "Only tria3 elements supported at present"
        nids = m.getNodeIdsOfCell(e)
        elements_tria3[e + 1] = (nids[0] + 1, nids[1] + 1, nids[2] + 1)
    mesh = importToolsFem.make_femmesh(
        {
            "Nodes": nodes,
            "Seg2Elem": [],
            "Seg3Elem": [],
            "Tria3Elem": elements_tria3,
            "Tria6Elem": [],
            "Quad4Elem": [],
            "Quad8Elem": [],
            "Tetra4Elem": [],
            "Tetra10Elem": [],
            "Hexa8Elem": [],
            "Hexa20Elem": [],
            "Penta6Elem": [],
            "Penta15Elem": [],
        }
    )
    return mesh


def read_med_result(medfile):
    """
    Function to read results from a med file into a result set object.
    (So far only reads displacements.)
    """

    fieldnames = mc.GetAllFieldNames(medfile)
    result_set = {}
    if "reslin__DEPL" in fieldnames:
        fieldname = "reslin__DEPL"
        q = mc.ReadField(medfile, fieldname).getArrays()[0]
        disp = {}
        for i in range(q.getNumberOfTuples()):
            tup = q.getTuple(i)
            disp[i + 1] = FreeCAD.Vector(tup[0], tup[1], tup[2])
        result_set["disp"] = disp
    return result_set


def read_med_resultVTK(medfile, grid):
    """
    Function to read results from a med file and apply to a VTKgrid object.
    (So far only reads displacements.)
    """

    fieldnames = mc.GetAllFieldNames(medfile)
    for fn in fieldnames:
        if "DEPL" in fn:
            fieldname = fn
            q = mc.ReadField(medfile, fieldname).getArrays()[0]
            disp = []
            for i in range(q.getNumberOfTuples()):
                tup = q.getTuple(i)
                disp.append(tup)
            disp = np.array(disp)
            # TODO: Not quite sure how units are handled, but it seems to assume m for displacements
            disp_vtk = vtk_np.numpy_to_vtk(disp[:, 0:3] / 1000, deep=True)
            disp_vtk.SetName("Displacement")
            grid.GetPointData().AddArray(disp_vtk)
            # set rotations in degrees here until we have a general method to set units
            disp_vtk = vtk_np.numpy_to_vtk(disp[:, 3:6] * 180 / np.pi, deep=True)
            disp_vtk.SetName("Rotation")
            grid.GetPointData().AddArray(disp_vtk)

        if "EFGE_NOEU" in fn:
            fieldname = fn
            q = mc.ReadField(medfile, fieldname).getArrays()[0]
            stresses = []
            for i in range(q.getNumberOfTuples()):
                tup = q.getTuple(i)
                stresses.append(tup)
            stresses = np.array(stresses)

            compnames = []
            for i in range(len(stresses[0])):
                compnames.append(q.getInfoOnComponent(i))

            N = stresses[:, 0:3]
            add_vtk_array(grid, N, "Stress_membrane", compnames[0:3])
            sigma_1, sigma_2, sigma_1_axis, sigma_2_axis = calc_principals(N)
            add_vtk_array(grid, sigma_1, "Stress_membrane_major")
            add_vtk_array(grid, sigma_2, "Stress_membrane_minor")
            add_vtk_array(grid, sigma_1_axis, "Stress_membrane_major_axis")

            N = stresses[:, 3:6]
            add_vtk_array(grid, N, "Stress_bending", compnames[3:6])
            sigma_1, sigma_2, sigma_1_axis, sigma_2_axis = calc_principals(N)
            add_vtk_array(grid, sigma_1, "Stress_bending_major")
            add_vtk_array(grid, sigma_2, "Stress_bending_minor")
            add_vtk_array(grid, sigma_1_axis, "Stress_bending_major_axis")

            N = stresses[:, 6:8]
            add_vtk_array(grid, N, "Stress_shear_outplane", compnames[6:8])

        if "DEGE_NOEU" in fn:
            fieldname = fn
            q = mc.ReadField(medfile, fieldname).getArrays()[0]
            strains = []
            for i in range(q.getNumberOfTuples()):
                tup = q.getTuple(i)
                strains.append(tup)
            strains = np.array(strains)

            compnames = []
            for i in range(len(strains[0])):
                compnames.append(q.getInfoOnComponent(i))

            N = strains[:, 0:3]
            add_vtk_array(grid, N, "Strain_membrane", compnames[0:3])
            sigma_1, sigma_2, sigma_1_axis, sigma_2_axis = calc_principals(N)
            add_vtk_array(grid, sigma_1, "Strain_membrane_major")
            add_vtk_array(grid, sigma_2, "Strain_membrane_minor")
            add_vtk_array(grid, sigma_1_axis, "Strain_membrane_major_axis")

            N = strains[:, 3:6]
            add_vtk_array(grid, N, "Strain_bending", compnames[3:6])
            sigma_1, sigma_2, sigma_1_axis, sigma_2_axis = calc_principals(N)
            add_vtk_array(grid, sigma_1, "Strain_bending_major")
            add_vtk_array(grid, sigma_2, "Strain_bending_minor")
            add_vtk_array(grid, sigma_1_axis, "Strain_bending_major_axis")

            N = strains[:, 6:8]
            add_vtk_array(grid, N, "Strain_shear_outplane", compnames[6:8])


def calc_principals(N):
    Nxx, Nyy, Nxy = N[:, 0], N[:, 1], N[:, 2]
    sym_stress = np.array([Nxx, Nxy, Nxy, Nyy]).transpose().reshape(-1, 2, 2)
    prin_val, prin_axes = np.linalg.eigh(sym_stress)
    sigma_1 = prin_val[:, 1]
    sigma_2 = prin_val[:, 0]
    sigma_1_axis = prin_axes[:, :, 1]
    sigma_2_axis = prin_axes[:, :, 0]
    return sigma_1, sigma_2, sigma_1_axis, sigma_2_axis


def add_vtk_array(g, data, dataname, compnames=[], deep=True):
    disp_vtk = vtk_np.numpy_to_vtk(data, deep=deep)
    disp_vtk.SetName(dataname)
    g.GetPointData().AddArray(disp_vtk)
    if len(compnames) > 0:
        for i, cn in zip(range(len(compnames)), compnames):
            disp_vtk.SetComponentName(i, cn)
