# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Generate the BIM wall joins example document.

Run this script with the GUI FreeCAD binary from the repository root:

    ./build/bin/FreeCAD data/examples/BIMWallJoins.py

The GUI binary is used intentionally. The example contains visible ShapeString
labels and relation view providers, so generating it with FreeCADCmd does not
exercise the same save path as an interactively-created example document.
"""

import os

import Arch
import Draft
import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUTPUT_PATH = os.path.join(ROOT, "data", "examples", "BIMWallJoins.FCStd")
FONT_PATH = os.path.join(ROOT, "data", "examples", "osifont-lgpl3fe.ttf")


def make_wall(doc, name, start, end, width=200.0, height=1500.0, align="Center"):
    """Create a baseless wall between two global points."""
    line = end.sub(start)
    wall = Arch.makeWall(length=line.Length, width=width, height=height)
    wall.Label = name
    wall.Align = align
    wall.Placement = App.Placement(
        (start + end) * 0.5,
        App.Rotation(App.Vector(1, 0, 0), line.normalize()),
    )
    doc.recompute()
    return wall


def add_group(doc, label, *objects):
    """Create a document group and add the given objects to it."""
    group = doc.addObject("App::DocumentObjectGroup", label)
    group.Label = label
    for obj in objects:
        group.addObject(obj)
    return group


def add_label(text, point, size=180):
    """Add visible 3D label geometry above an example cluster."""
    label = Draft.make_shapestring(text, FONT_PATH, Size=size, Tracking=0)
    label.Label = text
    label.Placement.Base = point
    label.ViewObject.ShapeColor = (0.05, 0.05, 0.05, 0.0)
    label.ViewObject.LineColor = (0.05, 0.05, 0.05, 0.0)
    label.ViewObject.LineWidth = 2
    return label


def add_label_pair(title, subtitle, point):
    """Add a short title label and an optional smaller explanatory subtitle."""
    labels = [add_label(title, point, size=180)]
    if subtitle:
        labels.append(add_label(subtitle, point + App.Vector(0, -260, 0), size=110))
    return labels


def add_reference_line(name, start, end, color):
    """Add a visible guide line for examples where the baseline matters."""
    line = Draft.make_line(start, end)
    line.Label = name
    line.ViewObject.LineColor = color
    line.ViewObject.LineWidth = 4
    return line


def add_guide_label(text, point):
    """Add a small explanatory label near visual guide geometry."""
    return add_label(text, point, size=90)


def build_document():
    """Build and save the wall join example document."""
    if os.path.exists(OUTPUT_PATH):
        os.remove(OUTPUT_PATH)

    doc = App.newDocument("BIMWallJoins")
    doc.Label = "BIM Wall Joins"
    App.setActiveDocument(doc.Name)
    Gui.ActiveDocument = Gui.getDocument(doc.Name)

    miter_a = make_wall(
        doc,
        "Miter wall A",
        App.Vector(0, 0, 0),
        App.Vector(2000, 0, 0),
    )
    miter_b = make_wall(
        doc,
        "Miter wall B",
        App.Vector(2000, 0, 0),
        App.Vector(2000, 1800, 0),
    )
    miter = Arch.makeWallJoint(miter_a, miter_b, "Miter", name="WallJoint_Miter")
    miter.Label = "WallJoint - Miter"
    miter_label = add_label("01 Miter", App.Vector(0, -700, 1800))
    add_group(doc, "01 Miter WallJoint", miter_label, miter_a, miter_b, miter)

    butt_a = make_wall(
        doc,
        "Butt support wall",
        App.Vector(0, 3000, 0),
        App.Vector(3000, 3000, 0),
    )
    butt_b = make_wall(
        doc,
        "Butt oblique trimmed wall",
        App.Vector(1500, 3000, 0),
        App.Vector(2200, 4400, 0),
    )
    butt = Arch.makeWallJoint(butt_a, butt_b, "Butt", name="WallJoint_Butt")
    butt.Label = "WallJoint - Butt"
    butt.ButtTrimmed = "WallB"
    butt.EndA = "None"
    butt.EndB = "Auto"
    butt_label = add_label("02 Butt", App.Vector(0, 2300, 1800))
    add_group(doc, "02 Butt WallJoint", butt_label, butt_a, butt_b, butt)

    tee_top = make_wall(
        doc,
        "Tee top wall",
        App.Vector(0, 6000, 0),
        App.Vector(2600, 6000, 0),
    )
    tee_stem = make_wall(
        doc,
        "Tee stem wall",
        App.Vector(1300, 6000, 0),
        App.Vector(1300, 7600, 0),
    )
    tee = Arch.makeWallJoint(tee_top, tee_stem, "Tee", name="WallJoint_Tee")
    tee.Label = "WallJoint - Tee"
    tee_label = add_label("03 Tee", App.Vector(0, 5300, 1800))
    add_group(doc, "03 Tee WallJoint", tee_label, tee_top, tee_stem, tee)

    junction_carrier = make_wall(
        doc,
        "Junction carrier wall",
        App.Vector(4200, 0, 0),
        App.Vector(6800, 0, 0),
    )
    junction_branch_up = make_wall(
        doc,
        "Junction branch up",
        App.Vector(5500, 0, 0),
        App.Vector(5500, 1500, 0),
    )
    junction_branch_down = make_wall(
        doc,
        "Junction branch down",
        App.Vector(5500, 0, 0),
        App.Vector(5500, -1500, 0),
    )
    junction = Arch.makeWallJunction(
        [junction_carrier, junction_branch_up, junction_branch_down],
        carrier_wall=junction_carrier,
        name="WallJunction_Three_Walls",
    )
    junction.Label = "WallJunction - Three Walls"
    junction_label = add_label("04 Junction", App.Vector(4200, -1900, 1800))
    add_group(
        doc,
        "04 WallJunction",
        junction_label,
        junction_carrier,
        junction_branch_up,
        junction_branch_down,
        junction,
    )

    conflict_base = make_wall(
        doc,
        "Conflict shared wall",
        App.Vector(4200, 3000, 0),
        App.Vector(6800, 3000, 0),
    )
    conflict_branch_a = make_wall(
        doc,
        "Conflict branch priority 0",
        App.Vector(5200, 3000, 0),
        App.Vector(5200, 4400, 0),
    )
    conflict_branch_b = make_wall(
        doc,
        "Conflict branch priority 1",
        App.Vector(5300, 3000, 0),
        App.Vector(5300, 4400, 0),
    )
    conflict_winner = Arch.makeWallJoint(
        conflict_base,
        conflict_branch_a,
        "Miter",
        name="WallJoint_Priority_0",
    )
    conflict_loser = Arch.makeWallJoint(
        conflict_base,
        conflict_branch_b,
        "Miter",
        name="WallJoint_Conflict",
    )
    conflict_winner.Label = "WallJoint - Priority 0"
    conflict_loser.Label = "WallJoint - Conflict"
    conflict_labels = add_label_pair(
        "05 Conflict",
        "winner owns contested end",
        App.Vector(4200, 2300, 1800),
    )
    conflict_winner_label = add_guide_label(
        "winner",
        App.Vector(4860, 4520, 1800),
    )
    conflict_loser_label = add_guide_label(
        "blocked",
        App.Vector(5380, 4520, 1800),
    )
    add_group(
        doc,
        "05 Priority Conflict - disabling winner transfers ownership",
        *conflict_labels,
        conflict_winner_label,
        conflict_loser_label,
        conflict_base,
        conflict_branch_a,
        conflict_branch_b,
        conflict_winner,
        conflict_loser,
    )

    extension_a = make_wall(
        doc,
        "RequiresExtension finite wall",
        App.Vector(4200, 6000, 0),
        App.Vector(5200, 6000, 0),
    )
    extension_b = make_wall(
        doc,
        "RequiresExtension crossing wall",
        App.Vector(6200, 5600, 0),
        App.Vector(6200, 7000, 0),
    )
    extension = Arch.makeWallJoint(
        extension_a,
        extension_b,
        "Miter",
        name="WallJoint_RequiresExtension",
    )
    extension.Label = "WallJoint - RequiresExtension"
    extension_labels = add_label_pair(
        "06 RequiresExtension",
        "finite baselines miss",
        App.Vector(4200, 5300, 1800),
    )
    extension_a_reference = add_reference_line(
        "RequiresExtension finite baseline",
        App.Vector(4200, 6000, 1800),
        App.Vector(5200, 6000, 1800),
        (0.9, 0.15, 0.15, 0.0),
    )
    extension_a_virtual = add_reference_line(
        "RequiresExtension virtual extension",
        App.Vector(5200, 6000, 1800),
        App.Vector(6200, 6000, 1800),
        (0.9, 0.55, 0.15, 0.0),
    )
    extension_b_reference = add_reference_line(
        "RequiresExtension crossing baseline",
        App.Vector(6200, 5600, 1800),
        App.Vector(6200, 7000, 1800),
        (0.15, 0.25, 0.9, 0.0),
    )
    extension_virtual_label = add_guide_label(
        "virtual intersection",
        App.Vector(6260, 6060, 1800),
    )
    add_group(
        doc,
        "06 RequiresExtension - baselines intersect only outside finite wall spans",
        *extension_labels,
        extension_a_reference,
        extension_a_virtual,
        extension_b_reference,
        extension_virtual_label,
        extension_a,
        extension_b,
        extension,
    )

    disabled_a = make_wall(
        doc,
        "Disabled relation wall A",
        App.Vector(8400, 0, 0),
        App.Vector(10400, 0, 0),
    )
    disabled_b = make_wall(
        doc,
        "Disabled relation wall B",
        App.Vector(10400, 0, 0),
        App.Vector(10400, 1600, 0),
    )
    disabled = Arch.makeWallJoint(
        disabled_a,
        disabled_b,
        "Miter",
        name="WallJoint_Disabled",
    )
    disabled.Label = "WallJoint - Disabled"
    disabled.Enabled = False
    disabled_label = add_label("07 Disabled", App.Vector(8400, -700, 1800))
    add_group(doc, "07 Disabled Relation", disabled_label, disabled_a, disabled_b, disabled)

    manual_a = make_wall(
        doc,
        "Manual precedence relation wall",
        App.Vector(8400, 3000, 0),
        App.Vector(10400, 3000, 0),
    )
    manual_b = make_wall(
        doc,
        "Manual precedence branch wall",
        App.Vector(10400, 3000, 0),
        App.Vector(10400, 4600, 0),
    )
    manual_a.EndingEnd = App.Placement(
        App.Vector(9500, 3000, 0),
        App.Rotation(App.Vector(1, 0, 0), App.Vector(0, 0, 1)),
    )
    manual_a.EndConditionOrderEnd = ["Relation", "Manual"]
    manual_relation = Arch.makeWallJoint(
        manual_a,
        manual_b,
        "Miter",
        name="WallJoint_Relation_Overrides_Manual",
    )
    manual_relation.Label = "WallJoint - Relation Overrides Manual"
    manual_label = add_label(
        "08 Relation > Manual",
        App.Vector(8400, 2300, 1800),
    )
    manual_reference = add_reference_line(
        "Relation overrides manual trim reference",
        App.Vector(9500, 2700, 1800),
        App.Vector(9500, 3300, 1800),
        (0.9, 0.55, 0.15, 0.0),
    )
    manual_reference_label = add_guide_label(
        "manual trim ignored",
        App.Vector(9020, 3380, 1800),
    )
    add_group(
        doc,
        "08 Relation Overrides Manual End",
        manual_label,
        manual_reference,
        manual_reference_label,
        manual_a,
        manual_b,
        manual_relation,
    )

    unequal_a = make_wall(
        doc,
        "Unequal width miter wide wall",
        App.Vector(8400, 6000, 0),
        App.Vector(10400, 6000, 0),
        width=400.0,
    )
    unequal_b = make_wall(
        doc,
        "Unequal width miter narrow wall",
        App.Vector(10400, 6000, 0),
        App.Vector(10400, 7800, 0),
        width=200.0,
    )
    unequal = Arch.makeWallJoint(
        unequal_a,
        unequal_b,
        "Miter",
        name="WallJoint_Unequal_Width_Miter",
    )
    unequal.Label = "WallJoint - Unequal Width Miter"
    unequal_label = add_label("09 Unequal Width", App.Vector(8400, 5300, 1800))
    add_group(
        doc,
        "09 Unequal Width Miter",
        unequal_label,
        unequal_a,
        unequal_b,
        unequal,
    )

    aligned_a = make_wall(
        doc,
        "Left-aligned butt support wall",
        App.Vector(3000, 9000, 0),
        App.Vector(0, 9000, 0),
        align="Left",
    )
    aligned_b = make_wall(
        doc,
        "Left-aligned butt trimmed wall",
        App.Vector(1500, 9000, 0),
        App.Vector(2200, 10400, 0),
        align="Left",
    )
    aligned = Arch.makeWallJoint(
        aligned_a,
        aligned_b,
        "Butt",
        name="WallJoint_Left_Aligned_Butt",
    )
    aligned.Label = "WallJoint - Left-Aligned Butt"
    aligned.ButtTrimmed = "WallB"
    aligned.EndA = "None"
    aligned.EndB = "Auto"
    aligned_label = add_label("10 Left Aligned Butt", App.Vector(0, 8300, 1800))
    aligned_a_reference = add_reference_line(
        "Left-aligned butt support baseline",
        App.Vector(3000, 9000, 1800),
        App.Vector(0, 9000, 1800),
        (0.9, 0.15, 0.15, 0.0),
    )
    aligned_b_reference = add_reference_line(
        "Left-aligned butt trimmed baseline",
        App.Vector(1500, 9000, 1800),
        App.Vector(2200, 10400, 1800),
        (0.15, 0.25, 0.9, 0.0),
    )
    add_group(
        doc,
        "10 Left-Aligned Butt - visible baselines",
        aligned_label,
        aligned_a_reference,
        aligned_b_reference,
        aligned_a,
        aligned_b,
        aligned,
    )

    doc.recompute()
    doc.saveAs(OUTPUT_PATH)
    App.closeDocument(doc.Name)


build_document()
QtCore.QTimer.singleShot(0, Gui.getMainWindow().close)
