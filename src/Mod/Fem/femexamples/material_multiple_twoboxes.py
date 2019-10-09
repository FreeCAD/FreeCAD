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
    # create a CompSolid of two Boxes extract the CompSolid
    # we are able to remesh if needed
    boxlow = doc.addObject("Part::Box", "BoxLower")
    boxupp = doc.addObject("Part::Box", "BoxUpper")
    boxupp.Placement.Base = (0, 0, 10)

    # for BooleanFragments Occt >=6.9 is needed
    """
    import BOPTools.SplitFeatures
    bf = BOPTools.SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    bf.Objects = [boxlow, boxupp]
    bf.Mode = "CompSolid"
    self.active_doc.recompute()
    bf.Proxy.execute(bf)
    bf.purgeTouched()
    for obj in bf.ViewObject.Proxy.claimChildren():
        obj.ViewObject.hide()
    self.active_doc.recompute()
    import CompoundTools.CompoundFilter
    cf = CompoundTools.CompoundFilter.makeCompoundFilter(name="MultiMatCompSolid")
    cf.Base = bf
    cf.FilterType = "window-volume"
    cf.Proxy.execute(cf)
    cf.purgeTouched()
    cf.Base.ViewObject.hide()
    """
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
    if solvertype == "calculix" or solvertype == "ccxtools":
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
    # fixed_constraint.References = [(cf, "Face3")]
    fixed_constraint.References = [(boxlow, "Face5")]

    # pressure_constraint
    pressure_constraint = analysis.addObject(
        ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    )[0]
    # pressure_constraint.References = [(cf, "Face3")]
    pressure_constraint.References = [(boxlow, "Face5")]
    # pressure_constraint.References = [(cf, "Face9")]
    pressure_constraint.References = [(boxupp, "Face6")]
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


"""
from femexamples import material_multiple_twoboxes as twoboxes
twoboxes.setup()

"""
