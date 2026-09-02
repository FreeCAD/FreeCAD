# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for NativeIFC's isolated geometry-export adapter."""

import importlib
import sys
from types import ModuleType
import unittest
from unittest.mock import Mock, patch

from nativeifc import backend


class TestIfcGeometryExport(unittest.TestCase):
    def _load_adapter(self, fake_exporter):
        module_name = "nativeifc.ifc_geometry_export"
        sys.modules.pop(module_name, None)
        with patch.dict(sys.modules, {"importers.exportIFC": fake_exporter}):
            return importlib.import_module(module_name)

    def tearDown(self):
        sys.modules.pop("nativeifc.ifc_geometry_export", None)

    def test_representation_uses_operation_state(self):
        legacy = ModuleType("importers.exportIFC")
        state = object()
        legacy.create_export_state = Mock(return_value=state)
        legacy.getRepresentation = Mock(return_value=("representation", "placement", "Brep"))
        legacy.create_annotation = Mock()
        ifc_module = object()

        with patch.object(backend, "get_backend", return_value=ifc_module):
            adapter_module = self._load_adapter(legacy)
            adapter = adapter_module.GeometryExporter("ifc-file")
            result = adapter.create_representation("context", "object", {"option": True})

        legacy.create_export_state.assert_called_once_with("ifc-file", ifc_module)
        legacy.getRepresentation.assert_called_once_with(
            "ifc-file",
            "context",
            "object",
            preferences={"option": True},
            export_state=state,
        )
        self.assertEqual(result, ("representation", "placement"))

    def test_annotation_uses_same_operation_state(self):
        legacy = ModuleType("importers.exportIFC")
        state = object()
        legacy.create_export_state = Mock(return_value=state)
        legacy.getRepresentation = Mock()
        legacy.create_annotation = Mock(return_value="annotation")

        with patch.object(backend, "get_backend", return_value=object()):
            adapter_module = self._load_adapter(legacy)
            adapter = adapter_module.GeometryExporter("ifc-file")
            result = adapter.create_annotation("context", "object", "history", {"option": True})

        legacy.create_annotation.assert_called_once_with(
            "object",
            "ifc-file",
            "context",
            "history",
            {"option": True},
            export_state=state,
        )
        self.assertEqual(result, "annotation")


if __name__ == "__main__":
    unittest.main()
