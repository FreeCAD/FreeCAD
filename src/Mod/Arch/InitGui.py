"""Initialization of the Arch workbench (graphical interface)."""
# ***************************************************************************
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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


class ArchWorkbench(FreeCADGui.Workbench):
    """The Arch workbench definition."""

    def __init__(self):
        def QT_TRANSLATE_NOOP(context, text):
            return text

        __dirname__ = os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch")
        _tooltip = ("The Arch workbench is used to model "
                    "architectural components, and entire buildings")
        self.__class__.Icon = os.path.join(__dirname__,
                                           "Resources", "icons",
                                           "ArchWorkbench.svg")
        self.__class__.MenuText = QT_TRANSLATE_NOOP("Arch", "Arch")
        self.__class__.ToolTip = QT_TRANSLATE_NOOP("Arch", _tooltip)

    def Initialize(self):
        """When the workbench is first loaded."""

        def QT_TRANSLATE_NOOP(context, text):
            return text

        import Draft_rc
        import DraftTools
        import DraftGui
        from draftguitools import gui_circulararray
        from draftguitools import gui_polararray
        from draftguitools import gui_orthoarray
        from draftguitools import gui_arrays
        import Arch_rc
        import Arch

        # Set up command lists
        self.archtools = ["Arch_Wall", "Arch_Structure", "Arch_Rebar",
                          "Arch_BuildingPart",
                          "Arch_Project", "Arch_Site", "Arch_Building",
                          "Arch_Floor", "Arch_Reference",
                          "Arch_Window", "Arch_Roof", "Arch_AxisTools",
                          "Arch_SectionPlane", "Arch_Space", "Arch_Stairs",
                          "Arch_PanelTools", "Arch_Equipment",
                          "Arch_Frame", "Arch_Fence", "Arch_MaterialTools",
                          "Arch_Schedule", "Arch_PipeTools",
                          "Arch_CutPlane", "Arch_CutLine",
                          "Arch_Add", "Arch_Remove", "Arch_Survey"]
        self.utilities = ["Arch_Component", "Arch_CloneComponent",
                          "Arch_SplitMesh", "Arch_MeshToShape",
                          "Arch_SelectNonSolidMeshes", "Arch_RemoveShape",
                          "Arch_CloseHoles", "Arch_MergeWalls", "Arch_Check",
                          "Arch_ToggleIfcBrepFlag", "Arch_3Views",
                          "Arch_IfcSpreadsheet", "Arch_ToggleSubs"]

        # Add the rebar tools from the Reinforcement addon, if available
        try:
            import RebarTools
        except Exception:
            pass
        else:
            class RebarGroupCommand:
                def GetCommands(self):
                    return tuple(RebarTools.RebarCommands + ["Arch_Rebar"])

                def GetResources(self):
                    _tooltip = ("Create various types of rebars, "
                                "including U-shaped, L-shaped, and stirrup")
                    return {'MenuText': QT_TRANSLATE_NOOP("Arch", 'Rebar tools'),
                            'ToolTip': QT_TRANSLATE_NOOP("Arch", _tooltip)}

                def IsActive(self):
                    return not FreeCAD.ActiveDocument is None
            FreeCADGui.addCommand('Arch_RebarTools', RebarGroupCommand())
            self.archtools[2] = "Arch_RebarTools"

        # Set up Draft command lists
        import draftutils.init_tools as it
        self.draft_drawing_commands = it.get_draft_drawing_commands()
        self.draft_annotation_commands = it.get_draft_annotation_commands()
        self.draft_modification_commands = it.get_draft_modification_commands()
        self.draft_context_commands = it.get_draft_context_commands()
        self.draft_line_commands = it.get_draft_line_commands()
        self.draft_utility_commands = it.get_draft_utility_commands()

        # Set up toolbars
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Arch tools"), self.archtools)
        self.appendToolbar(QT_TRANSLATE_NOOP("Draft", "Draft creation tools"), self.draft_drawing_commands)
        self.appendToolbar(QT_TRANSLATE_NOOP("Draft", "Draft annotation tools"), self.draft_annotation_commands)
        self.appendToolbar(QT_TRANSLATE_NOOP("Draft", "Draft modification tools"), self.draft_modification_commands)

        # Set up menus
        self.appendMenu([QT_TRANSLATE_NOOP("arch", "&Arch"),
                         QT_TRANSLATE_NOOP("arch", "Utilities")],
                        self.utilities)
        self.appendMenu(QT_TRANSLATE_NOOP("arch", "&Arch"), self.archtools)

        self.appendMenu([QT_TRANSLATE_NOOP("arch", "&Draft"),
                         QT_TRANSLATE_NOOP("arch", "Creation")],
                        self.draft_drawing_commands)
        self.appendMenu([QT_TRANSLATE_NOOP("arch", "&Draft"),
                         QT_TRANSLATE_NOOP("arch", "Annotation")],
                        self.draft_annotation_commands)
        self.appendMenu([QT_TRANSLATE_NOOP("arch", "&Draft"),
                         QT_TRANSLATE_NOOP("arch", "Modification")],
                        self.draft_modification_commands)
        self.appendMenu([QT_TRANSLATE_NOOP("arch", "&Draft"),
                         QT_TRANSLATE_NOOP("arch", "Utilities")],
                        self.draft_utility_commands
                        + self.draft_context_commands)
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")

        # Set up preferences pages
        if hasattr(FreeCADGui, "draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar, "loadedArchPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-arch.ui", QT_TRANSLATE_NOOP("Arch", "Arch"))
                FreeCADGui.addPreferencePage(":/ui/preferences-archdefaults.ui", QT_TRANSLATE_NOOP("Arch", "Arch"))
                FreeCADGui.draftToolBar.loadedArchPreferences = True
            if not hasattr(FreeCADGui.draftToolBar, "loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-draft.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-draftsnap.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-draftvisual.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-drafttexts.ui", QT_TRANSLATE_NOOP("Draft", "Draft"))
                FreeCADGui.draftToolBar.loadedPreferences = True

        FreeCAD.Console.PrintLog('Loading Arch workbench, done.\n')

    def Activated(self):
        """When entering the workbench."""
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.Activated()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.show()
        FreeCAD.Console.PrintLog("Arch workbench activated.\n")

    def Deactivated(self):
        """When leaving the workbench."""
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.Deactivated()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.hide()
        FreeCAD.Console.PrintLog("Arch workbench deactivated.\n")

    def ContextMenu(self, recipient):
        """Define an optional custom context menu."""
        self.appendContextMenu("Utilities", self.draft_context_commands)

    def GetClassName(self):
        """Type of workbench."""
        return "Gui::PythonWorkbench"


FreeCADGui.addWorkbench(ArchWorkbench)

# Preference pages for importing and exporting various file formats
# are independent of the loading of the workbench and can be loaded at startup
import Arch_rc
from PySide.QtCore import QT_TRANSLATE_NOOP
FreeCADGui.addPreferencePage(":/ui/preferences-ifc.ui", QT_TRANSLATE_NOOP("Draft", "Import-Export"))
FreeCADGui.addPreferencePage(":/ui/preferences-dae.ui", QT_TRANSLATE_NOOP("Draft", "Import-Export"))

FreeCAD.__unit_test__ += ["TestArch"]
