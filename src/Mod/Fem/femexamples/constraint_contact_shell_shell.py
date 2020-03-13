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

# contact example shell to shell elements
# https://forum.freecadweb.org/viewtopic.php?f=18&t=42228
# based on https://forum.freecadweb.org/viewtopic.php?f=18&t=42228#p359488
# to run the example use:
"""
from femexamples.constraint_contact_shell_shell import setup
setup()

"""


import FreeCAD

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
    # TODO turn circle of upper tube to have the line on the other side
    # make a boolean fragment of them to be sure there is a mesh point on remesh
    # but as long as we do not remesh it works without the boolean fragment too

    # tubes
    tube_radius = 25
    tube_length = 500
    sh_lower_circle = Part.Wire(Part.makeCircle(tube_radius))
    sh_lower_tube = sh_lower_circle.extrude(FreeCAD.Vector(0, 0, tube_length))
    sh_lower_tube.reverse()
    lower_tube = doc.addObject("Part::Feature", "Lower_tube")
    lower_tube.Shape = sh_lower_tube

    sh_upper_circle = Part.Wire(Part.makeCircle(tube_radius))
    sh_upper_tube = sh_upper_circle.extrude(FreeCAD.Vector(0, 0, tube_length))
    sh_upper_tube.reverse()
    upper_tube = doc.addObject("Part::Feature", "Upper_tube")
    upper_tube.Shape = sh_upper_tube
    upper_tube.Placement = FreeCAD.Placement(
        FreeCAD.Vector(-25, 51, 475),
        FreeCAD.Rotation(90, 0, 90),
        FreeCAD.Vector(0, 0, 0),
    )

    # point for load
    v_force_pt = FreeCAD.Vector(0, 76, 475)
    sh_force_point = Part.Vertex(v_force_pt)
    force_point = doc.addObject("Part::Feature", "Load_place_point")
    force_point.Shape = sh_force_point
    if FreeCAD.GuiUp:
        force_point.ViewObject.PointSize = 10.0
        force_point.ViewObject.PointColor = (1.0, 0.0, 0.0)

    # boolean fragment of upper tubo and force point
    boolfrag = SplitFeatures.makeBooleanFragments(name='BooleanFragments')
    boolfrag.Objects = [upper_tube, force_point]
    if FreeCAD.GuiUp:
        upper_tube.ViewObject.hide()

    # compound out of bool frag and lower tube
    goem_obj = doc.addObject("Part::Compound", "AllGeomCompound")
    goem_obj.Links = [boolfrag, lower_tube]

    # line for load direction
    sh_load_line = Part.makeLine(v_force_pt, FreeCAD.Vector(0, 150, 475))
    load_line = doc.addObject("Part::Feature", "Load_direction_line")
    load_line.Shape = sh_load_line
    if FreeCAD.GuiUp:
        load_line.ViewObject.LineWidth = 5.0
        load_line.ViewObject.LineColor = (1.0, 0.0, 0.0)

    doc.recompute()

    if FreeCAD.GuiUp:
        import FreeCADGui
        goem_obj.ViewObject.Document.activeView().viewAxonometric()
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
        solver_object.BeamShellResultOutput3D = True
        solver_object.GeometricalNonlinearity = "linear"  # really?
        # TODO iterations parameter !!!
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.SplitInputWriter = False

    # shell thickness
    analysis.addObject(ObjectsFem.makeElementGeometry2D(doc, 0.5, 'ShellThickness'))

    # material
    material_obj = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    )[0]
    mat = material_obj.Material
    mat["Name"] = "AlCuMgPb"
    mat["YoungsModulus"] = "72000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    )[0]
    fixed_constraint.References = [
        (lower_tube, "Edge2"),
        (upper_tube, "Edge3"),
    ]

    # force_constraint
    force_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    # TODO use point of tube boolean fragment
    force_constraint.References = [(force_point, "Vertex1")]
    force_constraint.Force = 5000.0
    force_constraint.Direction = (load_line, ["Edge1"])
    force_constraint.Reversed = True

    # contact constraint
    contact_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintContact(doc, name="ConstraintContact")
    )[0]
    contact_constraint.References = [
        (lower_tube, "Face1"),
        (upper_tube, "Face1"),
    ]
    contact_constraint.Friction = 0.0
    # contact_constrsh_aint.Slope = "1000000.0 kg/(mm*s^2)"  # contact stiffness
    contact_constraint.Slope = 1000000.0  # should be 1000000.0 kg/(mm*s^2)

    # mesh
    from .meshes.mesh_contact_tube_tube_tria3 import create_nodes, create_elements
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
