# SPDX-License-Identifier: LGPL-2.1-or-later

"""Hermetic tests for the IfcOpenShell runtime boundary."""

import ast
from pathlib import Path
from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import patch

from nativeifc import backend


def _modules(*, serialized=True, iterator=True, version="0.8.5"):
    class EntityInstance:
        pass

    root = ModuleType("ifcopenshell")
    root.version = version
    root.open = lambda _filename: None
    root.file = lambda *args, **kwargs: None
    wrapper_values = {"schema_names": lambda: ("IFC2X3", "IFC4")}
    if serialized:
        wrapper_values["SERIALIZED"] = object()
    root.ifcopenshell_wrapper = SimpleNamespace(**wrapper_values)

    result = {name: ModuleType(name) for name in backend.REQUIRED_MODULES}
    result["ifcopenshell.guid"] = ModuleType("ifcopenshell.guid")
    result["ifcopenshell"] = root
    result["ifcopenshell.api"].run = lambda *args, **kwargs: None
    result["ifcopenshell.api.aggregate"].assign_object = lambda *args, **kwargs: None
    result["ifcopenshell.api.feature"].add_feature = lambda *args, **kwargs: None
    result["ifcopenshell.api.feature"].add_filling = lambda *args, **kwargs: None
    result["ifcopenshell.api.geometry"].add_wall_representation = lambda *args, **kwargs: None
    result["ifcopenshell.api.geometry"].add_profile_representation = lambda *args, **kwargs: None
    result["ifcopenshell.api.group"].add_group = lambda *args, **kwargs: None
    result["ifcopenshell.api.group"].assign_group = lambda *args, **kwargs: None
    result["ifcopenshell.api.material"].add_material = lambda *args, **kwargs: None
    result["ifcopenshell.api.material"].add_material_set = lambda *args, **kwargs: None
    result["ifcopenshell.api.material"].add_layer = lambda *args, **kwargs: None
    result["ifcopenshell.api.material"].edit_layer = lambda *args, **kwargs: None
    result["ifcopenshell.api.material"].add_profile = lambda *args, **kwargs: None
    result["ifcopenshell.api.material"].assign_material = lambda *args, **kwargs: None
    result["ifcopenshell.api.pset"].add_pset = lambda *args, **kwargs: None
    result["ifcopenshell.api.pset"].edit_pset = lambda *args, **kwargs: None
    result["ifcopenshell.api.pset"].add_qto = lambda *args, **kwargs: None
    result["ifcopenshell.api.pset"].edit_qto = lambda *args, **kwargs: None
    result["ifcopenshell.api.spatial"].assign_container = lambda *args, **kwargs: None
    result["ifcopenshell.api.style"].add_style = lambda *args, **kwargs: None
    result["ifcopenshell.api.style"].add_surface_style = lambda *args, **kwargs: None
    result["ifcopenshell.api.style"].assign_item_style = lambda *args, **kwargs: None
    result["ifcopenshell.api.type"].assign_type = lambda *args, **kwargs: None
    result["ifcopenshell.geom"].settings = lambda *args, **kwargs: None
    result["ifcopenshell.geom"].create_shape = lambda *args, **kwargs: None
    result["ifcopenshell.entity_instance"].entity_instance = EntityInstance
    result["ifcopenshell.guid"].new = lambda: "0" * 22
    if iterator:
        result["ifcopenshell.geom"].iterator = lambda *args, **kwargs: None
    return result


class TestIfcOpenShellBackend(unittest.TestCase):
    def tearDown(self):
        backend.invalidate()

    def test_invalidate_refreshes_python_import_caches(self):
        with patch.object(backend.importlib, "invalidate_caches") as invalidate_caches:
            backend.invalidate()
        invalidate_caches.assert_called_once_with()

    def test_nativeifc_has_no_direct_ifcopenshell_imports(self):
        nativeifc = Path(__file__).resolve().parents[1] / "nativeifc"
        violations = []
        for path in nativeifc.glob("*.py"):
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    names = [alias.name for alias in node.names]
                elif isinstance(node, ast.ImportFrom) and node.module:
                    names = [node.module]
                else:
                    continue
                if any(
                    name == "ifcopenshell" or name.startswith("ifcopenshell.") for name in names
                ):
                    violations.append(f"{path.name}:{node.lineno}")
        self.assertEqual(violations, [])

    def test_primary_importers_use_backend_boundary(self):
        bim = Path(__file__).resolve().parents[1]
        violations = []
        for name in ("importIFC.py", "importIFCmulticore.py"):
            path = bim / "importers" / name
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    names = [alias.name for alias in node.names]
                elif isinstance(node, ast.ImportFrom) and node.module:
                    names = [node.module]
                else:
                    continue
                if any(
                    module == "ifcopenshell" or module.startswith("ifcopenshell.")
                    for module in names
                ):
                    violations.append(f"{path.name}:{node.lineno}")
        self.assertEqual(violations, [])

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

    def test_read_does_not_require_geometry_or_export_apis(self):
        modules = _modules(serialized=False)
        read_modules = {name: modules[name] for name in backend.READ_MODULES}
        with patch.object(backend.importlib, "import_module", side_effect=read_modules.__getitem__):
            status = backend.get_status(capability=backend.READ)
            loaded = backend.get_backend(capability=backend.READ)

        self.assertTrue(status.available)
        self.assertFalse(status.geometry_iterator)
        self.assertIs(loaded, modules["ifcopenshell"])

    def test_read_requires_file_opening_api(self):
        modules = _modules()
        del modules["ifcopenshell"].open
        read_modules = {name: modules[name] for name in backend.READ_MODULES}
        with patch.object(backend.importlib, "import_module", side_effect=read_modules.__getitem__):
            status = backend.get_status(capability=backend.READ)

        self.assertFalse(status.available)
        self.assertIn("ifcopenshell.open", status.error)

    def test_guid_does_not_require_file_or_geometry_apis(self):
        modules = _modules()
        del modules["ifcopenshell"].open
        guid_modules = {name: modules[name] for name in backend.GUID_MODULES}
        with patch.object(backend.importlib, "import_module", side_effect=guid_modules.__getitem__):
            status = backend.get_status(capability=backend.GUID)
            guid = backend.new_guid()

        self.assertTrue(status.available)
        self.assertEqual(guid, "0" * 22)

    def test_guid_requires_generator_api(self):
        modules = _modules()
        del modules["ifcopenshell.guid"].new
        guid_modules = {name: modules[name] for name in backend.GUID_MODULES}
        with patch.object(backend.importlib, "import_module", side_effect=guid_modules.__getitem__):
            status = backend.get_status(capability=backend.GUID)

        self.assertFalse(status.available)
        self.assertIn("ifcopenshell.guid.new", status.error)

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

    def test_import_does_not_require_export_apis(self):
        modules = _modules()
        import_modules = {name: modules[name] for name in backend.IMPORT_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=import_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.IMPORT)
            loaded = backend.get_backend(capability=backend.IMPORT)
            geom = backend.get_module("ifcopenshell.geom", capability=backend.IMPORT)

        self.assertTrue(status.available)
        self.assertIs(loaded, modules["ifcopenshell"])
        self.assertIs(geom, modules["ifcopenshell.geom"])

    def test_import_requires_geometry_creation_api(self):
        modules = _modules()
        del modules["ifcopenshell.geom"].create_shape
        import_modules = {name: modules[name] for name in backend.IMPORT_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=import_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.IMPORT)

        self.assertFalse(status.available)
        self.assertIn("ifcopenshell.geom.create_shape", status.error)

    def test_import_requires_serialized_geometry_output(self):
        modules = _modules(serialized=False)
        import_modules = {name: modules[name] for name in backend.IMPORT_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=import_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.IMPORT)

        self.assertFalse(status.available)
        self.assertIn("serialized geometry output", status.error)

    def test_multicore_import_does_not_require_export_apis(self):
        modules = _modules()
        import_modules = {name: modules[name] for name in backend.IMPORT_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=import_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.MULTICORE_IMPORT)
            geom = backend.get_module("ifcopenshell.geom", capability=backend.MULTICORE_IMPORT)

        self.assertTrue(status.available)
        self.assertIs(geom, modules["ifcopenshell.geom"])

    def test_multicore_import_requires_geometry_iterator(self):
        modules = _modules(iterator=False)
        import_modules = {name: modules[name] for name in backend.IMPORT_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=import_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.MULTICORE_IMPORT)

        self.assertFalse(status.available)
        self.assertIn("ifcopenshell.geom.iterator", status.error)

    def test_explorer_does_not_require_serialized_or_export_apis(self):
        modules = _modules(serialized=False)
        explorer_modules = {name: modules[name] for name in backend.EXPLORER_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=explorer_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.EXPLORER)

        self.assertTrue(status.available)
        self.assertFalse(status.serialized_brep)

    def test_explorer_requires_entity_instance_type(self):
        modules = _modules()
        del modules["ifcopenshell.entity_instance"].entity_instance
        explorer_modules = {name: modules[name] for name in backend.EXPLORER_MODULES}
        with patch.object(
            backend.importlib, "import_module", side_effect=explorer_modules.__getitem__
        ):
            status = backend.get_status(capability=backend.EXPLORER)

        self.assertFalse(status.available)
        self.assertIn("ifcopenshell.entity_instance.entity_instance", status.error)

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
