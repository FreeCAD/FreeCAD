# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for classification preset access through the runtime backend."""

import ast
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

import FreeCADGui

if not hasattr(FreeCADGui, "addCommand"):
    FreeCADGui.addCommand = Mock()

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bimcommands"))

from BimClassification import BIM_Classification
from nativeifc import backend


class TestBimClassification(unittest.TestCase):
    def test_classification_loading_uses_read_backend(self):
        command = BIM_Classification()
        expected = object()
        ifcopenshell = Mock()
        ifcopenshell.open.return_value = expected

        with patch.object(backend, "get_backend", return_value=ifcopenshell) as get_backend:
            result = command.loadIfcClassification("classification.ifc")

        get_backend.assert_called_once_with(capability=backend.READ)
        ifcopenshell.open.assert_called_once_with("classification.ifc")
        self.assertIs(result, expected)

    def test_command_has_no_direct_ifcopenshell_imports(self):
        path = Path(__file__).resolve().parents[1] / "bimcommands" / "BimClassification.py"
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
