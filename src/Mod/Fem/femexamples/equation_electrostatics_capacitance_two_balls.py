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

import sys
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
        "equations": ["electrostatic"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_electrostatics_capacitance_two_balls import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=41488&start=90#p412047

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
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        eq_electrostatic = ObjectsFem.makeEquationElectrostatic(doc, solver_obj)
        eq_electrostatic.CalculateCapacitanceMatrix = True
        eq_electrostatic.CalculateElectricEnergy = True
        eq_electrostatic.CalculateElectricField = True
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)

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
    mat["RelativePermittivity"] = "1.00059"
    material_obj.Material = mat
    material_obj.References = [(geom_obj, "Solid1")]
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

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = "600 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "250 mm"
    mesh_region.References = [(geom_obj, "Face2"), (geom_obj, "Face3")]
    mesh_region.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools
    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    try:
        error = gmsh_mesh.create_mesh()
    except Exception:
        error = sys.exc_info()[1]
        FreeCAD.Console.PrintError(
            "Unexpected error when creating mesh: {}\n"
            .format(error)
        )
    if error:
        # try to create from existing rough mesh
        from .meshes.mesh_capacitance_two_balls_tetra10 import create_nodes, create_elements
        fem_mesh = Fem.FemMesh()
        control = create_nodes(fem_mesh)
        if not control:
            FreeCAD.Console.PrintError("Error on creating nodes.\n")
        control = create_elements(fem_mesh)
        if not control:
            FreeCAD.Console.PrintError("Error on creating elements.\n")
        femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc
