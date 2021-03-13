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

# to run the example use:
"""
from femexamples.elmer_nonguitutorial01_eigenvalue_of_elastic_beam import setup
setup()

"""

# https://forum.freecadweb.org/viewtopic.php?t=56590

import FreeCAD

import Fem
import ObjectsFem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Box Analysis Static",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": [],
            "solvers": ["calculix", "elmer"],
            "material": "solid",
            "equation": "elasticity"
            }
    return info


def setup_base(doc=None, solvertype="ccxtools"):
    # setup box base model

    if doc is None:
        doc = init_doc()

    # geometry object
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

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "100 GPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "2330 kg/m^3"
    material_object.Material = mat

    # mesh
    from .meshes.mesh_eigenvalue_of_elastic_beam_tetra10 import create_nodes
    from .meshes.mesh_eigenvalue_of_elastic_beam_tetra10 import create_elements

    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, mesh_name))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = "40.80 mm"

    doc.recompute()
    return doc


def setup(doc=None, solvertype="elmer"):
    # setup box static, add a fixed, force and a pressure constraint

    doc = setup_base(doc, solvertype)
    analysis = doc.Analysis

    # solver
    if solvertype == "calculix":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solvertype == "ccxtools":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        )[0]
        solver_object.WorkingDir = u""
    elif solvertype == "elmer":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        )[0]
        ObjectsFem.makeEquationElasticity(doc, solver_object)
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "static"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    doc.recompute()
    return doc
