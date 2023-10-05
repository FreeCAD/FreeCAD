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

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Lateral Torsional Buckling",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["displacement", "force"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["buckling"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.buckling_lateraltorsionalbuckling import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=20217&start=110#p510526

Simple supported I-beam with a fork support modelled with shell elements.
Loaded with constant bending moment.
The moment loads are done by line loads at the end of flanges.

analytical solution:
Mcr = 43.28 kNm = 43'280'000 Nmm

flange load for a buckling factor of 1.00:
43280000 Nmm / 278.6 mm = 155348 N

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    bottom_flange = doc.addObject("Part::Plane", "Bottom_Flange")
    bottom_flange.Length = 10000
    bottom_flange.Width = 150
    top_flange = doc.addObject("Part::Plane", "Top_Flange")
    top_flange.Length = 10000
    top_flange.Width = 150
    top_flange.Placement.Base = (0, 0, 278.6)
    web = doc.addObject("Part::Plane", "Top_Flange")
    web.Length = 10000
    web.Width = 278.6
    web.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0, 75, 0),
        FreeCAD.Rotation(0, 0, 90),
        FreeCAD.Vector(0, 0, 0),
    )

    geom_obj = doc.addObject("Part::MultiFuse", "Fusion")
    geom_obj.Shapes = [bottom_flange, top_flange, web]

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
        solver_obj.AnalysisType = "buckling"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.BucklingFactors = 1
    analysis.addObject(solver_obj)

    # shell thicknesses
    thickness_flanges = ObjectsFem.makeElementGeometry2D(doc, 10.7, 'Thickness_Flanges')
    thickness_flanges.References = [(geom_obj, ("Face1", "Face2", "Face3", "Face4"))]
    analysis.addObject(thickness_flanges)
    thickness_web = ObjectsFem.makeElementGeometry2D(doc, 7.1, 'Thickness_Web')
    thickness_web.References = [(geom_obj, "Face5")]
    analysis.addObject(thickness_web)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Steel")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraints displacement
    con_disp_x = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement_X")
    con_disp_x.References = [(geom_obj, "Vertex2")]
    con_disp_x.xFix = True
    con_disp_x.xFree = False
    analysis.addObject(con_disp_x)

    con_disp_yz = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement_YZ")
    con_disp_yz.References = [(geom_obj, ("Edge15", "Edge16"))]
    con_disp_yz.yFix = True
    con_disp_yz.yFree = False
    con_disp_yz.zFix = True
    con_disp_yz.zFree = False
    analysis.addObject(con_disp_yz)

    # constraints force
    con_force_in_x = ObjectsFem.makeConstraintForce(doc, "Force_in_X")
    con_force_in_x.References = [(geom_obj, ("Edge3", "Edge7", "Edge8", "Edge12"))]
    con_force_in_x.Force = "155350 N"
    con_force_in_x.Reversed = False
    con_force_in_x.Direction = (geom_obj, ["Edge4"])
    analysis.addObject(con_force_in_x)

    con_force_rev_x = ObjectsFem.makeConstraintForce(doc, "Force_rev_X")
    con_force_rev_x.References = [(geom_obj, ("Edge1", "Edge5", "Edge10", "Edge14"))]
    con_force_rev_x.Force = "155350 N"
    con_force_rev_x.Reversed = True
    con_force_rev_x.Direction = (geom_obj, ["Edge4"])
    analysis.addObject(con_force_rev_x)

    # mesh
    from .meshes.mesh_buckling_ibeam_tria6 import create_nodes, create_elements
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
    femmesh_obj.CharacteristicLengthMax = "50.0 mm"
    femmesh_obj.ElementDimension = "2D"

    doc.recompute()
    return doc
