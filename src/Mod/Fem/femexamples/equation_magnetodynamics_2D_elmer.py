# ***************************************************************************
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
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
from FreeCAD import Vector

import ObjectsFem
import Materials
import Part

from . import manager
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "Inductive heating - Elmer 2D",
        "meshtype": "face",
        "meshelement": "Tria3",
        "constraints": ["current density"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["electromagnetic"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.equation_magnetodynamics_2D_elmer import setup
setup()

Magnetodynamic2D equation - Elmer solver

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

    # wire defining the insulation area
    p1 = Vector(0.0, 0.0, 0.0)
    p2 = Vector(40.0, 0.0, 0.0)
    p3 = Vector(40.0, 120.0, 0.0)
    p4 = Vector(0.0, 120.0, 0.0)
    p5 = Vector(0.0, 100.0, 0.0)
    p6 = Vector(25.0, 100.0, 0.0)
    p7 = Vector(25.0, 20.0, 0.0)
    p8 = Vector(0.0, 20.0, 0.0)
    insulation = Part.makePolygon([p1, p2, p3, p4, p5, p6, p7, p8, p1])

    # wire defining the coil volume
    p1 = Vector(50.0, -10.0, 0.0)
    p2 = Vector(55.0, -10.0, 0.0)
    p3 = Vector(55.0, 110.0, 0.0)
    p4 = Vector(50.0, 110.0, 0.0)
    coil = Part.makePolygon([p1, p2, p3, p4, p1])

    # wire defining the crucible area
    p1 = Vector(0.0, 20.0, 0.0)
    p2 = Vector(25.0, 20.0, 0.0)
    p3 = Vector(25.0, 100.0, 0.0)
    p4 = Vector(0.0, 100.0, 0.0)
    p5 = Vector(0.0, 90.0, 0.0)
    p6 = Vector(20.0, 90.0, 0.0)
    p7 = Vector(20.0, 30.0, 0.0)
    p8 = Vector(0.0, 30.0, 0.0)
    crucible = Part.makePolygon([p1, p2, p3, p4, p5, p6, p7, p8, p1])

    # wire defining the powder volume
    p1 = Vector(0.0, 30.0, 0.0)
    p2 = Vector(20.0, 30.0, 0.0)
    p3 = Vector(20.0, 40.0, 0.0)
    p4 = Vector(0.0, 40.0, 0.0)
    powder = Part.makePolygon([p1, p2, p3, p4, p1])

    # a half circle defining later the air volume
    air_circle = Part.makeCircle(140.0, Vector(0.0, 60.0, 0.0), Vector(0.0, 0.0, 1.0), -90.0, 90.0)
    air_line = Part.makeLine((0.0, -80.0, 0.0), (0.0, 200.0, 0.0))
    air_area = Part.makeFace(Part.Wire((air_circle, air_line)))
    cut_area = Part.makeFace((coil, insulation, powder, crucible))
    air_area = air_area.cut(cut_area)

    shape = Part.makeShell(air_area.Faces + cut_area.Faces)
    shell = doc.addObject("Part::Feature", "Shell")
    shell.Shape = shape

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.CoordinateSystem = "Axi Symmetric"
        equation_magnetodynamic2D = ObjectsFem.makeEquationMagnetodynamic2D(doc, solver_obj)
        equation_magnetodynamic2D.AngularFrequency = "50 kHz"
        equation_magnetodynamic2D.CalculateCurrentDensity = True
        equation_magnetodynamic2D.CalculateElectricField = True
        equation_magnetodynamic2D.CalculateJouleHeating = True
        equation_magnetodynamic2D.IsHarmonic = True
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    analysis.addObject(solver_obj)

    # materials
    mat_manager = Materials.MaterialManager()

    # air around the objects and inside the coil
    air = mat_manager.getMaterial("94370b96-c97e-4a3f-83b2-11d7461f7da7")
    air_obj = ObjectsFem.makeMaterialFluid(doc, "Air")
    air_obj.UUID = air.UUID
    air_obj.Material = air.Properties
    air_obj.References = [(shell, ("Face1", "Face3", "Face5"))]
    analysis.addObject(air_obj)

    # graphite of the crucible
    graphite = mat_manager.getMaterial("5c67b675-69a2-4782-902a-bb90c3952f07")
    graphite_obj = ObjectsFem.makeMaterialSolid(doc, "Material-Crucible")
    graphite_obj.UUID = graphite.UUID
    graphite_obj.Material = graphite.Properties
    graphite_obj.References = [(shell, "Face4")]
    analysis.addObject(graphite_obj)

    # insulation of the crucible
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material-Insulation")
    mat = material_obj.Material
    mat["Name"] = "Insulation"
    mat["ElectricalConductivity"] = "2.0e3 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(shell, "Face2")]
    analysis.addObject(material_obj)

    # material of powder
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material-Powder")
    mat = material_obj.Material
    mat["Name"] = "Powder"
    mat["ElectricalConductivity"] = "1e4 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(shell, "Face6")]
    analysis.addObject(material_obj)

    # constraint current density
    CurrentDensity = ObjectsFem.makeConstraintCurrentDensity(doc, "CurrentDensity")
    CurrentDensity.References = [(shell, "Face3")]
    CurrentDensity.NormalCurrentDensity_re = "250000.000 A/m^2"
    CurrentDensity.Mode = "Normal"
    analysis.addObject(CurrentDensity)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Shape = shell
    femmesh_obj.CharacteristicLengthMax = "3 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "1 mm"
    mesh_region.References = [
        (shell, ("Face2", "Face3", "Face4", "Face6")),
    ]
    mesh_region.ViewObject.Visibility = False

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        import FemGui
        shell.ViewObject.Transparency = 50
        shell.ViewObject.Document.activeView().fitAll()
        FemGui.setActiveAnalysis(analysis)

    # generate the mesh
    generate_mesh.mesh_from_mesher(femmesh_obj, "gmsh")

    doc.recompute()
    return doc
