# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for IFC Explorer access through the runtime backend."""

import ast
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

import FreeCADGui

if not hasattr(FreeCADGui, "addCommand"):
    FreeCADGui.addCommand = Mock()

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bimcommands"))

from BimIfcExplorer import BIM_IfcExplorer
from nativeifc import backend


class TestBimIfcExplorer(unittest.TestCase):
    def test_file_loading_uses_explorer_backend(self):
        explorer = BIM_IfcExplorer()
        expected = object()
        ifcopenshell = Mock()
        ifcopenshell.open.return_value = expected

        with patch.object(backend, "get_backend", return_value=ifcopenshell) as get_backend:
            result = explorer.loadIfcFile("model.ifc")

        get_backend.assert_called_once_with(capability=backend.EXPLORER)
        ifcopenshell.open.assert_called_once_with("model.ifc")
        self.assertIs(result, expected)

    def test_command_has_no_direct_ifcopenshell_imports(self):
        path = Path(__file__).resolve().parents[1] / "bimcommands" / "BimIfcExplorer.py"
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        violations = []
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                names = [alias.name for alias in node.names]
            elif isinstance(node, ast.ImportFrom) and node.module:
                names = [node.module]
            else:
                continue
            if any(name == "ifcopenshell" or name.startswith("ifcopenshell.") for name in names):
                violations.append(node.lineno)
        self.assertEqual(violations, [])


if __name__ == "__main__":
    unittest.main()
