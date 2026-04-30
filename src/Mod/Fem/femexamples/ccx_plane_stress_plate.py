# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Jakub Michalski <jakub.j.michalski[at]gmail.com>   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************


import FreeCAD as App

import Part
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Plane stress plate with a hole",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["displacement", "force"],
        "solvers": ["calculix"],
        "material": "solid",
        "equations": ["mechanical"],
    }

def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_plane_stress_plate import setup
setup()

Analytical solution - max von Mises stress 2.31 MPa
"""
    )

    
def setup(doc=None, solvertype="calculix"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    l1 = Part.makeLine((25, 0, 0), (150, 0, 0))
    l2 = Part.makeLine((150, 0, 0), (150, 75, 0))
    l3 = Part.makeLine((150, 75, 0), (0, 75, 0))
    l4 = Part.makeLine((0, 75, 0), (0, 25, 0))
    arc = Part.makeCircle(25, App.Vector(0, 0, 0), App.Vector(0, 0, 1), 0, 90)
    wire = Part.Wire([l1, l2, l3, l4, arc])
    face = Part.Face(wire)
    face_obj = Part.show(face)
    doc.recompute()
    if App.GuiUp:
        face_obj.ViewObject.Document.activeView().viewTop()
        face_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculiX(doc, "SolverCalculiX")
        solver_obj.AnalysisType = "static"
        solver_obj.ModelSpace = "plane stress"
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # 2D element thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 10.0, "ShellThickness")
    analysis.addObject(thickness_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.0"
    material_obj.Material = mat
    material_obj.References = [(face_obj, "Face1")]
    analysis.addObject(material_obj)

    # constraint displacement 1
    con_disp1 = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement1")
    con_disp1.References = [(face_obj, "Edge4")]
    con_disp1.xFree = False
    con_disp1.xDisplacement = "0.0 mm"
    analysis.addObject(con_disp1)

    # constraint displacement 2
    con_disp2 = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement2")
    con_disp2.References = [(face_obj, "Edge1")]
    con_disp2.yFree = False
    con_disp2.yDisplacement = "0.0 mm"
    analysis.addObject(con_disp2)
    
    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(face_obj, "Edge2")]
    con_force.DirectionVector = (1, 0, 0) 
    con_force.Force = "500.00 N"
    con_force.Reversed = True
    analysis.addObject(con_force)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = face_obj
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "2 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "0.5 mm"
    mesh_region.References = [(face_obj, ("Edge5"))]
    mesh_region.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")
        
    doc.recompute()
    return doc
