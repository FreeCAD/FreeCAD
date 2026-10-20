# SPDX-License-Identifier: LGPL-2.1-or-later
"""Insert Component tree regressions for assemblies organized in plain groups."""

import unittest

import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    import AssemblyGui  # noqa: F401
    import Part  # noqa: F401
    import CommandInsertLink
    from PySide import QtCore, QtWidgets


@unittest.skipIf(not App.GuiUp, "GUI tests require FreeCAD GUI mode")
class TestGroupedAssemblyInsert(unittest.TestCase):
    def setUp(self):
        self.previous = App.activeDocument().Name if App.activeDocument() else None
        self.doc = App.newDocument("GroupedAssembly26537")
        self.assembly = self.doc.addObject("Assembly::AssemblyObject", "Assembly")
        self.source = self.doc.addObject("App::Part", "SourcePart")
        self.source.newObject("Part::Box", "SourceBox")
        self.task = None
        self.form = None
        pref = CommandInsertLink.Preferences.preferences()
        self.saved_preferences = {
            key: pref.GetBool(key) if key in pref.GetBools() else None
            for key in ("InsertShowOnlyParts", "InsertRigidSubAssemblies")
        }

    def tearDown(self):
        try:
            if self.task is not None:
                try:
                    self.task.reject()
                finally:
                    self.form.deleteLater()
                    self.task.deleteLater()
                    QtCore.QCoreApplication.sendPostedEvents(None, QtCore.QEvent.DeferredDelete)
                    QtWidgets.QApplication.processEvents()
        finally:
            pref = CommandInsertLink.Preferences.preferences()
            for key, value in self.saved_preferences.items():
                if value is None:
                    pref.RemBool(key)
                else:
                    pref.SetBool(key, value)
            if self.doc.Name in App.listDocuments():
                App.closeDocument(self.doc.Name)
            if self.previous in App.listDocuments():
                App.setActiveDocument(self.previous)

    def _listed(self, only_parts=False):
        self.doc.recompute()
        self.task = CommandInsertLink.TaskAssemblyInsertLink(
            self.assembly, Gui.activeDocument().activeView())
        # Keep both native QObject wrappers until normal task teardown.
        self.form = self.task.form
        self.form.CheckBox_ShowOnlyParts.setChecked(only_parts)
        self.task.buildPartList()
        listed = set()
        iterator = QtWidgets.QTreeWidgetItemIterator(self.form.partList)
        while iterator.value():
            item = iterator.value()
            obj = item.data(0, QtCore.Qt.UserRole)
            if obj is not None:
                listed.add((obj.Document.Name, obj.Name))
            iterator += 1
        return listed

    def _key(self, obj):
        return (obj.Document.Name, obj.Name)

    def test_root_level_source_control(self):
        listed = self._listed()
        self.assertIn(self._key(self.source), listed)
        self.assertNotIn(self._key(self.assembly), listed)

    def test_same_plain_group_sibling_is_available(self):
        group = self.doc.addObject("App::DocumentObjectGroup", "Group")
        group.Group = [self.assembly, self.source]
        self.assertIn(group, self.assembly.InListRecursive)
        self.assertNotIn(self.source, self.assembly.InListRecursive)
        listed = self._listed()
        self.assertIn(self._key(self.source), listed)
        self.assertNotIn(self._key(self.assembly), listed)
        self.assertNotIn(self._key(group), listed)

    def test_nested_plain_group_sibling_is_available_in_parts_filter(self):
        outer = self.doc.addObject("App::DocumentObjectGroup", "Outer")
        inner = self.doc.addObject("App::DocumentObjectGroup", "Inner")
        outer.Group = [self.assembly, inner]
        inner.Group = [self.source]
        self.assertIn(outer, self.assembly.InListRecursive)
        self.assertNotIn(self.source, self.assembly.InListRecursive)
        listed = self._listed(only_parts=True)
        self.assertIn(self._key(self.source), listed)
        self.assertNotIn(self._key(self.assembly), listed)
        self.assertNotIn(self._key(outer), listed)
        self.assertNotIn(self._key(inner), listed)

    def test_real_part_ancestor_remains_unavailable(self):
        parent = self.doc.addObject("App::Part", "ParentPart")
        parent.addObject(self.assembly)
        self.assertIn(parent, self.assembly.InListRecursive)
        listed = self._listed()
        self.assertNotIn(self._key(parent), listed)
        self.assertNotIn(self._key(self.assembly), listed)
        self.assertIn(self._key(self.source), listed)

    def test_real_assembly_ancestor_remains_unavailable(self):
        parent = self.doc.addObject("Assembly::AssemblyObject", "ParentAssembly")
        parent.addObject(self.assembly)
        self.assertIn(parent, self.assembly.InListRecursive)
        listed = self._listed()
        self.assertNotIn(self._key(parent), listed)
        self.assertNotIn(self._key(self.assembly), listed)
        self.assertIn(self._key(self.source), listed)
