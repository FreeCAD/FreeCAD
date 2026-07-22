# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCADGui as Gui


class MbDFEMWorkbench(Gui.Workbench):
    """Minimal MbDFEM workbench."""

    MenuText = "MbDFEM"
    ToolTip = "MbDFEM workbench"

    def Initialize(self):
        import MbDFEM  # noqa: F401

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(MbDFEMWorkbench())
