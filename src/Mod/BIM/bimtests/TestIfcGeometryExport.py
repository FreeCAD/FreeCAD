# SPDX-License-Identifier: LGPL-2.1-or-later

"""Hermetic tests for NativeIFC's geometry-export boundary."""

import importlib
import sys
import unittest
from unittest.mock import patch

import nativeifc
from nativeifc import backend


class TestIfcGeometryExport(unittest.TestCase):
    def _load_adapter(self):
        module_name = "nativeifc.ifc_geometry_export"
        sys.modules.pop(module_name, None)
        return importlib.import_module(module_name)

    def tearDown(self):
        sys.modules.pop("nativeifc.ifc_geometry_export", None)
        if hasattr(nativeifc, "ifc_geometry_export"):
            delattr(nativeifc, "ifc_geometry_export")

    def test_unsupported_empty_object_has_no_representation(self):
        with patch.object(backend, "get_backend", return_value=object()):
            adapter_module = self._load_adapter()
            adapter = adapter_module.GeometryExporter("ifc-file")
            with patch.object(adapter, "_identity_placement", return_value="placement"):
                result = adapter.create_representation("context", "object", {"option": True})
        self.assertEqual(result, (None, "placement"))


if __name__ == "__main__":
    unittest.main()
