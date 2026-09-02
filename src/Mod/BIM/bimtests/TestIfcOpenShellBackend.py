# SPDX-License-Identifier: LGPL-2.1-or-later

"""Hermetic tests for the IfcOpenShell runtime boundary."""

from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import patch

from nativeifc import backend


def _modules(*, serialized=True, iterator=True, version="0.8.5"):
    root = ModuleType("ifcopenshell")
    root.version = version
    root.open = lambda _filename: None
    root.file = lambda *args, **kwargs: None
    wrapper_values = {"schema_names": lambda: ("IFC2X3", "IFC4")}
    if serialized:
        wrapper_values["SERIALIZED"] = object()
    root.ifcopenshell_wrapper = SimpleNamespace(**wrapper_values)

    result = {name: ModuleType(name) for name in backend.REQUIRED_MODULES}
    result["ifcopenshell"] = root
    result["ifcopenshell.api"].run = lambda *args, **kwargs: None
    if iterator:
        result["ifcopenshell.geom"].iterator = lambda *args, **kwargs: None
    return result


class TestIfcOpenShellBackend(unittest.TestCase):
    def tearDown(self):
        backend.invalidate()

    def test_missing_package_is_unavailable(self):
        with patch.object(
            backend.importlib,
            "import_module",
            side_effect=ModuleNotFoundError("No module named 'ifcopenshell'"),
        ):
            status = backend.get_status()

        self.assertFalse(status.available)
        self.assertIn("ModuleNotFoundError", status.error)
        with self.assertRaises(backend.IfcOpenShellUnavailable):
            backend.get_backend()

    def test_partial_installation_is_unavailable(self):
        modules = _modules()

        def load(name):
            if name == "ifcopenshell.geom":
                raise ImportError("native geometry wrapper failed to load")
            return modules[name]

        with patch.object(backend.importlib, "import_module", side_effect=load):
            status = backend.get_status()

        self.assertFalse(status.available)
        self.assertIn("native geometry wrapper failed", status.error)

    def test_reports_validated_capabilities(self):
        modules = _modules()
        with patch.object(backend.importlib, "import_module", side_effect=modules.__getitem__):
            status = backend.get_status()
            loaded = backend.get_backend()

        self.assertTrue(status.available)
        self.assertEqual(status.version, "0.8.5")
        self.assertTrue(status.geometry_iterator)
        self.assertTrue(status.serialized_brep)
        self.assertTrue(status.schema_names)
        self.assertIs(loaded, modules["ifcopenshell"])

    def test_optional_capabilities_are_not_assumed(self):
        modules = _modules(serialized=False)
        with patch.object(backend.importlib, "import_module", side_effect=modules.__getitem__):
            status = backend.get_status()

        self.assertTrue(status.available)
        self.assertTrue(status.geometry_iterator)
        self.assertFalse(status.serialized_brep)

    def test_missing_required_api_is_unavailable(self):
        modules = _modules(iterator=False)
        with patch.object(backend.importlib, "import_module", side_effect=modules.__getitem__):
            status = backend.get_status()

        self.assertFalse(status.available)
        self.assertIn("ifcopenshell.geom.iterator", status.error)

    def test_refresh_revalidates_after_environment_change(self):
        modules = _modules()
        attempts = iter(
            (
                ModuleNotFoundError("not installed"),
                *(modules[name] for name in backend.REQUIRED_MODULES),
            )
        )

        def load(_name):
            value = next(attempts)
            if isinstance(value, Exception):
                raise value
            return value

        with patch.object(backend.importlib, "import_module", side_effect=load):
            self.assertFalse(backend.get_status().available)
            self.assertTrue(backend.get_status(refresh=True).available)


if __name__ == "__main__":
    unittest.main()
