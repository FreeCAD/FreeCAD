# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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
from FreeCAD import Rotation
from FreeCAD import Vector

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Electrostatics Capacitance Two Balls",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["electrostatic potential"],
        "solvers": ["elmer"],
        "material": "fluid",
        "equation": "electrostatic"
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_electrostatics_capacitance_two_balls import setup
setup()


See forum topic post:
https://forum.freecadweb.org/viewtopic.php?f=18&t=41488&start=90#p412047

Electrostatics equation in FreeCAD FEM-Elmer

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    small_sphere1 = doc.addObject("Part::Sphere", "Small_Sphere1")
    small_sphere1.Placement = FreeCAD.Placement(Vector(-1000, 0, 0), Rotation(Vector(0, 0, 1), 0))
    small_sphere1.Radius = "500 mm"

    small_sphere2 = doc.addObject("Part::Sphere", "Small_Sphere2")
    small_sphere2.Placement = FreeCAD.Placement(Vector(1000, 0, 0), Rotation(Vector(0, 0, 1), 0))
    small_sphere2.Radius = "500 mm"

    fusion = doc.addObject("Part::MultiFuse", "Fusion")
    fusion.Shapes = [small_sphere1, small_sphere2]

    large_sphere = doc.addObject("Part::Sphere", "Large_Sphere")
    large_sphere.Radius = "5000 mm"

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
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        eq_electrostatic = ObjectsFem.makeEquationElectrostatic(doc, solver_obj)
        eq_electrostatic.CalculateCapacitanceMatrix = True
        eq_electrostatic.CalculateElectricEnergy = True
        eq_electrostatic.CalculateElectricField = True
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialFluid(doc, "FemMaterial")
    mat = material_obj.Material
    mat["Name"] = "Air-Generic"
    mat["Density"] = "1.20 kg/m^3"
    mat["KinematicViscosity"] = "15.11 mm^2/s"
    mat["VolumetricThermalExpansionCoefficient"] = "0.00 mm/m/K"
    mat["ThermalConductivity"] = "0.03 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.0034/K"
    mat["SpecificHeat"] = "1.00 J/kg/K"
    mat["RelativePermittivity"] = "1.00"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint potential 1st
    name_pot1 = "ElectrostaticPotential1"
    con_elect_pot1 = ObjectsFem.makeConstraintElectrostaticPotential(doc, name_pot1)
    con_elect_pot1.References = [(geom_obj, "Face1")]
    con_elect_pot1.ElectricInfinity = True
    analysis.addObject(con_elect_pot1)

    # constraint potential 2nd
    name_pot2 = "ElectrostaticPotential2"
    con_elect_pot2 = ObjectsFem.makeConstraintElectrostaticPotential(doc, name_pot2)
    con_elect_pot2.References = [(geom_obj, "Face2")]
    con_elect_pot2.CapacitanceBody = 1
    con_elect_pot2.CapacitanceBodyEnabled = True
    analysis.addObject(con_elect_pot2)

    # constraint potential 3rd
    name_pot3 = "ElectrostaticPotential3"
    con_elect_pot3 = ObjectsFem.makeConstraintElectrostaticPotential(doc, name_pot3)
    con_elect_pot3.References = [(geom_obj, "Face3")]
    con_elect_pot3.CapacitanceBody = 2
    con_elect_pot3.CapacitanceBodyEnabled = True
    analysis.addObject(con_elect_pot3)

    # constant vacuum permittivity
    const_vacperm = ObjectsFem.makeConstantVacuumPermittivity(doc, "ConstantVacuumPermittivity")
    const_vacperm.VacuumPermittivity = "1 F/m"
    analysis.addObject(const_vacperm)

    # mesh
    from .meshes.mesh_capacitance_two_balls_tetra10 import create_nodes, create_elements
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

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "300 mm"
    mesh_region.References = [(geom_obj, "Face2"), (geom_obj, "Face3")]

    doc.recompute()
    return doc
