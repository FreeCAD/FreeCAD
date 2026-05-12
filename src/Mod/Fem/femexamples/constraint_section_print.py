# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

import ObjectsFem
import Materials
import Part

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Constraint Section Print",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["section_print", "fixed", "pressure"],
        "solvers": ["ccxtools"],
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.constraint_section_print import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?t=43044

constraint section print with volume elements

"""
    )


def setup(doc=None, solvertype="ccxtools", test_mode=False):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects. Create curves by points
    p1 = Vector(-105, 0, 0);
    p2 = Vector(47, 0, 0);
    p3 = Vector(67, 0, 0);
    p4 = Vector(-19, 65, 0);
    p5 = Vector(0, 47, 0);
    base_curves = [
        Part.ArcOfCircle(p1, p4, p3),
        Part.ArcOfCircle(-p2, p5, p2),
        Part.LineSegment(p1, -p2),
        Part.LineSegment(p2, p3)
    ]
    sorted_edges, = Part.sortEdges(tuple(map(Part.Edge, base_curves)))
    base_wire = Part.Wire(sorted_edges)
    face = Part.makeFace(base_wire)
    # split face with edge and retrieve faces
    tool = Part.LineSegment(Vector(-6.691961, -16.840161, 0), Vector(75.156087, 79.421394, 0))
    split = face.generalFuse(tool)[0]
    base_shape = Part.makeShell(split.Faces)
    shell = doc.addObject("Part::Feature", "BaseShell")
    shell.Shape = base_shape

    # the part extrusion
    geom_obj = doc.addObject("Part::Extrusion", "ArcExtrude")
    geom_obj.Base = shell
    geom_obj.DirMode = "Custom"
    geom_obj.Dir = Vector(0.00, 0.00, 1.00)
    geom_obj.DirLink = None
    geom_obj.LengthFwd = 30.00
    geom_obj.LengthRev = 0.00
    geom_obj.Solid = True
    geom_obj.Reversed = False
    geom_obj.Symmetric = True
    geom_obj.TaperAngle = 0.00
    geom_obj.TaperAngleRev = 0.00

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

    # material
    mat_manager = Materials.MaterialManager()

    steel = mat_manager.getMaterial("92589471-a6cb-4bbc-b748-d425a17dea7d")
    steel_obj = ObjectsFem.makeMaterialSolid(doc, "Material")
    steel_obj.UUID = steel.UUID
    steel_obj.Material = steel.Properties
    analysis.addObject(steel_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Face8")]
    analysis.addObject(con_fixed)

    # constraint pressure
    con_pressure = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_pressure.References = [(geom_obj, "Face4")]
    con_pressure.Pressure = "100.0 MPa"
    con_pressure.Reversed = False
    analysis.addObject(con_pressure)

    # constraint section print
    con_sectionpr = ObjectsFem.makeConstraintSectionPrint(doc, "ConstraintSectionPrint")
    con_sectionpr.References = [(geom_obj, "Face2")]
    analysis.addObject(con_sectionpr)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = geom_obj
    femmesh_obj.CharacteristicLengthMax = "10 mm"
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    # generate the mesh
    success = False
    if not test_mode:
        success = generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")
    if not success:
        # try to create from existing mesh
        from .meshes.mesh_section_print_tetra10 import create_nodes, create_elements

        fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
        femmesh_obj.FemMesh = fem_mesh

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Transparency = 50
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()
        femmesh_obj.ViewObject.Visibility = False
        shell.ViewObject.Visibility = False

    doc.recompute()
    return doc
