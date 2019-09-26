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


import FreeCAD
import ObjectsFem
import Fem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solver="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    p1 = FreeCAD.Vector(0, 0, 50)
    p2 = FreeCAD.Vector(0, 0, -50)
    p3 = FreeCAD.Vector(0, 0, -4300)
    p4 = FreeCAD.Vector(4950, 0, -4300)
    p5 = FreeCAD.Vector(5000, 0, -4300)
    p6 = FreeCAD.Vector(8535.53, 0, -7835.53)
    p7 = FreeCAD.Vector(8569.88, 0, -7870.88)
    p8 = FreeCAD.Vector(12105.41, 0, -11406.41)
    p9 = FreeCAD.Vector(12140.76, 0, -11441.76)
    p10 = FreeCAD.Vector(13908.53, 0, -13209.53)
    p11 = FreeCAD.Vector(13943.88, 0, -13244.88)
    p12 = FreeCAD.Vector(15046.97, 0, -14347.97)
    p13 = FreeCAD.Vector(15046.97, 0, -7947.97)
    p14 = FreeCAD.Vector(15046.97, 0, -7847.97)
    p15 = FreeCAD.Vector(0, 0, 0)
    p16 = FreeCAD.Vector(0, 0, -2175)
    p17 = FreeCAD.Vector(2475, 0, -4300)
    p18 = FreeCAD.Vector(4975, 0, -4300)
    p19 = FreeCAD.Vector(6767.765, 0, -6067.765)
    p20 = FreeCAD.Vector(8552.705, 0, -7853.205)
    p21 = FreeCAD.Vector(10337.645, 0, -9638.645)
    p22 = FreeCAD.Vector(12123.085, 0, -11424.085)
    p23 = FreeCAD.Vector(13024.645, 0, -12325.645)
    p24 = FreeCAD.Vector(13926.205, 0, -13227.205)
    p25 = FreeCAD.Vector(14495.425, 0, -13796.425)
    p26 = FreeCAD.Vector(15046.97, 0, -11147.97)
    p27 = FreeCAD.Vector(15046.97, 0, -7897.97)
    points = [
        p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
        p11, p12, p13, p14, p15, p16, p17, p18, p19, p20,
        p21, p22, p23, p24, p25, p26, p27
    ]
    from Draft import makeWire
    line = makeWire(
        points,
        closed=False,
        face=False,
        support=None
    )
    doc.recompute()

    if FreeCAD.GuiUp:
        import FreeCADGui
        FreeCADGui.ActiveDocument.activeView().viewAxonometric()
        FreeCADGui.SendMsgToActiveView("ViewFit")

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    # TODO How to pass multiple solver for one analysis in one doc
    if solver == "calculix":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solver == "ccxtools":
        solver_object = analysis.addObject(
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")  # CalculiX
        )[0]
        solver_object.WorkingDir = u""
    if solver == "calculix" or solver == "ccxtools":
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

    Flow1d_inlet = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_inlet.SectionType = "Liquid"
    Flow1d_inlet.LiquidSectionType = "PIPE INLET"
    Flow1d_inlet.InletPressure = 0.1
    Flow1d_inlet.References = [(line, "Edge1")]

    Flow1d_entrance = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_entrance.SectionType = "Liquid"
    Flow1d_entrance.LiquidSectionType = "PIPE ENTRANCE"
    Flow1d_entrance.EntrancePipeArea = 31416.00
    Flow1d_entrance.EntranceArea = 25133.00
    Flow1d_entrance.References = [(line, "Edge2")]

    Flow1d_manning = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_manning.SectionType = "Liquid"
    Flow1d_manning.LiquidSectionType = "PIPE MANNING"
    Flow1d_manning.ManningArea = 31416
    Flow1d_manning.ManningRadius = 50
    Flow1d_manning.ManningCoefficient = 0.002
    Flow1d_manning.References = [(line, "Edge3"), (line, "Edge5")]

    Flow1d_bend = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_bend.SectionType = "Liquid"
    Flow1d_bend.LiquidSectionType = "PIPE BEND"
    Flow1d_bend.BendPipeArea = 31416
    Flow1d_bend.BendRadiusDiameter = 1.5
    Flow1d_bend.BendAngle = 45
    Flow1d_bend.BendLossCoefficient = 0.4
    Flow1d_bend.References = [(line, "Edge4")]

    Flow1d_enlargement = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_enlargement.SectionType = "Liquid"
    Flow1d_enlargement.LiquidSectionType = "PIPE ENLARGEMENT"
    Flow1d_enlargement.EnlargeArea1 = 31416.00
    Flow1d_enlargement.EnlargeArea2 = 70686.00
    Flow1d_enlargement.References = [(line, "Edge6")]

    Flow1d_manning1 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_manning1.SectionType = "Liquid"
    Flow1d_manning1.LiquidSectionType = "PIPE MANNING"
    Flow1d_manning1.ManningArea = 70686.00
    Flow1d_manning1.ManningRadius = 75
    Flow1d_manning1.ManningCoefficient = 0.002
    Flow1d_manning1.References = [(line, "Edge7")]

    Flow1d_contraction = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_contraction.SectionType = "Liquid"
    Flow1d_contraction.LiquidSectionType = "PIPE CONTRACTION"
    Flow1d_contraction.ContractArea1 = 70686
    Flow1d_contraction.ContractArea2 = 17671
    Flow1d_contraction.References = [(line, "Edge8")]

    Flow1d_manning2 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_manning2.SectionType = "Liquid"
    Flow1d_manning2.LiquidSectionType = "PIPE MANNING"
    Flow1d_manning2.ManningArea = 17671.00
    Flow1d_manning2.ManningRadius = 37.5
    Flow1d_manning2.ManningCoefficient = 0.002
    Flow1d_manning2.References = [(line, "Edge11"), (line, "Edge9")]

    Flow1d_gate_valve = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_gate_valve.SectionType = "Liquid"
    Flow1d_gate_valve.LiquidSectionType = "PIPE GATE VALVE"
    Flow1d_gate_valve.GateValvePipeArea = 17671
    Flow1d_gate_valve.GateValveClosingCoeff = 0.5
    Flow1d_gate_valve.References = [(line, "Edge10")]

    Flow1d_enlargement1 = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_enlargement1.SectionType = "Liquid"
    Flow1d_enlargement1.LiquidSectionType = "PIPE ENLARGEMENT"
    Flow1d_enlargement1.EnlargeArea1 = 17671
    Flow1d_enlargement1.EnlargeArea2 = 1e12
    Flow1d_enlargement1.References = [(line, "Edge12")]

    Flow1d_outlet = analysis.addObject(
        ObjectsFem.makeElementFluid1D(doc, "ElementFluid1D")
    )[0]
    Flow1d_outlet.SectionType = "Liquid"
    Flow1d_outlet.LiquidSectionType = "PIPE OUTLET"
    Flow1d_outlet.OutletPressure = 0.1
    Flow1d_outlet.References = [(line, "Edge13")]

    Flow1d_self_weight = analysis.addObject(
        ObjectsFem.makeConstraintSelfWeight(doc, "ConstraintSelfWeight")
    )[0]
    Flow1d_self_weight.Gravity_x = 0.0
    Flow1d_self_weight.Gravity_y = 0.0
    Flow1d_self_weight.Gravity_z = -1.0

    # mesh
    from .meshes.mesh_thermomech_flow1d import create_nodes, create_elements
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


"""
from femexamples import thermomech_flow1d as flow
flow.setup()

"""
