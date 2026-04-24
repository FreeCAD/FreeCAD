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

import ObjectsFem
import Materials
from BOPTools import BOPFeatures

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Electrostatics Electricforce - Elmer NonGUI6",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["electromagnetic"],
        "solvers": ["calculix", "elmer"],
        "material": "fluid",
        "equations": ["electrostatic"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.equation_electrostatics_electricforce_elmer_nongui6 import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=41488&start=40#p373292

Electrostatics equation in FreeCAD FEM-Elmer

"""
    )


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    capacitor = doc.addObject("Part::Box", "Capacitor")
    capacitor.Length = "5 mm"
    capacitor.Width = "5 mm"
    capacitor.Height = "1 mm"

    hole = doc.addObject("Part::Box", "Hole")
    hole.Length = "1.5 mm"
    hole.Width = "1.5 mm"
    hole.Height = "1.5 mm"
    hole.AttachmentSupport = [(capacitor, "Face6")]
    hole.MapMode = "FlatFace"

    extension = doc.addObject("Part::Box", "Extension")
    extension.Length = "5 mm"
    extension.Width = "5 mm"
    extension.Height = "5 mm"
    extension.AttachmentSupport = [(hole, "Face6")]
    extension.MapMode = "FlatFace"

    bp = BOPFeatures.BOPFeatures(doc)
    fusion = bp.make_multi_fuse([capacitor.Name, hole.Name, extension.Name])
    fusion.Refine = True

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        ObjectsFem.makeEquationElectrostatic(doc, solver_obj)
        ObjectsFem.makeEquationElectricforce(doc, solver_obj)
    elif solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculiX(doc, "SolverCalculiX")
        solver_obj.AnalysisType = "electromagnetic"
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
    air_obj.UUID = air.UUID
    air_obj.Material = air.Properties
    air_obj.References = [(fusion, "Solid1")]
    analysis.addObject(air_obj)

    # constraint potential 0V
    name_pot1 = "ElectrostaticPotential1"
    con_elect_pot1 = ObjectsFem.makeConstraintElectromagnetic(doc, name_pot1)
    con_elect_pot1.References = [(fusion, "Face6")]
    con_elect_pot1.Potential = "0 V"
    con_elect_pot1.CapacitanceBody = 1
    con_elect_pot1.CapacitanceBodyEnabled = True
    con_elect_pot1.PotentialEnabled = True
    analysis.addObject(con_elect_pot1)

    # constraint potential 1V
    name_pot2 = "ElectrostaticPotential2"
    con_elect_pot2 = ObjectsFem.makeConstraintElectromagnetic(doc, name_pot2)
    con_elect_pot2.References = [(fusion, "Face4")]
    con_elect_pot2.Potential = "1 V"
    con_elect_pot2.CapacitanceBody = 2
    con_elect_pot2.CapacitanceBodyEnabled = True
    con_elect_pot2.PotentialEnabled = True
    con_elect_pot2.ElectricForcecalculation = True
    analysis.addObject(con_elect_pot2)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = fusion
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = "0.5 mm"

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "0.25 mm"
    mesh_region.References = [
        (fusion, ("Face3", "Face4", "Face8", "Face9")),
    ]

    doc.recompute()
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)
        fusion.ViewObject.Document.activeView().viewAxonometric()
        fusion.ViewObject.Document.activeView().fitAll()
        femmesh_obj.ViewObject.Visibility = False
        mesh_region.ViewObject.Visibility = False

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
