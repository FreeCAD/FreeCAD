# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# to run the example use:
"""
from femexamples.thermomech_flow1d import setup
setup()

"""


import FreeCAD
from FreeCAD import Vector as vec

import Fem
import ObjectsFem
from Draft import makeWire

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    # geometry objects
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
        import FreeCADGui
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        FreeCADGui.SendMsgToActiveView("ViewFit")

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solvertype == "ccxtools":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")  # CalculiX
        )[0]
        solver_object.WorkingDir = u""
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "thermomech"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = True
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsThermoMechMaximum = 2000
        solver_object.IterationsControlParameterTimeUse = False

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialFluid(doc, "FluidMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "Water"
    mat["Density"] = "998 kg/m^3"
    mat["SpecificHeat"] = "4.182 J/kg/K"
    mat["DynamicViscosity"] = "1.003e-3 kg/m/s"
    mat["VolumetricThermalExpansionCoefficient"] = "2.07e-4 m/m/K"
    mat["ThermalConductivity"] = "0.591 W/m/K"
    material_object.Material = mat

    inlet = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    inlet.SectionType = "Liquid"
    inlet.LiquidSectionType = "PIPE INLET"
    inlet.InletPressure = 0.1
    inlet.References = [(geom_obj, "Edge1")]

    entrance = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    entrance.SectionType = "Liquid"
    entrance.LiquidSectionType = "PIPE ENTRANCE"
    entrance.EntrancePipeArea = 31416.00
    entrance.EntranceArea = 25133.00
    entrance.References = [(geom_obj, "Edge2")]

    manning1 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    manning1.SectionType = "Liquid"
    manning1.LiquidSectionType = "PIPE MANNING"
    manning1.ManningArea = 31416
    manning1.ManningRadius = 50
    manning1.ManningCoefficient = 0.002
    manning1.References = [(geom_obj, "Edge3"), (geom_obj, "Edge5")]

    bend = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    bend.SectionType = "Liquid"
    bend.LiquidSectionType = "PIPE BEND"
    bend.BendPipeArea = 31416
    bend.BendRadiusDiameter = 1.5
    bend.BendAngle = 45
    bend.BendLossCoefficient = 0.4
    bend.References = [(geom_obj, "Edge4")]

    enlargement1 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    enlargement1.SectionType = "Liquid"
    enlargement1.LiquidSectionType = "PIPE ENLARGEMENT"
    enlargement1.EnlargeArea1 = 31416.00
    enlargement1.EnlargeArea2 = 70686.00
    enlargement1.References = [(geom_obj, "Edge6")]

    manning2 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    manning2.SectionType = "Liquid"
    manning2.LiquidSectionType = "PIPE MANNING"
    manning2.ManningArea = 70686.00
    manning2.ManningRadius = 75
    manning2.ManningCoefficient = 0.002
    manning2.References = [(geom_obj, "Edge7")]

    contraction = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    contraction.SectionType = "Liquid"
    contraction.LiquidSectionType = "PIPE CONTRACTION"
    contraction.ContractArea1 = 70686
    contraction.ContractArea2 = 17671
    contraction.References = [(geom_obj, "Edge8")]

    manning3 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    manning3.SectionType = "Liquid"
    manning3.LiquidSectionType = "PIPE MANNING"
    manning3.ManningArea = 17671.00
    manning3.ManningRadius = 37.5
    manning3.ManningCoefficient = 0.002
    manning3.References = [(geom_obj, "Edge11"), (geom_obj, "Edge9")]

    gate_valve = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    gate_valve.SectionType = "Liquid"
    gate_valve.LiquidSectionType = "PIPE GATE VALVE"
    gate_valve.GateValvePipeArea = 17671
    gate_valve.GateValveClosingCoeff = 0.5
    gate_valve.References = [(geom_obj, "Edge10")]

    enlargement2 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    enlargement2.SectionType = "Liquid"
    enlargement2.LiquidSectionType = "PIPE ENLARGEMENT"
    enlargement2.EnlargeArea1 = 17671
    enlargement2.EnlargeArea2 = 1e12
    enlargement2.References = [(geom_obj, "Edge12")]

    outlet = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    outlet.SectionType = "Liquid"
    outlet.LiquidSectionType = "PIPE OUTLET"
    outlet.OutletPressure = 0.1
    outlet.References = [(geom_obj, "Edge13")]

    self_weight = analysis.addObject(
        ObjectsFem.makeConstraintSelfWeight(doc, "ConstraintSelfWeight")
    )[0]
    self_weight.Gravity_x = 0.0
    self_weight.Gravity_y = 0.0
    self_weight.Gravity_z = -1.0

    # mesh
    from .meshes.mesh_thermomech_flow1d_seg3 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        doc.addObject("Fem::FemMeshObject", mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc
