# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - carregado pelo FreeCAD ao iniciar a interface.

import FreeCAD
import FreeCADGui

try:
    import SeworkGui

    SeworkGui.startup()
except Exception as exc:  # nunca impedir o FreeCAD de abrir
    FreeCAD.Console.PrintError("Sework: falha ao carregar o branding: {}\n".format(exc))


class SeworkWorkbench(Workbench):  # noqa: F821  (Workbench é injetado pelo FreeCAD)
    MenuText = "Sework"
    ToolTip = "Ferramentas e padrões da Sework Engenharia"

    def __init__(self):
        import SeworkGui

        self.__class__.Icon = SeworkGui.ICON_PATH

    def Initialize(self):
        import SeworkGui

        self.appendToolbar("Sework", SeworkGui.COMMANDS)
        self.appendMenu("Sework", SeworkGui.COMMANDS + ["Separator"] + SeworkGui.MENU_EXTRA)

    def Activated(self):
        pass

    def Deactivated(self):
        pass

    def GetClassName(self):
        return "Gui::PythonWorkbench"


FreeCADGui.addWorkbench(SeworkWorkbench())
