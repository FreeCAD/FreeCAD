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
        "name": "Plate Buckling",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["displacement", "force"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equation": "buckling"
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.buckling_platebuckling import setup
setup()


See forum topic post:
https://forum.freecadweb.org/viewtopic.php?f=18&t=20217&start=110#p509935

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    geom_obj = doc.addObject("Part::Plane", "Plate")
    geom_obj.Width = 6000
    geom_obj.Length = 8000
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
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "buckling"
        solver_obj.BucklingFactors = 10
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.BucklingFactors = 1
    analysis.addObject(solver_obj)

    # shell thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 50, 'Thickness')
    analysis.addObject(thickness_obj)

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
    con_disp_x.References = [(geom_obj, "Edge1")]
    con_disp_x.xFix = True
    con_disp_x.xFree = False
    analysis.addObject(con_disp_x)

    con_disp_y = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement_Y")
    con_disp_y.References = [(geom_obj, "Vertex1")]
    con_disp_y.yFix = True
    con_disp_y.yFree = False
    analysis.addObject(con_disp_y)

    con_disp_z = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement_Z")
    con_disp_z.References = [
        (geom_obj, "Edge1"),
        (geom_obj, "Edge2"),
        (geom_obj, "Edge3"),
        (geom_obj, "Edge4"),
    ]
    con_disp_z.zFix = True
    con_disp_z.zFree = False
    analysis.addObject(con_disp_z)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(geom_obj, "Edge3")]
    con_force.Force = 17162160  # 17'162.16 N
    con_force.Reversed = True
    con_force.Direction = (geom_obj, ["Edge2"])
    analysis.addObject(con_force)

    # mesh
    from .meshes.mesh_buckling_plate_tria6 import create_nodes, create_elements
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
    femmesh_obj.CharacteristicLengthMax = "300.0 mm"
    femmesh_obj.ElementDimension = "2D"

    doc.recompute()
    return doc
