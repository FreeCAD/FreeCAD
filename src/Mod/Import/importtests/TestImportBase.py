# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Furgo
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

"""Base class for Import module unit tests (non-GUI)."""

import tempfile
import unittest
import FreeCAD


class TestImportBase(unittest.TestCase):
    """Base class for non-GUI Import tests.

    Each test gets a document named after the test class, and a temporary directory
    (`self.temp_dir`) for files it writes. Both are removed after the test.
    """

    def setUp(self):
        """Create the test document and the temporary directory."""
        self.doc_name = self.__class__.__name__

        # Close a document of the same name left open by an aborted run.
        try:
            FreeCAD.getDocument(self.doc_name)
            FreeCAD.closeDocument(self.doc_name)
        except NameError:
            # No leftover document, nothing to close.
            pass

        # addCleanup removes the directory even if the test or the rest of setUp fails.
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        self.temp_dir = temp_dir.name

        self.document = FreeCAD.newDocument(self.doc_name)
        self.assertEqual(self.document.Name, self.doc_name)

    def tearDown(self):
        """Close the test document after each test."""
        if hasattr(self, "document") and self.document:
            try:
                FreeCAD.closeDocument(self.document.Name)
            except Exception as e:
                FreeCAD.Console.PrintError(
                    f"Error during tearDown in {self.__class__.__name__}: {e}\n"
                )
