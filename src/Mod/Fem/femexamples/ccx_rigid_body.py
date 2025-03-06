# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Jakub Michalski <jakub.j.michalski[at]gmail.com>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import FreeCAD as App

import Part
import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Rigid body constraint",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "rigid body"],
        "solvers": ["ccxtools"],
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_rigid_body import setup
setup()


Analytical solution - max xz stress = 2.547 MPa = 2.547e6 Pa

"""
    )


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    ellipse = doc.addObject("Part::Ellipse", "Ellipse")
    ellipse.MajorRadius = "100,00 mm"
    ellipse.MinorRadius = "50,00 mm"
    ellipse.Angle1 = "0,00 °"
    ellipse.Angle2 = "360,00 °"
    ellipse.Placement = App.Placement(
        App.Vector(0.00, 0.00, 0.00), App.Rotation(App.Vector(0.00, 0.00, 1.00), 0.00)
    )
    extrude = doc.addObject("Part::Extrusion", "Extrude")
    extrude.Base = ellipse
    extrude.LengthFwd = 1000
    extrude.Solid = True
    ellipse.Visibility = False
    doc.recompute()
    if App.GuiUp:
        extrude.ViewObject.Document.activeView().viewAxonometric()
        extrude.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver,
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.WorkingDir = ""
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    material_obj.References = [(extrude, "Solid1")]
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(extrude, "Face2")]
    analysis.addObject(con_fixed)

    # constraint rigid body
    con_rb = ObjectsFem.makeConstraintRigidBody(doc, "ConstraintRigidBody")
    con_rb.References = [(extrude, "Face3")]
    con_rb.ReferenceNode = App.Vector(0.000000, 0.000000, 1000.000000)
    con_rb.Rotation = App.Rotation(App.Vector(0.000000, 0.000000, 1.000000), Radian=0.000000)
    con_rb.MomentZ = "1,00 kJ"
    con_rb.RotationalModeZ = "Load"
    analysis.addObject(con_rb)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = extrude
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "15 mm"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools

    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    try:
        error = gmsh_mesh.create_mesh()
    except Exception:
        error = sys.exc_info()[1]
        FreeCAD.Console.PrintError(f"Unexpected error when creating mesh: {error}\n")

    doc.recompute()
    return doc
