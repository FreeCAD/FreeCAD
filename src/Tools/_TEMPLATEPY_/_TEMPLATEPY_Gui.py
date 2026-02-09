# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# FreeCAD tools of the _TEMPLATEPY_ workbench
# (c) 2001 Juergen Riegel
# License LGPL

import FreeCAD, FreeCADGui


class CmdHelloWorld:
    def Activated(self):
        FreeCAD.Console.PrintMessage("Hello, World!\n")

    def IsActive(self):
        return True

    def GetResources(self):
        return {
            "Pixmap": "freecad",
            "MenuText": "Hello World",
            "ToolTip": "Print Hello World",
        }


FreeCADGui.addCommand("_TEMPLATEPY__HelloWorld", CmdHelloWorld())
