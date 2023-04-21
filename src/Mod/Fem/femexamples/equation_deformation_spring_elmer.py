# ***************************************************************************
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

import sys
import FreeCAD
from FreeCAD import Vector

import ObjectsFem
import Part
import Sketcher

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Deformation (nonlinear elasticity) - Elmer",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["displacement", "spring"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["deformation"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_deformation_spring_elmer import setup
setup()

Deformation equation - Elmer solver

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects

    # sketch defining the spring form
    body = doc.addObject("PartDesign::Body", "Body")
    SketchPath = body.newObject("Sketcher::SketchObject", "Spring_Path")
    SketchPath.Support = (doc.getObject("XY_Plane"), [""])
    SketchPath.MapMode = "FlatFace"
    SketchPath.addGeometry(Part.LineSegment(Vector(
        -20.0, 30.0, 0.0), Vector(-20.0, 0.0, 0.0)), False)
    SketchPath.addConstraint(Sketcher.Constraint('PointOnObject', 0, 2, -1))
    SketchPath.addConstraint(Sketcher.Constraint('Vertical', 0))
    SketchPath.addGeometry(Part.ArcOfCircle(Part.Circle(
        Vector(0.0, 0.0, 0.0), Vector(0, 0, 1), 20.0), 3.141593, 6.283185), False)
    SketchPath.addConstraint(Sketcher.Constraint('Tangent', 0, 2, 1, 1))
    SketchPath.addConstraint(Sketcher.Constraint('PointOnObject', 1, 2, -1))
    SketchPath.addGeometry(Part.LineSegment(
        Vector(20.0, 0.0, 0.0), Vector(20.0, 30.0, 0.0)), False)
    SketchPath.addConstraint(Sketcher.Constraint('Tangent', 1, 2, 2, 1))
    SketchPath.addConstraint(Sketcher.Constraint('Equal', 2, 0))
    SketchPath.ViewObject.Visibility = False

    # sketch defining the spring cross section
    SketchCircle = body.newObject("Sketcher::SketchObject", "Spring_Circle")
    SketchCircle.Support = (doc.getObject("XZ_Plane"), [""])
    SketchCircle.MapMode = "FlatFace"
    SketchCircle.addGeometry(Part.Circle(Vector(-20.0, 0.0, 0.0), Vector(0, 0, 1), 7.5), False)
    SketchCircle.addConstraint(Sketcher.Constraint('PointOnObject', 0, 3, -1))
    SketchCircle.ViewObject.Visibility = False

    # the spring object
    SpringObject = body.newObject('PartDesign::AdditivePipe', 'Spring')
    SpringObject.Profile = SketchCircle
    SpringObject.Spine = SketchPath

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        SpringObject.ViewObject.Document.activeView().viewTop()
        SpringObject.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.OutputIntervals = [4]
        solver_obj.SimulationType = "Transient"
        solver_obj.TimestepIntervals = [21]
        solver_obj.TimestepSizes = [0.05]
        equation_deformation = ObjectsFem.makeEquationDeformation(doc, solver_obj)
        equation_deformation.setExpression("LinearTolerance", "1e-7")
        equation_deformation.LinearIterativeMethod = "Idrs"
        equation_deformation.LinearPreconditioning = "ILU1"
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc
    analysis.addObject(solver_obj)

    # material iron
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Iron")
    mat = material_obj.Material
    mat["Name"] = "Iron Generic"
    mat["YoungsModulus"] = "211 GPa"
    mat["PoissonRatio"] = "0.29"
    mat["Density"] = "7874 kg/m^3"
    material_obj.Material = mat
    material_obj.References = [(body, "Solid1")]
    analysis.addObject(material_obj)

    # constraints displacement
    DisplaceLeft = doc.addObject("Fem::ConstraintDisplacement", "DisplacementLeft")
    DisplaceLeft.xFree = False
    DisplaceLeft.hasXFormula = True
    DisplaceLeft.xDisplacementFormula = "Variable \"time\"; Real MATC \"0.006*tx\""
    DisplaceLeft.yFree = False
    DisplaceLeft.yFix = True
    DisplaceLeft.zFree = False
    DisplaceLeft.zFix = True
    DisplaceLeft.References = [(SpringObject, "Face1")]
    analysis.addObject(DisplaceLeft)

    DisplaceRight = doc.addObject("Fem::ConstraintDisplacement", "DisplacementRight")
    DisplaceRight.xFree = False
    DisplaceRight.hasXFormula = True
    DisplaceRight.xDisplacementFormula = "Variable \"time\"; Real MATC \"-0.006*tx\""
    DisplaceRight.yFree = False
    DisplaceRight.yFix = True
    DisplaceRight.zFree = False
    DisplaceRight.zFix = True
    DisplaceRight.References = [(SpringObject, "Face5")]
    analysis.addObject(DisplaceRight)

    # constraints spring
    StiffnessLeft = doc.addObject("Fem::ConstraintSpring", "StiffnessLeft")
    StiffnessLeft.TangentialStiffness = "50 N/m"
    StiffnessLeft.ElmerStiffness = "Tangential Stiffness"
    StiffnessLeft.References = [(SpringObject, "Face1")]
    analysis.addObject(StiffnessLeft)

    StiffnessRight = doc.addObject("Fem::ConstraintSpring", "StiffnessRight")
    StiffnessRight.TangentialStiffness = "50 N/m"
    StiffnessRight.ElmerStiffness = "Tangential Stiffness"
    StiffnessRight.References = [(SpringObject, "Face5")]
    analysis.addObject(StiffnessRight)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = body
    femmesh_obj.CharacteristicLengthMax = "1.25 mm"
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools
    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    try:
        error = gmsh_mesh.create_mesh()
    except Exception:
        error = sys.exc_info()[1]
        FreeCAD.Console.PrintError(
            "Unexpected error when creating mesh: {}\n"
            .format(error)
        )

    doc.recompute()
    return doc
