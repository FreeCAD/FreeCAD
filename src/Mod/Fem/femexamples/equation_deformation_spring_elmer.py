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

import FreeCAD

import ObjectsFem
import Materials
import Part

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Deformation (nonlinear elasticity) - Elmer",
        "meshtype": "solid",
        "meshelement": "Tet4",
        "constraints": ["displacement", "spring"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["deformation"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.equation_deformation_spring_elmer import setup
setup()

Deformation equation - Elmer solver

"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    profile = doc.addObject("Part::Circle", "Profile")
    profile.Radius = "7.5 mm"
    profile.Placement = FreeCAD.Placement(
        FreeCAD.Vector(-20, 0, 0),
        FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
    )

    line_left = Part.makeLine((-20.0, 30.0, 0.0), (-20.0, 0.0, 0.0))
    half_circle = Part.makeCircle(
        20.0,
        FreeCAD.Vector(0.0, 0.0, 0.0),
        FreeCAD.Vector(0.0, 0.0, 1.0),
        180.0,
        360.0
    )
    line_right = Part.makeLine((20.0, 0.0, 0.0), (20.0, 30.0, 0.0))
    path = doc.addObject("Part::Feature", "Path")
    path.Shape = Part.Wire((line_left, half_circle, line_right))

    spring = doc.addObject("Part::Sweep", "Spring")
    spring.Sections = [profile]
    spring.Spine = [path, ("Edge1", "Edge2", "Edge3")]

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

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
    mat_manager = Materials.MaterialManager()

    iron = mat_manager.getMaterial("1826c364-d26a-43fb-8f61-288281236836")
    iron_obj = ObjectsFem.makeMaterialSolid(doc, "Iron")
    iron_obj.UUID = iron.UUID
    iron_obj.Material = iron.Properties
    iron_obj.References = [(spring, "Solid1")]
    analysis.addObject(iron_obj)

    # constraints displacement
    displace_left = doc.addObject("Fem::ConstraintDisplacement", "DisplacementLeft")
    displace_left.xFree = False
    displace_left.hasXFormula = True
    displace_left.xDisplacementFormula = 'Variable "time"; Real MATC "0.006*tx"'
    displace_left.yFree = False
    displace_left.yDisplacement = 0
    displace_left.zFree = False
    displace_left.zDisplacement = 0
    displace_left.References = [(spring, "Face4")]
    analysis.addObject(displace_left)

    displace_right = doc.addObject("Fem::ConstraintDisplacement", "DisplacementRight")
    displace_right.xFree = False
    displace_right.hasXFormula = True
    displace_right.xDisplacementFormula = 'Variable "time"; Real MATC "-0.006*tx"'
    displace_right.yFree = False
    displace_right.yDisplacement = 0
    displace_right.zFree = False
    displace_right.zDisplacement = 0
    displace_right.References = [(spring, "Face5")]
    analysis.addObject(displace_right)

    # constraints spring
    stiffness_left = doc.addObject("Fem::ConstraintSpring", "StiffnessLeft")
    stiffness_left.TangentialStiffness = "50 N/m"
    stiffness_left.ElmerStiffness = "Tangential Stiffness"
    stiffness_left.References = [(spring, "Face4")]
    analysis.addObject(stiffness_left)

    stiffness_right = doc.addObject("Fem::ConstraintSpring", "StiffnessRight")
    stiffness_right.TangentialStiffness = "50 N/m"
    stiffness_right.ElmerStiffness = "Tangential Stiffness"
    stiffness_right.References = [(spring, "Face5")]
    analysis.addObject(stiffness_right)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = spring
    femmesh_obj.CharacteristicLengthMax = "1.25 mm"
    femmesh_obj.ElementOrder = "1st"

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)
        spring.ViewObject.Document.activeView().viewTop()
        spring.ViewObject.Document.activeView().fitAll()
        profile.ViewObject.Visibility = False
        path.ViewObject.Visibility = False
        femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
