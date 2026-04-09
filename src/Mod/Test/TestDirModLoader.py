# SPDX-License-Identifier: LGPL-2.1-or-later

import collections
import collections.abc as coll_abc
import dataclasses
from datetime import datetime
from enum import IntEnum
import functools
import importlib
import importlib.resources as resources
import inspect
import os
import pkgutil
import platform
import re
import sys
import tempfile
import traceback
import types
import unittest
from pathlib import Path

import FreeCAD
import FreeCADInitTools


class DirModLoaderTest(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.mod_root = Path(self.tempdir.name) / "AuditMod"
        self.mod_root.mkdir()
        self.init_py = self.mod_root / "Init.py"
        self.mod_name = f"audit_loader_{self._testMethodName}"
        self.compat_globals = FreeCADInitTools.dir_mod_compat_globals(
            {
                "__builtins__": __builtins__,
                "App": FreeCAD,
                "FreeCAD": FreeCAD,
                "Log": lambda *_args, **_kwargs: None,
                "Msg": lambda *_args, **_kwargs: None,
                "Err": lambda *_args, **_kwargs: None,
                "Wrn": lambda *_args, **_kwargs: None,
                "Crt": lambda *_args, **_kwargs: None,
                "Ntf": lambda *_args, **_kwargs: None,
                "Tnf": lambda *_args, **_kwargs: None,
                "IntEnum": IntEnum,
                "datetime": datetime,
                "Path": Path,
                "collections": collections,
                "coll_abc": coll_abc,
                "dataclasses": dataclasses,
                "functools": functools,
                "importlib": importlib,
                "inspect": inspect,
                "os": os,
                "pkgutil": pkgutil,
                "platform": platform,
                "re": re,
                "resources": resources,
                "sys": sys,
                "traceback": traceback,
                "types": types,
            },
            FreeCADInitTools.DIR_MOD_APP_COMPAT_GLOBAL_NAMES,
        )

    def tearDown(self):
        prefix = FreeCADInitTools.dir_mod_package_name(self.mod_name)
        for module_name in list(sys.modules):
            if module_name == prefix or module_name.startswith(f"{prefix}."):
                sys.modules.pop(module_name, None)
        self.tempdir.cleanup()

    def test_exec_dir_mod_file_preserves_module_identity(self):
        self.init_py.write_text(
            "class Marker:\n" "    pass\n" "value = App is FreeCAD\n",
            encoding="utf-8",
        )

        module = FreeCADInitTools.exec_dir_mod_file(
            self.mod_name,
            self.init_py,
            self.mod_root,
            self.compat_globals,
        )
        expected = FreeCADInitTools.dir_mod_module_name(
            self.mod_name,
            self.init_py,
            self.mod_root,
        )

        self.assertEqual(expected, module.__name__)
        self.assertEqual(expected, module.Marker.__module__)
        self.assertTrue(module.value)

    def test_failed_reload_restores_parent_binding(self):
        self.init_py.write_text("VALUE = 1\n", encoding="utf-8")
        module = FreeCADInitTools.exec_dir_mod_file(
            self.mod_name,
            self.init_py,
            self.mod_root,
            self.compat_globals,
        )
        expected = FreeCADInitTools.dir_mod_module_name(
            self.mod_name,
            self.init_py,
            self.mod_root,
        )

        parent_name, child_name = expected.rsplit(".", 1)
        parent = sys.modules[parent_name]
        self.assertIs(getattr(parent, child_name), module)

        self.init_py.write_text("raise RuntimeError('boom')\n", encoding="utf-8")
        with self.assertRaises(RuntimeError):
            FreeCADInitTools.exec_dir_mod_file(
                self.mod_name,
                self.init_py,
                self.mod_root,
                self.compat_globals,
            )

        self.assertIs(sys.modules[expected], module)
        self.assertIs(getattr(parent, child_name), module)

    def test_exec_dir_mod_file_handles_symlinked_build_tree_paths(self):
        source_root = Path(self.tempdir.name) / "src" / "Mod" / "AuditMod"
        build_root = Path(self.tempdir.name) / "build" / "Mod" / "AuditMod"
        source_root.mkdir(parents=True)
        build_root.mkdir(parents=True)

        source_init = source_root / "Init.py"
        source_init.write_text("VALUE = 1\n", encoding="utf-8")
        build_init = build_root / "Init.py"
        build_init.symlink_to(source_init)

        module = FreeCADInitTools.exec_dir_mod_file(
            self.mod_name,
            build_init,
            build_root,
            self.compat_globals,
        )
        expected = FreeCADInitTools.dir_mod_module_name(
            self.mod_name,
            build_init,
            build_root,
        )

        self.assertEqual(expected, module.__name__)
        self.assertEqual(str(build_init.absolute()), module.__file__)

    def test_dir_mod_module_name_accepts_source_file_with_build_root(self):
        source_root = Path(self.tempdir.name) / "src" / "Mod" / "AuditMod"
        build_root = Path(self.tempdir.name) / "build" / "Mod" / "AuditMod"
        source_root.mkdir(parents=True)
        build_root.mkdir(parents=True)

        source_init = source_root / "InitGui.py"
        source_init.write_text("VALUE = 1\n", encoding="utf-8")
        (build_root / "InitGui.py").symlink_to(source_init)

        module_name = FreeCADInitTools.dir_mod_module_name(
            self.mod_name,
            source_init,
            build_root,
        )

        self.assertEqual(
            f"{FreeCADInitTools.dir_mod_package_name(self.mod_name)}.InitGui",
            module_name,
        )

    def test_dir_mod_base_path_accepts_source_file_with_build_root(self):
        source_root = Path(self.tempdir.name) / "src" / "Mod" / "AuditMod"
        build_root = Path(self.tempdir.name) / "build" / "Mod" / "AuditMod"
        source_root.mkdir(parents=True)
        build_root.mkdir(parents=True)

        source_init_gui = source_root / "InitGui.py"
        source_init_gui.write_text("VALUE = 1\n", encoding="utf-8")
        (build_root / "InitGui.py").symlink_to(source_init_gui)

        base_path = FreeCADInitTools.dir_mod_base_path(
            source_init_gui,
            build_root,
        )

        self.assertEqual(source_root.resolve(), base_path)

    def test_exec_dir_mod_file_preserves_real_freecad_namespace_package(self):
        freecad = importlib.import_module("freecad")
        freecad_paths = tuple(freecad.__path__)
        from freecad import module_io

        self.init_py.write_text("VALUE = 1\n", encoding="utf-8")
        FreeCADInitTools.exec_dir_mod_file(
            self.mod_name,
            self.init_py,
            self.mod_root,
            self.compat_globals,
        )

        reloaded_freecad = importlib.import_module("freecad")
        from freecad import module_io as imported_module_io

        self.assertIs(freecad, reloaded_freecad)
        self.assertEqual(freecad_paths, tuple(reloaded_freecad.__path__))
        self.assertIs(module_io, imported_module_io)
        self.assertIn("freecad._dir_mods", sys.modules)

    def test_runtime_compat_globals_are_curated(self):
        self.assertIn("__builtins__", self.compat_globals)
        self.assertIn("App", self.compat_globals)
        self.assertIn("Log", self.compat_globals)
        self.assertIn("inspect", self.compat_globals)
        self.assertNotIn("exec_dir_mod_file", self.compat_globals)
        self.assertNotIn("_ensure_dir_mod_packages", self.compat_globals)

    def test_support_module_is_importable(self):
        self.assertEqual("FreeCADInitTools", FreeCADInitTools.__name__)

    def test_support_module_loads_from_library_dir(self):
        expected = Path(FreeCAD.getLibraryDir()).resolve() / "FreeCADInitTools.py"
        self.assertEqual(expected, Path(FreeCADInitTools.__file__).absolute())
