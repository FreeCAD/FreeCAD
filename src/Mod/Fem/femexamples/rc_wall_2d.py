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
from femexamples.rc_wall_2d import setup
setup()

"""

# example from Harry's epic topic: Concrete branch ready for testing
# https://forum.freecadweb.org/viewtopic.php?f=18&t=33106&start=80#p296469

import FreeCAD
from FreeCAD import Vector as vec

import Fem
import ObjectsFem
import Part
from Part import makeLine as ln

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup reinfoced wall in 2D

    if doc is None:
        doc = init_doc()

    # geom objects

    v1 = vec(0, -2000, 0)
    v2 = vec(500, -2000, 0)
    v3 = vec(500, 0, 0)
    v4 = vec(3500, 0, 0)
    v5 = vec(3500, -2000, 0)
    v6 = vec(4000, -2000, 0)
    v7 = vec(4000, 2000, 0)
    v8 = vec(0, 2000, 0)
    l1 = ln(v1, v2)
    l2 = ln(v2, v3)
    l3 = ln(v3, v4)
    l4 = ln(v4, v5)
    l5 = ln(v5, v6)
    l6 = ln(v6, v7)
    l7 = ln(v7, v8)
    l8 = ln(v8, v1)
    geom_obj = doc.addObject("Part::Feature", "FIB_Wall")
    geom_obj.Shape = Part.Face(Part.Wire([l1, l2, l3, l4, l5, l6, l7, l8]))
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solvertype == "ccxtools":
        solver = analysis.addObject(
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        )[0]
        solver.WorkingDir = u""
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver.SplitInputWriter = False
        solver.AnalysisType = "static"
        solver.GeometricalNonlinearity = "linear"
        solver.ThermoMechSteadyState = False
        solver.MatrixSolverType = "default"
        solver.IterationsControlParameterTimeUse = False

    # shell thickness
    thickness = analysis.addObject(
        ObjectsFem.makeElementGeometry2D(doc, 0, "ShellThickness")
    )[0]
    thickness.Thickness = 150.0

    # material
    matrixprop = {}
    matrixprop["Name"] = "Concrete-EN-C35/45"
    matrixprop["YoungsModulus"] = "32000 MPa"
    matrixprop["PoissonRatio"] = "0.17"
    matrixprop["CompressiveStrength"] = "15.75 MPa"
    # make some hint on the possible angle units in material system
    matrixprop["AngleOfFriction"] = "30 deg"
    matrixprop["Density"] = "2500 kg/m^3"
    reinfoprop = {}
    reinfoprop["Name"] = "Reinforcement-FIB-B500"
    reinfoprop["YieldStrength"] = "315 MPa"
    # not an official FreeCAD material property
    reinfoprop["ReinforcementRatio"] = "0.0"
    material_reinforced = analysis.addObject(
        ObjectsFem.makeMaterialReinforced(doc, "MaterialReinforced")
    )[0]
    material_reinforced.Material = matrixprop
    material_reinforced.Reinforcement = reinfoprop

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, name="ConstraintFixed")
    )[0]
    fixed_constraint.References = [(geom_obj, "Edge1"), (geom_obj, "Edge5")]

    # force constraint
    force_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    force_constraint.References = [(geom_obj, "Edge7")]
    force_constraint.Force = 1000000.0
    force_constraint.Direction = (geom_obj, ["Edge8"])
    force_constraint.Reversed = False

    # displacement_constraint
    displacement_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintDisplacement(doc, name="ConstraintDisplacmentPrescribed")
    )[0]
    displacement_constraint.References = [(geom_obj, "Face1")]
    displacement_constraint.zFix = True

    # mesh
    from .meshes.mesh_rc_wall_2d_tria6 import create_nodes, create_elements
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
