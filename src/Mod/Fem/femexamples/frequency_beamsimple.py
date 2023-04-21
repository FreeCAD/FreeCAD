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
        "name": "Frequency Analysis Simple Beam",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["frequency"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.frequency_beamsimple import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=58959#p506565

simple frequency analysis

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
    geom_obj.Length = 3000
    geom_obj.Width = 100
    geom_obj.Height = 50
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
        solver_obj.AnalysisType = "frequency"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.EigenmodesCount = 10
        solver_obj.EigenmodeHighLimit = 1000000.0
        solver_obj.EigenmodeLowLimit = 0.01
    analysis.addObject(solver_obj)

    # material
    material_obj = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    )[0]
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint displacement xyz
    con_disp_xyz = ObjectsFem.makeConstraintDisplacement(doc, "Fix_XYZ")
    con_disp_xyz.References = [(doc.Box, "Edge4")]
    con_disp_xyz.xFix = True
    con_disp_xyz.xFree = False
    con_disp_xyz.xDisplacement = 0.0
    con_disp_xyz.yFix = True
    con_disp_xyz.yFree = False
    con_disp_xyz.yDisplacement = 0.0
    con_disp_xyz.zFix = True
    con_disp_xyz.zFree = False
    con_disp_xyz.zDisplacement = 0.0
    analysis.addObject(con_disp_xyz)

    # constraint displacement yz
    con_disp_yz = ObjectsFem.makeConstraintDisplacement(doc, "Fix_YZ")
    con_disp_yz.References = [(doc.Box, "Edge8")]
    con_disp_yz.xFix = False
    con_disp_yz.xFree = True
    con_disp_yz.xDisplacement = 0.0
    con_disp_yz.yFix = True
    con_disp_yz.yFree = False
    con_disp_yz.yDisplacement = 0.0
    con_disp_yz.zFix = True
    con_disp_yz.zFree = False
    con_disp_yz.zDisplacement = 0.0
    analysis.addObject(con_disp_yz)

    # mesh
    from .meshes.mesh_beamsimple_tetra10 import create_nodes, create_elements
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
    femmesh_obj.CharacteristicLengthMax = "25.0 mm"

    doc.recompute()
    return doc
