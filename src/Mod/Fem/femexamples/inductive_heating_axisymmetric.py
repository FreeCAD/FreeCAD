# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

# to run the example use:
"""
from femexamples.inductive_heating_axisymmetric import setup
setup()
"""


import FreeCAD
from FreeCAD import Vector

import ObjectsFem
import Part

from . import manager
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Eddy currents heating",
        "meshtype": "face",
        "meshelement": "Tria3",
        "constraints": ["electromagnetic", "temperature", "current density"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["electromagnetic", "heat"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.inductive_heating_axisymmetric import setup
setup()

Magnetodynamic2d and heat equations - Elmer solver
Magnetodynamic2d and heat equations coupled on axisymmetric system.
Due to symmetry, only half of the geometry is considered.

Boundary conditions:
- outer temperature T = 300 K
- outer vector potential A = 0 Wb/m
- x-axis tangential B = 0 T (default)
- coil current density J = 10 A/mm^2

"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = manager.init_doc()

    # explanation object
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects

    # rod
    p1 = Vector(0, 0, 0)
    p2 = Vector(0, -250, 0)
    p3 = Vector(50, -250, 0)
    p4 = Vector(50, 0, 0)
    rod = Part.makePolygon([p1, p2, p3, p4, p1])

    # coil
    p5 = Vector(90, 0, 0)
    p6 = Vector(90, -10, 0)
    p7 = Vector(110, -10, 0)
    p8 = Vector(110, 0, 0)
    coil = Part.makePolygon([p5, p6, p7, p8, p5])

    # air
    air_circle = Part.makeCircle(500, Vector(0, 0, 0), Vector(0, 0, 1), -90, 0)
    air_line_1 = Part.makeLine(p1, Vector(0, -500, 0))
    air_line_2 = Part.makeLine(p1, Vector(500, 0, 0))
    air_area = Part.makeFace(Part.Wire((air_circle, air_line_1, air_line_2)))
    cut_area = Part.makeFace((coil, rod))
    air_area = air_area.cut(cut_area)

    shape = Part.makeShell(air_area.Faces + cut_area.Faces)
    shell = doc.addObject("Part::Feature", "Shell")
    shell.Shape = shape

    if FreeCAD.GuiUp:
        shell.ViewObject.Visibility = True
        shell.ViewObject.Document.ActiveView.viewTop()
        shell.ViewObject.Document.ActiveView.fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui

        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.SimulationType = "Steady State"
        solver_obj.CoordinateSystem = "Axi Symmetric"
        eq_mgdyn_2d = ObjectsFem.makeEquationMagnetodynamic2D(doc, solver_obj)
        eq_mgdyn_2d.Frequency = "50 Hz"
        eq_mgdyn_2d.IsHarmonic = True
        eq_mgdyn_2d.CalculateElementalFields = True
        eq_mgdyn_2d.CalculateJouleHeating = True
        eq_mgdyn_2d.CalculateElectricField = True
        eq_mgdyn_2d.CalculateCurrentDensity = True
        eq_heat = ObjectsFem.makeEquationHeat(doc, solver_obj)
        eq_heat.NonlinearIterations = 10
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)


    # materials
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Conductor")
    mat = material_obj.Material
    mat["Name"] = "Conductor"
    mat["Density"] = "8960.0 kg/m^3"
    mat["ThermalConductivity"] = "398.0 W/m/K"
    mat["SpecificHeat"] = "385.0 J/kg/K" 
    mat["ElectricalConductivity"] = "1.0e7 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(shell, "Face2")]
    analysis.addObject(material_obj)

    material_obj = ObjectsFem.makeMaterialFluid(doc, "Air")
    mat = material_obj.Material
    mat["Name"] = "Air"
    mat["Density"] = "1.204 kg/m^3"
    mat["ThermalConductivity"] = "0.0259 W/m/K"
    mat["SpecificHeat"] = "1010.0 J/kg/K" 
    mat["ElectricalConductivity"] = "1.0e3 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(shell, ("Face1", "Face3"))]
    analysis.addObject(material_obj)

    # boundary conditions and loads
    # A potential vector
    outer_A = ObjectsFem.makeConstraintElectromagnetic(doc, "OuterPotential")
    outer_A.References = [(shell, "Edge9")]
    outer_A.BoundaryCondition = "Dirichlet"
    outer_A.PotentialEnabled = False
    outer_A.EnableAV_3 = True
    outer_A.AV_re_3 = "0 Wb/m"
    outer_A.AV_im_3 = "0 Wb/m"
    analysis.addObject(outer_A)

    # temperature
    outer_Temp = ObjectsFem.makeConstraintTemperature(doc, "OuterTemp")
    outer_Temp.References = [(shell, "Edge9")]
    outer_Temp.ConstraintType = "Temperature"
    outer_Temp.Temperature = "300 K"
    analysis.addObject(outer_Temp)

    # current density
    current = ObjectsFem.makeConstraintCurrentDensity(doc, "CurrentDensity")
    current.References = [(shell, "Face3")]
    current.NormalCurrentDensity_re = "10.000 A/mm^2"
    current.Mode = "Normal"
    analysis.addObject(current)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, manager.get_meshname()))[0]
    femmesh_obj.Shape = shell
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.CharacteristicLengthMax = "30 mm"
    femmesh_obj.ViewObject.Visibility = False
    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "4 mm"
    mesh_region.References = [(shell, ("Edge4", "Face2", "Face3"))]
    mesh_region.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
