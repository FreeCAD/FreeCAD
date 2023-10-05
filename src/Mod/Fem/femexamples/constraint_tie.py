# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com    *
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
from FreeCAD import Vector

import Part
from BOPTools import SplitFeatures

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Constraint Tie",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "force", "tie"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.constraint_tie import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=42783

constraint tie, bond two surfaces together (solid mesh only)

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    # cones cut
    cone_outer_sh = Part.makeCone(1100, 1235, 1005, Vector(0, 0, 0), Vector(0, 0, 1), 359)
    cone_inner_sh = Part.makeCone(1050, 1185, 1005, Vector(0, 0, 0), Vector(0, 0, 1), 359)
    cone_cut_sh = cone_outer_sh.cut(cone_inner_sh)
    cone_cut_obj = doc.addObject("Part::Feature", "Cone_Cut")
    cone_cut_obj.Shape = cone_cut_sh

    # lines
    line_fix_sh = Part.Edge(Part.LineSegment(Vector(0, -1235, 1005), Vector(0, -1185, 1005)))
    line_fix_obj = doc.addObject("Part::Feature", "Line_Fix")
    line_fix_obj.Shape = line_fix_sh
    line_force_sh = Part.Edge(Part.LineSegment(Vector(0, 1185, 1005), Vector(0, 1235, 1005)))
    line_force_obj = doc.addObject("Part::Feature", "Line_Force")
    line_force_obj.Shape = line_force_sh

    geom_obj = SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    geom_obj.Objects = [cone_cut_obj, line_fix_obj, line_force_obj]
    if FreeCAD.GuiUp:
        cone_cut_obj.ViewObject.hide()
        line_fix_obj.ViewObject.hide()
        line_force_obj.ViewObject.hide()
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
    con_fixed.References = [(geom_obj, "Edge1")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(geom_obj, "Edge2")]
    con_force.Force = "10000.0 N"  # 10 kN
    con_force.Direction = (geom_obj, ["Edge2"])
    con_force.Reversed = False
    analysis.addObject(con_force)

    # constraint tie
    con_tie = ObjectsFem.makeConstraintTie(doc, "ConstraintTie")
    con_tie.References = [
        (geom_obj, "Face5"),
        (geom_obj, "Face7"),
    ]
    con_tie.Tolerance = 25.0
    analysis.addObject(con_tie)

    # mesh
    from .meshes.mesh_constraint_tie_tetra10 import create_nodes, create_elements
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
