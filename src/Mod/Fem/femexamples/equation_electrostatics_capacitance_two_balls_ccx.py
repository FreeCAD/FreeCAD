# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2025 Jakub Michalski <jakub.j.michalski@gmail.com>              *
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

import sys
import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Electrostatics Capacitance Two Balls CCX",
        "meshtype": "solid",
        "meshelement": "Tet4",
        "constraints": ["electromagnetic", "electric charge density"],
        "solvers": ["calculix"],
        "material": "fluid",
        "equations": ["electrostatic"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.equation_electrostatics_capacitance_two_balls_ccx import setup
setup()


See forum topic post for similar Elmer example:
https://forum.freecad.org/viewtopic.php?f=18&t=41488&start=90#p412047

Analytical solution - capacitance 6.68e-12 F

Source of analytical solution: 
https://en.wikipedia.org/wiki/Capacitance#Capacitance_of_conductors_with_simple_shapes

To obtain capacitance in postprocessing:
- cut the sphere in half with Region Clip Filter to see the results inside
- calculate the potential difference (you can use the legend of the Potential field)
- divide the total applied charge (1e-9 C) by this potential difference

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
    small_sphere1 = doc.addObject("Part::Sphere", "Small_Sphere1")
    small_sphere1.Placement = FreeCAD.Placement(Vector(-300, 0, 0), Rotation(Vector(0, 0, 1), 0))
    small_sphere1.Radius = "100 mm"

    small_sphere2 = doc.addObject("Part::Sphere", "Small_Sphere2")
    small_sphere2.Placement = FreeCAD.Placement(Vector(300, 0, 0), Rotation(Vector(0, 0, 1), 0))
    small_sphere2.Radius = "100 mm"

    fusion = doc.addObject("Part::MultiFuse", "Fusion")
    fusion.Shapes = [small_sphere1, small_sphere2]

    large_sphere = doc.addObject("Part::Sphere", "Large_Sphere")
    large_sphere.Radius = "1000 mm"

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
    if FreeCAD.GuiUp:
        import FemGui

        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculiX(doc, "SolverCalculiX")
        solver_obj.AnalysisType = "electromagnetic"
        solver_obj.ElectromagneticMode = "electrostatic"
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # material
    material_obj = ObjectsFem.makeMaterialFluid(doc, "Air")
    mat = material_obj.Material
    mat["Name"] = "Air"
    mat["Density"] = "1.204 kg/m^3"
    mat["ThermalConductivity"] = "0.02587 W/m/K"
    mat["ThermalExpansionCoefficient"] = "3.43e-3 1/K"
    mat["SpecificHeat"] = "1.01 kJ/kg/K"
    mat["ElectricalConductivity"] = "1e-12 S/m"
    mat["RelativePermeability"] = "1.0"
    # mat["RelativePermittivity"] = "1.00059"
    mat["RelativePermittivity"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(geom_obj, "Solid1")]
    analysis.addObject(material_obj)

    # constraint potential large sphere
    name_pot1 = "ElectrostaticPotential1"
    con_elect_pot1 = ObjectsFem.makeConstraintElectromagnetic(doc, name_pot1)
    con_elect_pot1.References = [(geom_obj, "Face1")]
    con_elect_pot1.Potential = "0.0 mV"
    analysis.addObject(con_elect_pot1)

    # constraint electric charge density small sphere 1
    name_ch1 = "ElectricChargeDensity1"
    con_elect_charge_dens1 = ObjectsFem.makeConstraintElectricChargeDensity(doc, name_ch1)
    con_elect_charge_dens1.References = [(geom_obj, "Face2")]
    con_elect_charge_dens1.Mode = "Interface"
    con_elect_charge_dens1.InterfaceChargeDensity = "7.94e-9 C/m^2"
    analysis.addObject(con_elect_charge_dens1)

    # constraint electric charge density small sphere 2
    name_ch2 = "ElectricChargeDensity2"
    con_elect_charge_dens2 = ObjectsFem.makeConstraintElectricChargeDensity(doc, name_ch2)
    con_elect_charge_dens2.References = [(geom_obj, "Face3")]
    con_elect_charge_dens2.Mode = "Interface"
    con_elect_charge_dens2.InterfaceChargeDensity = "-7.94e-9 C/m^2"
    analysis.addObject(con_elect_charge_dens2)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = "50 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "20 mm"
    mesh_region.References = [(geom_obj, "Face2"), (geom_obj, "Face3")]
    mesh_region.ViewObject.Visibility = False

    # generate the mesh
    success = generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")
    if not success:
        # try to create from existing rough mesh
        from .meshes.mesh_capacitance_two_balls_tetra10 import (
            create_nodes,
            create_elements,
        )

        fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
        femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc

