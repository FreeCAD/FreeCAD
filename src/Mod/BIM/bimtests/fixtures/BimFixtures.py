# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 Furgo

"""Reusable BIM test fixtures.

Exports:
 - create_test_model(document, **overrides)
 - DEFAULTS, LABELS

This module centralizes the complex sample model used across multiple tests.
"""

import FreeCAD
import Arch
import Draft

__all__ = ["create_test_model", "DEFAULTS", "LABELS"]

# Canonical defaults used by the fixture
DEFAULTS = {
    "building_length": 4000.0,
    "building_width": 3200.0,
    "ground_floor_height": 3200.0,
    "interior_wall_height": 3000.0,
    "slab_thickness": 200.0,
    "roof_overhang": 200.0,
}

# Canonical labels used by the fixture for predictable queries
LABELS = {
    "site": "Main Site",
    "building": "Main Building",
    "ground_floor": "Ground Floor",
    "upper_floor": "Upper Floor",
    "exterior_wall": "Exterior Wall",
    "interior_wall": "Interior Partition",
    "front_door": "Front Door",
    "living_window": "Living Room Window",
    "office_space": "Office Space",
    "living_space": "Living Space",
    "generic_box": "Generic Box",
}


def create_test_model(document, **overrides):
    """Create a complete, standard BIM model in the provided document.

    The function returns a dict of key objects for tests to reference. It
    accepts optional overrides for numeric defaults via keyword args that map
    to keys in DEFAULTS (e.g. building_length=5000.0).

    Returns None if document is falsy.
    """
    doc = document
    if not doc:
        FreeCAD.Console.PrintError(
            "Error: No active document found. Please create a new document first.\n"
        )
        return {}

    # Merge defaults with overrides
    cfg = DEFAULTS.copy()
    cfg.update(overrides)

    building_length = cfg["building_length"]
    building_width = cfg["building_width"]
    ground_floor_height = cfg["ground_floor_height"]
    interior_wall_height = cfg["interior_wall_height"]
    slab_thickness = cfg["slab_thickness"]
    roof_overhang = cfg["roof_overhang"]

    # --- 1. BIM Hierarchy (Site, Building, Levels) ---
    site = Arch.makeSite(name=LABELS["site"])
    building = Arch.makeBuilding(name=LABELS["building"])
    site.addObject(building)

    level_0 = Arch.makeFloor(name=LABELS["ground_floor"])
    level_0.Height = ground_floor_height
    level_1 = Arch.makeFloor(name=LABELS["upper_floor"])
    level_1.Height = ground_floor_height
    level_1.Placement.Base.z = ground_floor_height
    building.addObject(level_0)
    building.addObject(level_1)

    # --- 2. Ground Floor Walls ---
    p1 = FreeCAD.Vector(0, 0, 0)
    p2 = FreeCAD.Vector(building_length, 0, 0)
    p3 = FreeCAD.Vector(building_length, building_width, 0)
    p4 = FreeCAD.Vector(0, building_width, 0)
    exterior_wire = Draft.makeWire([p1, p2, p3, p4], closed=True)
    exterior_wall = Arch.makeWall(
        exterior_wire, name=LABELS["exterior_wall"], height=ground_floor_height
    )
    level_0.addObject(exterior_wall)

    p5 = FreeCAD.Vector(building_length / 2, 0, 0)
    p6 = FreeCAD.Vector(building_length / 2, building_width, 0)
    interior_wire = Draft.makeWire([p5, p6])
    interior_wall = Arch.makeWall(
        interior_wire, name=LABELS["interior_wall"], height=interior_wall_height
    )
    interior_wall.Width = 100.0
    level_0.addObject(interior_wall)

    doc.recompute()

    # --- 3. Openings (Doors and Windows) ---
    door = Arch.makeWindowPreset(
        "Simple door", width=900, height=2100, h1=50, h2=50, h3=50, w1=100, w2=40, o1=0, o2=0
    )
    door.Placement = FreeCAD.Placement(
        FreeCAD.Vector(800, 0, 0), FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
    )
    door.Label = LABELS["front_door"]
    door.Hosts = [exterior_wall]

    window = Arch.makeWindowPreset(
        "Open 1-pane", width=1500, height=1200, h1=50, h2=50, h3=50, w1=100, w2=50, o1=0, o2=50
    )
    window.Placement = FreeCAD.Placement(
        FreeCAD.Vector(building_length, building_width / 2, 900),
        FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 270),
    )
    window.Label = LABELS["living_window"]
    window.Hosts = [exterior_wall]

    doc.recompute()

    # --- 4. Spaces (from Volumetric Shapes) ---
    office_box = doc.addObject("Part::Box", "OfficeVolume")
    office_box.Length = building_length / 2
    office_box.Width = building_width
    office_box.Height = interior_wall_height
    room1_space = Arch.makeSpace(office_box, name=LABELS["office_space"])
    level_0.addObject(room1_space)

    living_box = doc.addObject("Part::Box", "LivingVolume")
    living_box.Length = building_length / 2
    living_box.Width = building_width
    living_box.Height = interior_wall_height
    living_box.Placement.Base = FreeCAD.Vector(building_length / 2, 0, 0)
    room2_space = Arch.makeSpace(living_box, name=LABELS["living_space"])
    level_0.addObject(room2_space)

    doc.recompute()

    # --- 5. Structural Elements ---
    column = Arch.makeStructure(
        length=200, width=200, height=interior_wall_height, name="Main Column"
    )
    column.IfcType = "Column"
    column.Placement.Base = FreeCAD.Vector(
        (building_length / 2) - 100, (building_width / 2) - 100, 0
    )
    level_0.addObject(column)

    beam = Arch.makeStructure(length=building_length, width=150, height=300, name="Main Beam")
    beam.IfcType = "Beam"
    beam.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0, building_width / 2, interior_wall_height), FreeCAD.Rotation()
    )
    level_0.addObject(beam)

    # --- 6. Upper Floor Slab and Roof ---
    slab_profile = Draft.makeRectangle(
        length=building_length,
        height=building_width,
        placement=FreeCAD.Placement(FreeCAD.Vector(0, 0, interior_wall_height), FreeCAD.Rotation()),
    )
    slab = Arch.makeStructure(slab_profile, height=slab_thickness, name="Floor Slab")
    slab.IfcType = "Slab"
    level_1.addObject(slab)

    roof_profile = Draft.makeRectangle(
        length=building_length + (2 * roof_overhang),
        height=building_width + (2 * roof_overhang),
        placement=FreeCAD.Placement(
            FreeCAD.Vector(-roof_overhang, -roof_overhang, ground_floor_height), FreeCAD.Rotation()
        ),
    )
    doc.recompute()

    safe_run = (max(roof_profile.Length.Value, roof_profile.Height.Value) / 2) + 100

    roof = Arch.makeRoof(roof_profile, angles=[30.0] * 4, run=[safe_run] * 4, name="Main Roof")
    level_1.addObject(roof)

    # --- 7. Non-BIM Object ---
    generic_box = doc.addObject("Part::Box", LABELS["generic_box"])
    generic_box.Placement.Base = FreeCAD.Vector(building_length + 1000, building_width + 1000, 0)

    # --- 8. Custom Dynamic Property ---
    exterior_wall.addProperty("App::PropertyString", "FireRating", "BIM")
    exterior_wall.FireRating = "60 minutes"

    # --- Final Step: Recompute and return references ---
    doc.recompute()

    model_objects = {
        "site": site,
        "building": building,
        "ground_floor": level_0,
        "upper_floor": level_1,
        "exterior_wall": exterior_wall,
        "interior_wall": interior_wall,
        "front_door": door,
        "living_window": window,
        "office_space": room1_space,
        "living_space": room2_space,
        "column": column,
        "slab": slab,
    }
    return model_objects
