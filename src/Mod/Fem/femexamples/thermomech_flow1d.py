# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com    *
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
from FreeCAD import Vector as vec

from Draft import makeWire

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Thermomech Flow 1D",
        "meshtype": "edge",
        "meshelement": "Seg3",
        "constraints": ["self weight"],
        "solvers": ["calculix", "ccxtools"],
        "material": "fluid",
        "equation": "thermomechanical"
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.thermomech_flow1d import setup
setup()


See forum topic post:
https://forum.freecadweb.org/viewtopic.php?f=18&t=20076


"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    p1 = vec(0, 0, 50)
    p2 = vec(0, 0, -50)
    p3 = vec(0, 0, -4300)
    p4 = vec(4950, 0, -4300)
    p5 = vec(5000, 0, -4300)
    p6 = vec(8535.53, 0, -7835.53)
    p7 = vec(8569.88, 0, -7870.88)
    p8 = vec(12105.41, 0, -11406.41)
    p9 = vec(12140.76, 0, -11441.76)
    p10 = vec(13908.53, 0, -13209.53)
    p11 = vec(13943.88, 0, -13244.88)
    p12 = vec(15046.97, 0, -14347.97)
    p13 = vec(15046.97, 0, -7947.97)
    p14 = vec(15046.97, 0, -7847.97)
    p15 = vec(0, 0, 0)
    p16 = vec(0, 0, -2175)
    p17 = vec(2475, 0, -4300)
    p18 = vec(4975, 0, -4300)
    p19 = vec(6767.765, 0, -6067.765)
    p20 = vec(8552.705, 0, -7853.205)
    p21 = vec(10337.645, 0, -9638.645)
    p22 = vec(12123.085, 0, -11424.085)
    p23 = vec(13024.645, 0, -12325.645)
    p24 = vec(13926.205, 0, -13227.205)
    p25 = vec(14495.425, 0, -13796.425)
    p26 = vec(15046.97, 0, -11147.97)
    p27 = vec(15046.97, 0, -7897.97)
    points = [
        p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
        p11, p12, p13, p14, p15, p16, p17, p18, p19, p20,
        p21, p22, p23, p24, p25, p26, p27
    ]
    geom_obj = makeWire(
        points,
        closed=False,
        face=False,
        support=None
    )
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver_obj = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        solver_obj.WorkingDir = u""
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "thermomech"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = True
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsThermoMechMaximum = 2000
        solver_obj.IterationsControlParameterTimeUse = False
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialFluid(doc, "FluidMaterial")
    mat = material_obj.Material
    mat["Name"] = "Water"
    mat["Density"] = "998 kg/m^3"
    mat["SpecificHeat"] = "4.182 J/kg/K"
    mat["DynamicViscosity"] = "1.003e-3 kg/m/s"
    mat["VolumetricThermalExpansionCoefficient"] = "2.07e-4 m/m/K"
    mat["ThermalConductivity"] = "0.591 W/m/K"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    inlet = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    inlet.SectionType = "Liquid"
    inlet.LiquidSectionType = "PIPE INLET"
    inlet.InletPressure = 0.1
    inlet.References = [(geom_obj, "Edge1")]
    analysis.addObject(inlet)

    entrance = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    entrance.SectionType = "Liquid"
    entrance.LiquidSectionType = "PIPE ENTRANCE"
    entrance.EntrancePipeArea = 31416.00
    entrance.EntranceArea = 25133.00
    entrance.References = [(geom_obj, "Edge2")]
    analysis.addObject(entrance)

    manning1 = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    manning1.SectionType = "Liquid"
    manning1.LiquidSectionType = "PIPE MANNING"
    manning1.ManningArea = 31416
    manning1.ManningRadius = 50
    manning1.ManningCoefficient = 0.002
    manning1.References = [(geom_obj, "Edge3"), (geom_obj, "Edge5")]
    analysis.addObject(manning1)

    bend = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    bend.SectionType = "Liquid"
    bend.LiquidSectionType = "PIPE BEND"
    bend.BendPipeArea = 31416
    bend.BendRadiusDiameter = 1.5
    bend.BendAngle = 45
    bend.BendLossCoefficient = 0.4
    bend.References = [(geom_obj, "Edge4")]
    analysis.addObject(bend)

    enlargement1 = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    enlargement1.SectionType = "Liquid"
    enlargement1.LiquidSectionType = "PIPE ENLARGEMENT"
    enlargement1.EnlargeArea1 = 31416.00
    enlargement1.EnlargeArea2 = 70686.00
    enlargement1.References = [(geom_obj, "Edge6")]
    analysis.addObject(enlargement1)

    manning2 = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    manning2.SectionType = "Liquid"
    manning2.LiquidSectionType = "PIPE MANNING"
    manning2.ManningArea = 70686.00
    manning2.ManningRadius = 75
    manning2.ManningCoefficient = 0.002
    manning2.References = [(geom_obj, "Edge7")]
    analysis.addObject(manning2)

    contraction = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    contraction.SectionType = "Liquid"
    contraction.LiquidSectionType = "PIPE CONTRACTION"
    contraction.ContractArea1 = 70686
    contraction.ContractArea2 = 17671
    contraction.References = [(geom_obj, "Edge8")]
    analysis.addObject(contraction)

    manning3 = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    manning3.SectionType = "Liquid"
    manning3.LiquidSectionType = "PIPE MANNING"
    manning3.ManningArea = 17671.00
    manning3.ManningRadius = 37.5
    manning3.ManningCoefficient = 0.002
    manning3.References = [(geom_obj, "Edge11"), (geom_obj, "Edge9")]
    analysis.addObject(manning3)

    gate_valve = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    gate_valve.SectionType = "Liquid"
    gate_valve.LiquidSectionType = "PIPE GATE VALVE"
    gate_valve.GateValvePipeArea = 17671
    gate_valve.GateValveClosingCoeff = 0.5
    gate_valve.References = [(geom_obj, "Edge10")]
    analysis.addObject(gate_valve)

    enlargement2 = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    enlargement2.SectionType = "Liquid"
    enlargement2.LiquidSectionType = "PIPE ENLARGEMENT"
    enlargement2.EnlargeArea1 = 17671
    enlargement2.EnlargeArea2 = 1e12
    enlargement2.References = [(geom_obj, "Edge12")]
    analysis.addObject(enlargement2)

    outlet = ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    outlet.SectionType = "Liquid"
    outlet.LiquidSectionType = "PIPE OUTLET"
    outlet.OutletPressure = 0.1
    outlet.References = [(geom_obj, "Edge13")]
    analysis.addObject(outlet)

    # self_weight_constraint
    self_weight = ObjectsFem.makeConstraintSelfWeight(doc, "ConstraintSelfWeight")
    self_weight.Gravity_x = 0.0
    self_weight.Gravity_y = 0.0
    self_weight.Gravity_z = -1.0
    analysis.addObject(self_weight)

    # mesh
    from .meshes.mesh_thermomech_flow1d_seg3 import create_nodes, create_elements
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

    doc.recompute()
    return doc
