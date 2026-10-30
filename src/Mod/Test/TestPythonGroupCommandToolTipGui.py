# SPDX-License-Identifier: LGPL-2.1-or-later
"""The toolbar tooltip of a Python group command is the tooltip of its active tool
(issue #24639): title, help text and command name appear once."""

import unittest
from uuid import uuid4

import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtWidgets


class _Child:
    def __init__(self, tag):
        self.tag = tag

    def GetResources(self):
        return {"MenuText": "Child menu " + self.tag, "ToolTip": "Child tip " + self.tag}

    def IsActive(self):
        return True

    def Activated(self):
        pass


class _Group:
    def __init__(self, commands):
        self.commands = commands

    def GetResources(self):
        return {"MenuText": "Group menu", "ToolTip": "Group tip"}

    def GetCommands(self):
        return self.commands

    def IsActive(self):
        return True


@unittest.skipUnless(App.GuiUp, "Requires the GUI")
class TestPythonGroupCommandToolTipGui(unittest.TestCase):
    def setUp(self):
        token = uuid4().hex[:8]
        self.child_a = "TipChildA_" + token
        self.child_b = "TipChildB_" + token
        self.group = "TipGroup_" + token
        self.toolbar = "TipToolbar_" + token
        self.workbench = "TipWorkbench_" + token
        Gui.addCommand(self.child_a, _Child("A"))
        Gui.addCommand(self.child_b, _Child("B"))
        Gui.addCommand(self.group, _Group((self.child_a, self.child_b)))

        active = Gui.activeWorkbench()
        self.previous = active.name() if active else None
        self.addCleanup(self._cleanup)

        toolbar, group = self.toolbar, self.group

        def initialize(workbench):
            workbench.appendToolbar(toolbar, [group])

        cls = type(
            self.workbench,
            (Gui.Workbench,),
            {
                "MenuText": self.workbench,
                "ToolTip": self.workbench,
                "Initialize": initialize,
                "GetClassName": lambda workbench: "Gui::PythonWorkbench",
            },
        )
        Gui.addWorkbench(cls())
        self.assertTrue(Gui.activateWorkbench(self.workbench))
        QtWidgets.QApplication.processEvents()

    def _cleanup(self):
        if self.previous:
            Gui.activateWorkbench(self.previous)
        Gui.removeWorkbench(self.workbench)
        QtWidgets.QApplication.processEvents()

    def _group_tooltip(self):
        bars = Gui.getMainWindow().findChildren(QtWidgets.QToolBar, self.toolbar)
        self.assertEqual(len(bars), 1)
        actions = [a for a in bars[0].actions() if not a.isSeparator()]
        self.assertEqual(len(actions), 1)
        return actions[0].toolTip()

    def testCollapsedGroupShowsActiveToolOnce(self):
        tip = self._group_tooltip()
        self.assertIn("<b>Child menu A</b>", tip)
        self.assertIn("Child tip A", tip)
        self.assertNotIn("Group menu", tip)
        self.assertEqual(tip.count("<b>"), 1, tip)
        self.assertEqual(tip.count("<i>"), 1, tip)
        self.assertEqual(tip.count(self.child_a), 1, tip)
        self.assertNotIn(self.child_b, tip)
        # like a C++ group command: "<active tool> (<group>:<index>)"
        self.assertIn("<i>{} ({}:0)</i>".format(self.child_a, self.group), tip)
