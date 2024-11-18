# _TEMPLATE_ gui init module
# (c) 2001 Juergen Riegel LGPL


class _TEMPLATE_Workbench(Workbench):
    "_TEMPLATE_ workbench object"
    MenuText = "_TEMPLATE_"
    ToolTip = "_TEMPLATE_ workbench"

    def Initialize(self):
        # load the module
        import _TEMPLATE_Gui

    def GetClassName(self):
        return "_TEMPLATE_Gui::Workbench"


Gui.addWorkbench(_TEMPLATE_Workbench())
