"""Initialization of the Draft workbench (graphical interface)."""
# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import os
import FreeCAD
import FreeCADGui

__title__ = "FreeCAD Draft Workbench - Init file"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "https://www.freecadweb.org"


class DraftWorkbench(FreeCADGui.Workbench):
    """The Draft Workbench definition."""

    def __init__(self):
        def QT_TRANSLATE_NOOP(context, text):
            return text

        __dirname__ = os.path.join(FreeCAD.getResourceDir(), "Mod", "Draft")
        _tooltip = "The Draft workbench is used for 2D drafting on a grid"
        self.__class__.Icon = os.path.join(__dirname__,
                                           "Resources", "icons",
                                           "DraftWorkbench.svg")
        self.__class__.MenuText = QT_TRANSLATE_NOOP("draft", "Draft")
        self.__class__.ToolTip = QT_TRANSLATE_NOOP("draft", _tooltip)

    def Initialize(self):
        """When the workbench is first loaded."""

        def QT_TRANSLATE_NOOP(context, text):
            return text

        # Run self-tests
        dependencies_OK = False
        try:
            from pivy import coin
            if FreeCADGui.getSoDBVersion() != coin.SoDB.getVersion():
                raise AssertionError("FreeCAD and Pivy use different versions "
                                     "of Coin. "
                                     "This will lead to unexpected behaviour.")
        except AssertionError:
            FreeCAD.Console.PrintWarning("Error: FreeCAD and Pivy "
                                         "use different versions of Coin. "
                                         "This will lead to unexpected "
                                         "behaviour.\n")
        except ImportError:
            FreeCAD.Console.PrintWarning("Error: Pivy not found, "
                                         "Draft Workbench will be disabled.\n")
        except Exception:
            FreeCAD.Console.PrintWarning("Error: Unknown error "
                                         "while trying to load Pivy.\n")
        else:
            dependencies_OK = True

        if not dependencies_OK:
            return

        # Import Draft tools, icons
        try:
            import Draft_rc
            import DraftTools
            import DraftGui
            import DraftFillet
            from draftguitools import gui_circulararray
            from draftguitools import gui_polararray
            from draftguitools import gui_orthoarray
            from draftguitools import gui_arrays
            FreeCADGui.addLanguagePath(":/translations")
            FreeCADGui.addIconPath(":/icons")
        except Exception as exc:
            FreeCAD.Console.PrintError(exc)
            FreeCAD.Console.PrintError("Error: Initializing one or more "
                                       "of the Draft modules failed, "
                                       "Draft will not work as expected.\n")

        # Set up command lists
        import draftutils.init_tools as it
        self.drawing_commands = it.get_draft_drawing_commands()
        self.annotation_commands = it.get_draft_annotation_commands()
        self.modification_commands = it.get_draft_modification_commands()
        self.context_commands = it.get_draft_context_commands()
        self.line_commands = it.get_draft_line_commands()
        self.utility_commands = it.get_draft_utility_commands()

        # Set up toolbars
        self.appendToolbar(QT_TRANSLATE_NOOP("Draft", "Draft creation tools"), self.drawing_commands)
        self.appendToolbar(QT_TRANSLATE_NOOP("Draft", "Draft annotation tools"), self.annotation_commands)
        self.appendToolbar(QT_TRANSLATE_NOOP("Draft", "Draft modification tools"), self.modification_commands)

        # Set up menus
        self.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Drafting"), self.drawing_commands)
        self.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Annotation"), self.annotation_commands)
        self.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Modification"), self.modification_commands)
        self.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Utilities"), self.utility_commands + self.context_commands)

        # Set up preferences pages
        if hasattr(FreeCADGui, "draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar, "loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-draft.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-draftsnap.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-draftvisual.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-drafttexts.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.draftToolBar.loadedPreferences = True

        FreeCAD.Console.PrintLog('Loading Draft workbench, done.\n')

    def Activated(self):
        """When entering the workbench."""
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.show()
        FreeCAD.Console.PrintLog("Draft workbench activated.\n")

    def Deactivated(self):
        """When quitting the workbench."""
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.hide()
        FreeCAD.Console.PrintLog("Draft workbench deactivated.\n")

    def ContextMenu(self, recipient):
        """Define an optional custom context menu."""
        from DraftGui import translate
        if recipient == "View":
            if FreeCAD.activeDraftCommand is None:
                if FreeCADGui.Selection.getSelection():
                    self.appendContextMenu("Draft", self.drawing_commands + self.modification_commands)
                    self.appendContextMenu("Utilities", self.context_commands)
                else:
                    self.appendContextMenu("Draft", self.drawing_commands)
            else:
                if FreeCAD.activeDraftCommand.featureName == translate("draft","Line"):
                    # BUG: line subcommands are not usable while another command is active
                    self.appendContextMenu("", self.line_commands)
        else:
            if FreeCADGui.Selection.getSelection():
                self.appendContextMenu("Utilities", self.context_commands)

    def GetClassName(self):
        """Type of workbench."""
        return "Gui::PythonWorkbench"


FreeCADGui.addWorkbench(DraftWorkbench)

# Preference pages for importing and exporting various file formats
# are independent of the loading of the workbench and can be loaded at startup
import Draft_rc
from PySide.QtCore import QT_TRANSLATE_NOOP
FreeCADGui.addPreferencePage(":/ui/preferences-dxf.ui", QT_TRANSLATE_NOOP("Draft", "Import-Export"))
FreeCADGui.addPreferencePage(":/ui/preferences-dwg.ui", QT_TRANSLATE_NOOP("Draft", "Import-Export"))
FreeCADGui.addPreferencePage(":/ui/preferences-svg.ui", QT_TRANSLATE_NOOP("Draft", "Import-Export"))
FreeCADGui.addPreferencePage(":/ui/preferences-oca.ui", QT_TRANSLATE_NOOP("Draft", "Import-Export"))

FreeCAD.__unit_test__ += ["TestDraft"]
