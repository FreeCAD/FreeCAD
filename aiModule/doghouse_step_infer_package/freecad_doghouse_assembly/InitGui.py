"""FreeCAD GUI bootstrap for Doghouse Assembly."""

import FreeCADGui as Gui

from doghouse_workbench import DoghouseAssemblyWorkbench


Gui.addWorkbench(DoghouseAssemblyWorkbench())
