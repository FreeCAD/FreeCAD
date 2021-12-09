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

import Part
from BOPTools import SplitFeatures

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Constraint Constact Shell Shell",
        "meshtype": "face",
        "meshelement": "Tria3",
        "constraints": ["fixed", "force", "contact"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equation": "mechanical"
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.constraint_contact_shell_shell import setup
setup()


See forum topic post:
https://forum.freecadweb.org/viewtopic.php?f=18&t=42228
based on https://forum.freecadweb.org/viewtopic.php?f=18&t=42228#p359488

contact example shell to shell elements

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    # TODO: turn circle of upper tube to have the line on the other side
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
    geom_obj = doc.addObject("Part::Compound", "AllGeomCompound")
    geom_obj.Links = [boolfrag, lower_tube]
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # line for load direction
    sh_load_line = Part.makeLine(v_force_pt, FreeCAD.Vector(0, 150, 475))
    load_line = doc.addObject("Part::Feature", "Load_direction_line")
    load_line.Shape = sh_load_line
    doc.recompute()
    if FreeCAD.GuiUp:
        load_line.ViewObject.LineWidth = 5.0
        load_line.ViewObject.LineColor = (1.0, 0.0, 0.0)

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
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.AnalysisType = "static"
        solver_obj.BeamShellResultOutput3D = True
        solver_obj.GeometricalNonlinearity = "linear"  # really?
        # TODO iterations parameter !!!
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.SplitInputWriter = False
    analysis.addObject(solver_obj)

    # shell thickness
    shell_thick = ObjectsFem.makeElementGeometry2D(doc, 0.5, 'ShellThickness')
    analysis.addObject(shell_thick)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "AlCuMgPb"
    mat["YoungsModulus"] = "72000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [
        (lower_tube, "Edge2"),
        (upper_tube, "Edge3"),
    ]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    # TODO use point of tube boolean fragment
    con_force.References = [(force_point, "Vertex1")]
    con_force.Force = 5000.0
    con_force.Direction = (load_line, ["Edge1"])
    con_force.Reversed = True
    analysis.addObject(con_force)

    # constraint contact
    con_contact = ObjectsFem.makeConstraintContact(doc, "ConstraintContact")
    con_contact.References = [
        (lower_tube, "Face1"),
        (upper_tube, "Face1"),
    ]
    con_contact.Friction = 0.0
    # con_contact.Slope = "1000000.0 kg/(mm*s^2)"  # contact stiffness
    con_contact.Slope = 1000000.0  # should be 1000000.0 kg/(mm*s^2)
    analysis.addObject(con_contact)

    # mesh
    from .meshes.mesh_contact_tube_tube_tria3 import create_nodes, create_elements
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
