"""
ArchGen AI GUI initialization
Registers the ArchGen AI workbench and global toolbar integration.
"""

import FreeCAD
import FreeCADGui as Gui


class ArchGenAIWorkbench(Gui.Workbench):
    """ArchGen AI Workbench - Optional workbench for AI features"""

    MenuText = "ArchGen AI"
    ToolTip = "AI-powered CAD generation and assistance"
    Icon = """
    /* XPM */
    static const char *archgen_icon[] = {
    "16 16 3 1",
    "   c None",
    ".  c #4a90e2",
    "+  c #ffffff",
    "                ",
    "   ........     ",
    "  ..........    ",
    " ....++++....   ",
    " ...+    +...   ",
    "....+    +....  ",
    "....+ ++ +....  ",
    "....+ ++ +....  ",
    "....+ ++ +....  ",
    "....+ ++ +....  ",
    "....+    +....  ",
    " ...+    +...   ",
    " ....++++....   ",
    "  ..........    ",
    "   ........     ",
    "                "
    };
    """

    def Initialize(self):
        """Initialize the workbench - commands are registered globally"""
        import ArchGen_Command

        # Commands are already registered globally
        self.appendToolbar("ArchGen AI", ["ArchGen_Generate"])
        self.appendMenu("&ArchGen AI", ["ArchGen_Generate", "ArchGen_Settings", "ArchGen_About"])

    def Activated(self):
        """Called when workbench is activated"""
        FreeCAD.Console.PrintMessage("ArchGen AI workbench activated\n")

    def Deactivated(self):
        """Called when workbench is deactivated"""
        pass

    def GetClassName(self):
        return "Gui::PythonWorkbench"


# Register the workbench
Gui.addWorkbench(ArchGenAIWorkbench())

print("ArchGen AI workbench registered successfully")
