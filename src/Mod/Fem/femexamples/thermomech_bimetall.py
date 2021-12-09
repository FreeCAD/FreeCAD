# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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
# to run the example use:
"""
from femexamples.thermomech_bimetall import setup
setup()

"""


import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

from BOPTools import SplitFeatures

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Thermomech Bimetall",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "initial temperature", "temperature"],
        "solvers": ["calculix", "ccxtools", "elmer"],
        "material": "multimaterial",
        "equation": "thermomechanical"
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.thermomech_bimetall import setup
setup()


See forum topic post:
https://forum.freecadweb.org/viewtopic.php?f=18&t=43040&start=10#p366664

thermomechanical bimetall
analytical solution 7.05 mm deflection in the invar material direction
see post in the forum link
this file has 7.15 mm max deflection


"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
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
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
    elif solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        solver_obj.WorkingDir = u""
    elif solvertype == "elmer":
        solver_obj = analysis.addObject(ObjectsFem.makeSolverElmer(doc, "SolverElmer"))[0]
        solver_obj.SteadyStateMinIterations = 1
        solver_obj.SteadyStateMaxIterations = 10
        eq_heat = ObjectsFem.makeEquationHeat(doc, solver_obj)
        eq_heat.Bubbles = True
        eq_heat.Priority = 2
        eq_elasticity = ObjectsFem.makeEquationElasticity(doc, solver_obj)
        eq_elasticity.Bubbles = True
        eq_elasticity.Priority = 1
        eq_elasticity.LinearSolverType = "Direct"
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.AnalysisType = "thermomech"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = True
        # solver_obj.MatrixSolverType = "default"
        solver_obj.MatrixSolverType = "spooles"  # thomas
        solver_obj.SplitInputWriter = False
        solver_obj.IterationsThermoMechMaximum = 2000
        # solver_obj.IterationsControlParameterTimeUse = True  # thermomech spine
    analysis.addObject(solver_obj)

    # material
    material_obj_bottom = ObjectsFem.makeMaterialSolid(doc, "MaterialCopper")
    mat = material_obj_bottom.Material
    mat["Name"] = "Copper"
    mat["YoungsModulus"] = "130000 MPa"
    mat["PoissonRatio"] = "0.354"
    mat["SpecificHeat"] = "385 J/kg/K"
    mat["ThermalConductivity"] = "200 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.00002 m/m/K"
    mat["Density"] = "1.00 kg/m^3"
    material_obj_bottom.Material = mat
    material_obj_bottom.References = [(geom_obj, "Solid1")]
    analysis.addObject(material_obj_bottom)

    material_obj_top = ObjectsFem.makeMaterialSolid(doc, "MaterialInvar")
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
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [
        (geom_obj, "Face1"),
        (geom_obj, "Face7"),
    ]
    analysis.addObject(con_fixed)

    # constraint initial temperature
    con_inittemp = ObjectsFem.makeConstraintInitialTemperature(doc, "ConstraintInitialTemperature")
    con_inittemp.initialTemperature = 273.0
    analysis.addObject(con_inittemp)

    # constraint temperature
    con_temp = ObjectsFem.makeConstraintTemperature(doc, "ConstraintTemperature")
    con_temp.References = [
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
    con_temp.Temperature = 373.0
    con_temp.CFlux = 0.0
    analysis.addObject(con_temp)

    # mesh
    from .meshes.mesh_thermomech_bimetall_tetra10 import create_nodes, create_elements
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
