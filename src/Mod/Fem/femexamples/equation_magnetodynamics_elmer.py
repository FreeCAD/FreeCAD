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

import Fem
import ObjectsFem

from BOPTools import SplitFeatures
from BasicShapes import Shapes
from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Magnetic Field Around Wire",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["electrostatic potential", "magnetization"],
        "solvers": ["elmer"],
        "material": "solid",
        "equations": ["electromagnetic"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_magnetodynamics_elmer import setup
setup()

Magnetodynamic equation - Elmer solver

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects

    # wire
    Wire = doc.addObject("Part::Cylinder", "Wire")
    Wire.Radius = "1 mm"
    Wire.Height = "10 mm"
    Wire.ViewObject.Visibility = False

    # air around wire
    Air = Shapes.addTube(doc, "Tube")
    Air.Label = "AirObject"
    Air.OuterRadius = "5 mm"
    Air.InnerRadius = "1 mm"
    Air.Height = "10 mm"
    Air.ViewObject.Visibility = False

    # BooleanFregments object to combine cut with rod
    BooleanFragments = SplitFeatures.makeBooleanFragments(name="BooleanFragments")
    BooleanFragments.Objects = [Wire, Air]

    # set view
    doc.recompute()
    if FreeCAD.GuiUp:
        BooleanFragments.ViewObject.Transparency = 75
        BooleanFragments.ViewObject.Document.activeView().viewAxonometric()
        BooleanFragments.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        eq_electrostatic = ObjectsFem.makeEquationMagnetodynamic(doc, solver_obj)
        eq_electrostatic.AngularFrequency = "100 kHz"
        eq_electrostatic.BiCGstablDegree = 4
        eq_electrostatic.IsHarmonic = True
        eq_electrostatic.LinearIterativeMethod = "BiCGStabl"
        eq_electrostatic.LinearPreconditioning = "None"
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)

    # material

    # air around the wire
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
    material_obj.References = [(BooleanFragments, "Solid2")]
    analysis.addObject(material_obj)

    # copper wire
    material_obj = ObjectsFem.makeMaterialSolid(doc, "Copper")
    mat = material_obj.Material
    mat["Name"] = "Copper Generic"
    mat["Density"] = "8960 kg/m^3"
    mat["PoissonRatio"] = "0.343"
    mat["ShearModulus"] = "46.0 GPa"
    mat["UltimateTensileStrength"] = "210 MPa"
    mat["YoungsModulus"] = "119 GPa"
    mat["ThermalConductivity"] = "398.0 W/m/K"
    mat["ThermalExpansionCoefficient"] = "16.5 µm/m/K"
    mat["SpecificHeat"] = "385.0 J/kg/K"
    mat["ElectricalConductivity"] = "59.59 MS/m"
    mat["RelativePermeability"] = "0.999994"
    material_obj.Material = mat
    material_obj.References = [(BooleanFragments, "Solid1")]
    analysis.addObject(material_obj)

    # axial field around the wire
    AxialField = ObjectsFem.makeConstraintElectrostaticPotential(doc, "AxialField")
    AxialField.References = [
        (BooleanFragments, "Face4"),
        (BooleanFragments, "Face5"),
        (BooleanFragments, "Face6")]
    AxialField.PotentialEnabled = False
    AxialField.AV_im_1_Disabled = False
    AxialField.AV_im_2_Disabled = False
    AxialField.AV_re_1_Disabled = False
    AxialField.AV_re_2_Disabled = False
    analysis.addObject(AxialField)

    # voltage on one end
    Voltage = ObjectsFem.makeConstraintElectrostaticPotential(doc, "Voltage")
    Voltage.References = [(BooleanFragments, "Face3")]
    Voltage.Potential = "10.000 mV"
    Voltage.AV_im_1_Disabled = False
    Voltage.AV_im_2_Disabled = False
    Voltage.AV_re_1_Disabled = False
    Voltage.AV_re_2_Disabled = False
    analysis.addObject(Voltage)

    # ground on other end
    Ground = ObjectsFem.makeConstraintElectrostaticPotential(doc, "Ground")
    Ground.References = [(BooleanFragments, "Face2")]
    Ground.Potential = "0 V"
    Ground.AV_im_1_Disabled = False
    Ground.AV_im_2_Disabled = False
    Ground.AV_re_1_Disabled = False
    Ground.AV_re_2_Disabled = False
    analysis.addObject(Ground)

    # magnetization
    Magnetization = ObjectsFem.makeConstraintMagnetization(doc, "Magnetization")
    Magnetization.References = [(BooleanFragments, "Solid1")]
    Magnetization.Magnetization_re_1 = "7500.000 A/m"
    Magnetization.Magnetization_re_2 = "7500.000 A/m"
    Magnetization.Magnetization_re_3 = "7500.000 A/m"
    Magnetization.Magnetization_re_2_Disabled = False
    analysis.addObject(Magnetization)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = BooleanFragments
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.CharacteristicLengthMax = "0.5 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "0.15 mm"
    mesh_region.References = [(BooleanFragments, "Solid1")]
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
    if error:
        # try to create from existing rough mesh
        from .meshes.mesh_capacitance_two_balls_tetra10 import create_nodes, create_elements
        fem_mesh = Fem.FemMesh()
        control = create_nodes(fem_mesh)
        if not control:
            FreeCAD.Console.PrintError("Error on creating nodes.\n")
        control = create_elements(fem_mesh)
        if not control:
            FreeCAD.Console.PrintError("Error on creating elements.\n")
        femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc
