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

import sys
import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector
from FreeCAD import Units

import Fem
import ObjectsFem
import Part
import Sketcher

from . import manager
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Electrostatics Electricforce - Elmer NonGUI6",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["electrostatic potential"],
        "solvers": ["elmer"],
        "material": "fluid",
        "equations": ["electrostatic"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.equation_electrostatics_electricforce_elmer_nongui6 import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=41488&start=40#p373292

Electrostatics equation in FreeCAD FEM-Elmer

"""


def setup(doc=None, solvertype="elmer"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # geometric objects
    # name is important because the other method in this module use obj name
    geom_obj = doc.addObject("PartDesign::Body", "Body")
    base_sketch = geom_obj.newObject("Sketcher::SketchObject", "Base_Sketch")
    base_sketch.Support = (doc.getObject("XY_Plane"), [""])
    base_sketch.MapMode = "FlatFace"
    base_geoList = [
        Part.LineSegment(Vector(0.000000, 0.000000, 0), Vector(57.407921, 0.000000, 0)),
        Part.LineSegment(Vector(57.407921, 0.000000, 0), Vector(57.407921, 35.205284, 0)),
        Part.LineSegment(Vector(57.407921, 35.205284, 0), Vector(0.000000, 35.205284, 0)),
        Part.LineSegment(Vector(0.000000, 35.205284, 0), Vector(0.000000, 0.000000, 0))]
    base_sketch.addGeometry(base_geoList, False)
    base_conList = [
        Sketcher.Constraint("Coincident", 0, 2, 1, 1),
        Sketcher.Constraint("Coincident", 1, 2, 2, 1),
        Sketcher.Constraint("Coincident", 2, 2, 3, 1),
        Sketcher.Constraint("Coincident", 3, 2, 0, 1),
        Sketcher.Constraint("Horizontal", 0),
        Sketcher.Constraint("Horizontal", 2),
        Sketcher.Constraint("Vertical", 1),
        Sketcher.Constraint("Vertical", 3),
        Sketcher.Constraint("Coincident", 0, 1, -1, 1),
        Sketcher.Constraint("DistanceY", 1, 1, 1, 2, 35.205284),
        Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 57.407921)]
    base_sketch.addConstraint(base_conList)
    base_sketch.setDatum(9, Units.Quantity("5000.000000 mm"))
    base_sketch.setDatum(10, Units.Quantity("5000.000000 mm"))
    base_sketch.ViewObject.Visibility = False

    pad = geom_obj.newObject("PartDesign::Pad", "Pad")
    pad.Profile = base_sketch
    pad.Length = 7500.0
    pad.Length2 = 1000.0

    upper_sketch = geom_obj.newObject("Sketcher::SketchObject", "Upper_Sketch")
    upper_sketch.Support = None
    upper_sketch.MapMode = "Deactivated"
    upper_sketch.Placement = FreeCAD.Placement(Vector(0, 0, 1000), Rotation(Vector(0, 0, 1), 0))
    upper_geoList = [
        Part.LineSegment(Vector(25.560951, 4958.778320, 0), Vector(5068.406250, 4958.778320, 0)),
        Part.LineSegment(Vector(5068.406250, 4958.778320, 0), Vector(5037.082520, -21.422216, 0)),
        Part.LineSegment(Vector(5037.082520, 0.000000, 0), Vector(1309.763672, -21.422216, 0)),
        Part.LineSegment(Vector(1309.763672, 0.000000, 0), Vector(1372.406982, 1544.678467, 0)),
        Part.LineSegment(Vector(1372.406982, 1544.678467, 0), Vector(-37.083382, 1544.678467, 0)),
        Part.LineSegment(Vector(0.000000, 1544.678467, 0), Vector(25.560951, 4958.778320, 0))]
    upper_sketch.addGeometry(upper_geoList, False)
    upper_conList = [
        Sketcher.Constraint("Horizontal", 0),
        Sketcher.Constraint("Coincident", 1, 1, 0, 2),
        Sketcher.Constraint("PointOnObject", 1, 2, -1),
        Sketcher.Constraint("Vertical", 1),
        Sketcher.Constraint("Coincident", 2, 1, 1, 2),
        Sketcher.Constraint("PointOnObject", 2, 2, -1),
        Sketcher.Constraint("Coincident", 3, 1, 2, 2),
        Sketcher.Constraint("Coincident", 4, 1, 3, 2),
        Sketcher.Constraint("PointOnObject", 4, 2, -2),
        Sketcher.Constraint("Horizontal", 4),
        Sketcher.Constraint("Coincident", 5, 1, 4, 2),
        Sketcher.Constraint("Coincident", 5, 2, 0, 1),
        Sketcher.Constraint("Vertical", 5),
        Sketcher.Constraint("Vertical", 3),
        Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 5037.082520),
        Sketcher.Constraint("DistanceY", 1, 2, 1, 1, 4958.778320),
        Sketcher.Constraint("DistanceY", 3, 1, 3, 2, 1544.678467),
        Sketcher.Constraint("DistanceX", 4, 2, 4, 1, 1309.763672)]
    upper_sketch.addConstraint(upper_conList)
    upper_sketch.setDatum(14, Units.Quantity("5000.000000 mm"))
    upper_sketch.setDatum(15, Units.Quantity("5000.000000 mm"))
    upper_sketch.setDatum(16, Units.Quantity("1500.000000 mm"))
    upper_sketch.setDatum(17, Units.Quantity("1500.000000 mm"))
    upper_sketch.ViewObject.Visibility = False

    pocket = geom_obj.newObject("PartDesign::Pocket", "Pocket")
    pocket.Profile = upper_sketch
    pocket.Length = 1500.0
    pocket.Length2 = 100.0
    pocket.Reversed = 1
    doc.recompute()

    if FreeCAD.GuiUp:
        geom_obj.ViewObject.Document.activeView().viewAxonometric()
        geom_obj.ViewObject.Document.activeView().fitAll()

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, "Analysis")
    if FreeCAD.GuiUp:
        import FemGui
        FemGui.setActiveAnalysis(analysis)

    # solver
    if solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        ObjectsFem.makeEquationElectrostatic(doc, solver_obj)
        ObjectsFem.makeEquationElectricforce(doc, solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    analysis.addObject(solver_obj)

    # material
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
    material_obj.References = [(geom_obj, "Solid1")]
    analysis.addObject(material_obj)

    # constraint potential 0V
    name_pot1 = "ElectrostaticPotential1"
    con_elect_pot1 = ObjectsFem.makeConstraintElectrostaticPotential(doc, name_pot1)
    con_elect_pot1.References = [(geom_obj, "Face2")]
    con_elect_pot1.Potential = "0 V"
    con_elect_pot1.CapacitanceBody = 1
    con_elect_pot1.CapacitanceBodyEnabled = True
    con_elect_pot1.PotentialEnabled = True
    analysis.addObject(con_elect_pot1)

    # constraint potential 1V
    name_pot2 = "ElectrostaticPotential2"
    con_elect_pot2 = ObjectsFem.makeConstraintElectrostaticPotential(doc, name_pot2)
    con_elect_pot2.References = [
        (geom_obj, "Face4"),
        (geom_obj, "Face5"),
        (geom_obj, "Face6"),
        (geom_obj, "Face11")]
    con_elect_pot2.Potential = "1 V"
    con_elect_pot2.CapacitanceBody = 2
    con_elect_pot2.CapacitanceBodyEnabled = True
    con_elect_pot2.PotentialEnabled = True
    con_elect_pot2.ElectricForcecalculation = True
    analysis.addObject(con_elect_pot2)

    # mesh
    femmesh_obj = analysis.addObject(ObjectsFem.makeMeshGmsh(doc, get_meshname()))[0]
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.CharacteristicLengthMax = "500 mm"
    femmesh_obj.ViewObject.Visibility = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj, name="MeshRegion")
    mesh_region.CharacteristicLength = "250 mm"
    mesh_region.References = [
        (geom_obj, "Face4"),
        (geom_obj, "Face5"),
        (geom_obj, "Face6"),
        (geom_obj, "Face11")]
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
        from .meshes.mesh_electricforce_elmer_nongui6_tetra10 import create_nodes, create_elements
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
