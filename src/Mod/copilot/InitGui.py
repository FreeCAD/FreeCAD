# SPDX-License-Identifier: LGPL-2.1-or-later
"""Graphical bootstrap for the Copilot workbench."""

import os

import FreeCAD as App
import FreeCADGui as Gui


class CopilotWorkbench(Gui.Workbench):
    """Workbench that exposes a natural-language CAD assistant."""

    def __init__(self):
        self.__class__.Icon = os.path.join(
            App.getResourceDir(), "Mod", "copilot", "Resources", "icons", "copilot.svg"
        )
        self.__class__.MenuText = "Copilot"
        self.__class__.ToolTip = "Create and edit CAD models with text prompts"

    def Initialize(self):
        import commands

        self.appendToolbar("Copilot", ["Copilot_ShowPanel"])
        self.appendMenu("Copilot", ["Copilot_ShowPanel"])
        App.Console.PrintLog("Loading Copilot workbench... done\n")

    def Activated(self):
        try:
            from panel import show_panel

            show_panel()
        except Exception as err:
            App.Console.PrintError("Failed to open Copilot panel: {0}\n".format(err))

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(CopilotWorkbench())
