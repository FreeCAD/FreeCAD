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

import FreeCAD

import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Heat conduction with radiation",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["temperature", "heat flux"],
        "solvers": ["calculix"],
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_radiation_benchmark import setup
setup()

Based on standard NAFEMS benchmark T2

Analytical solution - 927 K at the end with radiation heat flux

"""
    )


def setup(doc=None, solvertype="calculix"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    cylinder = doc.addObject("Part::Cylinder", "Cylinder")
    cylinder.Radius = "5,00 mm"
    cylinder.Height = "100,00 mm"
    doc.recompute()
    if FreeCAD.GuiUp:
        cylinder.ViewObject.Document.activeView().viewAxonometric()
        cylinder.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculiX(doc, "SolverCalculiX")
        solver_obj.AnalysisType = "thermomech"
        solver_obj.ThermoMechType = "pure heat transfer"
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc

    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel"
    mat["ThermalConductivity"] = "55.6 W/m/K"
    mat["YoungsModulus"] = "210 GPa"
    mat["PoissonRatio"] = "0.3"
    mat["SpecificHeat"] = "460 J/kg/K"
    mat["ThermalExpansionCoefficient"] = "11e-6 m/m/K"
    mat["ThermalExpansionReferenceTemperature"] = "273 K"
    mat["Density"] = "8000.0 kg/m^3"
    material_obj.Material = mat
    material_obj.References = [(cylinder, "Solid1")]
    analysis.addObject(material_obj)
    
    # initial temperature
    con_inittemp = ObjectsFem.makeConstraintInitialTemperature(doc, "InitialTemperature")
    con_inittemp.InitialTemperature = 273.0
    analysis.addObject(con_inittemp)

    # temperature
    con_temp = ObjectsFem.makeConstraintTemperature(doc, "Temperature")
    con_temp.References = [(cylinder, "Face3")]
    con_temp.Temperature = 1000.0
    con_temp.ConcentratedHeatFlux = 0.0
    analysis.addObject(con_temp)

    # heat flux
    con_hf = ObjectsFem.makeConstraintHeatflux(doc, "Radiation")
    con_hf.References = [(cylinder, "Face2")]
    con_hf.ConstraintType = "Radiation"
    con_hf.AmbientTemp = 300
    con_hf.Emissivity = 0.98
    analysis.addObject(con_hf)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = cylinder
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "1.5 mm"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
