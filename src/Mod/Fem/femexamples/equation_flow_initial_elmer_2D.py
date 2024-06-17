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
from FreeCAD import Placement
from FreeCAD import Rotation
from FreeCAD import Vector

import Draft
import ObjectsFem

from BOPTools import SplitFeatures
from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Initial Flow - Elmer 2D",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["initial pressure", "initial temperature", "initial velocity",
                        "temperature", "velocity"],
        "solvers": ["elmer"],
        "material": "fluid",
        "equations": ["flow", "heat"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_flow_initial_elmer_2D import setup
setup()

Flow and Heat equation with initial velocity - Elmer solver

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects

    # the wire defining the pipe volume in 2D
    p1 = Vector(400, -50.000, 0)
    p2 = Vector(400, -150.000, 0)
    p3 = Vector(1200, -150.000, 0)
    p4 = Vector(1200, 50.000, 0)
    p5 = Vector(0, 50.000, 0)
    p6 = Vector(0, -50.000, 0)
    wire = Draft.make_wire([p1, p2, p3, p4, p5, p6], closed=True)
    wire.MakeFace = True
    wire.Label = "Wire"

    # the circle defining the heating rod
    pCirc = Vector(160, 0, 0)
    axisCirc = Vector(1, 0, 0)
    placementCircle = Placement(pCirc, Rotation(axisCirc, 0))
    circle = Draft.make_circle(10, placement=placementCircle)
    circle.MakeFace = True
    circle.Label = "HeatingRod"
    circle.ViewObject.Visibility = False

    # a link of the circle
    circleLink = doc.addObject("App::Link", "Link-HeatingRod")
    circleLink.LinkTransform = True
    circleLink.LinkedObject = circle

    # cut rod from wire to get volume of fluid
    cut = doc.addObject("Part::Cut", "Cut")
    cut.Base = wire
    cut.Tool = circleLink
    cut.ViewObject.Visibility = False

    # BooleanFregments object to combine cut with rod
    BooleanFragments = SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    BooleanFragments.Objects = [cut, circle]

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        BooleanFragments.ViewObject.Transparency = 50
        BooleanFragments.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

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
    equation_flow.NonlinearIterations = 20
    equation_flow.NonlinearNewtonAfterIterations = 20
    equation_flow.RelaxationFactor = 0.15
    equation_flow.Variable = "Flow Solution[Velocity:2 Pressure:1]"
    equation_heat.Convection = "Computed"
    equation_heat.IdrsParameter = 3
    equation_heat.LinearIterativeMethod = "Idrs"
    equation_heat.LinearPreconditioning = "ILU1"
    equation_heat.NonlinearIterations = 20
    equation_heat.NonlinearNewtonAfterIterations = 20
    equation_heat.Priority = 5
    equation_heat.RelaxationFactor = 0.15
    equation_heat.Stabilize = True

    # material

    # fluid
    material_obj = ObjectsFem.makeMaterialFluid(doc, "Material_Fluid")
    mat = material_obj.Material
    mat["Name"] = "Carbon dioxide"
    mat["Density"] = "1.8393 kg/m^3"
    mat["DynamicViscosity"] = "14.7e-6 kg/m/s"
    mat["ThermalConductivity"] = "0.016242 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.00343 m/m/K"
    mat["SpecificHeat"] = "0.846 kJ/kg/K"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Face2")]
    analysis.addObject(material_obj)

    # tube wall
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material_Wall")
    mat = material_obj.Material
    mat["Name"] = "Aluminum Generic"
    mat["Density"] = "2700 kg/m^3"
    mat["PoissonRatio"] = "0.35"
    mat["ShearModulus"] = "25.0 GPa"
    mat["UltimateTensileStrength"] = "310 MPa"
    mat["YoungsModulus"] = "70000 MPa"
    mat["ThermalConductivity"] = "237.0 W/m/K"
    mat["ThermalExpansionCoefficient"] = "23.1 µm/m/K"
    mat["SpecificHeat"] = "897.0 J/kg/K"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Face1")]
    analysis.addObject(material_obj)

    # constraint inlet velocity
    FlowVelocity_Inlet = ObjectsFem.makeConstraintFlowVelocity(doc, "FlowVelocity_Inlet")
    FlowVelocity_Inlet.References = [(BooleanFragments, "Edge5")]
    FlowVelocity_Inlet.VelocityX = "20.0 mm/s"
    FlowVelocity_Inlet.VelocityXUnspecified = False
    analysis.addObject(FlowVelocity_Inlet)

    # constraint wall velocity
    FlowVelocity_Wall = ObjectsFem.makeConstraintFlowVelocity(doc, "FlowVelocity_Wall")
    FlowVelocity_Wall.References = [
        (BooleanFragments, "Edge2"),
        (BooleanFragments, "Edge3"),
        (BooleanFragments, "Edge4"),
        (BooleanFragments, "Edge7")]
    FlowVelocity_Wall.VelocityXUnspecified = False
    FlowVelocity_Wall.VelocityYUnspecified = False
    analysis.addObject(FlowVelocity_Wall)

    # constraint initial velocity
    FlowVelocity_Initial = ObjectsFem.makeConstraintInitialFlowVelocity(doc, "FlowVelocity_Initial")
    FlowVelocity_Initial.References = [(BooleanFragments, "Face2")]
    FlowVelocity_Initial.VelocityX = "20.0 mm/s"
    FlowVelocity_Initial.VelocityY = "-20.0 mm/s"
    FlowVelocity_Initial.VelocityXUnspecified = False
    FlowVelocity_Initial.VelocityYUnspecified = False
    analysis.addObject(FlowVelocity_Initial)

    # constraint initial temperature
    Temperature_Initial = ObjectsFem.makeConstraintInitialTemperature(doc, "Temperature_Initial")
    Temperature_Initial.initialTemperature = 300.0
    analysis.addObject(Temperature_Initial)

    # constraint wall temperature
    Temperature_Wall = ObjectsFem.makeConstraintTemperature(doc, "Temperature_Wall")
    Temperature_Wall.Temperature = 300.0
    Temperature_Wall.NormalDirection = Vector(0, 0, -1)
    Temperature_Wall.References = [
        (BooleanFragments, "Edge2"),
        (BooleanFragments, "Edge3"),
        (BooleanFragments, "Edge4"),
        (BooleanFragments, "Edge7")]
    analysis.addObject(Temperature_Wall)

    # constraint inlet temperature
    Temperature_Inlet = ObjectsFem.makeConstraintTemperature(doc, "Temperature_Inlet")
    Temperature_Inlet.Temperature = 300.0
    Temperature_Inlet.NormalDirection = Vector(-1, 0, 0)
    Temperature_Inlet.References = [(BooleanFragments, "Edge5")]
    analysis.addObject(Temperature_Inlet)

    # constraint heating rod temperature
    Temperature_HeatingRod = ObjectsFem.makeConstraintTemperature(doc, "Temperature_HeatingRod")
    Temperature_HeatingRod.Temperature = 373.0
    Temperature_HeatingRod.NormalDirection = Vector(0, -1, 0)
    Temperature_HeatingRod.References = [(BooleanFragments, "Edge1")]
    analysis.addObject(Temperature_HeatingRod)

    # constraint initial pressure
    Pressure_Initial = ObjectsFem.makeConstraintInitialPressure(doc, "Pressure_Initial")
    Pressure_Initial.Pressure = "100.0 kPa"
    Pressure_Initial.NormalDirection = Vector(0, -1, 0)
    Pressure_Initial.References = [(BooleanFragments, "Face2")]
    analysis.addObject(Pressure_Initial)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = BooleanFragments
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.CharacteristicLengthMax = "4 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "2 mm"
    mesh_region.References = [
        (BooleanFragments, "Edge1"),
        (BooleanFragments, "Vertex2"),
        (BooleanFragments, "Vertex4"),
        (BooleanFragments, "Vertex6")]
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

    doc.recompute()
    return doc
