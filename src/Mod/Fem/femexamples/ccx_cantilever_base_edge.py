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

import Fem
import ObjectsFem

from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def setup_cantilever_base_edge(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # geometric objects
    # load line
    load_line = doc.addObject("Part::Line", "LoadLine")
    load_line.X1 = 0
    load_line.Y1 = 0
    load_line.Z1 = 1000
    load_line.X2 = 0
    load_line.Y2 = 0
    load_line.Z2 = 0

    # cantilever line
    geom_obj = doc.addObject("Part::Line", "CantileverLine")
    geom_obj.X1 = 0
    geom_obj.Y1 = 500
    geom_obj.Z1 = 500
    geom_obj.X2 = 8000
    geom_obj.Y2 = 500
    geom_obj.Z2 = 500

    doc.recompute()

    if FreeCAD.GuiUp:
        load_line.ViewObject.Visibility = False
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
        solver_obj.WorkingDir = ""
    elif solvertype == "mystran":
        solver_obj = ObjectsFem.makeSolverMystran(doc, "SolverMystran")
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
    analysis.addObject(solver_obj)

    # beam section
    beamsection_obj = ObjectsFem.makeElementGeometry1D(
        doc,
        sectiontype="Rectangular",
        width=1000.0,
        height=1000.0,
        name="BeamCrossSection",
    )
    analysis.addObject(beamsection_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Calculix-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "ConstraintFixed")
    con_fixed.References = [(geom_obj, "Vertex1")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(geom_obj, "Vertex2")]
    con_force.Force = "9000000.0 N"  # 9 MN
    con_force.Direction = (load_line, ["Edge1"])
    con_force.Reversed = False
    analysis.addObject(con_force)

    # mesh
    from .meshes.mesh_canticcx_seg3 import create_nodes, create_elements

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Shape = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.ElementDimension = "1D"
    femmesh_obj.CharacteristicLengthMax = "1750.0 mm"
    femmesh_obj.CharacteristicLengthMin = "1750.0 mm"

    doc.recompute()
    return doc
