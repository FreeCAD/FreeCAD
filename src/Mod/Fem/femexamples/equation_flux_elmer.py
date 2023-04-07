# ***************************************************************************
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
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
from FreeCAD import Vector

import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Flux - Elmer",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["electrostatic potential", "temperature"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["electrostatic", "flux", "heat"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_flux_elmer import setup
setup()

Potential flux and heat flux - Elmer solver

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    cube = doc.addObject("Part::Box", "Cube")

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        cube.ViewObject.Transparency = 50
        cube.ViewObject.Document.activeView().viewAxonometric()
        cube.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        ObjectsFem.makeEquationElectrostatic(doc, solver_obj)
        ObjectsFem.makeEquationHeat(doc, solver_obj)
        equation_flux_potential = ObjectsFem.makeEquationFlux(doc, solver_obj)
        equation_flux_temperature = ObjectsFem.makeEquationFlux(doc, solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc
    analysis.addObject(solver_obj)

    # solver settings
    equation_flux_potential.Label = "Flux_Potential"
    equation_flux_potential.FluxCoefficient = "None"
    equation_flux_potential.FluxVariable = "Potential"
    equation_flux_temperature.Label = "Flux_Temperature"

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Graphite")
    mat = material_obj.Material
    mat["Name"] = "Graphite"
    mat["Density"] = "1750.0 kg/m^3"
    mat["PoissonRatio"] = "0.20"
    mat["ThermalConductivity"] = "96 W/m/K"
    mat["ThermalExpansionCoefficient"] = "8 µm/m/K"
    mat["SpecificHeat"] = "720.00 J/kg/K"
    mat["RelativePermittivity"] = "0.7"
    mat["ElectricalConductivity"] = "2e4 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(cube, "Solid1")]
    analysis.addObject(material_obj)

    # constraint potential 1V
    Potential_1V = ObjectsFem.makeConstraintElectrostaticPotential(doc, "Potential_1V")
    Potential_1V.Potential = "1 V"
    Potential_1V.NormalDirection = Vector(0, -1, 0)
    Potential_1V.References = [(cube, "Face3")]
    analysis.addObject(Potential_1V)

    # constraint potential 0V
    Potential_0V = ObjectsFem.makeConstraintElectrostaticPotential(doc, "Potential_0V")
    Potential_0V.Potential = "0 V"
    Potential_0V.NormalDirection = Vector(0, 0, 0)
    Potential_0V.References = [(cube, "Face4")]
    analysis.addObject(Potential_0V)

    # constraint wall temperature
    Temperature_300K = ObjectsFem.makeConstraintTemperature(doc, "Temperature_300K")
    Temperature_300K.Temperature = 300.0
    Temperature_300K.NormalDirection = Vector(-1, 0, 0)
    Temperature_300K.References = [(cube, "Face1")]
    analysis.addObject(Temperature_300K)

    # constraint inlet temperature
    Temperature_600K = ObjectsFem.makeConstraintTemperature(doc, "Temperature_600K")
    Temperature_600K.Temperature = 600.0
    Temperature_600K.NormalDirection = Vector(1, 0, 0)
    Temperature_600K.References = [(cube, "Face2")]
    analysis.addObject(Temperature_600K)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = cube
    femmesh_obj.CharacteristicLengthMax = "1 mm"
    femmesh_obj.ViewObject.Visibility = False

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

    doc.recompute()
    return doc
