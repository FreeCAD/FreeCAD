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
from femexamples.frequency_beamsimple import setup
setup()

"""
# simple frequency analysis
# https://forum.freecadweb.org/viewtopic.php?f=18&t=58959#p506565

import FreeCAD

import Fem
import ObjectsFem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Frequency Analysis Simple Beam",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["fixed"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "frequency"
            }
    return info


def setup(doc=None, solvertype="ccxtools"):
    # setup simple beam frequency

    if doc is None:
        doc = init_doc()

    # geometry object
    geom_obj = doc.addObject("Part::Box", "Box")
    geom_obj.Length = 3000
    geom_obj.Width = 100
    geom_obj.Height = 50
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

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
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "frequency"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.EigenmodesCount = 10
        solver_object.EigenmodeHighLimit = 1000000.0
        solver_object.EigenmodeLowLimit = 0.01

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

    # displacement_constraint 1
    displacement_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintDisplacement(doc, name="Fix_XYZ")
    )[0]
    displacement_constraint.References = [(doc.Box, "Edge4")]
    displacement_constraint.xFix = True
    displacement_constraint.xFree = False
    displacement_constraint.xDisplacement = 0.0
    displacement_constraint.yFix = True
    displacement_constraint.yFree = False
    displacement_constraint.yDisplacement = 0.0
    displacement_constraint.zFix = True
    displacement_constraint.zFree = False
    displacement_constraint.zDisplacement = 0.0

    # displacement_constraint 2
    displacement_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintDisplacement(doc, name="Fix_YZ")
    )[0]
    displacement_constraint.References = [(doc.Box, "Edge8")]
    displacement_constraint.xFix = False
    displacement_constraint.xFree = True
    displacement_constraint.xDisplacement = 0.0
    displacement_constraint.yFix = True
    displacement_constraint.yFree = False
    displacement_constraint.yDisplacement = 0.0
    displacement_constraint.zFix = True
    displacement_constraint.zFree = False
    displacement_constraint.zDisplacement = 0.0

    # mesh
    from .meshes.mesh_beamsimple_tetra10 import create_nodes, create_elements

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
    femmesh_obj.CharacteristicLengthMax = "25.0 mm"

    doc.recompute()
    return doc
