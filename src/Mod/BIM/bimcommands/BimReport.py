# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

import FreeCAD
import FreeCADGui


class BIM_Report:
    """The command to create a new BIM Report object."""

    def GetResources(self):
        return {
            "Pixmap": "BIM_Report",
            "MenuText": "BIM Report",
            "ToolTip": "Create a new BIM Report to query model data with SQL",
        }

    def Activated(self):
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("Arch.makeReport()")

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None


FreeCADGui.addCommand("BIM_Report", BIM_Report())
