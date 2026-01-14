# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Base class for Import module unit tests (non-GUI)."""

import unittest
import FreeCAD


class TestImportBase(unittest.TestCase):
    """Base class for non-GUI Import tests.

    Creates a uniquely-named document per test class to provide isolation.
    """

    def setUp(self):
        """
        Set up a new, clean document for the test. Ensure test isolation by creating a
        uniquely-named document and cleaning up any potential leftovers from a previously failed
        run.
        """
        self.doc_name = self.__class__.__name__

        # Close any document of the same name that might have been left over from a crashed or
        # aborted test run. FreeCAD.getDocument() raises a NameError if the document is not found,
        # so we wrap this check in a try...except block.
        try:
            FreeCAD.getDocument(self.doc_name)
            # If getDocument() succeeds, the document exists and must be closed.
            FreeCAD.closeDocument(self.doc_name)
        except NameError:
            # This is the expected path on a clean run; do nothing.
            pass

        # Create a fresh document for the current test.
        self.document = FreeCAD.newDocument(self.doc_name)
        self.assertEqual(self.document.Name, self.doc_name)

    def tearDown(self):
        """Close the test document after all tests in the class are complete."""
        if hasattr(self, "document") and self.document:
            try:
                FreeCAD.closeDocument(self.document.Name)
            except Exception as e:
                FreeCAD.Console.PrintError(
                    f"Error during tearDown in {self.__class__.__name__}: {e}\n"
                )
