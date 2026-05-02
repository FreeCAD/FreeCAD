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
        "name": "Axisymmetric stress concentration",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["fixed", "pressure"],
        "solvers": ["calculix"],
        "material": "solid",
        "equations": ["mechanical"],
    }

def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_axisymm_shaft import setup
setup()

Analytical solution - max von Mises stress 0.965 MPa
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
    l1 = Part.makeLine((0, 0, 0), (0, 100, 0))
    l2 = Part.makeLine((37.5, 0, 0), (0, 0, 0))
    l3 = Part.makeLine((37.5, 50, 0), (37.5, 0, 0))
    l4 = Part.makeLine((30, 50, 0), (37.5, 50, 0))
    l5 = Part.makeLine((25, 100, 0), (25, 55, 0))    
    l6 = Part.makeLine((0, 100, 0), (25, 100, 0))
    arc = Part.makeCircle(5, App.Vector(30, 55, 0), App.Vector(0, 0, 1), 180, 270)
    wire = Part.Wire([l1, l2, l3, l4, arc, l5, l6])
    face = Part.Face(wire)
    face.reverse()
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
        solver_obj.ModelSpace = "axisymmetric"
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # 2D element thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 1.0, "ShellThickness")
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

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(face_obj, "Edge2")]
    analysis.addObject(con_fixed)
    
    # constraint pressure
    con_press = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_press.References = [(face_obj, "Edge7")]
    con_press.Pressure = "0.509 MPa"
    con_press.Reversed = True
    analysis.addObject(con_press)

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
