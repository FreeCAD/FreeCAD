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
        "name": "Square Pipe End Twisted Nodeforces",
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
from femexamples.square_pipe_end_twisted_nodeforces import setup
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

    geoforces_obj = doc.addObject("Part::Feature", "Forces")
    geoforces_obj.Shape = Part.makeCompound(points_forces)
    geofixes_obj = doc.addObject("Part::Feature", "Fixes")
    geofixes_obj.Shape = Part.makeCompound(points_fixes)

    doc.recompute()

    if FreeCAD.GuiUp:
        geoforces_obj.ViewObject.PointColor = (1.0, 0.0, 0.0, 0.0)
        geoforces_obj.ViewObject.PointSize = 10.0
        geofixes_obj.ViewObject.PointColor = (1.0, 0.0, 0.0, 0.0)
        geofixes_obj.ViewObject.PointSize = 10.0
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
        (geofixes_obj, "Vertex6"),
        (geofixes_obj, "Vertex15"),
        (geofixes_obj, "Vertex5"),
        (geofixes_obj, "Vertex29"),
        (geofixes_obj, "Vertex42"),
        (geofixes_obj, "Vertex30"),
        (geofixes_obj, "Vertex9"),
        (geofixes_obj, "Vertex31"),
        (geofixes_obj, "Vertex33"),
        (geofixes_obj, "Vertex32"),
        (geofixes_obj, "Vertex3"),
        (geofixes_obj, "Vertex34"),
        (geofixes_obj, "Vertex46"),
        (geofixes_obj, "Vertex1"),
        (geofixes_obj, "Vertex36"),
        (geofixes_obj, "Vertex11"),
        (geofixes_obj, "Vertex38"),
        (geofixes_obj, "Vertex12"),
        (geofixes_obj, "Vertex39"),
        (geofixes_obj, "Vertex13"),
        (geofixes_obj, "Vertex40"),
        (geofixes_obj, "Vertex16"),
        (geofixes_obj, "Vertex35"),
        (geofixes_obj, "Vertex14"),
        (geofixes_obj, "Vertex47"),
        (geofixes_obj, "Vertex20"),
        (geofixes_obj, "Vertex37"),
        (geofixes_obj, "Vertex18"),
        (geofixes_obj, "Vertex41"),
        (geofixes_obj, "Vertex17"),
        (geofixes_obj, "Vertex10"),
        (geofixes_obj, "Vertex26"),
        (geofixes_obj, "Vertex43"),
        (geofixes_obj, "Vertex21"),
        (geofixes_obj, "Vertex44"),
        (geofixes_obj, "Vertex19"),
        (geofixes_obj, "Vertex4"),
        (geofixes_obj, "Vertex28"),
        (geofixes_obj, "Vertex48"),
        (geofixes_obj, "Vertex22"),
        (geofixes_obj, "Vertex8"),
        (geofixes_obj, "Vertex23"),
        (geofixes_obj, "Vertex7"),
        (geofixes_obj, "Vertex24"),
        (geofixes_obj, "Vertex45"),
        (geofixes_obj, "Vertex27"),
        (geofixes_obj, "Vertex2"),
        (geofixes_obj, "Vertex25")]
    analysis.addObject(con_fixed)

    # con_force1
    con_force1 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce1")
    con_force1.References = [(geoforces_obj, "Vertex1"), (geoforces_obj, "Vertex14")]
    con_force1.Force = "5555.56 N"
    con_force1.Direction = (geom_obj, ["Edge9"])
    con_force1.Reversed = False
    analysis.addObject(con_force1)

    # con_force2
    con_force2 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce2")
    con_force2.References = [(geoforces_obj, "Vertex2"), (geoforces_obj, "Vertex8")]
    con_force2.Force = "5555.56 N"
    con_force2.Direction = (geom_obj, ["Edge3"])
    con_force2.Reversed = False
    analysis.addObject(con_force2)

    # con_force3
    con_force3 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce3")
    con_force3.References = [
        (geoforces_obj, "Vertex20"),
        (geoforces_obj, "Vertex21"),
        (geoforces_obj, "Vertex22"),
        (geoforces_obj, "Vertex23"),
        (geoforces_obj, "Vertex24"), ]
    con_force3.Force = "27777.78 N"
    con_force3.Direction = (geom_obj, ["Edge9"])
    con_force3.Reversed = False
    analysis.addObject(con_force3)

    # con_force4
    con_force4 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce4")
    con_force4.References = [
        (geoforces_obj, "Vertex9"),
        (geoforces_obj, "Vertex10"),
        (geoforces_obj, "Vertex11"),
        (geoforces_obj, "Vertex12"),
        (geoforces_obj, "Vertex13"), ]
    con_force4.Force = "27777.78 N"
    con_force4.Direction = (geom_obj, ["Edge3"])
    con_force4.Reversed = False
    analysis.addObject(con_force4)

    # con_force5
    con_force5 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce5")
    con_force5.References = [
        (geoforces_obj, "Vertex43"),
        (geoforces_obj, "Vertex44"),
        (geoforces_obj, "Vertex45"),
        (geoforces_obj, "Vertex46"),
        (geoforces_obj, "Vertex47"),
        (geoforces_obj, "Vertex48"), ]
    con_force5.Force = "66666.67 N"
    con_force5.Direction = (geom_obj, ["Edge9"])
    con_force5.Reversed = False
    analysis.addObject(con_force5)

    # con_force6
    con_force6 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce6")
    con_force6.References = [
        (geoforces_obj, "Vertex31"),
        (geoforces_obj, "Vertex32"),
        (geoforces_obj, "Vertex33"),
        (geoforces_obj, "Vertex34"),
        (geoforces_obj, "Vertex35"),
        (geoforces_obj, "Vertex36"), ]
    con_force6.Force = "66666.67 N"
    con_force6.Direction = (geom_obj, ["Edge3"])
    con_force6.Reversed = False
    analysis.addObject(con_force6)

    # con_force7
    con_force7 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce7")
    con_force7.References = [(geoforces_obj, "Vertex1"), (geoforces_obj, "Vertex2")]
    con_force7.Force = "5555.56 N"
    con_force7.Direction = (geom_obj, ["Edge11"])
    con_force7.Reversed = False
    analysis.addObject(con_force7)

    # con_force8
    con_force8 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce8")
    con_force8.References = [(geoforces_obj, "Vertex8"), (geoforces_obj, "Vertex14")]
    con_force8.Force = "5555.56 N"
    con_force8.Direction = (geom_obj, ["Edge6"])
    con_force8.Reversed = False
    analysis.addObject(con_force8)

    # con_force9
    con_force9 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce9")
    con_force9.References = [
        (geoforces_obj, "Vertex3"),
        (geoforces_obj, "Vertex4"),
        (geoforces_obj, "Vertex5"),
        (geoforces_obj, "Vertex6"),
        (geoforces_obj, "Vertex7"), ]
    con_force9.Force = "27777.78 N"
    con_force9.Direction = (geom_obj, ["Edge11"])
    con_force9.Reversed = False
    analysis.addObject(con_force9)

    # con_force10
    con_force10 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce10")
    con_force10.References = [
        (geoforces_obj, "Vertex15"),
        (geoforces_obj, "Vertex16"),
        (geoforces_obj, "Vertex17"),
        (geoforces_obj, "Vertex18"),
        (geoforces_obj, "Vertex19"), ]
    con_force10.Force = "27777.78 N"
    con_force10.Direction = (geom_obj, ["Edge6"])
    con_force10.Reversed = False
    analysis.addObject(con_force10)

    # con_force11
    con_force11 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce11")
    con_force11.References = [
        (geoforces_obj, "Vertex25"),
        (geoforces_obj, "Vertex26"),
        (geoforces_obj, "Vertex27"),
        (geoforces_obj, "Vertex28"),
        (geoforces_obj, "Vertex29"),
        (geoforces_obj, "Vertex30"), ]
    con_force11.Force = "66666.67 N"
    con_force11.Direction = (geom_obj, ["Edge11"])
    con_force11.Reversed = False
    analysis.addObject(con_force11)

    # con_force12
    con_force12 = ObjectsFem.makeConstraintForce(doc, name="ConstraintForce12")
    con_force12.References = [
        (geoforces_obj, "Vertex37"),
        (geoforces_obj, "Vertex38"),
        (geoforces_obj, "Vertex39"),
        (geoforces_obj, "Vertex40"),
        (geoforces_obj, "Vertex41"),
        (geoforces_obj, "Vertex42"), ]
    con_force12.Force = "66666.67 N"
    con_force12.Direction = (geom_obj, ["Edge6"])
    con_force12.Reversed = False
    analysis.addObject(con_force12)

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
