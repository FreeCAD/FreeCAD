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
        "name": "Inductive heating - Elmer 2D",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["current density"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["electromagnetic"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_magnetodynamics_2D_elmer import setup
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

    # wire defining the insulation area
    p1 = Vector(0.0, 0.0, 0.0)
    p2 = Vector(40.0, 0.0, 0.0)
    p3 = Vector(40.0, 120.0, 0.0)
    p4 = Vector(0.0, 120.0, 0.0)
    p5 = Vector(0.0, 100.0, 0.0)
    p6 = Vector(25.0, 100.0, 0.0)
    p7 = Vector(25.0, 20.0, 0.0)
    p8 = Vector(0.0, 20.0, 0.0)
    Insulation = Draft.make_wire([p1, p2, p3, p4, p5, p6, p7, p8], closed=True)
    Insulation.Label = "Insulation"
    Insulation.ViewObject.Visibility = False

    # wire defining the coil volume
    p1 = Vector(50.0, -10.0, 0.0)
    p2 = Vector(55.0, -10.0, 0.0)
    p3 = Vector(55.0, 110.0, 0.0)
    p4 = Vector(50.0, 110.0, 0.0)
    Coil = Draft.make_wire([p1, p2, p3, p4], closed=True)
    Coil.Label = "Coil"
    Coil.ViewObject.Visibility = False

    # wire defining the crucible area
    p1 = Vector(0.0, 20.0, 0.0)
    p2 = Vector(25.0, 20.0, 0.0)
    p3 = Vector(25.0, 100.0, 0.0)
    p4 = Vector(0.0, 100.0, 0.0)
    p5 = Vector(0.0, 90.0, 0.0)
    p6 = Vector(20.0, 90.0, 0.0)
    p7 = Vector(20.0, 30.0, 0.0)
    p8 = Vector(0.0, 30.0, 0.0)
    Crucible = Draft.make_wire([p1, p2, p3, p4, p5, p6, p7, p8], closed=True)
    Crucible.Label = "Crucible"
    Crucible.ViewObject.Visibility = False

    # wire defining the powder volume
    p1 = Vector(0.0, 30.0, 0.0)
    p2 = Vector(20.0, 30.0, 0.0)
    p3 = Vector(20.0, 40.0, 0.0)
    p4 = Vector(0.0, 40.0, 0.0)
    Powder = Draft.make_wire([p1, p2, p3, p4], closed=True)
    Powder.Label = "Powder"
    Powder.ViewObject.Visibility = False

    # a half circle defining later the air volume
    Air_Circle = Part.makeCircle(
        140.0, Vector(0.0, 60.0, 0.0), Vector(0.0, 0.0, 1.0), -90.0, 90.0)
    Air_Line = Part.makeLine((0.0, -80.0, 0.0), (0.0, 200.0, 0.0))
    Air_Area = doc.addObject("Part::Feature", "Air_Area")
    Air_Area.Shape = Part.Face([Part.Wire([Air_Circle, Air_Line])])

    # a link of the Insulation
    InsulationLink = doc.addObject("App::Link", "Link_Insulation")
    InsulationLink.LinkTransform = True
    InsulationLink.LinkedObject = Insulation
    InsulationLink.ViewObject.Visibility = False

    # a link of the Coil
    CoilLink = doc.addObject("App::Link", "Link_Coil")
    CoilLink.LinkTransform = True
    CoilLink.LinkedObject = Coil
    CoilLink.ViewObject.Visibility = False

    # a link of the Crucible
    CrucibleLink = doc.addObject("App::Link", "Link_Crucible")
    CrucibleLink.LinkTransform = True
    CrucibleLink.LinkedObject = Crucible
    CrucibleLink.ViewObject.Visibility = False

    # a link of the Powder
    PowderLink = doc.addObject("App::Link", "Link_Powder")
    PowderLink.LinkTransform = True
    PowderLink.LinkedObject = Powder
    PowderLink.ViewObject.Visibility = False

    # fusion of all links
    Fusion = doc.addObject("Part::MultiFuse", "Fusion")
    Fusion.Shapes = [InsulationLink, CoilLink, CrucibleLink, PowderLink]
    Fusion.ViewObject.Visibility = False

    # cut all objects from Air wire to get volume of fluid
    Cut = doc.addObject("Part::Cut", "Cut_Air")
    Cut.Base = Air_Area
    Cut.Tool = Fusion
    Cut.ViewObject.Visibility = False

    # BooleanFregments object to combine cut with rod
    BooleanFragments = SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    BooleanFragments.Objects = [Insulation, Coil, Crucible, Powder, Cut]

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
    analysis.addObject(solver_obj)

    # materials

    # air around the objects and inside the coil
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
    material_obj.References = [
        (BooleanFragments, "Face2"),
        (BooleanFragments, "Face5"),
        (BooleanFragments, "Face6")]
    analysis.addObject(material_obj)

    # graphite of the crucible
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material-Crucible")
    mat = material_obj.Material
    mat["Name"] = "Graphite"
    mat["ElectricalConductivity"] = "2e4 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Face3")]
    analysis.addObject(material_obj)

    # insulation of the crucible
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material-Insulation")
    mat = material_obj.Material
    mat["Name"] = "Insulation"
    mat["ElectricalConductivity"] = "2.0e3 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Face1")]
    analysis.addObject(material_obj)

    # material of powder
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Material-Powder")
    mat = material_obj.Material
    mat["Name"] = "Powder"
    mat["ElectricalConductivity"] = "1e4 S/m"
    mat["RelativePermeability"] = "1.0"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Face4")]
    analysis.addObject(material_obj)

    # constraint current density
    CurrentDensity = ObjectsFem.makeConstraintCurrentDensity(doc, "CurrentDensity")
    CurrentDensity.References = [(BooleanFragments, "Face2")]
    CurrentDensity.CurrentDensity_re_1 = "250000.000 A/m^2"
    CurrentDensity.CurrentDensity_re_1_Disabled = False
    analysis.addObject(CurrentDensity)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = BooleanFragments
    femmesh_obj.CharacteristicLengthMax = "3 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "1 mm"
    mesh_region.References = [
        (BooleanFragments, "Face1"),
        (BooleanFragments, "Face2"),
        (BooleanFragments, "Face3"),
        (BooleanFragments, "Face4")]
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
