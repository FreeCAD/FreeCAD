# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

# to run the example use:
"""
from femexamples.equation_electrostatics_capacitance_two_balls import setup
setup()
"""
# Electrostatics equation in FreeCAD FEM-Elmer
# https://forum.freecadweb.org/viewtopic.php?f=18&t=41488&start=90#p412047

import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

import Fem
import ObjectsFem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Electrostatics Capacitance Two Balls",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["electrostatic potential"],
            "solvers": ["elmer"],
            "material": "fluid",
            "equation": "electrostatic"
            }
    return info


def setup(doc=None, solvertype="elmer"):
    # setup base model

    if doc is None:
        doc = init_doc()

    # geometry object
    # name is important because the other method in this module use obj name
    small_sphere1 = doc.addObject("Part::Sphere", "Small_Sphere1")
    small_sphere1.Placement = FreeCAD.Placement(Vector(-1000, 0, 0), Rotation(Vector(0, 0, 1), 0))
    small_sphere1.Radius = '500 mm'

    small_sphere2 = doc.addObject("Part::Sphere", "Small_Sphere2")
    small_sphere2.Placement = FreeCAD.Placement(Vector(1000, 0, 0), Rotation(Vector(0, 0, 1), 0))
    small_sphere2.Radius = '500 mm'

    fusion = doc.addObject("Part::MultiFuse", "Fusion")
    fusion.Shapes = [small_sphere1, small_sphere2]

    large_sphere = doc.addObject("Part::Sphere", "Large_Sphere")
    large_sphere.Radius = '5000 mm'

    geom_obj = doc.addObject("Part::Cut", "Cut")
    geom_obj.Base = large_sphere
    geom_obj.Tool = fusion
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Transparency = 75
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "elmer":
        solver_object = analysis.addObject(ObjectsFem.makeSolverElmer(doc, "SolverElmer"))[0]
        eq_electrostatic = ObjectsFem.makeEquationElectrostatic(doc, solver_object)
        eq_electrostatic.CalculateCapacitanceMatrix = True
        eq_electrostatic.CalculateElectricEnergy = True
        eq_electrostatic.CalculateElectricField = True
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialFluid(doc, "FemMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "Air-Generic"
    mat["Density"] = "1.20 kg/m^3"
    mat["KinematicViscosity"] = "15.11 mm^2/s"
    mat["VolumetricThermalExpansionCoefficient"] = "0.00 mm/m/K"
    mat["ThermalConductivity"] = "0.03 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.0034/K"
    mat["SpecificHeat"] = "1.00 J/kg/K"
    mat["RelativePermittivity"] = "1.00"
    material_object.Material = mat

    # 1st potential_constraint
    constraint_elect_pot0 = analysis.addObject(
        ObjectsFem.makeConstraintElectrostaticPotential(doc))[0]
    constraint_elect_pot0.References = [(geom_obj, "Face1")]
    constraint_elect_pot0.ElectricInfinity = True

    # 2nd potential_constraint
    constraint_elect_pot1 = analysis.addObject(
        ObjectsFem.makeConstraintElectrostaticPotential(doc))[0]
    constraint_elect_pot1.References = [(geom_obj, "Face2")]
    constraint_elect_pot1.CapacitanceBody = 1
    constraint_elect_pot1.CapacitanceBodyEnabled = True

    # 3rd potential_constraint
    constraint_elect_pot2 = analysis.addObject(
        ObjectsFem.makeConstraintElectrostaticPotential(doc))[0]
    constraint_elect_pot2.References = [(geom_obj, "Face3")]
    constraint_elect_pot2.CapacitanceBody = 2
    constraint_elect_pot2.CapacitanceBodyEnabled = True

    # constant vacuum permittivity
    const_vaccum_permittivity = analysis.addObject(
        ObjectsFem.makeConstantVacuumPermittivity(doc))[0]
    const_vaccum_permittivity.VacuumPermittivity = '1 F/m'

    # mesh
    from .meshes.mesh_capacitance_two_balls_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        ObjectsFem.makeMeshGmsh(doc, mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj)
    mesh_region.CharacteristicLength = '300 mm'
    mesh_region.References = [(geom_obj, "Face2"), (geom_obj, "Face3")]

    doc.recompute()
    return doc
