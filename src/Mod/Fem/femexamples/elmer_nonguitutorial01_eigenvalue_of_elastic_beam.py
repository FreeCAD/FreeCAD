# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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
        "name": "NonGui Tutorial 01 - Eigenvalue of elastic beam",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": [],
        "solvers": ["calculix", "ccxtools", "elmer"],
        "material": "solid",
        "equation": "elasticity"  # "frequency", but list not allowed here
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.elmer_nonguitutorial01_eigenvalue_of_elastic_beam import setup
setup()


See forum topic post:
https://forum.freecadweb.org/viewtopic.php?t=56590

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
    geom_obj = doc.addObject("Part::Box", "Box")
    geom_obj.Length = 1000
    geom_obj.Width = 200
    geom_obj.Height = 100
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
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        eq_obj = ObjectsFem.makeEquationElasticity(doc, solver_obj)
        eq_obj.LinearSolverType = "Direct"
        # direct solver was used in the tutorial, thus used here too
        # the iterative is much faster and gives the same results
        eq_obj.DoFrequencyAnalysis = True
        eq_obj.CalculateStresses = True
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.AnalysisType = "frequency"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.EigenmodesCount = 5
        solver_obj.EigenmodeHighLimit = 1000000.0
        solver_obj.EigenmodeLowLimit = 0.01
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "100 GPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "2330 kg/m^3"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [
        (geom_obj, "Face1"),
        (geom_obj, "Face2")
    ]
    analysis.addObject(con_fixed)

    # mesh
    from .meshes.mesh_eigenvalue_of_elastic_beam_tetra10 import create_nodes
    from .meshes.mesh_eigenvalue_of_elastic_beam_tetra10 import create_elements
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
    femmesh_obj.CharacteristicLengthMax = "40.80 mm"

    doc.recompute()
    return doc
