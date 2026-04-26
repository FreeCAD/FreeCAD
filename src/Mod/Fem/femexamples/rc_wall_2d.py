# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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
from FreeCAD import Vector as vec

import Part
from Part import makeLine as ln

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "RC Wall 2D",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["fixed", "force", "displacement"],
        "solvers": ["ccxtools"],
        "material": "reinforced",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.rc_wall_2d import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=33106&start=80#p296469

example from Harry's epic topic: Concrete branch ready for testing

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
        solver_obj.GeometricalNonlinearity = False
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
    analysis.addObject(solver_obj)

    # shell thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 150.0, "ShellThickness")
    analysis.addObject(thickness_obj)

    # material
    matrixprop = {}
    matrixprop["Name"] = "Concrete-EN-C35_45"
    matrixprop["YoungsModulus"] = "32000 MPa"
    matrixprop["PoissonRatio"] = "0.17"
    matrixprop["CompressiveStrength"] = "15.75 MPa"
    # make some hint on the possible angle units in material system
    matrixprop["AngleOfFriction"] = "30 deg"
    reinfoprop = {}
    reinfoprop["Name"] = "Reinforcement-FIB-B500"
    reinfoprop["YieldStrength"] = "315 MPa"
    # not an official FreeCAD material property
    reinfoprop["ReinforcementRatio"] = "0.0"
    material_reinforced = ObjectsFem.makeMaterialReinforced(doc, "MaterialReinforced")
    material_reinforced.Material = matrixprop
    material_reinforced.Reinforcement = reinfoprop
    analysis.addObject(material_reinforced)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Edge1"), (geom_obj, "Edge5")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(geom_obj, "Edge7")]
    con_force.Force = "1000000.0 N"
    con_force.Direction = (geom_obj, ["Edge8"])
    con_force.Reversed = False
    analysis.addObject(con_force)

    # constraint displacement
    con_disp = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacementPrescribed")
    con_disp.References = [(geom_obj, "Face1")]
    con_disp.zFree = False
    con_disp.zDisplacement = 0
    analysis.addObject(con_disp)

    # mesh
    from .meshes.mesh_rc_wall_2d_tria6 import create_nodes, create_elements

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
