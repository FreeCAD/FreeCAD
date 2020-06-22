# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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
from femexamples import boxanalysis as box

box.setup()

"""

import FreeCAD

import Fem
import ObjectsFem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup_base(doc=None, solvertype="ccxtools"):
    # setup box base model

    if doc is None:
        doc = init_doc()

    # geometry object
    geom_obj = doc.addObject("Part::Box", "Box")
    geom_obj.Height = geom_obj.Width = geom_obj.Length = 10
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
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    material_object.Material = mat

    # mesh
    from .meshes.mesh_boxanalysis_tetra10 import create_nodes, create_elements

    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(doc.addObject("Fem::FemMeshObject", mesh_name))[0]
    femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup box static, add a fixed, force and a pressure constraint

    doc = setup_base(doc, solvertype)
    geom_obj = doc.Box
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
        analysis.addObject(ObjectsFem.makeSolverElmer(doc, "SolverElmer"))
    elif solvertype == "z88":
        analysis.addObject(ObjectsFem.makeSolverZ88(doc, "SolverZ88"))
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "static"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, name="FemConstraintFixed")
    )[0]
    fixed_constraint.References = [(geom_obj, "Face1")]

    # force_constraint
    force_constraint = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="FemConstraintForce")
    )[0]
    force_constraint.References = [(geom_obj, "Face6")]
    force_constraint.Force = 40000.0
    force_constraint.Direction = (geom_obj, ["Edge5"])
    force_constraint.Reversed = True

    # pressure_constraint
    pressure_constraint = analysis.addObject(
        ObjectsFem.makeConstraintPressure(doc, name="FemConstraintPressure")
    )[0]
    pressure_constraint.References = [(geom_obj, "Face2")]
    pressure_constraint.Pressure = 1000.0
    pressure_constraint.Reversed = False

    doc.recompute()
    return doc
