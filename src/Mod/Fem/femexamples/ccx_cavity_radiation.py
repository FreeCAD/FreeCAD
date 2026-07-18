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


import FreeCAD as App

import Part
import ObjectsFem
import Materials

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Radiation between two parallel surfaces",
        "meshtype": "face",
        "meshelement": "Tria3",
        "constraints": ["temperature", "heat flux", "initial temperature", "section print"],
        "solvers": ["calculix"],
        "material": "solid",
        "equations": ["mechanical"],
    }

def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_cavity_radiation import setup
setup()

Theory and example from: https://web.mit.edu/16.unified/www/FALL/thermodynamics/notes/node136.html

Analytical solution - heat flux 6.9 W/m^2 = 6.9e-3 mW/mm^2

Special approaches required:
- long transient instead of steady-state analysis
- negative ambient temperature to specify that the cavity is closed
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
    l1 = Part.makeLine((0, 24, 0), (100, 24, 0))
    l2 = Part.makeLine((100, 24, 0), (100, 4, 0))
    l3 = Part.makeLine((100, 4, 0), (0, 4, 0))
    l4 = Part.makeLine((0, 4, 0), (0, 24, 0))
    l5 = Part.makeLine((0, -4, 0), (100, -4, 0))
    l6 = Part.makeLine((100, -4, 0), (100, -24, 0))
    l7 = Part.makeLine((100, -24, 0), (0, -24, 0))
    l8 = Part.makeLine((0, -24, 0), (0, -4, 0))    
    wire1 = Part.Wire([l1, l2, l3, l4])
    wire2 = Part.Wire([l5, l6, l7, l8])
    face1 = Part.Face(wire1)
    face2 = Part.Face(wire2)
    compound = Part.makeCompound([face1, face2])
    compound.reverse()
    compound_obj = Part.show(compound)
    doc.recompute()
    if App.GuiUp:
        compound_obj.ViewObject.Document.activeView().viewTop()
        compound_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculiX(doc, "SolverCalculiX")
        solver_obj.AnalysisType = "thermomech"
        solver_obj.ThermoMechType = "pure heat transfer"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.ModelSpace = "plane stress"
        solver_obj.OutputFrequency = 100000
        solver_obj.IncrementsMaximum = 2000000
        solver_obj.TimeInitialIncrement = 10000
        solver_obj.TimeMaximumIncrement = 0
        solver_obj.TimeMinimumIncrement = 0
        solver_obj.TimePeriod = 5000000
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # 2D element thickness
    thickness_obj = ObjectsFem.makeElementGeometry2D(doc, 1.0, "ShellThickness")
    analysis.addObject(thickness_obj)

    # material
    mat_manager = Materials.MaterialManager()
    steel = mat_manager.getMaterial("92589471-a6cb-4bbc-b748-d425a17dea7d")
    steel_obj = ObjectsFem.makeMaterialSolid(doc, "Material")
    steel_obj.UUID = steel.UUID
    steel_obj.Material = steel.Properties
    analysis.addObject(steel_obj)

    # constraint initial temperature
    con_inittemp = ObjectsFem.makeConstraintInitialTemperature(doc, "InitialTemperature")
    con_inittemp.InitialTemperature = 300.0
    analysis.addObject(con_inittemp)

    # constraint temperature
    con_temp1 = ObjectsFem.makeConstraintTemperature(doc, "TemperatureHot")
    con_temp1.References = [(compound_obj, "Edge1")]
    con_temp1.Temperature = 373.0
    con_temp1.ConcentratedHeatFlux = 0.0
    analysis.addObject(con_temp1)

    con_temp2 = ObjectsFem.makeConstraintTemperature(doc, "TemperatureCold")
    con_temp2.References = [(compound_obj, "Edge7")]
    con_temp2.Temperature = 293.0
    con_temp2.ConcentratedHeatFlux = 0.0
    analysis.addObject(con_temp2)
    
    # heat flux
    con_hf1 = ObjectsFem.makeConstraintHeatflux(doc, "RadiationHot")
    con_hf1.References = [(compound_obj, "Edge3")]
    con_hf1.ConstraintType = "Radiation"
    con_hf1.AmbientTemp = -300
    con_hf1.Emissivity = 0.02
    con_hf1.CavityRadiation = True
    con_hf1.CavityName = "cav"
    analysis.addObject(con_hf1)
    
    con_hf2 = ObjectsFem.makeConstraintHeatflux(doc, "RadiationCold")
    con_hf2.References = [(compound_obj, "Edge5")]
    con_hf2.ConstraintType = "Radiation"
    con_hf2.AmbientTemp = -300
    con_hf2.Emissivity = 0.02
    con_hf2.CavityRadiation = True
    con_hf2.CavityName = "cav"
    analysis.addObject(con_hf2)
    
    # section print
    con_sectionpr1 = ObjectsFem.makeConstraintSectionPrint(doc, "SectionPrintHot")
    con_sectionpr1.References = [(compound_obj, "Edge3")]
    con_sectionpr1.Variable = "Heat Flux" 
    analysis.addObject(con_sectionpr1)
    
    con_sectionpr2 = ObjectsFem.makeConstraintSectionPrint(doc, "SectionPrintCold")
    con_sectionpr2.References = [(compound_obj, "Edge5")]
    con_sectionpr2.Variable = "Heat Flux" 
    analysis.addObject(con_sectionpr2)   

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = compound_obj
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.CharacteristicLengthMax = "1 mm"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")
        
    doc.recompute()
    return doc
