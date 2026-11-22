# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar          *
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
from FreeCAD import Vector

import ObjectsFem
import Materials
import Part

from . import manager
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Axisymmetric spark gap",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["electromagnetic"],
        "solvers": ["elmer", "calculix"],
        "material": "fluid",
        "equations": ["electrostatic"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.spark_gap import setup
setup()

Spark gap - Axisymmetric model
The electrodes are represented by their profiles
"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = manager.init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # wire defining the electrode profiles and border
    d = 20.0
    r_border = 100.0

    # voltage points
    p1 = Vector(0.0, 0.0, 0.0)
    p2 = Vector(10.0, 20.0, 0.0)
    p3 = Vector(2.0, 20.0, 0.0)
    p4 = Vector(2, (r_border**2 - 2**2)**0.5, 0.0)
    
    # ground points    
    p5 = Vector(2, -(r_border**2 - 2**2)**0.5, 0.0)
    p6 = Vector(0, -d, 0.0)
    p7 = Vector(30, -d, 0.0)
    p8 = Vector(30, -d - 5, 0.0)
    p9 = Vector(2, -d - 5, 0.0)
    contour = Part.makePolygon([p5, p9, p8, p7, p6, p1, p2, p3, p4])

    # border
    p10 = Vector(r_border, 0.0, 0.0)
    border = Part.ArcOfCircle(p5, p10, p4).toShape()

    shape = Part.makeFace(Part.Wire([contour, border]))
    face = FreeCAD.ActiveDocument.addObject("Part::Feature", "Face")
    face.Shape = shape
    
    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.CoordinateSystem = "Axi Symmetric"
        equation_electrostatic = ObjectsFem.makeEquationElectrostatic(doc, solver_obj)
        equation_electrostatic.CalculateElectricFlux = True
    elif solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculiX(doc, "SolverCalculiX")
        solver_obj.AnalysisType = "electromagnetic"
        solver_obj.ElectromagneticMode = "electrostatic"
        solver_obj.ModelSpace = "axisymmetric"
        geom2d_obj = ObjectsFem.makeElementGeometry2D(doc)
        analysis.addObject(geom2d_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc

    analysis.addObject(solver_obj)

    # materials
    mat_manager = Materials.MaterialManager()

    # air around the objects and inside the coil
    air = mat_manager.getMaterial("94370b96-c97e-4a3f-83b2-11d7461f7da7")
    air_obj = ObjectsFem.makeMaterialFluid(doc, "Air")
    air_obj.UUID = air.UUID
    air_obj.Material = air.Properties
    air_obj.References = [(face, ("Face1",))]
    analysis.addObject(air_obj)

    # voltage
    voltage_obj = ObjectsFem.makeConstraintElectromagnetic(doc, "Voltage")
    voltage_obj.References = [(face, ("Edge6", "Edge7", "Edge8"))]
    voltage_obj.Potential = "1000 V"
    voltage_obj.BoundaryCondition = "Dirichlet"
    analysis.addObject(voltage_obj)

    # ground voltage
    ground_obj = ObjectsFem.makeConstraintElectromagnetic(doc, "Ground")
    ground_obj.References = [(face, ("Edge1", "Edge2", "Edge3", "Edge4"))]
    ground_obj.Potential = "0 V"
    ground_obj.BoundaryCondition = "Dirichlet"
    analysis.addObject(ground_obj)

    # border
    border_obj = ObjectsFem.makeConstraintElectromagnetic(doc, "Border")
    border_obj.References = [(face, ("Edge9",))]
    border_obj.ElectricFluxDensity = "0 C/mm^2"
    border_obj.BoundaryCondition = "Neumann"
    analysis.addObject(border_obj)

    # symmetry
    symmetry_obj = ObjectsFem.makeConstraintElectromagnetic(doc, "Symmetry")
    symmetry_obj.References = [(face, ("Edge5",))]
    symmetry_obj.ElectricFluxDensity = "0 C/mm^2"
    symmetry_obj.BoundaryCondition = "Neumann"
    analysis.addObject(symmetry_obj)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, manager.get_meshname()))[0]
    femmesh_obj.Shape = face
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "5 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "1.0 mm"
    mesh_region.References = [
        (face, ("Edge4", "Edge5", "Edge6")),
    ]
    mesh_region.ViewObject.Visibility = False

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        import FemGui
        face.ViewObject.Transparency = 50
        face.ViewObject.Document.activeView().fitAll()
        FemGui.setActiveAnalysis(analysis)

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
