# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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
from FreeCAD import Rotation
from FreeCAD import Vector

import Part

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Constraint Contact Solid Solid",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "pressure", "contact"],
        "solvers": ["ccxtools"],
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.constraint_contact_solid_solid import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=20276
constraint contact for solid to solid mesh

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
    # bottom box
    bottom_box_obj = doc.addObject("Part::Box", "BottomBox")
    bottom_box_obj.Length = 100
    bottom_box_obj.Width = 25
    bottom_box_obj.Height = 500
    bottom_box_obj.Placement = FreeCAD.Placement(
        Vector(186, 0, -247),
        Rotation(0, 0, 0),
        Vector(0, 0, 0),
    )
    doc.recompute()

    # top half cylinder, https://forum.freecad.org/viewtopic.php?f=18&t=43001#p366111
    top_halfcyl_obj = doc.addObject("Part::Cylinder", "TopHalfCylinder")
    top_halfcyl_obj.Radius = 30
    top_halfcyl_obj.Height = 500
    top_halfcyl_obj.Angle = 180
    top_halfcyl_sh = Part.getShape(top_halfcyl_obj, "", needSubElement=False, refine=True)
    top_halfcyl_obj.Shape = top_halfcyl_sh
    top_halfcyl_obj.Placement = FreeCAD.Placement(
        Vector(0, -42, 0),
        Rotation(0, 90, 0),
        Vector(0, 0, 0),
    )
    doc.recompute()

    # all geom fusion
    geom_obj = doc.addObject("Part::MultiFuse", "AllGeomFusion")
    geom_obj.Shapes = [bottom_box_obj, top_halfcyl_obj]
    geom_obj.Refine = True
    if FreeCAD.GuiUp:
        bottom_box_obj.ViewObject.hide()
        top_halfcyl_obj.ViewObject.hide()
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
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = False
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
        solver_obj.SplitInputWriter = False
        """
        # solver parameter from fandaL, but they are not needed (see forum topic)
        solver_obj.IterationsControlParameterTimeUse = True
        solver_obj.IterationsControlParameterCutb = '0.25,0.5,0.75,0.85,,,1.5,'
        solver_obj.IterationsControlParameterIter = '4,8,9,200,10,400,,200,,'
        solver_obj.IterationsUserDefinedTimeStepLength = True
        solver_obj.TimeInitialStep = 0.1
        solver_obj.TimeEnd = 1.0
        solver_obj.IterationsUserDefinedIncrementations = True  # parameter DIRECT
        """
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [
        (geom_obj, "Face5"),
        (geom_obj, "Face6"),
        (geom_obj, "Face8"),
        (geom_obj, "Face9"),
    ]
    analysis.addObject(con_fixed)

    # constraint pressure
    con_pressure = ObjectsFem.makeConstraintPressure(doc, "ConstraintPressure")
    con_pressure.References = [(geom_obj, "Face10")]
    con_pressure.Pressure = "100.0 MPa"
    con_pressure.Reversed = False
    analysis.addObject(con_pressure)

    # constraint contact
    con_contact = ObjectsFem.makeConstraintContact(doc, "ConstraintContact")
    con_contact.References = [
        (geom_obj, "Face7"),  # first seems slave face, TODO proof in writer code!
        (geom_obj, "Face3"),  # second seems master face, TODO proof in writer code!
    ]
    con_contact.Friction = False
    con_contact.Slope = "1000000.0 GPa/m"
    analysis.addObject(con_contact)

    # mesh
    from .meshes.mesh_contact_box_halfcylinder_tetra10 import (
        create_nodes,
        create_elements,
    )

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False

    doc.recompute()
    return doc
