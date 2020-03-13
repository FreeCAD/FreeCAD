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

# to run the example use:
"""
from femexamples.material_multiple_twoboxes import setup
setup()

"""

import FreeCAD

import Fem
import ObjectsFem
from BOPTools import SplitFeatures
from CompoundTools import CompoundFilter

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    # geometry objects
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
    if FreeCAD.GuiUp:
        for child in bf.ViewObject.Proxy.claimChildren():
            child.ViewObject.hide()
    doc.recompute()

    # extract CompSolid by compound filter tool
    geom_obj = CompoundFilter.makeCompoundFilter(name="MultiMatCompSolid")
    geom_obj.Base = bf
    geom_obj.FilterType = "window-volume"
    geom_obj.Proxy.execute(geom_obj)
    geom_obj.purgeTouched()
    if FreeCAD.GuiUp:
        geom_obj.Base.ViewObject.hide()
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
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "static"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    # material
    material_object_low = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterialLow")
    )[0]
    mat = material_object_low.Material
    mat["Name"] = "Aluminium-Generic"
    mat["YoungsModulus"] = "70000 MPa"
    mat["PoissonRatio"] = "0.35"
    mat["Density"] = "2700  kg/m^3"
    material_object_low.Material = mat
    material_object_low.References = [(boxlow, "Solid1")]
    analysis.addObject(material_object_low)

    material_object_upp = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterialUpp")
    )[0]
    mat = material_object_upp.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7980 kg/m^3"
    material_object_upp.Material = mat
    material_object_upp.References = [(boxupp, "Solid1")]

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    )[0]
    fixed_constraint.References = [(geom_obj, "Face5")]

    # pressure_constraint
    pressure_constraint = analysis.addObject(
        ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    )[0]
    pressure_constraint.References = [(geom_obj, "Face11")]
    pressure_constraint.Pressure = 1000.0
    pressure_constraint.Reversed = False

    # mesh
    from .meshes.mesh_boxes_2_vertikal_tetra10 import create_nodes, create_elements
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
