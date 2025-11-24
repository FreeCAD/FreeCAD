# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Jakub Michalski <jakub.j.michalski[at]gmail.com>         *
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


def get_information():
    return {
        "name": "Thick Pipe Internal Pressure 2D",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["displacement", "pressure"],
        "solvers": ["ccxtools"],
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_pipe_pressure_2D import setup
setup()


Analytical solution - max von mises stress (at the inner surface) = 2.764 MPa = 2.764e6 Pa

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
    arc1 = Part.makeCircle(30, App.Vector(0, 0, 0), App.Vector(0, 0, 1), 0, 90)
    arc2 = Part.makeCircle(50, App.Vector(0, 0, 0), App.Vector(0, 0, 1), 0, 90)
    line1 = Part.makeLine((30,0,0),(50,0,0))
    line2 = Part.makeLine((0,30,0),(0,50,0))
    wire = Part.Wire([arc1,line1,arc2,line2])
    Part.show(wire)
    face = Part.Face(wire)
    faceo = Part.show(face)
    faceobj = doc.addObject("Part::Reverse", "Reverse")
    faceobj.Source = faceo
    doc.recompute()
    if App.GuiUp:
        faceobj.ViewObject.Document.activeView().viewTop()
        faceobj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver,
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.WorkingDir = ""
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
        solver_obj.ModelSpace = "plane strain"
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # 2D element thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 50.0, "ShellThickness")
    analysis.addObject(thickness_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.0"
    material_obj.Material = mat
    material_obj.References = [(faceobj, "Face1")]
    analysis.addObject(material_obj)

    # constraint displacement 1
    con_disp1 = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement1")
    con_disp1.References = [(faceobj, "Edge2")]
    con_disp1.yFree = False
    con_disp1.yDisplacement = "0.0 mm"
    analysis.addObject(con_disp1)

    # constraint displacement 2
    con_disp2 = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement2")
    con_disp2.References = [(faceobj, "Edge4")]
    con_disp2.xFree = False
    con_disp2.xDisplacement = "0.0 mm"
    analysis.addObject(con_disp2)
    
    # constraint pressure
    con_press = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_press.References = [(faceobj, "Edge1")]
    con_press.Pressure = "1 MPa"
    analysis.addObject(con_press)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = faceobj
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "1 mm"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools

    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    gmsh_mesh.create_mesh()

    doc.recompute()
    return doc


