# SPDX-License-Identifier: LGPL-2.1-or-later
"""The toolbar tooltip of a Python group command is the tooltip of its active tool
(issue #24639): title, help text, shortcut and command name appear once."""

import unittest
from uuid import uuid4

import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtGui, QtWidgets
else:
    Gui = QtGui = QtWidgets = None

ACCEL = "Ctrl+Alt+Shift+F11"


class _Child:
    def __init__(self, tag, accel=""):
        self.tag = tag
        self.accel = accel

    def GetResources(self):
        """Command resources, with an optional shortcut."""
        res = {"MenuText": f"Child menu {self.tag}", "ToolTip": f"Child tip {self.tag}"}
        if self.accel:
            res["Accel"] = self.accel
        return res

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
    """Tooltip of the toolbar button of a Python group command."""

    def setUp(self):
        token = uuid4().hex[:8]
        self.child_a = f"TipChildA_{token}"
        self.child_b = f"TipChildB_{token}"
        self.group = f"TipGroup_{token}"
        self.toolbar = f"TipToolbar_{token}"
        self.workbench = f"TipWorkbench_{token}"
        Gui.addCommand(self.child_a, _Child("A", ACCEL))
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

    def _assert_single_tool(self, tip, tool, other, tag, index):
        self.assertIn(f"<b>Child menu {tag}</b>", tip)
        self.assertIn(f"Child tip {tag}", tip)
        self.assertNotIn("Group menu", tip)
        self.assertEqual(tip.count("<b>"), 1, tip)
        self.assertEqual(tip.count("<i>"), 1, tip)
        self.assertEqual(tip.count(tool), 1, tip)
        self.assertNotIn(other, tip)
        # like a C++ group command: "<active tool> (<group>:<index>)"
        self.assertIn(f"<i>{tool} ({self.group}:{index})</i>", tip)

    def testCollapsedGroupShowsActiveToolOnce(self):
        """The default tool is described once, with its shortcut."""
        tip = self._group_tooltip()
        self._assert_single_tool(tip, self.child_a, self.child_b, "A", 0)
        shortcut = QtGui.QKeySequence(ACCEL).toString(QtGui.QKeySequence.NativeText)
        self.assertIn(f"<b>Child menu A</b> ({shortcut})", tip)

    def testPickedToolShowsCommandNameLikeDefaultTool(self):
        """After picking a tool from the drop-down, its tooltip has the same form."""
        children = Gui.Command.get(self.group).getAction()
        self.assertEqual(len(children), 2)
        children[1].trigger()
        QtWidgets.QApplication.processEvents()

        tip = self._group_tooltip()
        self._assert_single_tool(tip, self.child_b, self.child_a, "B", 1)
        self.assertIn("<b>Child menu B</b></p>", tip)
