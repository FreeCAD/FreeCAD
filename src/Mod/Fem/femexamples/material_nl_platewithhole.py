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

# to run the example use:
"""
from femexamples.material_nl_platewithhole import setup
setup()
"""

import FreeCAD
from FreeCAD import Vector as vec

import Part
from Part import makeCircle as ci
from Part import makeLine as ln

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Material NL Plate with Hole",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "force"],
        "solvers": ["calculix", "ccxtools"],
        "material": "nonlinear",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.material_nl_platewithhole import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=24&t=31997&start=30
https://forum.freecad.org/viewtopic.php?t=33974&start=90
https://forum.freecad.org/viewtopic.php?t=35893
https://forum.freecad.org/viewtopic.php?t=23101

Nonlinear material example, plate with hole.

plate: 400x200x10 mm
hole: diameter 100 mm (half cross section)
load: 130 MPa tension
linear material: Steel, E = 210000 MPa, my = 0.3
nonlinear material: '240.0, 0.0' to '270.0, 0.025'
TODO nonlinear material: give more information, use values from harry
TODO compare results with example from HarryvL

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    v1 = vec(-200, -100, 0)
    v2 = vec(200, -100, 0)
    v3 = vec(200, 100, 0)
    v4 = vec(-200, 100, 0)
    l1 = ln(v1, v2)
    l2 = ln(v2, v3)
    l3 = ln(v3, v4)
    l4 = ln(v4, v1)
    v5 = vec(0, 0, 0)
    c1 = ci(50, v5)
    face = Part.makeFace([Part.Wire([l1, l2, l3, l4]), c1], "Part::FaceMakerBullseye")
    geom_obj = doc.addObject("Part::Feature", "Hole_Plate")
    geom_obj.Shape = face.extrude(vec(0, 0, 10))
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
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.GeometricalNonlinearity = 'nonlinear'
        solver_obj.MaterialNonlinearity = 'nonlinear'
    analysis.addObject(solver_obj)

    # linear material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material_lin")
    matprop = material_obj.Material
    matprop["Name"] = "CalculiX-Steel"
    matprop["YoungsModulus"] = "210000 MPa"
    matprop["PoissonRatio"] = "0.30"
    material_obj.Material = matprop
    analysis.addObject(material_obj)

    # nonlinear material
    name_nlm = "Material_nonlin"
    nonlinear_mat = ObjectsFem.makeMaterialMechanicalNonlinear(doc, material_obj, name_nlm)
    nonlinear_mat.YieldPoints = ['240.0, 0.0', '270.0, 0.025']
    analysis.addObject(nonlinear_mat)
    # check solver attributes, Nonlinearity needs to be set to nonlinear

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Face4")]
    analysis.addObject(con_fixed)

    # pressure constraint
    con_pressure = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_pressure.References = [(geom_obj, "Face2")]
    con_pressure.Pressure = "130.0 MPa"
    con_pressure.Reversed = True
    analysis.addObject(con_pressure)

    # mesh
    from .meshes.mesh_platewithhole_tetra10 import create_nodes, create_elements
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
