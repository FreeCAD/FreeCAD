# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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
from femexamples.square_pipe_end_twisted_edgeforces import setup
setup()
"""

import FreeCAD
from FreeCAD import Vector

import Fem
import ObjectsFem
import Part

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Square Pipe End Twisted Edgeforces",
            "meshtype": "solid",
            "meshelement": "Tria6",
            "constraints": ["force", "fixed"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup(doc=None, solvertype="ccxtools"):

    if doc is None:
        doc = init_doc()

    # geometry object
    # name is important because the other method in this module use obj name
    l1 = Part.makeLine((-142.5, -142.5, 0), (142.5, -142.5, 0))
    l2 = Part.makeLine((142.5, -142.5, 0), (142.5, 142.5, 0))
    l3 = Part.makeLine((142.5, 142.5, 0), (-142.5, 142.5, 0))
    l4 = Part.makeLine((-142.5, 142.5, 0), (-142.5, -142.5, 0))
    wire = Part.Wire([l1, l2, l3, l4])
    shape = wire.extrude(Vector(0, 0, 1000))
    geom_obj = doc.addObject('Part::Feature', 'SquareTube')
    geom_obj.Shape = shape
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

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
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "static"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    # shell thickness
    thickness = analysis.addObject(
        ObjectsFem.makeElementGeometry2D(doc, 0, "ShellThickness")
    )[0]
    thickness.Thickness = 15.0

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "FemMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    material_object.Material = mat

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, name="ConstraintFixed"))[0]
    fixed_constraint.References = [
        (doc.SquareTube, "Edge4"),
        (doc.SquareTube, "Edge7"),
        (doc.SquareTube, "Edge10"),
        (doc.SquareTube, "Edge12")]

    # force_constraint1
    force_constraint1 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce1"))[0]
    force_constraint1.References = [(doc.SquareTube, "Edge9")]
    force_constraint1.Force = 100000.00
    force_constraint1.Direction = (doc.SquareTube, ["Edge9"])
    force_constraint1.Reversed = True

    # force_constraint2
    force_constraint2 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce2"))[0]
    force_constraint2.References = [(doc.SquareTube, "Edge3")]
    force_constraint2.Force = 100000.00
    force_constraint2.Direction = (doc.SquareTube, ["Edge3"])
    force_constraint2.Reversed = True

    # force_constraint3
    force_constraint3 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce3"))[0]
    force_constraint3.References = [(doc.SquareTube, "Edge11")]
    force_constraint3.Force = 100000.00
    force_constraint3.Direction = (doc.SquareTube, ["Edge11"])
    force_constraint3.Reversed = True

    # force_constraint4
    force_constraint4 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce4"))[0]
    force_constraint4.References = [(doc.SquareTube, "Edge6")]
    force_constraint4.Force = 100000.00
    force_constraint4.Direction = (doc.SquareTube, ["Edge6"])
    force_constraint4.Reversed = True

    # mesh
    from .meshes.mesh_square_pipe_end_twisted_tria6 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        ObjectsFem.makeMeshGmsh(doc, mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
