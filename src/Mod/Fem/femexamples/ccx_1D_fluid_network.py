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

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Basic 1D Fluid Network",
        "meshtype": "edge",
        "meshelement": "Seg3",
        "constraints": ["self weight"],
        "solvers": ["ccxtools"],
        "material": "fluid",
        "equations": ["flow"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_1D_fluid_network import setup
setup()

Based on the artery1.inp example from the CalculiX documentation

Analytical solution - outlet pressure approx. 0.11 MPa

"""
    )


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    line1 = Part.makeLine((0,11,0),(0,10,0))
    line2 = Part.makeLine((0,10,0),(0,0,0))
    line3 = Part.makeLine((0,0,0),(0,-1,0))
    geom_obj = Part.Wire([line1,line2,line3])
    wireo = Part.show(geom_obj)
    doc.recompute()
    if App.GuiUp:
        wireo.ViewObject.Document.activeView().viewTop()
        wireo.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver,
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.WorkingDir = ""
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "thermomech"
        solver_obj.ThermoMechType = "pure heat transfer"
        analysis.addObject(solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # 1D element sections
    inlet_obj = ObjectsFem.makeElementFluid1D(doc, "Inlet_section")
    inlet_obj.SectionType = "Liquid"
    inlet_obj.LiquidSectionType = "PIPE INLET"
    inlet_obj.InletFlowRateActive = True
    inlet_obj.InletFlowRate = 0.011962
    inlet_obj.InletPressureActive = True
    inlet_obj.InletPressure = 0.13
    inlet_obj.References = [(wireo, "Edge1")]
    analysis.addObject(inlet_obj)
    
    pipe_obj = ObjectsFem.makeElementFluid1D(doc, "Pipe_section")
    pipe_obj.SectionType = "Liquid"
    pipe_obj.LiquidSectionType = "PIPE MANNING"
    pipe_obj.ManningArea = "12.566 mm^2"
    pipe_obj.ManningCoefficient = 0.015
    pipe_obj.ManningRadius = "1.00 mm"
    pipe_obj.References = [(wireo, "Edge2")]
    analysis.addObject(pipe_obj)
    
    outlet_obj = ObjectsFem.makeElementFluid1D(doc, "Outlet_section")
    outlet_obj.SectionType = "Liquid"
    outlet_obj.LiquidSectionType = "PIPE OUTLET"
    outlet_obj.OutletFlowRateActive = False
    outlet_obj.OutletPressureActive = False
    outlet_obj.References = [(wireo, "Edge3")]
    analysis.addObject(outlet_obj)    

    # material
    material_obj = ObjectsFem.makeMaterialFluid(doc, "FluidMaterial")
    mat = material_obj.Material
    mat["Name"] = "Water"
    mat["Density"] = "1000 kg/m^3"
    mat["KinematicViscosity"] = "1.75 mm^2/s"
    mat["ThermalConductivity"] = "0.59 W/m/K"
    mat["SpecificHeat"] = "4217 J/kg/K"
    material_obj.Material = mat
#    material_obj.References = [(wireo, "Edge1", "Edge2", "Edge3")]
    analysis.addObject(material_obj)

    # constraint selfweight
    con_selfweight = ObjectsFem.makeConstraintSelfWeight(doc, "ConstraintSelfWeight")
    con_selfweight.GravityDirection = (-1, 0, 0)
    con_selfweight.GravityAcceleration = "9810 mm/s^2"
    analysis.addObject(con_selfweight)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = wireo
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
