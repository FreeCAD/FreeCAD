# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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


# connstraint contact for solid to solid mesh
# https://forum.freecadweb.org/viewtopic.php?f=18&t=20276
# to run the example use:
"""
from femexamples.constraint_contact_solid_solid import setup
setup()

"""


import FreeCAD
import ObjectsFem
import Fem
from FreeCAD import Vector, Rotation


mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup(doc=None, solvertype="ccxtools"):
    # setup model

    if doc is None:
        doc = init_doc()

    # parts
    # bottom box
    bottom_box_obj = doc.addObject("Part::Box", "TopBox")
    bottom_box_obj.Length = 100
    bottom_box_obj.Width = 25
    bottom_box_obj.Height = 500
    bottom_box_obj.Placement = FreeCAD.Placement(
        Vector(186, 0, -247),
        Rotation(0, 0, 0),
        Vector(0, 0, 0),
    )

    # top half cylinder
    top_cylinder_obj = doc.addObject("Part::Cylinder", "BottomCylinder")
    top_cylinder_obj.Radius = 30
    top_cylinder_obj.Height = 500
    top_cylinder_obj.Placement = FreeCAD.Placement(
        Vector(0, -42, 0),
        Rotation(0, 90, 0),
        Vector(0, 0, 0),
    )
    top_box_obj = doc.addObject("Part::Box", "BottomBox")
    top_box_obj.Length = 600
    top_box_obj.Width = 100
    top_box_obj.Height = 100
    top_box_obj.Placement = FreeCAD.Placement(
        Vector(-10, -142, -52),
        Rotation(0, 0, 0),
        Vector(0, 0, 0),
    )
    top_halfcyl_obj = doc.addObject("Part::Cut", "BottomHalfCylinder")
    top_halfcyl_obj.Base = top_cylinder_obj
    top_halfcyl_obj.Tool = top_box_obj
    if FreeCAD.GuiUp:
        top_cylinder_obj.ViewObject.hide()
        top_box_obj.ViewObject.hide()
    doc.recompute()

    # all geom fusion
    all_geom_fusion_obj = doc.addObject("Part::MultiFuse", "AllGeomFusion")
    all_geom_fusion_obj.Shapes = [bottom_box_obj, top_halfcyl_obj]
    if FreeCAD.GuiUp:
        bottom_box_obj.ViewObject.hide()
        top_halfcyl_obj.ViewObject.hide()

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
        solver_object.SplitInputWriter = False

        """
        # solver parameter from fandaL, but they are not needed (see forum topic)
        solver_object.IterationsControlParameterTimeUse = True
        solver_object.IterationsControlParameterCutb = '0.25,0.5,0.75,0.85,,,1.5,'
        solver_object.IterationsControlParameterIter = '4,8,9,200,10,400,,200,,'
        solver_object.IterationsUserDefinedTimeStepLength = True
        solver_object.TimeInitialStep = 0.1
        solver_object.TimeEnd = 1.0
        solver_object.IterationsUserDefinedIncrementations = True  # parameter DIRECT
        """

    # material
    material_obj = analysis.addObject(
        ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    )[0]
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = analysis.addObject(
        ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    )[0]
    con_fixed.References = [
        (all_geom_fusion_obj, "Face5"),
        (all_geom_fusion_obj, "Face6"),
        (all_geom_fusion_obj, "Face8"),
        (all_geom_fusion_obj, "Face10"),
    ]

    # constraint pressure
    con_pressure = analysis.addObject(
        ObjectsFem.makeConstraintPressure(doc, name="ConstraintPressure")
    )[0]
    con_pressure.References = [(all_geom_fusion_obj, "Face9")]
    con_pressure.Pressure = 100.0  # Pa ? = 100 Mpa ?
    con_pressure.Reversed = False

    # constraint contact
    con_contact = doc.Analysis.addObject(
        ObjectsFem.makeConstraintContact(doc, name="ConstraintContact")
    )[0]
    con_contact.References = [
        (all_geom_fusion_obj, "Face7"),  # first seams slave face, TODO proof in writer code!
        (all_geom_fusion_obj, "Face3"),  # second seams master face, TODO proof in writer code!
    ]
    con_contact.Friction = 0.0
    con_contact.Slope = 1000000.0  # contact stiffness 1000000.0 kg/(mm*s^2)

    # mesh
    from .meshes.mesh_contact_box_halfcylinder_tetra10 import create_nodes, create_elements
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
