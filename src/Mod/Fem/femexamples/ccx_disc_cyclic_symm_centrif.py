# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Jakub Michalski <jakub.j.michalski[at]gmail.com>   *
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

import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Rotating Disc Cyclic Symmetry Centrif",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["tie", "centrif", "displacement"],
        "solvers": ["ccxtools"],
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_disc_cyclic_symm_centrif import setup
setup()

Analytical solution - max von Mises stress 109.78 MPa = 1.0987e8 Pa

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
    cylinder1 = doc.addObject("Part::Cylinder", "Cylinder1")
    cylinder1.Placement = FreeCAD.Placement(Vector(0, 0, 0), Rotation(Vector(0, 0, 1), 0))
    cylinder1.Radius = "400 mm"
    cylinder1.Height = "50 mm"
    cylinder1.Angle = "45 deg"
    

    cylinder2 = doc.addObject("Part::Cylinder", "Cylinder2")
    cylinder2.Placement = FreeCAD.Placement(Vector(0, 0, 0), Rotation(Vector(0, 0, 1), 0))
    cylinder2.Radius = "200 mm"
    cylinder2.Height = "50 mm"
    cylinder2.Angle = "45 deg"

    geom_obj = doc.addObject("Part::Cut", "Cut")
    geom_obj.Base = cylinder1
    geom_obj.Tool = cylinder2
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # line for centrif axis
    axis = doc.addObject("Part::Line", "Axis")
    axis.Placement = FreeCAD.Placement(Vector(0, 0, 0), Rotation(Vector(0, 0, 1), 0))
    axis.X1 = "0.00 mm"
    axis.Y1 = "0.00 mm"
    axis.Z1 = "0.00 mm"
    axis.X2 = "0.00 mm"
    axis.Y2 = "0.00 mm"
    axis.Z2 = "50.00 mm"
    doc.recompute()
    if FreeCAD.GuiUp:
        axis.ViewObject.LineWidth = 5.0
        axis.ViewObject.LineColor = (1.0, 0.0, 0.0)

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui

        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.AnalysisType = "static"
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Steel")
    mat = material_obj.Material
    mat["Name"] = "Calculix-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "8000 kg/m^3"
    material_obj.Material = mat
    material_obj.References = [(geom_obj, "Solid1")]
    analysis.addObject(material_obj)

    # constraint tie
    con_tie = ObjectsFem.makeConstraintTie(doc, "ConstraintTie")
    con_tie.References = [
        (geom_obj, "Face2"),
        (geom_obj, "Face3"),
    ]
    con_tie.Tolerance = 0.2
    con_tie.CyclicSymmetry = True
    con_tie.Sectors = 8
    con_tie.ConnectedSectors = 1
    analysis.addObject(con_tie)
    
    # constraint centrif
    con_centrif = ObjectsFem.makeConstraintCentrif(doc, "ConstraintCentrif")
    con_centrif.References = [(geom_obj, "Solid1")]
    con_centrif.RotationFrequency = "50 Hz"
    con_centrif.RotationAxis = [(axis, "Edge1")]
    analysis.addObject(con_centrif)
    
    # constraint displacement
    con_disp = ObjectsFem.makeConstraintDisplacement(doc, "ConstraintDisplacement")
    con_disp.References = [(geom_obj, ("Vertex8"))]
    analysis.addObject(con_disp)
    con_disp.zFree = False
    con_disp.zDisplacement = "0.00 mm"

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = geom_obj
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "10 mm"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools

    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    gmsh_mesh.create_mesh()

    doc.recompute()
    return doc

