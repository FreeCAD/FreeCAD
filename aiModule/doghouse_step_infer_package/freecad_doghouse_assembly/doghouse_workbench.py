"""FreeCAD Workbench registration for doghouse clip assembly preview."""
from __future__ import annotations

try:
    import FreeCADGui

    _WorkbenchBase = FreeCADGui.Workbench
except Exception:
    _WorkbenchBase = object


class DoghouseAutoAssembleCommand:
    def GetResources(self):
        return {
            "MenuText": "Doghouse Auto Assembly",
            "ToolTip": "Analyze doghouse mounting holes and preview selected clips",
        }

    def IsActive(self):
        try:
            import FreeCAD

            return FreeCAD.ActiveDocument is not None
        except Exception:
            return False

    def Activated(self):
        import FreeCADGui

        from doghouse_task_panel import DoghouseTaskPanel

        FreeCADGui.Control.showDialog(DoghouseTaskPanel())


class DoghouseAssemblyWorkbench(_WorkbenchBase):
    MenuText = "Doghouse Assembly"
    ToolTip = "Doghouse mounting face, hole analysis, clip recommendation and preview"
    Icon = ""

    def Initialize(self):
        import FreeCADGui

        FreeCADGui.addCommand("Doghouse_Auto_Assemble", DoghouseAutoAssembleCommand())
        commands = ["Doghouse_Auto_Assemble"]
        self.appendToolbar("Doghouse Assembly", commands)
        self.appendMenu("Doghouse Assembly", commands)

    def Activated(self):
        pass

    def Deactivated(self):
        pass

    def GetClassName(self):
        return "Gui::PythonWorkbench"
