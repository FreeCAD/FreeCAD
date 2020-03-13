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
from femexamples.material_nl_platewithhole import setup
setup()

"""


import FreeCAD
from FreeCAD import Vector as vec

import Fem
import ObjectsFem
import Part
from Part import makeCircle as ci
from Part import makeLine as ln

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    """ Nonlinear material example, plate with hole.

    https://forum.freecadweb.org/viewtopic.php?f=24&t=31997&start=30
    https://forum.freecadweb.org/viewtopic.php?t=33974&start=90
    https://forum.freecadweb.org/viewtopic.php?t=35893

    plate: 400x200x10 mm
    hole: diameter 100 mm (half cross section)
    load: 130 MPa tension
    linear material: Steel, E = 210000 MPa, my = 0.3
    nonlinear material: '240.0, 0.0' to '270.0, 0.025'
    TODO nonlinear material: give more information, use values from harry
    TODO compare results with example from HarryvL

    """

    if doc is None:
        doc = init_doc()

    # geometry objects

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
        import FreeCADGui
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        FreeCADGui.SendMsgToActiveView("ViewFit")

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "calculix":
        solver = analysis.addObject(
            ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
        )[0]
    elif solvertype == "ccxtools":
        solver = analysis.addObject(
            ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        )[0]
        solver.WorkingDir = u""
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver.SplitInputWriter = False
        solver.AnalysisType = "static"
        solver.GeometricalNonlinearity = "linear"
        solver.ThermoMechSteadyState = False
        solver.MatrixSolverType = "default"
        solver.IterationsControlParameterTimeUse = False
        solver.GeometricalNonlinearity = 'nonlinear'
        solver.MaterialNonlinearity = 'nonlinear'

    # linear material
    matprop = {}
    matprop["Name"] = "CalculiX-Steel"
    matprop["YoungsModulus"] = "210000 MPa"
    matprop["PoissonRatio"] = "0.30"
    matprop["Density"] = "7900 kg/m^3"
    material = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "Material_lin")
    )[0]
    material.Material = matprop

    # nonlinear material
    nonlinear_material = analysis.addObject(
        ObjectsFem.makeMaterialMechanicalNonlinear(doc, material, "Material_nonlin")
    )[0]
    nonlinear_material.YieldPoint1 = '240.0, 0.0'
    nonlinear_material.YieldPoint2 = '270.0, 0.025'
    # check solver attributes, Nonlinearity needs to be set to nonlinear

    # fixed_constraint
    fixed_constraint = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    )[0]
    fixed_constraint.References = [(geom_obj, "Face4")]

    # force constraint
    pressure_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    )[0]
    pressure_constraint.References = [(geom_obj, "Face2")]
    pressure_constraint.Pressure = 130.0
    pressure_constraint.Reversed = True

    # mesh
    from .meshes.mesh_platewithhole_tetra10 import create_nodes, create_elements
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
