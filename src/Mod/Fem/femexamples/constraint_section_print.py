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

import math

import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

import Part
import Sketcher
from BOPTools.SplitFeatures import makeBooleanFragments
from BOPTools.SplitFeatures import makeSlice
from CompoundTools.CompoundFilter import makeCompoundFilter

import Fem
import ObjectsFem

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


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    # the part sketch
    arc_sketch = doc.addObject("Sketcher::SketchObject", "Arc_Sketch")
    arc_sketch.Placement = FreeCAD.Placement(Vector(0, 0, 0), Rotation(0, 0, 0, 1))
    arc_sketch.MapMode = "Deactivated"
    # not the exact geometry which makes a closed wire
    # exact geometry will be made by the constraints
    # the order is important for the constraints definition
    geoList = [
        Part.ArcOfCircle(Part.Circle(Vector(0, 0, 0), Vector(0, 0, 1), 47), 0, math.pi),
        Part.ArcOfCircle(
            Part.Circle(Vector(-19, -22, 0), Vector(0, 0, 1), 89),
            math.pi / 12,
            math.pi / 1.1,
        ),
        Part.LineSegment(Vector(-105, 0, 0), Vector(-47, 0, 0)),
        Part.LineSegment(Vector(47, 0, 0), Vector(67, 0, 0)),
    ]
    arc_sketch.addGeometry(geoList, False)
    # https://wiki.freecad.org/Sketcher_ConstrainCoincident
    # but the best way is to add a constraint is watching
    # FreeCAD python console while create this one by the Gui
    conList = [
        Sketcher.Constraint("Coincident", 0, 3, -1, 1),
        Sketcher.Constraint("PointOnObject", 0, 2, -1),
        Sketcher.Constraint("PointOnObject", 0, 1, -1),
        Sketcher.Constraint("PointOnObject", 1, 2, -1),
        Sketcher.Constraint("PointOnObject", 1, 1, -1),
        Sketcher.Constraint("Coincident", 2, 1, 0, 2),
        Sketcher.Constraint("Coincident", 2, 2, 1, 2),
        Sketcher.Constraint("Coincident", 3, 1, 1, 1),
        Sketcher.Constraint("Coincident", 3, 2, 0, 1),
        Sketcher.Constraint("DistanceX", 2, 2, 2, 1, 58),
        Sketcher.Constraint("DistanceX", 3, 2, 3, 1, 20),
        Sketcher.Constraint("Radius", 0, 47),
        Sketcher.Constraint("Radius", 1, 89),
    ]
    arc_sketch.addConstraint(conList)

    # the part extrusion
    extrude_part = doc.addObject("Part::Extrusion", "ArcExtrude")
    extrude_part.Base = arc_sketch
    extrude_part.DirMode = "Custom"
    extrude_part.Dir = Vector(0.00, 0.00, 1.00)
    extrude_part.DirLink = None
    extrude_part.LengthFwd = 30.00
    extrude_part.LengthRev = 0.00
    extrude_part.Solid = True
    extrude_part.Reversed = False
    extrude_part.Symmetric = True
    extrude_part.TaperAngle = 0.00
    extrude_part.TaperAngleRev = 0.00

    # section plane sketch
    section_sketch = doc.addObject("Sketcher::SketchObject", "Section_Sketch")
    section_sketch.Placement = FreeCAD.Placement(
        Vector(0.000000, 0.000000, 0.000000),
        Rotation(0.000000, 0.000000, 0.000000, 1.000000),
    )
    section_sketch.MapMode = "Deactivated"
    section_sketch.addGeometry(
        Part.LineSegment(Vector(-6.691961, -16.840161, 0), Vector(75.156087, 79.421394, 0)),
        False,
    )
    # section_sketch.ExternalGeometry = extrude_part

    # section plane extrusion
    extrude_section_plane = doc.addObject("Part::Extrusion", "SectionPlaneExtrude")
    extrude_section_plane.Base = section_sketch
    extrude_section_plane.DirMode = "Custom"
    extrude_section_plane.Dir = Vector(0.00, 0.00, -1.00)
    extrude_section_plane.DirLink = None
    extrude_section_plane.LengthFwd = 40.00
    extrude_section_plane.LengthRev = 0.00
    extrude_section_plane.Solid = False
    extrude_section_plane.Reversed = False
    extrude_section_plane.Symmetric = True
    extrude_section_plane.TaperAngle = 0.00
    extrude_section_plane.TaperAngleRev = 0.00

    # TODO the extrusions could be done with much less code, see BOLTS

    if FreeCAD.GuiUp:
        arc_sketch.ViewObject.hide()
        section_sketch.ViewObject.hide()
        extrude_part.ViewObject.hide()
        extrude_section_plane.ViewObject.hide()

    Slice = makeSlice(name="Slice")
    Slice.Base = extrude_part
    Slice.Tools = extrude_section_plane
    Slice.Mode = "Split"
    # Slice.Proxy.execute(Slice)
    Slice.purgeTouched()

    # compound filter to get the solids of the slice
    solid_one = makeCompoundFilter(name="SolidOne")
    solid_one.Base = Slice
    solid_one.FilterType = "specific items"
    solid_one.items = "0"
    # solid_one.Proxy.execute(solid_one)
    solid_one.purgeTouched()
    if FreeCAD.GuiUp:
        solid_one.Base.ViewObject.hide()

    solid_two = makeCompoundFilter(name="SolidTwo")
    solid_two.Base = Slice
    solid_two.FilterType = "specific items"
    solid_two.items = "1"
    # solid_two.Proxy.execute(solid_two)
    solid_two.purgeTouched()
    if FreeCAD.GuiUp:
        solid_two.Base.ViewObject.hide()

    # CompSolid out of the two solids
    geom_obj = makeBooleanFragments(name="BooleanFragments")
    geom_obj.Objects = [solid_one, solid_two]
    geom_obj.Mode = "CompSolid"
    # geom_obj.Proxy.execute(geom_obj)
    geom_obj.purgeTouched()
    if FreeCAD.GuiUp:
        solid_one.ViewObject.hide()
        solid_two.ViewObject.hide()
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Transparency = 50
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

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Face9")]
    analysis.addObject(con_fixed)

    # constraint pressure
    con_pressure = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_pressure.References = [(geom_obj, "Face1")]
    con_pressure.Pressure = "100.0 MPa"
    con_pressure.Reversed = False
    analysis.addObject(con_pressure)

    # constraint section print
    con_sectionpr = ObjectsFem.makeConstraintSectionPrint(doc, "ConstraintSectionPrint")
    con_sectionpr.References = [(geom_obj, "Face6")]
    analysis.addObject(con_sectionpr)

    # mesh
    from .meshes.mesh_section_print_tetra10 import create_nodes, create_elements

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
