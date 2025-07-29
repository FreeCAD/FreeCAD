# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2025 Weston Schmidt <weston_schmidt@alumni.purdue.edu>   *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# ***************************************************************************/

"""
Unit tests for CommandInsertLink module.

This module contains unit tests to verify the proper handling of null
LinkedObject references in the TaskAssemblyInsertLink.accept() method.
Tests ensure that invalid objects are gracefully skipped without causing
AttributeError crashes.
"""

import unittest
import sys
import os
import FreeCAD as App

# Handle both direct execution and module execution
try:
    # Try relative import first (when run as module)
    from . import mocks  # pylint: disable=unused-import
except ImportError:
    # Fall back to absolute import (when run directly)
    sys.path.insert(0, os.path.dirname(__file__))
    import mocks  # pylint: disable=unused-import

import CommandInsertLink


def _msg(text, end="\n"):
    """Write messages to the console including the line ending."""
    App.Console.PrintMessage(text + end)


class TestCommandInsertLink(unittest.TestCase):
    """Unit tests for CommandInsertLink module."""

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """

    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        """
        doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != doc_name:
                App.newDocument(doc_name)
        else:
            App.newDocument(doc_name)
        App.setActiveDocument(doc_name)
        self.doc = App.ActiveDocument

        self.assembly = App.ActiveDocument.addObject("Assembly::AssemblyObject", "Assembly")

        _msg(f"  Temporary document '{self.doc.Name}'")

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        App.closeDocument(self.doc.Name)

    def test_mixed_valid_and_invalid_objects(self):
        """Test that accept() handles a mix of valid and invalid objects correctly."""
        operation = "Handle mixed valid/invalid objects"
        _msg(f"  Test '{operation}'")

        mock_view = type("MockView", (), {"getSize": lambda: (800, 600)})()
        task = CommandInsertLink.TaskAssemblyInsertLink(self.assembly, mock_view)

        # Create a mix of valid and invalid objects
        test_objects = [
            # Valid object (would work in real scenario)
            {
                "addedObject": type(
                    "ValidObject",
                    (),
                    {
                        "Name": "ValidObject",
                        "Label": "Valid Object",
                        "LinkedObject": type(
                            "ValidLinkedObject", (), {"Name": "ValidLinkedObjectName"}
                        )(),
                    },
                )(),
                "translation": App.Vector(1, 1, 1),
            },
            # Invalid: LinkedObject is None
            {
                "addedObject": type(
                    "InvalidObject1",
                    (),
                    {"Name": "InvalidObject1", "Label": "Invalid Object 1", "LinkedObject": None},
                )(),
                "translation": App.Vector(2, 2, 2),
            },
            # Invalid: No LinkedObject attribute
            {
                "addedObject": type(
                    "InvalidObject2", (), {"Name": "InvalidObject2", "Label": "Invalid Object 2"}
                )(),
                "translation": App.Vector(3, 3, 3),
            },
            # Invalid: LinkedObject.Name is None
            {
                "addedObject": type(
                    "InvalidObject3",
                    (),
                    {
                        "Name": "InvalidObject3",
                        "Label": "Invalid Object 3",
                        "LinkedObject": type("LinkedObjectWithNoneName", (), {"Name": None})(),
                    },
                )(),
                "translation": App.Vector(4, 4, 4),
            },
            # Invalid: No Name attribute
            {
                "addedObject": type(
                    "InvalidObject4",
                    (),
                    {
                        "Label": "Invalid Object 4",
                        "LinkedObject": type("LinkedObject", (), {"Name": "Something"})(),
                    },
                )(),
                "translation": App.Vector(5, 5, 5),
            },
        ]

        # Add all objects to insertion stack
        for obj_data in test_objects:
            task.insertionStack.append(obj_data)

        # Should handle the mix gracefully - invalid objects skipped, valid ones processed
        try:
            result = task.accept()
            self.assertTrue(
                result, "accept() should return True even with mixed valid/invalid objects"
            )
            _msg("  Successfully handled mixed valid/invalid objects")
        except Exception as e:  # pylint: disable=broad-exception-caught
            self.fail(f"Failed to handle mixed objects: {e}")

    def test_empty_insertion_stack(self):
        """Test that accept() handles empty insertion stack correctly."""
        operation = "Handle empty insertion stack"
        _msg(f"  Test '{operation}'")

        mock_view = type("MockView", (), {"getSize": lambda: (800, 600)})()
        task = CommandInsertLink.TaskAssemblyInsertLink(self.assembly, mock_view)

        # Don't add anything to insertion stack - it should remain empty
        self.assertEqual(len(task.insertionStack), 0, "Insertion stack should be empty")

        try:
            result = task.accept()
            self.assertTrue(result, "accept() should return True even with empty insertion stack")
            _msg("  Successfully handled empty insertion stack")
        except Exception as e:  # pylint: disable=broad-exception-caught
            self.fail(f"Failed to handle empty insertion stack: {e}")
