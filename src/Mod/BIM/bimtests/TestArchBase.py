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

"""Defines the base class for Arch module unit tests."""

import unittest
import FreeCAD

class TestArchBase(unittest.TestCase):

    def setUp(self):
        print(f"Initializing: {self.__class__.__name__}")
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def printTestMessage(self, text, prepend_text="Test ", end="\n"):
        """Write messages to the console including the line ending.

        Messages will be prepended with "Test ", unless an empty string is
        passed as the prepend_text argument
        """
        FreeCAD.Console.PrintMessage(prepend_text + text + end)
