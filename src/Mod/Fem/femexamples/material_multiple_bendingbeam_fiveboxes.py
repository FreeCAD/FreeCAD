# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

import BOPTools.SplitFeatures

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Multimaterial bending beam 5 boxes",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "force"],
        "solvers": ["ccxtools"],
        "material": "multimaterial",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.material_multiple_bendingbeam_fiveboxes import setup
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
    # name is important because the other method in this module use obj name
    box_obj1 = doc.addObject("Part::Box", "Box1")
    box_obj1.Height = 10
    box_obj1.Width = 10
    box_obj1.Length = 20
    box_obj2 = doc.addObject("Part::Box", "Box2")
    box_obj2.Height = 10
    box_obj2.Width = 10
    box_obj2.Length = 20
    box_obj2.Placement.Base = (20, 0, 0)
    box_obj3 = doc.addObject("Part::Box", "Box3")
    box_obj3.Height = 10
    box_obj3.Width = 10
    box_obj3.Length = 20
    box_obj3.Placement.Base = (40, 0, 0)
    box_obj4 = doc.addObject("Part::Box", "Box4")
    box_obj4.Height = 10
    box_obj4.Width = 10
    box_obj4.Length = 20
    box_obj4.Placement.Base = (60, 0, 0)
    box_obj5 = doc.addObject("Part::Box", "Box5")
    box_obj5.Height = 10
    box_obj5.Width = 10
    box_obj5.Length = 20
    box_obj5.Placement.Base = (80, 0, 0)

    # make a CompSolid out of the boxes, to be able to remesh with GUI
    j = BOPTools.SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    j.Objects = [box_obj1, box_obj2, box_obj3, box_obj4, box_obj5]
    j.Mode = "CompSolid"
    j.Proxy.execute(j)
    j.purgeTouched()
    doc.recompute()
    if FreeCAD.GuiUp:
        for obj in j.ViewObject.Proxy.claimChildren():
            obj.ViewObject.hide()

    geom_obj = doc.addObject("Part::Feature", "CompSolid")
    geom_obj.Shape = j.Shape.CompSolids[0]
    if FreeCAD.GuiUp:
        j.ViewObject.hide()
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

    # material
    material_obj1 = ObjectsFem.makeMaterialSolid(doc, "FemMaterial1")
    material_obj1.References = [(doc.Box3, "Solid1")]
    mat = material_obj1.Material
    mat["Name"] = "Concrete-Generic"
    mat["YoungsModulus"] = "32000 MPa"
    mat["PoissonRatio"] = "0.17"
    material_obj1.Material = mat
    analysis.addObject(material_obj1)

    material_obj2 = ObjectsFem.makeMaterialSolid(doc, "FemMaterial2")
    material_obj2.References = [(doc.Box2, "Solid1"), (doc.Box4, "Solid1")]
    mat = material_obj2.Material
    mat["Name"] = "PLA"
    mat["YoungsModulus"] = "3640 MPa"
    mat["PoissonRatio"] = "0.36"
    material_obj2.Material = mat
    analysis.addObject(material_obj2)

    material_obj3 = ObjectsFem.makeMaterialSolid(doc, "FemMaterial3")
    material_obj3.References = []
    mat = material_obj3.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj3.Material = mat
    analysis.addObject(material_obj3)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(doc.Box1, "Face1"), (doc.Box5, "Face2")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [
        (doc.Box1, "Face6"),
        (doc.Box2, "Face6"),
        (doc.Box3, "Face6"),
        (doc.Box4, "Face6"),
        (doc.Box5, "Face6"),
    ]
    con_force.Force = "10000.00 N"
    con_force.Direction = (doc.Box1, ["Edge1"])
    con_force.Reversed = True
    analysis.addObject(con_force)

    # mesh
    from .meshes.mesh_multibodybeam_tetra10 import create_nodes, create_elements

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
