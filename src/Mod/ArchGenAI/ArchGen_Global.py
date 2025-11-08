"""
Global ArchGen AI integration - called from C++ on startup
Provides global AI toolbar and panel that's available in all workbenches.
"""

import FreeCAD
import FreeCADGui as Gui
import sys
import os

# Add module path
module_path = os.path.dirname(__file__)
if module_path not in sys.path:
    sys.path.insert(0, module_path)


def initialize_ai_toolbar():
    """
    Initialize the global ArchGen AI toolbar
    This function is called from C++ during application startup
    """
    try:
        from PySide2 import QtCore, QtWidgets
        from ArchGen_Command import (
            ArchGen_Generate_Command,
            ArchGen_Settings_Command,
            ArchGen_About_Command
        )
        from ArchGen_UI import ArchGenAIPanel

        # Register all AI commands globally
        Gui.addCommand('ArchGen_Generate', ArchGen_Generate_Command())
        Gui.addCommand('ArchGen_Settings', ArchGen_Settings_Command())
        Gui.addCommand('ArchGen_About', ArchGen_About_Command())

        # Get main window
        mw = Gui.getMainWindow()
        if not mw:
            FreeCAD.Console.PrintWarning("Main window not available for ArchGen AI integration\n")
            return

        # Check if toolbar already exists
        existing_toolbar = mw.findChild(QtWidgets.QToolBar, "ArchGenAIToolbar")
        if existing_toolbar:
            FreeCAD.Console.PrintMessage("ArchGen AI toolbar already exists\n")
            return

        # Create new global toolbar
        ai_toolbar = mw.addToolBar("ArchGen AI")
        ai_toolbar.setObjectName("ArchGenAIToolbar")
        ai_toolbar.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)

        # Add AI generation button to toolbar
        ai_toolbar.addAction(Gui.getCommandManager().getAction("ArchGen_Generate"))

        # Create and add the AI panel as a dockable widget
        ai_dock = QtWidgets.QDockWidget("ArchGen AI Assistant", mw)
        ai_dock.setObjectName("ArchGenAIDock")

        try:
            ai_panel = ArchGenAIPanel()
            ai_dock.setWidget(ai_panel)

            # Add dock to right side by default
            mw.addDockWidget(QtCore.Qt.RightDockWidgetArea, ai_dock)

            # Hide by default - user can show via toolbar button
            ai_dock.hide()

            FreeCAD.Console.PrintMessage("ArchGen AI toolbar and panel initialized successfully\n")

        except Exception as panel_error:
            FreeCAD.Console.PrintWarning(f"Could not create AI panel: {panel_error}\n")
            # Continue without panel - toolbar will still work

    except ImportError as e:
        FreeCAD.Console.PrintWarning(f"ArchGen AI requires PySide2: {e}\n")
    except Exception as e:
        FreeCAD.Console.PrintError(f"Error initializing ArchGen AI: {str(e)}\n")
        import traceback
        traceback.print_exc()


def show_ai_panel():
    """Show the AI assistant panel"""
    try:
        from PySide2 import QtWidgets

        mw = Gui.getMainWindow()
        if not mw:
            return

        ai_dock = mw.findChild(QtWidgets.QDockWidget, "ArchGenAIDock")
        if ai_dock:
            ai_dock.show()
            ai_dock.raise_()
            ai_dock.activateWindow()

            # Focus on the prompt input
            ai_panel = ai_dock.widget()
            if hasattr(ai_panel, 'prompt_text'):
                ai_panel.prompt_text.setFocus()
        else:
            FreeCAD.Console.PrintWarning("ArchGen AI panel not found\n")

    except Exception as e:
        FreeCAD.Console.PrintError(f"Error showing AI panel: {str(e)}\n")


# Initialize on module import (when called from C++)
if __name__ != "__main__":
    # Auto-initialize when imported
    try:
        # Delay initialization slightly to ensure GUI is ready
        from PySide2 import QtCore

        def delayed_init():
            initialize_ai_toolbar()

        QtCore.QTimer.singleShot(100, delayed_init)
    except:
        # Fallback: initialize immediately
        initialize_ai_toolbar()
