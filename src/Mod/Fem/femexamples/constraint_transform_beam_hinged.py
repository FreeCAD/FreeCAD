# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

from CompoundTools import CompoundFilter

import Fem
import ObjectsFem

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Constraint Transform Beam Hinged",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["pressure", "displacement", "transform"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.constraint_transform_beam_hinged import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=20238#p157643

Constraint transform on a beam

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric object
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
        solver_obj = ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
    elif solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        solver_obj.WorkingDir = u""
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
    analysis.addObject(solver_obj)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "FemMaterial")
    mat = material_obj.Material
    mat["Name"] = "CalculiX-Steel"
    mat["YoungsModulus"] = "210000 MPa"
    mat["PoissonRatio"] = "0.30"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # constraint pressure
    con_pressure = ObjectsFem.makeConstraintPressure(doc, name="FemConstraintPressure")
    con_pressure.References = [(geom_obj, "Face8")]
    con_pressure.Pressure = "10.0 MPa"
    con_pressure.Reversed = False
    analysis.addObject(con_pressure)

    # constraint displacement
    con_disp = ObjectsFem.makeConstraintDisplacement(doc, name="FemConstraintDisplacment")
    con_disp.References = [(geom_obj, "Face4"), (geom_obj, "Face5")]
    con_disp.xFree = False
    con_disp.xFix = True
    analysis.addObject(con_disp)

    # constraints transform
    con_transform1 = ObjectsFem.makeConstraintTransform(doc, name="FemConstraintTransform1")
    con_transform1.References = [(geom_obj, "Face4")]
    con_transform1.TransformType = "Cylindrical"
    con_transform1.X_rot = 0.0
    con_transform1.Y_rot = 0.0
    con_transform1.Z_rot = 0.0
    analysis.addObject(con_transform1)

    con_transform2 = ObjectsFem.makeConstraintTransform(doc, name="FemConstraintTransform2")
    con_transform2.References = [(geom_obj, "Face5")]
    con_transform2.TransformType = "Cylindrical"
    con_transform2.X_rot = 0.0
    con_transform2.Y_rot = 0.0
    con_transform2.Z_rot = 0.0
    analysis.addObject(con_transform2)

    # mesh
    from .meshes.mesh_transform_beam_hinged_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = '7 mm'

    doc.recompute()
    return doc
