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
# https://forum.freecad.org/viewtopic.php?t=19037
# https://forum.freecad.org/viewtopic.php?t=18970

import FreeCAD

import Fem
import ObjectsFem
from Part import makeLine

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Constraint Transform Torque",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "force", "transform"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.constraint_transform_torque import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=19037&start=10#p515447
https://forum.freecad.org/viewtopic.php?t=19037
https://forum.freecad.org/viewtopic.php?t=18970

constraint transform with a constraint force

"""


def setup(doc=None, solvertype="ccxtools"):

    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

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
        solver_obj = ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
    elif solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        solver_obj.WorkingDir = u""
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.SplitInputWriter = False
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Calculix-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Face3")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(geom_obj, "Face1")]
    con_force.Force = "2500.0 N"  # 2.5 kN
    con_force.Direction = (load_line, ["Edge1"])
    con_force.Reversed = True
    analysis.addObject(con_force)

    # constraint transform
    con_transform = ObjectsFem.makeConstraintTransform(doc, name="ConstraintTransform")
    con_transform.References = [(geom_obj, "Face1")]
    con_transform.TransformType = "Cylindrical"
    con_transform.X_rot = 0.0
    con_transform.Y_rot = 0.0
    con_transform.Z_rot = 0.0
    analysis.addObject(con_transform)

    # mesh
    from .meshes.mesh_transform_torque_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
