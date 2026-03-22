# SPDX-License-Identifier: LGPL-2.1-or-later
# --------------------------------------------------------------------------
#                                                                          *
#    Copyright (c) 2026 Chris Jones github.com/ipatch                      *
#                                                                          *
#    This file is part of FreeCAD.                                         *
#                                                                          *
#    FreeCAD is free software: you can redistribute it and/or modify it    *
#    under the terms of the GNU Lesser General Public License as           *
#    published by the Free Software Foundation, either version 2.1 of the  *
#    License, or (at your option) any later version.                       *
#                                                                          *
#    FreeCAD is distributed in the hope that it will be useful, but        *
#    WITHOUT ANY WARRANTY; without even the implied warranty of            *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#    Lesser General Public License for more details.                       *
#                                                                          *
#    You should have received a copy of the GNU Lesser General Public      *
#    License along with FreeCAD. If not, see                               *
#    <https://www.gnu.org/licenses/>.                                      *
#                                                                          *
# --------------------------------------------------------------------------

"""Unit tests for Tree widget 'Select all instances' functionality.

See GitHub issue #26478.
- https://github.com/FreeCAD/FreeCAD/issues/26478

To run tests:
    FreeCAD -t TestTreeSelection.TestSelectAllInstances
"""

import unittest
import FreeCAD
import FreeCADGui
from FreeCADGui import Selection

import Draft

from PySide6 import QtWidgets


class TestSelectAllInstances(unittest.TestCase):
    """Test cases for 'Select all instances' tree functionality.

    These tests verify that the Std_TreeSelectAllInstances command
    correctly selects all instances of an object, including those
    that appear under link-like objects (e.g., Part::Mirroring).

    See GitHub issue #26478.
    """

    def setUp(self):
        """Set up test document with objects."""
        self.doc = FreeCAD.newDocument("TestSelectAllInstances")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)

    def tearDown(self):
        """Clean up test document."""
        Selection.clearSelection()
        FreeCAD.closeDocument(self.doc.Name)

    def _get_tree_widget(self):
        """Get the main tree widget."""
        mw = FreeCADGui.getMainWindow()
        for widget in mw.findChildren(QtWidgets.QTreeWidget):
            if widget.topLevelItemCount() > 0:
                return widget
        return None

    def _count_tree_selections_by_name(self, name):
        """Count how many tree items with given name are selected."""
        tree = self._get_tree_widget()
        if not tree:
            return 0
        selected = tree.selectedItems()
        return len([item for item in selected if item.text(0) == name])

    def test_select_all_instances_with_mirror(self):
        """Test that 'Select all instances' selects objects under Mirror objects.

        test for issue #26478: When an object exists in a Group
        and is also referenced by Mirror objects, 'Select all instances'
        should select all instances including those under the Mirror objects.
        """
        # Create a Draft Rectangle (matches original Part::Part2DObjectPython)
        rect = Draft.make_rectangle(10, 10)
        rect.Label = "Rectangle"

        # Create a group containing the rectangle and mirrors
        group = self.doc.addObject("App::DocumentObjectGroup", "Group")
        group.addObject(rect)

        # Create mirror objects that reference the rectangle
        # Use same naming as original document
        mirror1 = self.doc.addObject("Part::Mirroring", "Rectangle (mirrored)")
        mirror1.Source = rect
        mirror1.Normal = FreeCAD.Vector(1, 0, 0)
        mirror1.Base = FreeCAD.Vector(20, 0, 0)
        group.addObject(mirror1)

        mirror2 = self.doc.addObject("Part::Mirroring", "Rectangle (mirrored)001")
        mirror2.Source = rect
        mirror2.Normal = FreeCAD.Vector(0, 1, 0)
        mirror2.Base = FreeCAD.Vector(0, 20, 0)
        group.addObject(mirror2)

        self.doc.recompute()
        FreeCADGui.updateGui()

        self._debug_print_document_structure()

        # Get tree widget
        tree = self._get_tree_widget()
        self.assertIsNotNone(tree, "Could not find tree widget")

        # Clear any existing selection
        tree.clearSelection()
        Selection.clearSelection()

        # Select the rectangle via Gui.Selection (this will select it in the tree)
        Selection.addSelection(self.doc.Name, rect.Name)
        FreeCADGui.updateGui()

        # Run the "Select all instances" command
        FreeCADGui.runCommand("Std_TreeSelectAllInstances", 0)
        FreeCADGui.updateGui()

        # Count selected Rectangle items in tree
        count = self._count_tree_selections_by_name("Rectangle")

        # We expect 3 instances:
        # 1. Rectangle directly in Group
        # 2. Rectangle under Rectangle (mirrored)
        # 3. Rectangle under Rectangle (mirrored)001
        self.assertEqual(
            count,
            3,
            f"Expected 3 'Rectangle' instances selected in tree, got {count}. "
            f"See GitHub issue #26478.",
        )

    def test_select_all_instances_with_cuts(self):
        """Test that 'Select all instances' selects objects used in Part::Cut operations.

        When the same object is used as the Tool in multiple Cut operations,
        'Select all instances' should select all instances.

        loosely taken from the below freecad wiki page,
        https://wiki.freecad.org/Std_TreeSelectAllInstances
        """
        # Create the shared cube (tool for all cuts)
        cube = self.doc.addObject("Part::Box", "Cube")
        cube.Length = 10
        cube.Width = 10
        cube.Height = 10

        # Create base shapes
        cylinder = self.doc.addObject("Part::Cylinder", "Cylinder")
        cylinder.Radius = 8
        cylinder.Height = 15

        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        sphere.Radius = 10

        cone = self.doc.addObject("Part::Cone", "Cone")
        cone.Radius1 = 10
        cone.Radius2 = 0
        cone.Height = 15

        # Create cuts - all using the same Cube as the tool
        cut1 = self.doc.addObject("Part::Cut", "Cut")
        cut1.Base = cylinder
        cut1.Tool = cube

        cut2 = self.doc.addObject("Part::Cut", "Cut001")
        cut2.Base = sphere
        cut2.Tool = cube

        cut3 = self.doc.addObject("Part::Cut", "Cut002")
        cut3.Base = cone
        cut3.Tool = cube

        self.doc.recompute()
        FreeCADGui.updateGui()

        self._debug_print_document_structure()

        # Get tree widget
        tree = self._get_tree_widget()
        self.assertIsNotNone(tree, "Could not find tree widget")

        # Clear any existing selection
        tree.clearSelection()
        Selection.clearSelection()

        # Select the cube
        Selection.addSelection(self.doc.Name, cube.Name)
        FreeCADGui.updateGui()

        # Run the "Select all instances" command
        FreeCADGui.runCommand("Std_TreeSelectAllInstances", 0)
        FreeCADGui.updateGui()

        # Count selected Cube items in tree
        count = self._count_tree_selections_by_name("Cube")

        self._debug_print_tree_selection(tree, count)

        # We expect 3 instances:
        # 1. Cube under Cut
        # 2. Cube under Cut001
        # 3. Cube under Cut002
        self.assertEqual(
            count,
            3,
            f"Expected 3 'Cube' instances selected in tree, got {count}.",
        )

    def test_select_all_instances_basic(self):
        """Test basic 'Select all instances' with single object in group."""
        # Create a simple box
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10

        # Create a group containing the box
        group = self.doc.addObject("App::DocumentObjectGroup", "Group")
        group.addObject(box)

        self.doc.recompute()
        FreeCADGui.updateGui()

        # Get tree widget
        tree = self._get_tree_widget()
        self.assertIsNotNone(tree, "Could not find tree widget")

        # Clear and select
        tree.clearSelection()
        Selection.clearSelection()
        Selection.addSelection(self.doc.Name, box.Name)
        FreeCADGui.updateGui()

        # Run command
        FreeCADGui.runCommand("Std_TreeSelectAllInstances", 0)
        FreeCADGui.updateGui()

        # Should have at least 1 selection
        count = self._count_tree_selections_by_name("Box")
        self.assertGreaterEqual(
            count, 1, f"Expected at least 1 'Box' instance selected, got {count}."
        )
