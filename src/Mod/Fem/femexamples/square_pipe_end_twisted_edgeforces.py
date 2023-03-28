# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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
from FreeCAD import Vector

import Part

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Square Pipe End Twisted Edgeforces",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["force", "fixed"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.square_pipe_end_twisted_edgeforces import setup
setup()


See forum topic post:
...

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    # name is important because the other method in this module use obj name
    l1 = Part.makeLine((-142.5, -142.5, 0), (142.5, -142.5, 0))
    l2 = Part.makeLine((142.5, -142.5, 0), (142.5, 142.5, 0))
    l3 = Part.makeLine((142.5, 142.5, 0), (-142.5, 142.5, 0))
    l4 = Part.makeLine((-142.5, 142.5, 0), (-142.5, -142.5, 0))
    wire = Part.Wire([l1, l2, l3, l4])
    shape = wire.extrude(Vector(0, 0, 1000))
    geom_obj = doc.addObject("Part::Feature", "SquareTube")
    geom_obj.Shape = shape
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
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
    analysis.addObject(solver_obj)

    # shell thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 15.0, "ShellThickness")
    analysis.addObject(thickness_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "FemMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [
        (doc.SquareTube, "Edge4"),
        (doc.SquareTube, "Edge7"),
        (doc.SquareTube, "Edge10"),
        (doc.SquareTube, "Edge12")]
    analysis.addObject(con_fixed)

    # con_force1
    con_force1 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce1")
    con_force1.References = [(geom_obj, "Edge9")]
    con_force1.Force = "100000.00 N"
    con_force1.Direction = (geom_obj, ["Edge9"])
    con_force1.Reversed = True
    analysis.addObject(con_force1)

    # con_force2
    con_force2 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce2")
    con_force2.References = [(geom_obj, "Edge3")]
    con_force2.Force = "100000.00 N"
    con_force2.Direction = (geom_obj, ["Edge3"])
    con_force2.Reversed = True
    analysis.addObject(con_force2)

    # con_force3
    con_force3 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce3")
    con_force3.References = [(geom_obj, "Edge11")]
    con_force3.Force = "100000.00 N"
    con_force3.Direction = (geom_obj, ["Edge11"])
    con_force3.Reversed = True
    analysis.addObject(con_force3)

    # con_force4
    con_force4 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce4")
    con_force4.References = [(geom_obj, "Edge6")]
    con_force4.Force = "100000.00 N"
    con_force4.Direction = (geom_obj, ["Edge6"])
    con_force4.Reversed = True
    analysis.addObject(con_force4)

    # mesh
    from .meshes.mesh_square_pipe_end_twisted_tria6 import create_nodes, create_elements
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
