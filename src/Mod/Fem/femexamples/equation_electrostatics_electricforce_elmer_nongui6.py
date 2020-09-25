# ***************************************************************************
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

# to run the example use:
"""
from femexamples.equation_electrostatics_electricforce_elmer_nongui6 import setup
setup()
"""
# Electrostatics equation in FreeCAD FEM-Elmer
# https://forum.freecadweb.org/viewtopic.php?f=18&t=41488&start=40#p373292

import FreeCAD
from FreeCAD import Rotation
from FreeCAD import Vector
from FreeCAD import Units

import Fem
import Part
import ObjectsFem
import Sketcher

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "Electrostatics Electricforce - Elmer NonGUI6",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["electrostatic potential"],
            "solvers": ["elmer"],
            "material": "fluid",
            "equation": "electrostatic"
            }
    return info


def setup(doc=None, solvertype="elmer"):
    # setup base model

    if doc is None:
        doc = init_doc()

    # geometry object
    # name is important because the other method in this module use obj name
    geom_obj = doc.addObject('PartDesign::Body', 'Body')
    base_sketch = geom_obj.newObject('Sketcher::SketchObject', 'Base_Sketch')
    base_sketch.Support = (doc.getObject('XY_Plane'), [''])
    base_sketch.MapMode = 'FlatFace'
    base_geoList = [
        Part.LineSegment(Vector(0.000000, 0.000000, 0), Vector(57.407921, 0.000000, 0)),
        Part.LineSegment(Vector(57.407921, 0.000000, 0), Vector(57.407921, 35.205284, 0)),
        Part.LineSegment(Vector(57.407921, 35.205284, 0), Vector(0.000000, 35.205284, 0)),
        Part.LineSegment(Vector(0.000000, 35.205284, 0), Vector(0.000000, 0.000000, 0))]
    base_sketch.addGeometry(base_geoList, False)
    base_conList = [
        Sketcher.Constraint('Coincident', 0, 2, 1, 1),
        Sketcher.Constraint('Coincident', 1, 2, 2, 1),
        Sketcher.Constraint('Coincident', 2, 2, 3, 1),
        Sketcher.Constraint('Coincident', 3, 2, 0, 1),
        Sketcher.Constraint('Horizontal', 0),
        Sketcher.Constraint('Horizontal', 2),
        Sketcher.Constraint('Vertical', 1),
        Sketcher.Constraint('Vertical', 3),
        Sketcher.Constraint('Coincident', 0, 1, -1, 1),
        Sketcher.Constraint('DistanceY', 1, 1, 1, 2, 35.205284),
        Sketcher.Constraint('DistanceX', 0, 1, 0, 2, 57.407921)]
    base_sketch.addConstraint(base_conList)
    base_sketch.setDatum(9, Units.Quantity('5000.000000 mm'))
    base_sketch.setDatum(10, Units.Quantity('5000.000000 mm'))

    pad = geom_obj.newObject('PartDesign::Pad', 'Pad')
    pad.Profile = base_sketch
    pad.Length = 7500.0
    pad.Length2 = 1000.0

    upper_sketch = geom_obj.newObject('Sketcher::SketchObject', 'Upper_Sketch')
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
        Sketcher.Constraint('Horizontal', 0),
        Sketcher.Constraint('Coincident', 1, 1, 0, 2),
        Sketcher.Constraint('PointOnObject', 1, 2, -1),
        Sketcher.Constraint('Vertical', 1),
        Sketcher.Constraint('Coincident', 2, 1, 1, 2),
        Sketcher.Constraint('PointOnObject', 2, 2, -1),
        Sketcher.Constraint('Coincident', 3, 1, 2, 2),
        Sketcher.Constraint('Coincident', 4, 1, 3, 2),
        Sketcher.Constraint('PointOnObject', 4, 2, -2),
        Sketcher.Constraint('Horizontal', 4),
        Sketcher.Constraint('Coincident', 5, 1, 4, 2),
        Sketcher.Constraint('Coincident', 5, 2, 0, 1),
        Sketcher.Constraint('Vertical', 5),
        Sketcher.Constraint('Vertical', 3),
        Sketcher.Constraint('DistanceX', 0, 1, 0, 2, 5037.082520),
        Sketcher.Constraint('DistanceY', 1, 2, 1, 1, 4958.778320),
        Sketcher.Constraint('DistanceY', 3, 1, 3, 2, 1544.678467),
        Sketcher.Constraint('DistanceX', 4, 2, 4, 1, 1309.763672)]
    upper_sketch.addConstraint(upper_conList)
    upper_sketch.setDatum(14, Units.Quantity('5000.000000 mm'))
    upper_sketch.setDatum(15, Units.Quantity('5000.000000 mm'))
    upper_sketch.setDatum(16, Units.Quantity('1500.000000 mm'))
    upper_sketch.setDatum(17, Units.Quantity('1500.000000 mm'))

    pocket = geom_obj.newObject('PartDesign::Pocket', 'Pocket')
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

    # solver
    if solvertype == "elmer":
        solver_object = analysis.addObject(ObjectsFem.makeSolverElmer(doc, "SolverElmer"))[0]
        ObjectsFem.makeEquationElectrostatic(doc, solver_object)
        ObjectsFem.makeEquationElectricforce(doc, solver_object)
    else:
        FreeCAD.Console.PrintWarning(
            "Not known or not supported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )

    # material
    material_object = analysis.addObject(
        ObjectsFem.makeMaterialFluid(doc, "FemMaterial")
    )[0]
    mat = material_object.Material
    mat["Name"] = "Air-Generic"
    mat["Density"] = "1.20 kg/m^3"
    mat["KinematicViscosity"] = "15.11 mm^2/s"
    mat["VolumetricThermalExpansionCoefficient"] = "0.00 mm/m/K"
    mat["ThermalConductivity"] = "0.03 W/m/K"
    mat["ThermalExpansionCoefficient"] = "0.0034/K"
    mat["SpecificHeat"] = "1.00 J/kg/K"
    mat["RelativePermittivity"] = "1.00"
    material_object.Material = mat

    # 0V_potential_constraint
    constraint_elect_pot0 = analysis.addObject(
        ObjectsFem.makeConstraintElectrostaticPotential(doc))[0]
    constraint_elect_pot0.References = [(geom_obj, "Face2")]
    constraint_elect_pot0.Potential = 0.00
    constraint_elect_pot0.CapacitanceBody = 1
    constraint_elect_pot0.CapacitanceBodyEnabled = True
    constraint_elect_pot0.PotentialEnabled = True

    # 1e6V_potential_constraint
    constraint_elect_pot1 = analysis.addObject(
        ObjectsFem.makeConstraintElectrostaticPotential(doc))[0]
    constraint_elect_pot1.References = [
        (geom_obj, "Face4"),
        (geom_obj, "Face5"),
        (geom_obj, "Face6"),
        (geom_obj, "Face11")]
    constraint_elect_pot1.Potential = 1000000.00
    constraint_elect_pot1.CapacitanceBody = 2
    constraint_elect_pot1.CapacitanceBodyEnabled = True
    constraint_elect_pot1.PotentialEnabled = True
    constraint_elect_pot1.ElectricForcecalculation = True

    # mesh
    from .meshes.mesh_electricforce_elmer_nongui6_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")
    femmesh_obj = analysis.addObject(
        ObjectsFem.makeMeshGmsh(doc, mesh_name)
    )[0]
    femmesh_obj.FemMesh = fem_mesh
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False

    # mesh_region
    mesh_region = ObjectsFem.makeMeshRegion(doc, femmesh_obj)
    mesh_region.CharacteristicLength = '300 mm'
    mesh_region.References = [
        (geom_obj, "Face4"),
        (geom_obj, "Face5"),
        (geom_obj, "Face6"),
        (geom_obj, "Face11")]

    doc.recompute()
    return doc
