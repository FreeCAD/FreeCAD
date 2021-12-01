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

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Thermomech Spine",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "initial temperature", "temperature", "heatflux"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equation": "thermomechanical"
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.thermomech_spine import setup
setup()


See forum topic post:
...

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    geom_obj = doc.addObject("Part::Box", "Box")
    geom_obj.Height = 25.4
    geom_obj.Width = 25.4
    geom_obj.Length = 203.2
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
    # should be possible with elmer too
    # elif solvertype == "elmer":
    #     analysis.addObject(ObjectsFem.makeSolverElmer(doc, "SolverElmer"))
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
        solver_obj.IterationsControlParameterTimeUse = True
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    mat["ThermalConductivity"] = "43.27 W/m/K"  # SvdW: Change to Ansys model values
    mat["ThermalExpansionCoefficient"] = "12 um/m/K"
    mat["SpecificHeat"] = "500 J/kg/K"  # SvdW: Change to Ansys model values
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "FemConstraintFixed")
    con_fixed.References = [(geom_obj, "Face1")]
    analysis.addObject(con_fixed)

    # constraint initialtemperature
    name_inittemp = "FemConstraintInitialTemperature"
    con_inittemp = ObjectsFem.makeConstraintInitialTemperature(doc, name_inittemp)
    con_inittemp.initialTemperature = 300.0
    analysis.addObject(con_inittemp)

    # constraint temperature
    con_temp = ObjectsFem.makeConstraintTemperature(doc, "FemConstraintTemperature")
    con_temp.References = [(geom_obj, "Face1")]
    con_temp.Temperature = 310.93
    analysis.addObject(con_temp)

    # constraint heatflux
    con_heatflux = ObjectsFem.makeConstraintHeatflux(doc, "FemConstraintHeatflux")
    con_heatflux.References = [
        (geom_obj, "Face3"),
        (geom_obj, "Face4"),
        (geom_obj, "Face5"),
        (geom_obj, "Face6")
    ]
    con_heatflux.AmbientTemp = 255.3722
    con_heatflux.FilmCoef = 5.678
    analysis.addObject(con_heatflux)

    # mesh
    from .meshes.mesh_thermomech_spine_tetra10 import create_nodes, create_elements
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
