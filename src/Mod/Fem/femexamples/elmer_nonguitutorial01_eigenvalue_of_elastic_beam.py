# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

import FreeCAD

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "NonGui Tutorial 01 - Eigenvalue of elastic beam",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": [],
        "solvers": ["ccxtools", "elmer"],
        "material": "solid",
        "equations": ["elasticity"],  # "frequency", but list not allowed here
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.elmer_nonguitutorial01_eigenvalue_of_elastic_beam import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?t=56590

"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    geom_obj = doc.addObject("Part::Box", "Box")
    geom_obj.Length = 1000
    geom_obj.Width = 200
    geom_obj.Height = 100
    doc.recompute()
    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.WorkingDir = ""
    elif solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        eq_obj = ObjectsFem.makeEquationElasticity(doc, solver_obj)
        eq_obj.EigenAnalysis = True
        eq_obj.CalculateStresses = True
        eq_obj.DisplaceMesh = False
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "ccxtools":
        solver_obj.AnalysisType = "frequency"
        solver_obj.GeometricalNonlinearity = False
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.EigenmodesCount = 5
        solver_obj.EigenmodeHighLimit = 1000000.0
        solver_obj.EigenmodeLowLimit = 0.01
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "100 GPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "2330 kg/m^3"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Face1"), (geom_obj, "Face2")]
    analysis.addObject(con_fixed)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = "40.80 mm"

    # generate the mesh
    success = generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")
    if not success:
        # try to create from existing mesh
        from .meshes.mesh_eigenvalue_of_elastic_beam_tetra10 import create_nodes, create_elements

        fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
        femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc
