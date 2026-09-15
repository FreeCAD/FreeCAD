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

from types import SimpleNamespace
from unittest.mock import Mock, patch

import Arch
import ArchReference
from bimtests import TestArchBase
from nativeifc import backend


class TestArchReference(TestArchBase.TestArchBase):

    def test_ifc_reference_uses_validated_backend(self):
        proxy = object.__new__(ArchReference.ArchReference)
        expected = object()
        ifcopenshell = SimpleNamespace(open=Mock(return_value=expected))

        with patch.object(backend, "get_backend", return_value=ifcopenshell) as get_backend:
            result = proxy.getIfcFile("reference.ifc")

        self.assertIs(result, expected)
        get_backend.assert_called_once_with(capability=backend.READ)
        ifcopenshell.open.assert_called_once_with("reference.ifc")

    def test_ifc_reference_handles_unavailable_backend(self):
        proxy = object.__new__(ArchReference.ArchReference)
        error = backend.IfcOpenShellUnavailable("missing required API")

        with patch.object(backend, "get_backend", side_effect=error):
            result = proxy.getIfcFile("reference.ifc")

        self.assertIsNone(result)

    def test_makeReference(self):
        """Test the makeReference function."""
        operation = "Testing makeReference..."
        self.printTestMessage(operation)

        obj = Arch.makeReference()
        self.assertIsNotNone(obj, "makeReference failed to create an object")
        self.assertEqual(obj.Label, "External Reference", "Incorrect default label for Reference")
