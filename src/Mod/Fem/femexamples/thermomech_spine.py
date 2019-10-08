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


def setup(doc=None, solvertype="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    # part
    box_obj = doc.addObject("Part::Box", "Box")
    box_obj.Height = 25.4
    box_obj.Width = 25.4
    box_obj.Length = 203.2
    doc.recompute()

    if FreeCAD.GuiUp:
        import FreeCADGui
        FreeCADGui.ActiveDocument.activeView().viewAxonometric()
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
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        )[0]
        solver_object.WorkingDir = u""
    # should be possible with elmer too
    # elif solvertype == "elmer":
    #     analysis.addObject(ObjectsFem.makeSolverElmer(doc, "SolverElmer"))
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.AnalysisType = "thermomech"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = True
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsThermoMechMaximum = 2000
        solver_object.IterationsControlParameterTimeUse = True

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    mat["ThermalConductivity"] = "43.27 W/m/K"  # SvdW: Change to Ansys model values
    mat["ThermalExpansionCoefficient"] = "12 um/m/K"
    mat["SpecificHeat"] = "500 J/kg/K"  # SvdW: Change to Ansys model values
    material_object.Material = mat

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "FemConstraintFixed")
    )[0]
    fixed_constraint.References = [(box_obj, "Face1")]

    # initialtemperature_constraint
    initialtemperature_constraint = analysis.addObject(
        ObjectsFem.makeConstraintInitialTemperature(doc, "FemConstraintInitialTemperature")
    )[0]
    initialtemperature_constraint.initialTemperature = 300.0

    # temperature_constraint
    temperature_constraint = analysis.addObject(
        ObjectsFem.makeConstraintTemperature(doc, "FemConstraintTemperature")
    )[0]
    temperature_constraint.References = [(box_obj, "Face1")]
    temperature_constraint.Temperature = 310.93

    # heatflux_constraint
    heatflux_constraint = analysis.addObject(
        ObjectsFem.makeConstraintHeatflux(doc, "FemConstraintHeatflux")
    )[0]
    heatflux_constraint.References = [
        (box_obj, "Face3"),
        (box_obj, "Face4"),
        (box_obj, "Face5"),
        (box_obj, "Face6")
    ]
    heatflux_constraint.AmbientTemp = 255.3722
    heatflux_constraint.FilmCoef = 5.678

    # mesh
    from .meshes.mesh_thermomech_spine_tetra10 import create_nodes, create_elements
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
from femexamples import thermomech_spine as spine
spine.setup()

"""
