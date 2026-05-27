# SPDX-License-Identifier: LGPL-2.1-or-later
"""FreeCAD GUI commands for Copilot."""

import FreeCAD as App
import FreeCADGui as Gui


class ShowCopilotPanelCommand:
    """Open the Copilot dock panel."""

    def GetResources(self):
        return {
            "MenuText": "Open Copilot",
            "ToolTip": "Open the Copilot prompt panel",
            "Pixmap": "applications-python",
        }

    def Activated(self):
        from panel import show_panel

        show_panel()

    def IsActive(self):
        return True


Gui.addCommand("Copilot_ShowPanel", ShowCopilotPanelCommand())
