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
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# to run the example use:
"""
from femexamples import ccx_cantilever_std as canti

canti.setup_cantileverbase()
canti.setup_cantileverfaceload()
canti.setup_cantilevernodeload()
canti.setup_cantileverprescribeddisplacement()
canti.setup_cantileverhexa20faceload()

"""


import FreeCAD

import Fem
import ObjectsFem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup_cantileverbase(doc=None, solvertype="ccxtools"):
    # setup CalculiX cantilever base model

    if doc is None:
        doc = init_doc()

    # geometry object
    # name is important because the other method in this module use obj name
    geom_obj = doc.addObject("Part::Box", "Box")
    geom_obj.Height = geom_obj.Width = 1000
    geom_obj.Length = 8000
    doc.recompute()

    if FreeCAD.GuiUp:
        import FreeCADGui
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        FreeCADGui.SendMsgToActiveView("ViewFit")

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

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

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "FemMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    mat["ThermalExpansionCoefficient"] = "0.012 mm/m/K"
    material_object.Material = mat

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, name="ConstraintFixed")
    )[0]
    fixed_constraint.References = [(geom_obj, "Face1")]

    # mesh
    from .meshes.mesh_canticcx_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        doc.addObject("Fem::FemMeshObject", mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc


def setup_cantileverfaceload(doc=None, solvertype="ccxtools"):
    # setup CalculiX cantilever, apply 9 MN on surface of front end face

    doc = setup_cantileverbase(doc, solvertype)

    # force_constraint
    force_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    force_constraint.References = [(doc.Box, "Face2")]
    force_constraint.Force = 9000000.0
    force_constraint.Direction = (doc.Box, ["Edge5"])
    force_constraint.Reversed = True

    doc.recompute()
    return doc


def setup_cantilevernodeload(doc=None, solvertype="ccxtools"):
    # setup CalculiX cantilever, apply 9 MN on the 4 nodes of the front end face

    doc = setup_cantileverbase(doc, solvertype)

    # force_constraint
    force_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    # should be possible in one tuple too
    force_constraint.References = [
        (doc.Box, "Vertex5"),
        (doc.Box, "Vertex6"),
        (doc.Box, "Vertex7"),
        (doc.Box, "Vertex8")
    ]
    force_constraint.Force = 9000000.0
    force_constraint.Direction = (doc.Box, ["Edge5"])
    force_constraint.Reversed = True

    doc.recompute()
    return doc


def setup_cantileverprescribeddisplacement(doc=None, solvertype="ccxtools"):
    # setup CalculiX cantilever
    # apply a prescribed displacement of 250 mm in -z on the front end face

    doc = setup_cantileverbase(doc, solvertype)

    # displacement_constraint
    displacement_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintDisplacement(doc, name="ConstraintDisplacmentPrescribed")
    )[0]
    displacement_constraint.References = [(doc.Box, "Face2")]
    displacement_constraint.zFix = False
    displacement_constraint.zFree = False
    displacement_constraint.zDisplacement = -250.0

    doc.recompute()
    return doc


def setup_cantileverhexa20faceload(doc=None, solvertype="ccxtools"):
    doc = setup_cantileverfaceload(doc, solvertype)

    # load the hexa20 mesh
    from .meshes.mesh_canticcx_hexa20 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")

    # overwrite mesh with the hexa20 mesh
    doc.getObject(mesh_name).FemMesh = fem_mesh

    doc.recompute()
    return doc
