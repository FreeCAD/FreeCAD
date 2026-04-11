# InitGui.py - GUI integration for Hatch Generator
# This makes the tool available in BIM, Draft, and other workbenches

import FreeCAD
import FreeCADGui
import os as _os
import sys as _sys

# ============================================================================
# PATCH: Robust module directory detection using a helper function
# FreeCAD addon loader does not set __file__, causing NameError in os.path.
# We scan sys.path and standard Mod directories for the HatchGenerator module.
# ============================================================================

def _get_hatch_mod_dir():
    """Locate the HatchGenerator addon directory in any FreeCAD load context."""
    # Standard Python context: __file__ is defined
    try:
        return _os.path.dirname(_os.path.abspath(__file__))
    except NameError:
        pass
    
    # FreeCAD addon context: __file__ not defined — scan sys.path
    for _p in _sys.path:
        if _os.path.isfile(_os.path.join(_p, "HatchGenerator.py")):
            return _p
            
    # Fallback: check standard FreeCAD addon locations
    for _base in (
        _os.path.join(FreeCAD.getUserAppDataDir(), "Mod"),
        _os.path.join(FreeCAD.getResourceDir(), "Mod"),
    ):
        for _candidate_name in ("FreeCadHatch", "HatchGenerator", "hatch_generator"):
            _candidate = _os.path.join(_base, _candidate_name)
            if _os.path.isdir(_candidate):
                return _candidate
                
    return ""   # last resort — icon simply won't load, but no crash

_HATCH_MOD_DIR = _get_hatch_mod_dir()


class HatchGeneratorWorkbench(FreeCADGui.Workbench):
    """
    Workbench class for Hatch Generator (optional standalone workbench)
    """
    MenuText = "Hatch Generator"
    ToolTip = "Parametric hatch patterns for surfaces, walls, and roofs"
    # PATCH: Use the calculated _HATCH_MOD_DIR variable
    Icon = _os.path.join(_HATCH_MOD_DIR, "Resources", "icons", "HatchGenerator.svg")
    
    def Initialize(self):
        """This is called when the workbench is first activated"""
        # Import the commands (this registers them)
        import HatchGenerator
        
        # Define toolbar with both commands
        self.appendToolbar("Hatch Tools", ["BIM_Hatch_Dialog", "BIM_FaceExtractor"])
        
        # Define menu with both commands
        self.appendMenu("Hatch", ["BIM_Hatch_Dialog", "BIM_FaceExtractor"])
        
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
# Add the commands to other workbenches (BIM, Draft, Part, etc.)
# ============================================================================
def addCommandsToOtherWorkbenches():
    """
    Inject the Hatch Generator commands into existing workbenches
    so users don't have to switch workbenches to use them.
    """
    try:
        # Import the module to ensure commands are registered
        import HatchGenerator
        
        # List of workbenches to inject into
        target_workbenches = [
            "BIMWorkbench",
            "DraftWorkbench", 
            "PartWorkbench",
            "ArchWorkbench",
            "CompleteWorkbench"
        ]
        
        # List of commands to add
        commands = ["BIM_Hatch_Dialog", "BIM_FaceExtractor"]
        
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
                            for cmd in commands:
                                FreeCADGui.addCommandToToolbar(cmd, target_toolbar, wb_name)
                            FreeCAD.Console.PrintMessage(f"Added Hatch Generator commands to {wb_name} toolbar\n")
            except:
                pass  # Workbench might not be loaded yet
                
    except Exception as e:
        FreeCAD.Console.PrintWarning(f"Could not inject commands into workbenches: {str(e)}\n")


# ============================================================================
# Initialization when FreeCAD starts
# ============================================================================
# Register the workbench
FreeCADGui.addWorkbench(HatchGeneratorWorkbench)

# Also try to add the commands to existing workbenches
# Use a timer to delay injection until workbenches are fully loaded
from PySide import QtCore
QtCore.QTimer.singleShot(1000, addCommandsToOtherWorkbenches)

FreeCAD.Console.PrintMessage("Hatch Generator GUI integration complete\n")
FreeCAD.Console.PrintMessage("Access via: Workbench dropdown → Hatch Generator\n")
FreeCAD.Console.PrintMessage("Or find 'Create Hatch' and 'Extract Face' in BIM/Draft/Part toolbars\n")