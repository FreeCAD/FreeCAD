# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# to run the example use:
"""
from femexamples.magnetic_shielding_2D import setup
setup()
"""


import sys
import FreeCAD

import ObjectsFem
import Materials
import Part

from . import manager
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Magnetic shielding 2D",
        "meshtype": "face",
        "meshelement": "Tria6",
        "constraints": ["electrostatic potential"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["electromagnetic"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.magnetic_shielding_2D import setup
setup()

Magnetostatic equation - Elmer solver

"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = manager.init_doc()

    # explanation object
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    c1 = Part.makeCircle(15)
    c2 = Part.makeCircle(20)
    c3 = Part.makeCircle(100)
    f1 = Part.makeFace([c1])
    f2 = Part.makeFace([c1, c2])
    f3 = Part.makeFace([c2, c3])

    shape = Part.makeShell([f1, f2, f3])
    shell = doc.addObject("Part::Feature", "Shell")
    shell.Shape = shape

    if FreeCAD.GuiUp:
        shell.ViewObject.Visibility = True
        shell.ViewObject.Document.ActiveView.viewTop()
        shell.ViewObject.Document.ActiveView.fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui

        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.SimulationType = "Steady State"
        solver_obj.CoordinateSystem = "Cartesian 2D"
        eq_mgdyn_2d = ObjectsFem.makeEquationMagnetodynamic2D(doc, solver_obj)
        eq_mgdyn_2d.References = [(shell, ("Face1", "Face2", "Face3"))]
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)

    # material
    mat_manager = Materials.MaterialManager()
    air = mat_manager.getMaterial("94370b96-c97e-4a3f-83b2-11d7461f7da7")
    air_obj = ObjectsFem.makeMaterialFluid(doc, "Air")
    air_obj.UUID = "94370b96-c97e-4a3f-83b2-11d7461f7da7"
    air_obj.Material = air.Properties

    iron = mat_manager.getMaterial("1826c364-d26a-43fb-8f61-288281236836")
    iron_obj = ObjectsFem.makeMaterialFluid(doc, "Iron")
    iron_obj.UUID = "1826c364-d26a-43fb-8f61-288281236836"
    iron_obj.Material = iron.Properties

    air_obj.References = [(shell, ("Face1","Face3"))]
    iron_obj.References = [(shell, ("Face2",))]
    analysis.addObject(air_obj)
    analysis.addObject(iron_obj)

    # boundary condition
    mg_den = ObjectsFem.makeConstraintElectrostaticPotential(doc, "MagneticDensity")
    mg_den.References = [(shell, "Edge3")]
    mg_den.BoundaryCondition = "Neumann"
    mg_den.EnableMagnetic_1 = True
    mg_den.Magnetic_re_1 = "10.0 G"
    analysis.addObject(mg_den)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, manager.get_meshname()))[0]
    femmesh_obj.Shape = shell
    femmesh_obj.ElementOrder = "2nd"
    femmesh_obj.CharacteristicLengthMax = "8 mm"
    femmesh_obj.ViewObject.Visibility = False
    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "2 mm"
    mesh_region.References = [(shell, "Face2")]
    mesh_region.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
