# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
# *   Copyright (c) 2021 Tobias Vaara <t@vaara.se>                          *
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
from femexamples.ccx_buckling_flexuralbuckling import setup
setup()

"""

# Forum discussion
# https://forum.freecadweb.org/viewtopic.php?f=18&t=20217&start=90

# This example is based on Calculix own verification example.
# http://www.feacluster.com/CalculiX/ccx_2.13/doc/ccx/input_deck_viewer.php?input_deck=beam8b.inp

import FreeCAD
import Fem
import ObjectsFem


mesh_name = "Mesh"  # needs to be Mesh to work with unit tests

# Example geometry input

b = 1.5             # Width
h = 8               # Height
l = 1               # Length


def addbox(
        docxx, height, width, length, 
        x, y, z, box_name):

    box_obj = docxx.addObject('Part::Box', box_name)
    box_obj.Height = height
    box_obj.Width = width
    box_obj.Length = length
    box_obj.Placement = FreeCAD.Placement(
            FreeCAD.Vector(x, y, z), 
            FreeCAD.Rotation(0, 0, 0))


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Flexural Buckling Analysis",
            "meshtype": "solid",
            "meshelement": "Hexa8",
            "constraints": ["force", "displacement"],
            "solvers": ["calculix"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup_base(doc=None, solvertype="ccxtools"):

    # setup box base model
    if doc is None:
        doc = init_doc()

    addbox(doc, h, b, l, 0, 0, 0, 'beam')

    doc.recompute()

    geom_obj = doc.beam

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # material
    material_object = analysis.addObject(ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial"))[0]
    mat = material_object.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    material_object.Material = mat

    # mesh
    from .meshes.mesh_flexural_buckling import create_nodes, create_elements

    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, mesh_name))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = doc.beam


    return doc


def setup(doc=None, solvertype="ccxtools"):
    #setup

    doc = setup_base(doc, solvertype)

    analysis = doc.Analysis

    # solver,
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
        solver_object.AnalysisType = "buckling"
        solver_object.BucklingFactors = 10
        solver_object.GeometricalNonlinearity = "linear"
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = "default"
        solver_object.IterationsControlParameterTimeUse = False

    ## displacement constraint
    displacement_constraint = ObjectsFem.makeConstraintFixed(doc, "FemConstraintDisplacement")
    displacement_constraint.References = [(doc.beam, "Face5")]
    analysis.addObject(displacement_constraint)

    ## force_constraint
    force_constraint = ObjectsFem.makeConstraintForce(doc, "FemConstraintForce")
    force_constraint.References = [(doc.beam, "Face6")]
    force_constraint.Force = 21
    force_constraint.Reversed = True
    analysis.addObject(force_constraint)

    doc.recompute()

    return doc

