# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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
from femexamples.constraint_transform_torque import setup()
setup()

"""

# constraint transform with a constraint force
# https://forum.freecadweb.org/viewtopic.php?t=19037
# https://forum.freecadweb.org/viewtopic.php?t=18970

import FreeCAD

import Fem
import ObjectsFem
from Part import makeLine

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Constraint Transform Torque",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["fixed", "force", "transform"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup(doc=None, solvertype="ccxtools"):

    if doc is None:
        doc = init_doc()

    # line for load direction
    sh_load_line = makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 10, 0))
    load_line = doc.addObject("Part::Feature", "Load_direction_line")
    load_line.Shape = sh_load_line
    doc.recompute()
    if FreeCAD.GuiUp:
        load_line.ViewObject.LineWidth = 5.0
        load_line.ViewObject.LineColor = (1.0, 0.0, 0.0)

    # geometry object
    # name is important because the other method in this module use obj name
    cylinder1 = doc.addObject("Part::Cylinder", "Cylinder1")
    cylinder1.Height = "50 mm"
    cylinder1.Radius = "5 mm"
    cylinder2 = doc.addObject("Part::Cylinder", "Cylinder2")
    cylinder2.Height = "50 mm"
    cylinder2.Radius = "4 mm"
    geom_obj = doc.addObject("Part::Cut", "Cut")
    geom_obj.Base = cylinder1
    geom_obj.Tool = cylinder2
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
    material_object.Material = mat

    # constraint fixed
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, name="ConstraintFixed")
    )[0]
    fixed_constraint.References = [(geom_obj, "Face3")]

    # constraint force
    force_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    force_constraint.References = [(geom_obj, "Face1")]
    force_constraint.Force = 100000.0
    force_constraint.Direction = (load_line, ["Edge1"])
    force_constraint.Reversed = True

    # constraint transform
    transform_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintTransform(doc, name="ConstraintTransform")
    )[0]
    transform_constraint.References = [(geom_obj, "Face1")]
    transform_constraint.TransformType = "Cylindrical"
    transform_constraint.X_rot = 0.0
    transform_constraint.Y_rot = 0.0
    transform_constraint.Z_rot = 0.0

    # mesh
    from .meshes.mesh_transform_torque_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        ObjectsFem.makeMeshGmsh(doc, mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
