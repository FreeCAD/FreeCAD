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

import FreeCAD

import ObjectsFem
import Materials
import Part

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Flow - Elmer 2D",
        "meshtype": "face",
        "meshelement": "Tria3",
        "constraints": [
            "initial pressure",
            "initial temperature",
            "temperature",
            "velocity",
        ],
        "solvers": ["elmer"],
        "material": "fluid",
        "equations": ["flow", "heat"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.equation_flow_elmer_2D import setup
setup()

Flow and Heat equation - Elmer solver

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

    # the wire defining the pipe volume in 2D
    p1 = FreeCAD.Vector(400, -50.000, 0)
    p2 = FreeCAD.Vector(400, -150.000, 0)
    p3 = FreeCAD.Vector(1200, -150.000, 0)
    p4 = FreeCAD.Vector(1200, 50.000, 0)
    p5 = FreeCAD.Vector(0, 50.000, 0)
    p6 = FreeCAD.Vector(0, -50.000, 0)
    wire = Part.makePolygon([p1, p2, p3, p4, p5, p6, p1])
    circ_center = FreeCAD.Vector(160, 0, 0)
    circle = Part.makeCircle(10, circ_center)

    f1 = Part.makeFace([circle])
    f2 = Part.makeFace([wire, circle])

    shape = Part.makeShell([f1, f2])

    shell = doc.addObject("Part::Feature", "Shell")
    shell.Shape = shape

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.CoordinateSystem = "Cartesian 2D"
        equation_flow = ObjectsFem.makeEquationFlow(doc, solver_obj)
        equation_heat = ObjectsFem.makeEquationHeat(doc, solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc
    analysis.addObject(solver_obj)

    # solver settings
    equation_flow.IdrsParameter = 3
    equation_flow.LinearIterativeMethod = "Idrs"
    equation_flow.LinearPreconditioning = "ILU1"
    equation_flow.Variable = "Flow Solution[Velocity:2 Pressure:1]"
    equation_heat.Convection = "Computed"
    equation_heat.IdrsParameter = 3
    equation_heat.LinearIterativeMethod = "Idrs"
    equation_heat.LinearPreconditioning = "ILU1"
    equation_heat.Priority = 5
    equation_heat.Stabilize = True

    # material
    mat_manager = Materials.MaterialManager()

    # fluid - air
    air = mat_manager.getMaterial("94370b96-c97e-4a3f-83b2-11d7461f7da7")
    air_obj = ObjectsFem.makeMaterialFluid(doc, "Material_Fluid")
    air_obj.UUID = air.UUID
    air_obj.Material = air.Properties
    air_obj.References = [(shell, "Face2")]
    analysis.addObject(air_obj)

    # tube wall - aluminium generic
    alum = mat_manager.getMaterial("9bf060e9-1663-44a2-88e2-2ff6ee858efe")
    alum_obj = ObjectsFem.makeMaterialSolid(doc, "Material_Wall")
    alum_obj.UUID = alum.UUID
    alum_obj.Material = alum.Properties
    alum_obj.References = [(shell, "Face1")]
    analysis.addObject(alum_obj)

    inlet_refs = (shell, "Edge6")
    walls_refs = (shell, ("Edge2", "Edge3", "Edge5", "Edge7"))

    # constraint inlet velocity
    FlowVelocity_Inlet = ObjectsFem.makeConstraintFlowVelocity(doc, "FlowVelocity_Inlet")
    FlowVelocity_Inlet.References = [inlet_refs]
    FlowVelocity_Inlet.VelocityXFormula = (
        'Variable Coordinate 2; Real MATC "10*(tx+50e-3)*(50e-3-tx)"'
    )
    FlowVelocity_Inlet.VelocityXUnspecified = False
    FlowVelocity_Inlet.VelocityXHasFormula = True
    FlowVelocity_Inlet.VelocityYUnspecified = False
    analysis.addObject(FlowVelocity_Inlet)

    # constraint wall velocity
    FlowVelocity_Wall = ObjectsFem.makeConstraintFlowVelocity(doc, "FlowVelocity_Wall")
    FlowVelocity_Wall.References = [walls_refs]
    FlowVelocity_Wall.VelocityXUnspecified = False
    FlowVelocity_Wall.VelocityYUnspecified = False
    analysis.addObject(FlowVelocity_Wall)

    # constraint initial temperature
    Temperature_Initial = ObjectsFem.makeConstraintInitialTemperature(doc, "Temperature_Initial")
    Temperature_Initial.InitialTemperature = "300.0 K"
    analysis.addObject(Temperature_Initial)

    # constraint wall temperature
    Temperature_Wall = ObjectsFem.makeConstraintTemperature(doc, "Temperature_Wall")
    Temperature_Wall.Temperature = "300.0 K"
    Temperature_Wall.References = [walls_refs]
    analysis.addObject(Temperature_Wall)

    # constraint inlet temperature
    Temperature_Inlet = ObjectsFem.makeConstraintTemperature(doc, "Temperature_Inlet")
    Temperature_Inlet.Temperature = "300.0 K"
    Temperature_Inlet.References = [inlet_refs]
    analysis.addObject(Temperature_Inlet)

    # constraint heating rod temperature
    Temperature_HeatingRod = ObjectsFem.makeConstraintTemperature(doc, "Temperature_HeatingRod")
    Temperature_HeatingRod.Temperature = "373.0 K"
    Temperature_HeatingRod.References = [(shell, "Edge1")]
    analysis.addObject(Temperature_HeatingRod)

    # constraint initial pressure
    Pressure_Initial = ObjectsFem.makeConstraintInitialPressure(doc, "Pressure_Initial")
    Pressure_Initial.Pressure = "100.0 kPa"
    Pressure_Initial.References = [(shell, "Face2")]
    analysis.addObject(Pressure_Initial)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = shell
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.CharacteristicLengthMax = "4 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "2 mm"
    mesh_region.References = [
        (shell, ("Edge1", "Vertex2", "Vertex6", "Vertex7")),
    ]
    mesh_region.ViewObject.Visibility = False

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        import FemGui
        shell.ViewObject.Transparency = 50
        shell.ViewObject.Document.activeView().fitAll()
        FemGui.setActiveAnalysis(analysis)

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
