# InitGui.py - GUI integration for Hatch Generator
# This makes the tool available in BIM, Draft, and other workbenches

import FreeCAD
import FreeCADGui
import os

class HatchGeneratorWorkbench(FreeCADGui.Workbench):
    """
    Workbench class for Hatch Generator (optional standalone workbench)
    """
    MenuText = "Hatch Generator"
    ToolTip = "Parametric hatch patterns for surfaces, walls, and roofs"
    Icon = os.path.join(os.path.dirname(__file__), "Resources", "icons", "HatchGenerator.svg")
    
    def Initialize(self):
        """This is called when the workbench is first activated"""
        # Import the commands
        import HatchGenerator
        
        # Define toolbar
        self.appendToolbar("Hatch Tools", ["BIM_Hatch_Dialog"])
        
        # Define menu
        self.appendMenu("Hatch", ["BIM_Hatch_Dialog"])
        
        FreeCAD.Console.PrintMessage("Hatch Generator Workbench initialized\n")
    
    def Activated(self):
        """Called when workbench is activated"""
        FreeCAD.Console.PrintMessage("Hatch Generator Workbench activated\n")
        return
    
    def Deactivated(self):
        """Called when workbench is deactivated"""
        return
    
    def GetClassName(self):
        return "Gui::PythonWorkbench"


# ============================================================================
# Add the command to other workbenches (BIM, Draft, Part, etc.)
# ============================================================================
def addCommandToOtherWorkbenches():
    """
    Inject the Hatch Generator command into existing workbenches
    so users don't have to switch workbenches to use it.
    """
    try:
        # Import the module to ensure command is registered
        import HatchGenerator
        
        # List of workbenches to inject into
        target_workbenches = [
            "BIMWorkbench",
            "DraftWorkbench", 
            "PartWorkbench",
            "ArchWorkbench",
            "CompleteWorkbench"
        ]
        
        # Try to add to each workbench if it exists
        for wb_name in target_workbenches:
            try:
                wb = FreeCADGui.getWorkbench(wb_name)
                if wb:
                    # Check if command already in toolbar
                    if hasattr(wb, 'listToolbars'):
                        toolbars = wb.listToolbars()
                        # Try to find a suitable toolbar to add to
                        target_toolbar = None
                        for toolbar_name in toolbars:
                            if any(name in toolbar_name.lower() for name in ['draft', 'modify', 'tools', 'bim']):
                                target_toolbar = toolbar_name
                                break
                        
                        if target_toolbar:
                            FreeCADGui.addCommandToToolbar('BIM_Hatch_Dialog', target_toolbar, wb_name)
                            FreeCAD.Console.PrintMessage(f"Added Hatch Generator to {wb_name} toolbar\n")
            except:
                pass  # Workbench might not be loaded yet
                
    except Exception as e:
        FreeCAD.Console.PrintWarning(f"Could not inject command into workbenches: {str(e)}\n")


# ============================================================================
# Initialization when FreeCAD starts
# ============================================================================
# Register the workbench
FreeCADGui.addWorkbench(HatchGeneratorWorkbench)

# Also try to add the command to existing workbenches
# Use a timer to delay injection until workbenches are fully loaded
from PySide import QtCore
QtCore.QTimer.singleShot(1000, addCommandToOtherWorkbenches)

FreeCAD.Console.PrintMessage("Hatch Generator GUI integration complete\n")
FreeCAD.Console.PrintMessage("Access via: Workbench dropdown → Hatch Generator\n")
FreeCAD.Console.PrintMessage("Or find 'Create Hatch' in BIM/Draft/Part toolbars\n")