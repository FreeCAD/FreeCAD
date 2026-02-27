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

from BOPTools import SplitFeatures
from CompoundTools import CompoundFilter

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Multimaterial tension rod 2 boxes",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "pressure"],
        "solvers": ["ccxtools"],
        "material": "multimaterial",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.material_multiple_tensionrod_twoboxes import setup
setup()


See forum topic post:
...

"""
    )


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    # two boxes
    boxlow = doc.addObject("Part::Box", "BoxLower")
    boxupp = doc.addObject("Part::Box", "BoxUpper")
    boxupp.Placement.Base = (0, 0, 10)

    # boolean fragment of the two boxes
    bf = SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    bf.Objects = [boxlow, boxupp]
    bf.Mode = "CompSolid"
    doc.recompute()
    bf.Proxy.execute(bf)
    bf.purgeTouched()
    doc.recompute()
    if FreeCAD.GuiUp:
        for child in bf.ViewObject.Proxy.claimChildren():
            child.ViewObject.hide()

    # extract CompSolid by compound filter tool
    geom_obj = CompoundFilter.makeCompoundFilter(name="MultiMatCompSolid")
    geom_obj.Base = bf
    geom_obj.FilterType = "window-volume"
    geom_obj.Proxy.execute(geom_obj)
    geom_obj.purgeTouched()
    if FreeCAD.GuiUp:
        bf.ViewObject.hide()
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.WorkingDir = ""
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = False
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
    analysis.addObject(solver_obj)

    # materials
    material_obj_low = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterialLow")
    mat = material_obj_low.Material
    mat["Name"] = "Aluminium-Generic"
    mat["YoungsModulus"] = "70000 MPa"
    mat["PoissonRatio"] = "0.35"
    material_obj_low.Material = mat
    material_obj_low.References = [(boxlow, "Solid1")]
    analysis.addObject(material_obj_low)

    material_obj_upp = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterialUpp")
    mat = material_obj_upp.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj_upp.Material = mat
    material_obj_upp.References = [(boxupp, "Solid1")]
    analysis.addObject(material_obj_upp)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Face5")]
    analysis.addObject(con_fixed)

    # constraint pressure
    con_pressure = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_pressure.References = [(geom_obj, "Face11")]
    con_pressure.Pressure = "1000.0 MPa"
    con_pressure.Reversed = False
    analysis.addObject(con_pressure)

    # mesh
    from .meshes.mesh_boxes_2_vertikal_tetra10 import create_nodes, create_elements

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
