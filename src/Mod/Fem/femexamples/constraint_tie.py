# ***************************************************************************
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

# constraint tie, bond two surfaces together (solid mesh only)
# https://forum.freecadweb.org/viewtopic.php?f=18&t=42783
# to run the example use:
"""
from femexamples.constraint_tie import setup
setup()

"""


import FreeCAD
from FreeCAD import Vector

import Fem
import ObjectsFem
import Part
from BOPTools import SplitFeatures

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    # geometry objects
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

    geom_obj = SplitFeatures.makeBooleanFragments(name='BooleanFragments')
    geom_obj.Objects = [cone_cut_obj, line_fix_obj, line_force_obj]
    if FreeCAD.GuiUp:
        cone_cut_obj.ViewObject.hide()
        line_fix_obj.ViewObject.hide()
        line_force_obj.ViewObject.hide()

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
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.AnalysisType = "static"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.SplitInputWriter = False

    # material
    material_obj = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    )[0]
    mat = material_obj.Material
    mat["Name"] = "Calculix-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    )[0]
    con_fixed.References = [(geom_obj, "Edge1")]

    # constraint force
    con_force = doc.Analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    con_force.References = [(geom_obj, "Edge2")]
    con_force.Force = 10000.0  # 10000 N = 10 kN
    con_force.Direction = (geom_obj, ["Edge2"])
    con_force.Reversed = False

    # constraint tie
    con_tie = doc.Analysis.addObject(
        ObjectsFem.makeConstraintTie(doc, name="ConstraintTie")
    )[0]
    con_tie.References = [
        (geom_obj, "Face5"),
        (geom_obj, "Face7"),
    ]
    con_tie.Tolerance = 25.0

    # mesh
    from .meshes.mesh_constraint_tie_tetra10 import create_nodes, create_elements
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
