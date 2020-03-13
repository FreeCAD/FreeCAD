# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

# thermomechanical bimetall
# https://forum.freecadweb.org/viewtopic.php?f=18&t=43040&start=10#p366664
# analytical solution 7.05 mm deflection in the invar material direction
# see post in the forum link
# this file has 7.15 mm max deflection
# to run the example use:
"""
from femexamples.thermomech_bimetall import setup
setup()

"""

import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

import Fem
import ObjectsFem
from BOPTools import SplitFeatures

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    # geom objects
    # bottom box
    bottom_box_obj = doc.addObject("Part::Box", "BottomBox")
    bottom_box_obj.Length = 100
    bottom_box_obj.Width = 5
    bottom_box_obj.Height = 1

    # top box
    top_box_obj = doc.addObject("Part::Box", "TopBox")
    top_box_obj.Length = 100
    top_box_obj.Width = 5
    top_box_obj.Height = 1
    top_box_obj.Placement = FreeCAD.Placement(
        Vector(0, 0, 1),
        Rotation(0, 0, 0),
        Vector(0, 0, 0),
    )
    doc.recompute()

    # all geom boolean fragment
    geom_obj = SplitFeatures.makeBooleanFragments(name='BooleanFragments')
    geom_obj.Objects = [bottom_box_obj, top_box_obj]
    if FreeCAD.GuiUp:
        bottom_box_obj.ViewObject.hide()
        top_box_obj.ViewObject.hide()
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
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        )[0]
        solver_object.WorkingDir = u""
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.AnalysisType = "thermomech"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = True
        # solver_object.MatrixSolverType = "default"
        solver_object.MatrixSolverType = "spooles"  # thomas
        solver_object.SplitInputWriter = False
        solver_object.IterationsThermoMechMaximum = 2000
        # solver_object.IterationsControlParameterTimeUse = True  # thermomech spine

    # material
    material_obj_bottom = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MaterialCopper")
    )[0]
    mat = material_obj_bottom.Material
    mat["Name"] = "Copper"
    mat["YoungsModulus"] = "130000 MPa"
    mat["PoissonRatio"] = "0.354"
    mat["SpecificHeat"] = "385 J/kg/K"
    mat["ThermalConductivity"] = "200 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.00002 m/m/K"
    material_obj_bottom.Material = mat
    material_obj_bottom.References = [(geom_obj, "Solid1")]
    analysis.addObject(material_obj_bottom)

    material_obj_top = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MaterialInvar")
    )[0]
    mat = material_obj_top.Material
    mat["Name"] = "Invar"
    mat["YoungsModulus"] = "137000 MPa"
    mat["PoissonRatio"] = "0.28"
    mat["SpecificHeat"] = "510 J/kg/K"
    mat["ThermalConductivity"] = "13 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.0000012 m/m/K"
    material_obj_top.Material = mat
    material_obj_top.References = [(geom_obj, "Solid2")]
    analysis.addObject(material_obj_top)

    # constraint fixed
    con_fixed = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    )[0]
    con_fixed.References = [
        (geom_obj, "Face1"),
        (geom_obj, "Face7"),
    ]

    # constraint initial temperature
    constraint_initialtemp = analysis.addObject(
        ObjectsFem.makeConstraintInitialTemperature(doc, "ConstraintInitialTemperature")
    )[0]
    constraint_initialtemp.initialTemperature = 273.0

    # constraint temperature
    constraint_temperature = analysis.addObject(
        ObjectsFem.makeConstraintTemperature(doc, "ConstraintTemperature")
    )[0]
    constraint_temperature.References = [
        (geom_obj, "Face1"),
        (geom_obj, "Face2"),
        (geom_obj, "Face3"),
        (geom_obj, "Face4"),
        (geom_obj, "Face5"),
        (geom_obj, "Face7"),
        (geom_obj, "Face8"),
        (geom_obj, "Face9"),
        (geom_obj, "Face10"),
        (geom_obj, "Face11"),
    ]
    constraint_temperature.Temperature = 373.0
    constraint_temperature.CFlux = 0.0

    # mesh
    from .meshes.mesh_thermomech_bimetall_tetra10 import create_nodes, create_elements
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
