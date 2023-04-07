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

import sys
import FreeCAD
from FreeCAD import Vector

import Draft
import ObjectsFem
import Part

from BOPTools import SplitFeatures
from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Magnetostatic - Elmer 2D",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["magnetization"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["magnetostatic"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_magnetostatics_2D_elmer import setup
setup()

Magnetodynamic2D equation - Elmer solver

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects

    # wire defining the lower horse shoe end
    p1 = Vector(0.0, -200.0, 0.0)
    p2 = Vector(200.0, -200.0, 0.0)
    p3 = Vector(200.0, -100.0, 0.0)
    p4 = Vector(0.0, -100.0, 0.0)
    Horseshoe_lower = Draft.make_wire([p1, p2, p3, p4], closed=True)
    Horseshoe_lower.Label = "Lower_End"
    Horseshoe_lower.ViewObject.Visibility = False

    # wire defining the upper horse shoe end
    p1 = Vector(0.0, 100.0, 0.0)
    p2 = Vector(200.0, 100.0, 0.0)
    p3 = Vector(200.0, 200.0, 0.0)
    p4 = Vector(0.0, 200.0, 0.0)
    Horseshoe_upper = Draft.make_wire([p1, p2, p3, p4], closed=True)
    Horseshoe_upper.Label = "Upper_End"
    Horseshoe_upper.ViewObject.Visibility = False

    # the U-part of the horse shoe
    # credits: https://forum.freecad.org/viewtopic.php?p=663051#p663051
    vpairs = [[Vector(340.0, 200.0, 0.0), Vector(200.0, 200.0, 0.0)],
              [Vector(200.0, 200.0, 0.0), Vector(200.0, 100.0, 0.0)],
              [Vector(200.0, 100.0, 0.0), Vector(325.0, 100.0, 0.0)],
              [Vector(325.0, 100.0, 0.0), Vector(325.0, -100.0, 0.0)],
              [Vector(325.0, -100.0, 0.0), Vector(200.0, -100.0, 0.0)],
              [Vector(200.0, -100.0, 0.0), Vector(200.0, -200.0, 0.0)],
              [Vector(200.0, -200.0, 0.0), Vector(340.0, -200.0, 0.0)],
              [Vector(340.0, 200.0, 0.0), Vector(340.0, -200.0, 0.0)]]
    typeId = ['Part::GeomLine', 'Part::GeomLine', 'Part::GeomLine', 'Part::GeomBSplineCurve',
              'Part::GeomLine', 'Part::GeomLine', 'Part::GeomLine', 'Part::GeomBSplineCurve']
    e3Poles = [Vector(325.0, 100.0, 0.0), Vector(400.0, 100.0, 0.0),
               Vector(400.0, 0.0, 0.0), Vector(400.0, -100.0, 0.0),
               Vector(325.0, -100.0, 0.0)]
    e3Knots = [0.0, 0.5, 1.0]
    e3Mults = [4, 1, 4]
    e3Degree = 3
    e7Poles = [Vector(340.0, 200.0, 0.0), Vector(500.0, 200.0, 0.0),
               Vector(500.0, 0.0, 0.0), Vector(500.0, -200.0, 0.0),
               Vector(340.0, -200.0, 0.0)]
    e7Knots = [0.0, 0.5, 1.0]
    e7Mults = [4, 1, 4]
    e7Degree = 3
    c3 = Part.BSplineCurve()
    c3.buildFromPolesMultsKnots(e3Poles, e3Mults, e3Knots, False, e3Degree)
    c7 = Part.BSplineCurve()
    c7.buildFromPolesMultsKnots(e7Poles, e7Mults, e7Knots, False, e7Degree)
    edges = [c3.toShape(), c7.toShape()]
    for i in range(len(typeId)):
        if typeId[i] == 'Part::GeomLine':
            edges.append(Part.makeLine(*vpairs[i]))

    sedges = Part.__sortEdges__(edges)
    Horseshoe_U = doc.addObject("Part::Feature", "Horseshoe_U")
    Horseshoe_U.Shape = Part.Face(Part.Wire(sedges))

    # a circle defining later the air volume
    Air_Circle = doc.addObject("Part::Feature", "Air_Circle")
    Air_Circle.Shape = Part.Face(Part.Wire(Part.makeCircle(1500.0)))

    # a link of the Horseshoe_lower
    Horseshoe_lowerLink = doc.addObject("App::Link", "Link_Lower_End")
    Horseshoe_lowerLink.LinkTransform = True
    Horseshoe_lowerLink.LinkedObject = Horseshoe_lower
    Horseshoe_lowerLink.ViewObject.Visibility = False

    # a link of the Horseshoe_upper
    Horseshoe_upperLink = doc.addObject("App::Link", "Link_Upper_End")
    Horseshoe_upperLink.LinkTransform = True
    Horseshoe_upperLink.LinkedObject = Horseshoe_upper
    Horseshoe_upperLink.ViewObject.Visibility = False

    # a link of the Horseshoe_U
    Horseshoe_ULink = doc.addObject("App::Link", "Link_Horseshoe_U")
    Horseshoe_ULink.LinkTransform = True
    Horseshoe_ULink.LinkedObject = Horseshoe_U
    Horseshoe_ULink.ViewObject.Visibility = False

    # fusion of all links
    Fusion = doc.addObject("Part::MultiFuse", "Fusion")
    Fusion.Shapes = [Horseshoe_lowerLink, Horseshoe_upperLink, Horseshoe_ULink]
    Fusion.ViewObject.Visibility = False

    # cut all objects from Air wire to get volume of fluid
    Cut = doc.addObject("Part::Cut", "Cut_Air")
    Cut.Base = Air_Circle
    Cut.Tool = Fusion
    Cut.ViewObject.Visibility = False

    # BooleanFregments object to combine cut with rod
    BooleanFragments = SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    BooleanFragments.Objects = [Horseshoe_lower, Horseshoe_upper, Horseshoe_U, Cut]

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        BooleanFragments.ViewObject.Document.activeView().viewTop()
        BooleanFragments.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        solver_obj.CoordinateSystem = "Cartesian 2D"
        equation_magnetodynamic2D = ObjectsFem.makeEquationMagnetodynamic2D(doc, solver_obj)
        equation_magnetodynamic2D.CalculateMagneticFieldStrength = True
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
        return doc
    analysis.addObject(solver_obj)

    # materials

    # air around the horse shoe
    material_obj = ObjectsFem.makeMaterialFluid(doc, "Air")
    mat = material_obj.Material
    mat["Name"] = "Air"
    mat["Density"] = "1.204 kg/m^3"
    mat["ThermalConductivity"] = "0.02587 W/m/K"
    mat["ThermalExpansionCoefficient"] = "3.43e-3 1/K"
    mat["SpecificHeat"] = "1.01 kJ/kg/K"
    mat["ElectricalConductivity"] = "1e-12 S/m"
    mat["RelativePermeability"] = "1.0"
    mat["RelativePermittivity"] = "1.00059"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Face4")]
    analysis.addObject(material_obj)

    # iron of the horse shoe
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Iron")
    mat = material_obj.Material
    mat["Name"] = "Iron Generic"
    mat["ElectricalConductivity"] = "10.3 MS/m"
    mat["RelativePermeability"] = "5000.0"
    material_obj.Material = mat
    material_obj.References = [
        (BooleanFragments, "Face1"),
        (BooleanFragments, "Face2"),
        (BooleanFragments, "Face3")]
    analysis.addObject(material_obj)

    # magnetization lower
    Magnetization_lower = ObjectsFem.makeConstraintMagnetization(doc, "Magnetization_Lower_End")
    Magnetization_lower.References = [(BooleanFragments, "Face1")]
    Magnetization_lower.Magnetization_re_1 = "-7500.0 A/m"
    Magnetization_lower.Magnetization_re_1_Disabled = False
    analysis.addObject(Magnetization_lower)

    # magnetization upper
    Magnetization_upper = ObjectsFem.makeConstraintMagnetization(doc, "Magnetization_Upper_End")
    Magnetization_upper.References = [(BooleanFragments, "Face2")]
    Magnetization_upper.Magnetization_re_1 = "7500.0 A/m"
    Magnetization_upper.Magnetization_re_1_Disabled = False
    analysis.addObject(Magnetization_upper)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = BooleanFragments
    femmesh_obj.CharacteristicLengthMax = "100.0 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "6.5 mm"
    mesh_region.References = [
        (BooleanFragments, "Face1"),
        (BooleanFragments, "Face2"),
        (BooleanFragments, "Face3")]
    mesh_region.ViewObject.Visibility = False

    # generate the mesh
    from femmesh import gmshtools
    gmsh_mesh = gmshtools.GmshTools(femmesh_obj, analysis)
    try:
        error = gmsh_mesh.create_mesh()
    except Exception:
        error = sys.exc_info()[1]
        FreeCAD.Console.PrintError(
            "Unexpected error when creating mesh: {}\n"
            .format(error)
        )

    doc.recompute()
    return doc
