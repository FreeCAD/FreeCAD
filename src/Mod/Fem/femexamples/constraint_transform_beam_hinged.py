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
from femexamples.constraint_transform_beam_hinged import setup
setup()

"""
# Constraint transform on a beam
# https://forum.freecadweb.org/viewtopic.php?f=18&t=20238#p157643

import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector

import Fem
import ObjectsFem
from CompoundTools import CompoundFilter

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Constraint Transform Beam Hinged",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["pressure", "displacement", "transform"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup(doc=None, solvertype="ccxtools"):
    # setup cylinder base model

    if doc is None:
        doc = init_doc()

    # geometry object
    # name is important because the other method in this module use obj name
    cube = doc.addObject("Part::Box", "Cube")
    cube.Height = "20 mm"
    cube.Length = "100 mm"
    cylinder = doc.addObject("Part::Cylinder", "Cylinder")
    cylinder.Height = "20 mm"
    cylinder.Radius = "6 mm"
    cylinder.Placement = FreeCAD.Placement(
        Vector(10, 12, 10), Rotation(0, 0, 90), Vector(0, 0, 0),
    )
    cut = doc.addObject("Part::Cut", "Cut")
    cut.Base = cube
    cut.Tool = cylinder

    # mirroring
    mirror = doc.addObject("Part::Mirroring", "Mirror")
    mirror.Source = cut
    mirror.Normal = (1, 0, 0)
    mirror.Base = (100, 100, 20)

    # fusing
    fusion = doc.addObject("Part::Fuse", "Fusion")
    fusion.Base = cut
    fusion.Tool = mirror
    fusion.Refine = True

    # compound filter
    geom_obj = CompoundFilter.makeCompoundFilter(name='CompoundFilter')
    geom_obj.Base = fusion
    geom_obj.FilterType = 'window-volume'
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.Base.ViewObject.hide()
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

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "FemMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    mat["ThermalExpansionCoefficient"] = "0.012 mm/m/K"
    material_object.Material = mat

    # constraint pressure
    pressure_constraint = analysis.addObject(
        ObjectsFem.makeConstraintPressure(doc, name="FemConstraintPressure")
    )[0]
    pressure_constraint.References = [(geom_obj, "Face8")]
    pressure_constraint.Pressure = 10.0
    pressure_constraint.Reversed = False

    # constraint_displacement
    displacement_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintDisplacement(doc, name="FemConstraintDisplacment")
    )[0]
    displacement_constraint.References = [(geom_obj, "Face4"), (geom_obj, "Face5")]
    displacement_constraint.xFree = False
    displacement_constraint.xFix = True

    # constraint_transform_1
    transform_constraint1 = doc.Analysis.addObject(
        ObjectsFem.makeConstraintTransform(doc, name="FemConstraintTransform1")
    )[0]
    transform_constraint1.References = [(geom_obj, "Face4")]
    transform_constraint1.TransformType = "Cylindrical"
    transform_constraint1.X_rot = 0.0
    transform_constraint1.Y_rot = 0.0
    transform_constraint1.Z_rot = 0.0

    # constraint_transform_2
    transform_constraint2 = doc.Analysis.addObject(
        ObjectsFem.makeConstraintTransform(doc, name="FemConstraintTransform2")
    )[0]
    transform_constraint2.References = [(geom_obj, "Face5")]
    transform_constraint2.TransformType = "Cylindrical"
    transform_constraint2.X_rot = 0.0
    transform_constraint2.Y_rot = 0.0
    transform_constraint2.Z_rot = 0.0

    # mesh
    from .meshes.mesh_transform_beam_hinged_tetra10 import create_nodes, create_elements
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
    femmesh_obj.CharacteristicLengthMax = '7 mm'

    doc.recompute()
    return doc
