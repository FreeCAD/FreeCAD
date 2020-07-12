# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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
from femexamples.material_multiple_bendingbeam_fivefaces import setup
setup()
"""

import FreeCAD

import Fem
import ObjectsFem

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Multimaterial bending beam 5 faces",
            "meshtype": "solid",
            "meshelement": "Tria6",
            "constraints": ["fixed", "force"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup(doc=None, solvertype="ccxtools"):

    if doc is None:
        doc = init_doc()

    # geometry object
    # name is important because the other method in this module use obj name
    # parts
    face_obj1 = doc.addObject('Part::Plane', 'Face1')
    face_obj1.Width = 10
    face_obj1.Length = 20
    face_obj2 = doc.addObject('Part::Plane', 'Face2')
    face_obj2.Width = 10
    face_obj2.Length = 20
    face_obj2.Placement.Base = (20, 0, 0)
    face_obj3 = doc.addObject('Part::Plane', 'Face3')
    face_obj3.Width = 10
    face_obj3.Length = 20
    face_obj3.Placement.Base = (40, 0, 0)
    face_obj4 = doc.addObject('Part::Plane', 'Face4')
    face_obj4.Width = 10
    face_obj4.Length = 20
    face_obj4.Placement.Base = (60, 0, 0)
    face_obj5 = doc.addObject('Part::Plane', 'Face5')
    face_obj5.Width = 10
    face_obj5.Length = 20
    face_obj5.Placement.Base = (80, 0, 0)
    doc.recompute()

    # make a Shell out of the facees, to be able to remesh with GUI
    geom_obj = doc.addObject("Part::MultiFuse", "Fusion")
    geom_obj.Shapes = [face_obj1, face_obj2, face_obj3, face_obj4, face_obj5]
    if FreeCAD.GuiUp:
        face_obj1.ViewObject.hide()
        face_obj2.ViewObject.hide()
        face_obj3.ViewObject.hide()
        face_obj4.ViewObject.hide()
        face_obj5.ViewObject.hide()
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

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
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_object.SplitInputWriter = False
        solver_object.AnalysisType = "static"
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    # shell thickness
    analysis.addObject(
        ObjectsFem.makeElementGeometry2D(doc, 10, 'ShellThickness')
    )

    # materials
    # material1
    material_object1 = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, 'FemMaterial1')
    )[0]
    material_object1.References = [(doc.Face3, "Face1")]
    mat = material_object1.Material
    mat['Name'] = "Concrete-Generic"
    mat['YoungsModulus'] = "32000 MPa"
    mat['PoissonRatio'] = "0.17"
    mat['Density'] = "0 kg/m^3"
    material_object1.Material = mat

    # material2
    material_object2 = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, 'FemMaterial2')
    )[0]
    material_object2.References = [
        (doc.Face2, "Face1"),
        (doc.Face4, "Face1")
    ]
    mat = material_object2.Material
    mat['Name'] = "PLA"
    mat['YoungsModulus'] = "3640 MPa"
    mat['PoissonRatio'] = "0.36"
    mat['Density'] = "0 kg/m^3"
    material_object2.Material = mat

    # material3
    material_object3 = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, 'FemMaterial3')
    )[0]
    material_object3.References = []
    mat = material_object3.Material
    mat['Name'] = "Steel-Generic"
    mat['YoungsModulus'] = "200000 MPa"
    mat['PoissonRatio'] = "0.30"
    mat['Density'] = "7900 kg/m^3"
    material_object3.Material = mat

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, name="ConstraintFixed")
    )[0]
    fixed_constraint.References = [
        (doc.Face1, "Edge1"),
        (doc.Face5, "Edge3")
    ]

    # force_constraint
    force_constraint = analysis.addObject(
        ObjectsFem.makeConstraintForce(doc, name="ConstraintForce")
    )[0]
    force_constraint.References = [
        (doc.Face1, "Edge4"),
        (doc.Face2, "Edge4"),
        (doc.Face3, "Edge4"),
        (doc.Face4, "Edge4"),
        (doc.Face5, "Edge4")
    ]
    force_constraint.Force = 10000.00
    force_constraint.Direction = (doc.Face1, ["Edge1"])
    force_constraint.Reversed = True

    # mesh
    from .meshes.mesh_multibodybeam_tria6 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        ObjectsFem.makeMeshGmsh(doc, mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
