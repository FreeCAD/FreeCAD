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
from femexamples.square_pipe_end_twisted_nodeforces import setup
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
    info = {"name": "Square Pipe End Twisted Nodeforces",
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

    points_forces = []
    points_forces.append(Part.Vertex(-142.5, 142.5, 0.0))
    points_forces.append(Part.Vertex(-142.5, -142.5, 0.0))
    points_forces.append(Part.Vertex(-142.5, 95.0, 0.0))
    points_forces.append(Part.Vertex(-142.5, 47.5, 0.0))
    points_forces.append(Part.Vertex(-142.5, 0.0, 0.0))
    points_forces.append(Part.Vertex(-142.5, -47.5, 0.0))
    points_forces.append(Part.Vertex(-142.5, -95.0, 0.0))
    points_forces.append(Part.Vertex(142.5, -142.5, 0.0))
    points_forces.append(Part.Vertex(-95.0, -142.5, 0.0))
    points_forces.append(Part.Vertex(-47.5, -142.5, 0.0))
    points_forces.append(Part.Vertex(0.0, -142.5, 0.0))
    points_forces.append(Part.Vertex(47.5, -142.5, 0.0))
    points_forces.append(Part.Vertex(95.0, -142.5, 0.0))
    points_forces.append(Part.Vertex(142.5, 142.5, 0.0))
    points_forces.append(Part.Vertex(142.5, -95.0, 0.0))
    points_forces.append(Part.Vertex(142.5, -47.5, 0.0))
    points_forces.append(Part.Vertex(142.5, 0.0, 0.0))
    points_forces.append(Part.Vertex(142.5, 47.5, 0.0))
    points_forces.append(Part.Vertex(142.5, 95.0, 0.0))
    points_forces.append(Part.Vertex(95.0, 142.5, 0.0))
    points_forces.append(Part.Vertex(47.5, 142.5, 0.0))
    points_forces.append(Part.Vertex(0.0, 142.5, 0.0))
    points_forces.append(Part.Vertex(-47.5, 142.5, 0.0))
    points_forces.append(Part.Vertex(-95.0, 142.5, 0.0))
    points_forces.append(Part.Vertex(-142.5, 118.75, 0.0))
    points_forces.append(Part.Vertex(-142.5, -118.75, 0.0))
    points_forces.append(Part.Vertex(-142.5, 71.25, 0.0))
    points_forces.append(Part.Vertex(-142.5, 23.75, 0.0))
    points_forces.append(Part.Vertex(-142.5, -23.75, 0.0))
    points_forces.append(Part.Vertex(-142.5, -71.25, 0.0))
    points_forces.append(Part.Vertex(118.75, -142.5, 0.0))
    points_forces.append(Part.Vertex(-71.25, -142.5, 0.0))
    points_forces.append(Part.Vertex(-118.75, -142.5, 0.0))
    points_forces.append(Part.Vertex(-23.75, -142.5, 0.0))
    points_forces.append(Part.Vertex(23.75, -142.5, 0.0))
    points_forces.append(Part.Vertex(71.25, -142.5, 0.0))
    points_forces.append(Part.Vertex(142.5, 118.75, 0.0))
    points_forces.append(Part.Vertex(142.5, -71.25, 0.0))
    points_forces.append(Part.Vertex(142.5, -118.75, 0.0))
    points_forces.append(Part.Vertex(142.5, -23.75, 0.0))
    points_forces.append(Part.Vertex(142.5, 23.75, 0.0))
    points_forces.append(Part.Vertex(142.5, 71.25, 0.0))
    points_forces.append(Part.Vertex(71.25, 142.5, 0.0))
    points_forces.append(Part.Vertex(118.75, 142.5, 0.0))
    points_forces.append(Part.Vertex(23.75, 142.5, 0.0))
    points_forces.append(Part.Vertex(-23.75, 142.5, 0.0))
    points_forces.append(Part.Vertex(-71.25, 142.5, 0.0))
    points_forces.append(Part.Vertex(-118.75, 142.5, 0.0))

    points_fixes = []
    points_fixes.append(Part.Vertex(-142.5, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, 95.0, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, 47.5, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, 0.0, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, -47.5, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, -95.0, 1000.0))
    points_fixes.append(Part.Vertex(142.5, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(-95.0, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(-47.5, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(0.0, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(47.5, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(95.0, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(142.5, -95.0, 1000.0))
    points_fixes.append(Part.Vertex(142.5, -47.5, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 0.0, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 47.5, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 95.0, 1000.0))
    points_fixes.append(Part.Vertex(95.0, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(47.5, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(0.0, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-47.5, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-95.0, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, 118.75, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, -118.75, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, 71.25, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, 23.75, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, -23.75, 1000.0))
    points_fixes.append(Part.Vertex(-142.5, -71.25, 1000.0))
    points_fixes.append(Part.Vertex(118.75, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(-71.25, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(-118.75, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(-23.75, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(23.75, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(71.25, -142.5, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 118.75, 1000.0))
    points_fixes.append(Part.Vertex(142.5, -71.25, 1000.0))
    points_fixes.append(Part.Vertex(142.5, -118.75, 1000.0))
    points_fixes.append(Part.Vertex(142.5, -23.75, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 23.75, 1000.0))
    points_fixes.append(Part.Vertex(142.5, 71.25, 1000.0))
    points_fixes.append(Part.Vertex(71.25, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(118.75, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(23.75, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-23.75, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-71.25, 142.5, 1000.0))
    points_fixes.append(Part.Vertex(-118.75, 142.5, 1000.0))

    forces_obj = doc.addObject('Part::Feature', 'Forces')
    forces_obj.Shape = Part.makeCompound(points_forces)
    fixes_obj = doc.addObject('Part::Feature', 'Fixes')
    fixes_obj.Shape = Part.makeCompound(points_fixes)

    doc.recompute()

    if FreeCAD.GuiUp:
        forces_obj.ViewObject.PointColor = (1.0, 0.0, 0.0, 0.0)
        forces_obj.ViewObject.PointSize = 10.0
        fixes_obj.ViewObject.PointColor = (1.0, 0.0, 0.0, 0.0)
        fixes_obj.ViewObject.PointSize = 10.0
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
        (doc.Fixes, 'Vertex6'),
        (doc.Fixes, 'Vertex15'),
        (doc.Fixes, 'Vertex5'),
        (doc.Fixes, 'Vertex29'),
        (doc.Fixes, 'Vertex42'),
        (doc.Fixes, 'Vertex30'),
        (doc.Fixes, 'Vertex9'),
        (doc.Fixes, 'Vertex31'),
        (doc.Fixes, 'Vertex33'),
        (doc.Fixes, 'Vertex32'),
        (doc.Fixes, 'Vertex3'),
        (doc.Fixes, 'Vertex34'),
        (doc.Fixes, 'Vertex46'),
        (doc.Fixes, 'Vertex1'),
        (doc.Fixes, 'Vertex36'),
        (doc.Fixes, 'Vertex11'),
        (doc.Fixes, 'Vertex38'),
        (doc.Fixes, 'Vertex12'),
        (doc.Fixes, 'Vertex39'),
        (doc.Fixes, 'Vertex13'),
        (doc.Fixes, 'Vertex40'),
        (doc.Fixes, 'Vertex16'),
        (doc.Fixes, 'Vertex35'),
        (doc.Fixes, 'Vertex14'),
        (doc.Fixes, 'Vertex47'),
        (doc.Fixes, 'Vertex20'),
        (doc.Fixes, 'Vertex37'),
        (doc.Fixes, 'Vertex18'),
        (doc.Fixes, 'Vertex41'),
        (doc.Fixes, 'Vertex17'),
        (doc.Fixes, 'Vertex10'),
        (doc.Fixes, 'Vertex26'),
        (doc.Fixes, 'Vertex43'),
        (doc.Fixes, 'Vertex21'),
        (doc.Fixes, 'Vertex44'),
        (doc.Fixes, 'Vertex19'),
        (doc.Fixes, 'Vertex4'),
        (doc.Fixes, 'Vertex28'),
        (doc.Fixes, 'Vertex48'),
        (doc.Fixes, 'Vertex22'),
        (doc.Fixes, 'Vertex8'),
        (doc.Fixes, 'Vertex23'),
        (doc.Fixes, 'Vertex7'),
        (doc.Fixes, 'Vertex24'),
        (doc.Fixes, 'Vertex45'),
        (doc.Fixes, 'Vertex27'),
        (doc.Fixes, 'Vertex2'),
        (doc.Fixes, 'Vertex25')]

    # force_constraint1
    force_constraint1 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce1"))[0]
    force_constraint1.References = [(forces_obj, 'Vertex1'), (forces_obj, 'Vertex14')]
    force_constraint1.Force = 5555.56
    force_constraint1.Direction = (doc.SquareTube, ["Edge9"])
    force_constraint1.Reversed = False

    # force_constraint2
    force_constraint2 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce2"))[0]
    force_constraint2.References = [(forces_obj, 'Vertex2'), (forces_obj, 'Vertex8')]
    force_constraint2.Force = 5555.56
    force_constraint2.Direction = (doc.SquareTube, ["Edge3"])
    force_constraint2.Reversed = False

    # force_constraint3
    force_constraint3 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce3"))[0]
    force_constraint3.References = [
        (forces_obj, 'Vertex20'),
        (forces_obj, 'Vertex21'),
        (forces_obj, 'Vertex22'),
        (forces_obj, 'Vertex23'),
        (forces_obj, 'Vertex24'), ]
    force_constraint3.Force = 27777.78
    force_constraint3.Direction = (doc.SquareTube, ["Edge9"])
    force_constraint3.Reversed = False

    # force_constraint4
    force_constraint4 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce4"))[0]
    force_constraint4.References = [
        (forces_obj, 'Vertex9'),
        (forces_obj, 'Vertex10'),
        (forces_obj, 'Vertex11'),
        (forces_obj, 'Vertex12'),
        (forces_obj, 'Vertex13'), ]
    force_constraint4.Force = 27777.78
    force_constraint4.Direction = (doc.SquareTube, ["Edge3"])
    force_constraint4.Reversed = False

    # force_constraint5
    force_constraint5 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce5"))[0]
    force_constraint5.References = [
        (forces_obj, 'Vertex43'),
        (forces_obj, 'Vertex44'),
        (forces_obj, 'Vertex45'),
        (forces_obj, 'Vertex46'),
        (forces_obj, 'Vertex47'),
        (forces_obj, 'Vertex48'), ]
    force_constraint5.Force = 66666.67
    force_constraint5.Direction = (doc.SquareTube, ["Edge9"])
    force_constraint5.Reversed = False

    # force_constraint6
    force_constraint6 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce6"))[0]
    force_constraint6.References = [
        (forces_obj, 'Vertex31'),
        (forces_obj, 'Vertex32'),
        (forces_obj, 'Vertex33'),
        (forces_obj, 'Vertex34'),
        (forces_obj, 'Vertex35'),
        (forces_obj, 'Vertex36'), ]
    force_constraint6.Force = 66666.67
    force_constraint6.Direction = (doc.SquareTube, ["Edge3"])
    force_constraint6.Reversed = False

    # force_constraint7
    force_constraint7 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce7"))[0]
    force_constraint7.References = [(forces_obj, 'Vertex1'), (forces_obj, 'Vertex2')]
    force_constraint7.Force = 5555.56
    force_constraint7.Direction = (doc.SquareTube, ["Edge11"])
    force_constraint7.Reversed = False

    # force_constraint8
    force_constraint8 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce8"))[0]
    force_constraint8.References = [(forces_obj, 'Vertex8'), (forces_obj, 'Vertex14')]
    force_constraint8.Force = 5555.56
    force_constraint8.Direction = (doc.SquareTube, ["Edge6"])
    force_constraint8.Reversed = False

    # force_constraint9
    force_constraint9 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce9"))[0]
    force_constraint9.References = [
        (forces_obj, 'Vertex3'),
        (forces_obj, 'Vertex4'),
        (forces_obj, 'Vertex5'),
        (forces_obj, 'Vertex6'),
        (forces_obj, 'Vertex7'), ]
    force_constraint9.Force = 27777.78
    force_constraint9.Direction = (doc.SquareTube, ["Edge11"])
    force_constraint9.Reversed = False

    # force_constraint10
    force_constraint10 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce10"))[0]
    force_constraint10.References = [
        (forces_obj, 'Vertex15'),
        (forces_obj, 'Vertex16'),
        (forces_obj, 'Vertex17'),
        (forces_obj, 'Vertex18'),
        (forces_obj, 'Vertex19'), ]
    force_constraint10.Force = 27777.78
    force_constraint10.Direction = (doc.SquareTube, ["Edge6"])
    force_constraint10.Reversed = False

    # force_constraint11
    force_constraint11 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce11"))[0]
    force_constraint11.References = [
        (forces_obj, 'Vertex25'),
        (forces_obj, 'Vertex26'),
        (forces_obj, 'Vertex27'),
        (forces_obj, 'Vertex28'),
        (forces_obj, 'Vertex29'),
        (forces_obj, 'Vertex30'), ]
    force_constraint11.Force = 66666.67
    force_constraint11.Direction = (doc.SquareTube, ["Edge11"])
    force_constraint11.Reversed = False

    # force_constraint12
    force_constraint12 = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce12"))[0]
    force_constraint12.References = [
        (forces_obj, 'Vertex37'),
        (forces_obj, 'Vertex38'),
        (forces_obj, 'Vertex39'),
        (forces_obj, 'Vertex40'),
        (forces_obj, 'Vertex41'),
        (forces_obj, 'Vertex42'), ]
    force_constraint12.Force = 66666.67
    force_constraint12.Direction = (doc.SquareTube, ["Edge6"])
    force_constraint12.Reversed = False

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
