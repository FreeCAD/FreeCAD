# ***************************************************************************
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

from BOPTools import SplitFeatures

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Mystran Plate",
        "meshtype": "face",
        "meshelement": "Quad4",
        "constraints": ["fixed", "force"],
        "solvers": ["calculix", "ccxtools", "elmer", "mystran"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.buckling_platebuckling import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=60320&start=10#p517884
This version here uses some real value for the Young's Modulus
The CalculiX steel is used
plate 10 mm x 10 mm * 0.3 mm
one each mesh node on one edge 100 N tension force


Does not work on Z88 because Z88 does not support quad4 elements

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    plate = doc.addObject("Part::Plane", "Plate")
    plate.Width = 10
    plate.Length = 10
    force_pt1 = doc.addObject("Part::Vertex", "ForcePT1")
    force_pt1.X = 10
    force_pt1.Y = 2
    force_pt2 = doc.addObject("Part::Vertex", "ForcePT2")
    force_pt2.X = 10
    force_pt2.Y = 4
    force_pt3 = doc.addObject("Part::Vertex", "ForcePT3")
    force_pt3.X = 10
    force_pt3.Y = 6
    force_pt4 = doc.addObject("Part::Vertex", "ForcePT4")
    force_pt4.X = 10
    force_pt4.Y = 8
    doc.recompute()

    # all geom boolean fragment
    geom_obj = SplitFeatures.makeBooleanFragments(name='ThePointPlate')
    geom_obj.Objects = [plate, force_pt1, force_pt2, force_pt3, force_pt4]
    doc.recompute()
    if FreeCAD.GuiUp:
        plate.ViewObject.hide()
        force_pt1.ViewObject.hide()
        force_pt2.ViewObject.hide()
        force_pt3.ViewObject.hide()
        force_pt4.ViewObject.hide()
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.PointSize = 10
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
    elif solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        ObjectsFem.makeEquationElasticity(doc, solver_obj)
    elif solvertype == "mystran":
        solver_obj = ObjectsFem.makeSolverMystran(doc, "SolverMystran")
    elif solvertype == "z88":
        solver_obj = ObjectsFem.makeSolverZ88(doc, "SolverZ88")
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
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 0.3, 'Thickness')
    analysis.addObject(thickness_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "FemMaterial")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Edge1")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [
        (geom_obj, "Vertex7"),
        (geom_obj, "Vertex1"),
        (geom_obj, "Vertex2"),
        (geom_obj, "Vertex3"),
        (geom_obj, "Vertex4"),
        (geom_obj, "Vertex8"),
    ]
    con_force.Force = "600 N"  # 600 N on six nodes == 100 N/Node
    con_force.Reversed = False
    con_force.Direction = (geom_obj, ["Edge2"])
    analysis.addObject(con_force)

    # mesh
    from .meshes.mesh_plate_mystran_quad4 import create_nodes, create_elements
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
    femmesh_obj.CharacteristicLengthMax = "1.0 mm"
    femmesh_obj.ElementDimension = "2D"
    femmesh_obj.ElementOrder = "1st"

    doc.recompute()
    return doc
