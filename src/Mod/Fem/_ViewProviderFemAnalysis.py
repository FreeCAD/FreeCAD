import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui


class _ViewProviderFemAnalysis:
    "A View Provider for the FemAnalysis container object"

    def __init__(self):
        #vobj.addProperty("App::PropertyLength", "BubbleSize", "Base", str(translate("Fem", "The size of the axis bubbles")))
        pass

    def getIcon(self):
        return ":/icons/fem-analysis.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        if not FemGui.getActiveAnalysis() == self.Object:
            if FreeCADGui.activeWorkbench().name() != 'FemWorkbench':
                FreeCADGui.activateWorkbench("FemWorkbench")
            FemGui.setActiveAnalysis(self.Object)
            return True
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
