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
# to run the example use:
"""
from femexamples.equation_staticcurrent_elmer import setup
setup()

"""


import sys
import FreeCAD

import Fem
import ObjectsFem

from BasicShapes import Shapes
from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Joule heating",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["electrostatic potential", "current density"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["static current"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.equation_staticcurrent_elmer import setup
setup()

Static current equation - Elmer solver

analytical solution - temperature 351.09 K at 50 s

"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects

    # wire
    Wire = doc.addObject("Part::Cylinder", "Wire")
    Wire.Radius = "2 mm"
    Wire.Height = "60 mm"
    Wire.ViewObject.Visibility = True

    if FreeCAD.GuiUp:
        Wire.ViewObject.Document.activeView().viewAxonometric()
        Wire.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui

        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.SimulationType = "Transient"
        solver_obj.TimestepSizes = [50]
        solver_obj.TimestepIntervals = [1]
        eq_staticcurrent = ObjectsFem.makeEquationStaticCurrent(doc, solver_obj)
        eq_staticcurrent.References = [(Wire, "Solid1")]
        eq_staticcurrent.CalculateJouleHeating = True
        eq_staticcurrent.HeatSource = True
        eq_staticcurrent.Priority = 2
        eq_heat = ObjectsFem.makeEquationHeat(doc, solver_obj)
        eq_heat.References = [(Wire, "Solid1")]
        eq_heat.Priority = 1
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)

    # material

    # metal wire
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Metal for Joule heating")
    mat = material_obj.Material
    mat["Name"] = "Metal for Joule heating"
    mat["Density"] = "7850 kg/m^3"
    mat["ThermalConductivity"] = "16.0 W/m/K"
    mat["SpecificHeat"] = "490.0 J/kg/K"
    mat["ElectricalConductivity"] = "1450000 S/m"
    material_obj.Material = mat
    material_obj.References = [(Wire, "Solid1")]
    analysis.addObject(material_obj)

    # voltage on one end
    Voltage = ObjectsFem.makeConstraintElectrostaticPotential(doc, "Voltage")
    Voltage.References = [(Wire, "Face2")]
    Voltage.Potential = "0 V"
    analysis.addObject(Voltage)

    # current on other end
    Current = ObjectsFem.makeConstraintCurrentDensity(doc, "Current")
    Current.References = [(Wire, "Face3")]
    Current.Mode = "Normal"
    Current.NormalCurrentDensity_re = "2387324 A/m^2"
    analysis.addObject(Current)

    # constraint initial temperature
    con_inittemp = ObjectsFem.makeConstraintInitialTemperature(doc, "ConstraintInitialTemperature")
    con_inittemp.initialTemperature = 300.0
    analysis.addObject(con_inittemp)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = Wire
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "0.7 mm"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools

    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    try:
        error = gmsh_mesh.create_mesh()
    except Exception:
        error = sys.exc_info()[1]
        FreeCAD.Console.PrintError(f"Unexpected error when creating mesh: {error}\n")

    doc.recompute()
    return doc
