# Cloud gui init module
# (c) 2001 Juergen Riegel LGPL
# (c) 2019 Jean-Marie Verdun LGPL


class CloudWorkbench(Workbench):
    "Cloud workbench object"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Cloud/Resources/icons/CloudWorkbench.svg"
        )

    MenuText = "Cloud"
    ToolTip = "Cloud workbench"

    def Initialize(self):
        # load the module
        import CloudGui

    def GetClassName(self):
        return "CloudGui::Workbench"


Gui.addWorkbench(CloudWorkbench())
