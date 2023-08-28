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

import FreeCAD
from FreeCAD import Vector as vec

from BasicShapes import Shapes
from Draft import clone
from Part import makeLine

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Constraint Centrif",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["centrif", "fixed"],
        "solvers": ["calculix", "ccxtools"],
        "material": "multimaterial",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.constraint_centrif import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=57770

constraint centrif, concerning CENTRIF label from ccx's *DLOAD card

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    # ring
    stiffener = doc.addObject("Part::Box", "Stiffener")
    stiffener.Length = 10
    stiffener.Width = 200
    stiffener.Height = 10
    stiffener.Placement.Base = (-5, -100, 0)
    circumference = Shapes.addTube(doc, "Circumference")
    circumference.Height = 10.0
    circumference.InnerRadius = 97.5
    circumference.OuterRadius = 102.5
    doc.recompute()

    fusion = doc.addObject("Part::MultiFuse", "Fusion")
    fusion.Shapes = [stiffener, circumference]
    doc.recompute()

    centerhole = doc.addObject("Part::Cylinder", "CenterHole")
    centerhole.Radius = 3
    centerhole.Height = 20
    doc.recompute()

    ring_bottom = doc.addObject("Part::Cut", "RingBottom")
    ring_bottom.Base = fusion
    ring_bottom.Tool = centerhole
    doc.recompute()

    # standard ring
    ring_top = clone(ring_bottom, delta=vec(0, 0, 20))
    ring_top.Label = "RingTop"

    # compound of both rings
    geom_obj = doc.addObject("Part::Compound", "TheRingOfFire")
    geom_obj.Links = [ring_bottom, ring_top]
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # line for centrif axis
    sh_axis_line = makeLine(vec(0, 0, 0), vec(0, 0, 31))
    axis_line = doc.addObject("Part::Feature", "CentrifAxis")
    axis_line.Shape = sh_axis_line
    doc.recompute()
    if FreeCAD.GuiUp:
        axis_line.ViewObject.LineWidth = 5.0
        axis_line.ViewObject.LineColor = (1.0, 0.0, 0.0)

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

    # materials
    material_obj_scotty = ObjectsFem.makeMaterialSolid(doc, "Steel_Scotty")
    mat = material_obj_scotty.Material
    mat["Name"] = "Steel_Scotty"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "4000 kg/m^3"
    material_obj_scotty.Material = mat
    material_obj_scotty.References = [(ring_bottom, "Solid1")]
    analysis.addObject(material_obj_scotty)

    material_obj_std = ObjectsFem.makeMaterialSolid(doc, "Steel_Std")
    mat = material_obj_std.Material
    mat["Name"] = "Steel_Std"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "8000 kg/m^3"
    material_obj_std.Material = mat
    material_obj_std.References = [(ring_top, "Solid1")]
    analysis.addObject(material_obj_std)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, ("Face4", "Face12"))]
    analysis.addObject(con_fixed)

    # constraint centrif
    con_centrif = ObjectsFem.makeConstraintCentrif(doc, "ConstraintCentrif")
    con_centrif.RotationFrequency = "100 Hz"
    con_centrif.RotationAxis = [(axis_line, "Edge1")]
    analysis.addObject(con_centrif)

    # mesh
    from .meshes.mesh_constraint_centrif_tetra10 import create_nodes, create_elements
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
    femmesh_obj.CharacteristicLengthMax = "5.0 mm"

    doc.recompute()
    return doc
