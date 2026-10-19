# SPDX-License-Identifier: LGPL-2.1-or-later
"""GUI restore regressions for saved group and member visibility."""

from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

import FreeCAD as App
import Part


class GroupViewProxy:
    def dumps(self):
        return None

    def loads(self, state):
        pass


@unittest.skipIf(not App.GuiUp, "GUI tests require FreeCAD GUI mode")
class TestGroupVisibilityRestoreGui(unittest.TestCase):
    def _rewrite_archive(self, source, target, mode, group_first):
        with zipfile.ZipFile(source) as archive, zipfile.ZipFile(target, "w") as output:
            for entry in archive.infolist():
                data = archive.read(entry.filename)
                if entry.filename == "GuiDocument.xml":
                    if mode == "missing_gui":
                        continue
                    root = ET.fromstring(data)
                    reordered = False
                    for parent in root.iter():
                        providers = [child for child in parent if child.tag == "ViewProvider"]
                        if providers:
                            self.assertEqual(
                                {child.get("name") for child in providers}, {"Box", "Group"}
                            )
                            for child in providers:
                                parent.remove(child)
                            providers.sort(
                                key=lambda child: child.get("name") == "Group", reverse=group_first
                            )
                            parent.extend(providers)
                            reordered = True
                    self.assertTrue(reordered)
                    data = ET.tostring(root, encoding="utf-8", xml_declaration=True)
                elif entry.filename == "Document.xml" and mode == "legacy_without_app_visibility":
                    root = ET.fromstring(data)
                    removed = 0
                    for properties in root.iter("Properties"):
                        for prop in list(properties):
                            if prop.tag == "Property" and prop.get("name") == "Visibility":
                                properties.remove(prop)
                                removed += 1
                        if "Count" in properties.attrib:
                            properties.set(
                                "Count", str(sum(prop.tag == "Property" for prop in properties))
                            )
                    self.assertEqual(removed, 2)
                    data = ET.tostring(root, encoding="utf-8", xml_declaration=True)
                output.writestr(entry, data)

    def _check_visibility(self, kind, mode, group_first=True):
        # https://github.com/FreeCAD/FreeCAD/issues/33675
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            original = directory / "original.FCStd"
            document = App.newDocument("GroupVisibilityRestore")
            try:
                box = document.addObject("Part::Box", "Box")
                group = document.addObject(kind, "Group")
                if kind.endswith("Python"):
                    group.ViewObject.Proxy = GroupViewProxy()
                group.addObject(box)
                box.ViewObject.Visibility = False
                document.recompute()
                self.assertTrue(group.Visibility)
                self.assertFalse(box.Visibility)
                self.assertFalse(box.ViewObject.Visibility)
                document.saveAs(str(original))
            finally:
                App.closeDocument(document.Name)

            target = directory / "restored.FCStd"
            self._rewrite_archive(original, target, mode, group_first)
            restored = App.openDocument(str(target))
            try:
                self.assertTrue(restored.Group.Visibility)
                self.assertTrue(restored.Group.ViewObject.Visibility)
                self.assertFalse(restored.Box.Visibility)
                self.assertFalse(restored.Box.ViewObject.Visibility)

                # Ordinary post-restore GUI/App visibility synchronization must continue.
                restored.Group.ViewObject.Visibility = False
                self.assertFalse(restored.Group.Visibility)
                restored.Group.ViewObject.Visibility = True
                self.assertTrue(restored.Group.Visibility)
                self.assertTrue(restored.Box.Visibility)
                self.assertTrue(restored.Box.ViewObject.Visibility)
                restored.Box.Visibility = False
                self.assertFalse(restored.Box.ViewObject.Visibility)
            finally:
                App.closeDocument(restored.Name)

    def _check_both_orders(self, kind, mode):
        for group_first in (True, False):
            with self.subTest(group_first=group_first):
                self._check_visibility(kind, mode, group_first)

    def test_plain_group_restores_member_visibility_in_both_orders(self):
        self._check_both_orders("App::DocumentObjectGroup", "normal")

    def test_python_group_restores_member_visibility_in_both_orders(self):
        self._check_both_orders("App::DocumentObjectGroupPython", "normal")

    def test_plain_group_restores_legacy_gui_visibility_in_both_orders(self):
        self._check_both_orders("App::DocumentObjectGroup", "legacy_without_app_visibility")

    def test_python_group_restores_legacy_gui_visibility_in_both_orders(self):
        self._check_both_orders("App::DocumentObjectGroupPython", "legacy_without_app_visibility")

    def test_plain_group_uses_app_visibility_without_gui_document(self):
        self._check_visibility("App::DocumentObjectGroup", "missing_gui")

    def test_python_group_uses_app_visibility_without_gui_document(self):
        self._check_visibility("App::DocumentObjectGroupPython", "missing_gui")
