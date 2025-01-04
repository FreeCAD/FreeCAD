# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
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

"""Unit tests for the Draft Workbench, base classes."""

import unittest

import FreeCAD as App
from drafttests import auxiliary as aux
from draftutils.messages import _msg
from draftutils.todo import ToDo


class DraftTestCaseDoc(unittest.TestCase):
    """Base class for Draft tests that require a document."""

    def setUp(self):
        """Set up a new document for each test."""
        aux.draw_header()
        # name = self.__class__.__name__
        name = "___".join(self.id().split(".")[2:])  # unique name for each test
        if not name in App.listDocuments():
            App.newDocument(name)
        App.setActiveDocument(name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc.Name))

    def tearDown(self):
        """Close the document after each test."""
        App.closeDocument(self.doc.Name)


class DraftTestCaseNoDoc(unittest.TestCase):
    """Base class for Draft tests that do not require a document."""

    def setUp(self):
        """Draw the header."""
        aux.draw_header()
