# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for structural IFC GUID generation through the runtime backend."""

import ast
from pathlib import Path
import unittest
from unittest.mock import Mock, patch

from importers import exportIFCStructuralTools
from nativeifc import backend


class TestIfcStructuralExport(unittest.TestCase):
    def tearDown(self):
        backend.invalidate()

    def test_structural_entities_receive_unique_guids_in_supported_schemas(self):
        status = backend.get_status(capability=backend.GUID)
        if not status.available:
            self.skipTest(f"IfcOpenShell GUID generation is unavailable: {status.error}")
        ifcopenshell = backend.get_backend(capability=backend.GUID)

        identifiers = set()
        for schema in ("IFC2X3", "IFC4"):
            model = ifcopenshell.file(schema=schema)
            identifier = backend.new_guid()
            entity = model.create_entity(
                "IfcStructuralPointConnection",
                GlobalId=identifier,
            )
            self.assertEqual(entity.GlobalId, identifier)
            self.assertEqual(len(identifier), 22)
            identifiers.add(identifier)

        self.assertEqual(len(identifiers), 2)

    def test_structural_relationship_uses_backend_guid(self):
        ifcfile = Mock()
        owner_history = object()
        ifcfile.by_type.return_value = [owner_history]
        aobj = object()
        sobj = object()

        with patch.object(backend, "new_guid", return_value="structural-guid"):
            exportIFCStructuralTools.associates(ifcfile, aobj, sobj)

        ifcfile.createIfcRelAssignsToProduct.assert_called_once_with(
            "structural-guid", owner_history, None, None, [sobj], None, aobj
        )

    def test_structural_tools_have_no_direct_ifcopenshell_imports(self):
        path = Path(exportIFCStructuralTools.__file__).resolve()
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
