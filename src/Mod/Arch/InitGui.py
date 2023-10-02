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

"""Initialization of the Arch workbench (graphical interface)."""

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
        import Arch_rc
        import Arch

        # Load Reinforcement WB translations
        try:
            import RebarTools
            RebarTools.load_translations()
        except Exception:
            pass

        from ArchStructure import _ArchStructureGroupCommand
        from ArchAxis import _ArchAxisGroupCommand
        from ArchPanel import CommandPanelGroup
        from ArchMaterial import _ArchMaterialToolsCommand
        from ArchPipe import _ArchPipeGroupCommand

        stru_group = _ArchStructureGroupCommand
        axis_group = _ArchAxisGroupCommand
        pan_group  = CommandPanelGroup
        mat_group  = _ArchMaterialToolsCommand
        pipe_group = _ArchPipeGroupCommand

        # Set up command lists
        self.archtools = ["Arch_Wall",
                          ([QT_TRANSLATE_NOOP("Workbench", "Structure tools")],
                              list(stru_group.GetCommands(stru_group))), # tuple len=2: submenu
                          ("Arch_StructureTools", ),                     # tuple len=1: toolbar flyout
                          "Arch_Rebar_Submenu",      # will be replaced or removed
                          "Arch_Rebar",              # may be replaced
                          "Arch_CurtainWall",
                          "Arch_BuildingPart",
                          "Arch_Project",
                          "Arch_Site",
                          "Arch_Building",
                          "Arch_Floor",
                          "Arch_Reference",
                          "Arch_Window",
                          "Arch_Roof",
                          ([QT_TRANSLATE_NOOP("Workbench", "Axis tools")],
                              list(axis_group.GetCommands(axis_group))),
                          ("Arch_AxisTools", ),
                          "Arch_SectionPlane",
                          "Arch_Space",
                          "Arch_Stairs",
                          ([QT_TRANSLATE_NOOP("Workbench", "Panel tools")],
                              list(pan_group.GetCommands(pan_group))),
                          ("Arch_PanelTools", ),
                          "Arch_Equipment",
                          "Arch_Frame",
                          "Arch_Fence",
                          "Arch_Truss",
                          "Arch_Profile",
                          ([QT_TRANSLATE_NOOP("Workbench", "Material tools")],
                              list(mat_group.GetCommands(mat_group))),
                          ("Arch_MaterialTools", ),
                          "Arch_Schedule",
                          ([QT_TRANSLATE_NOOP("Workbench", "Pipe tools")],
                              list(pipe_group.GetCommands(pipe_group))),
                          ("Arch_PipeTools", ),
                          "Arch_CutPlane",
                          "Arch_CutLine",
                          "Arch_Add",
                          "Arch_Remove",
                          "Arch_Survey"]

        self.utilities = ["Arch_Component",
                          "Arch_CloneComponent",
                          "Arch_SplitMesh",
                          "Arch_MeshToShape",
                          "Arch_SelectNonSolidMeshes",
                          "Arch_RemoveShape",
                          "Arch_CloseHoles",
                          "Arch_MergeWalls",
                          "Arch_Check",
                          "Arch_ToggleIfcBrepFlag",
                          "Arch_3Views",
                          "Arch_IfcSpreadsheet",
                          "Arch_ToggleSubs"]

        # Add the rebar tools from the Reinforcement addon, if available
        try:
            import RebarTools
        except Exception:
            del self.archtools[3] # remove "Arch_Rebar_Submenu"
        else:
            class RebarGroupCommand:
                def GetCommands(self):
                    return tuple(RebarTools.RebarCommands + ["Arch_Rebar"])

                def GetResources(self):
                    return {'MenuText': QT_TRANSLATE_NOOP("Arch_RebarTools", "Rebar tools"),
                            'ToolTip': QT_TRANSLATE_NOOP("Arch_RebarTools",
                                                         "Create various types of rebars, "
                                                         "including U-shaped, L-shaped, and stirrup")}

                def IsActive(self):
                    return not FreeCAD.ActiveDocument is None
            FreeCADGui.addCommand('Arch_RebarTools', RebarGroupCommand())
            self.archtools[3] = ([QT_TRANSLATE_NOOP("Workbench", "Rebar tools")],
                                    RebarTools.RebarCommands + ["Arch_Rebar"])
            self.archtools[4] = ("Arch_RebarTools", )

        # Set up Draft command lists
        import draftutils.init_tools as it
        self.draft_drawing_commands = it.get_draft_drawing_commands()
        self.draft_annotation_commands = it.get_draft_annotation_commands()
        self.draft_modification_commands = it.get_draft_modification_commands()
        self.draft_utility_commands = it.get_draft_utility_commands_menu()
        self.draft_context_commands = it.get_draft_context_commands()
        self.draft_snap_commands = it.get_draft_snap_commands()

        # Set up toolbars
        it.init_toolbar(self,
                        QT_TRANSLATE_NOOP("Workbench", "Arch tools"),
                        self.archtools)
        it.init_toolbar(self,
                        QT_TRANSLATE_NOOP("Workbench", "Draft creation tools"),
                        self.draft_drawing_commands)
        it.init_toolbar(self,
                        QT_TRANSLATE_NOOP("Workbench", "Draft annotation tools"),
                        self.draft_annotation_commands)
        it.init_toolbar(self,
                        QT_TRANSLATE_NOOP("Workbench", "Draft modification tools"),
                        self.draft_modification_commands)
        it.init_toolbar(self,
                        QT_TRANSLATE_NOOP("Workbench", "Draft snap"),
                        self.draft_snap_commands)

        # Set up menus
        it.init_menu(self,
                     [QT_TRANSLATE_NOOP("Workbench", "&Arch"),
                      QT_TRANSLATE_NOOP("Workbench", "Utilities")],
                     self.utilities)
        it.init_menu(self,
                     [QT_TRANSLATE_NOOP("Workbench", "&Arch")],
                     self.archtools)
        it.init_menu(self,
                     [QT_TRANSLATE_NOOP("Workbench", "&Draft"),
                      QT_TRANSLATE_NOOP("Workbench", "Creation")],
                     self.draft_drawing_commands)
        it.init_menu(self,
                     [QT_TRANSLATE_NOOP("Workbench", "&Draft"),
                      QT_TRANSLATE_NOOP("Workbench", "Annotation")],
                     self.draft_annotation_commands)
        it.init_menu(self,
                     [QT_TRANSLATE_NOOP("Workbench", "&Draft"),
                      QT_TRANSLATE_NOOP("Workbench", "Modification")],
                     self.draft_modification_commands)
        it.init_menu(self,
                     [QT_TRANSLATE_NOOP("Workbench", "&Draft"),
                      QT_TRANSLATE_NOOP("Workbench", "Utilities")],
                     self.draft_utility_commands)

        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")

        # Set up preferences pages
        if hasattr(FreeCADGui, "draftToolBar"):
            if not hasattr(FreeCADGui.draftToolBar, "loadedArchPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-arch.ui", QT_TRANSLATE_NOOP("QObject", "Arch"))
                FreeCADGui.addPreferencePage(":/ui/preferences-archdefaults.ui", QT_TRANSLATE_NOOP("QObject", "Arch"))
                FreeCADGui.draftToolBar.loadedArchPreferences = True
            if not hasattr(FreeCADGui.draftToolBar, "loadedPreferences"):
                FreeCADGui.addPreferencePage(":/ui/preferences-draft.ui", QT_TRANSLATE_NOOP("QObject", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-draftsnap.ui", QT_TRANSLATE_NOOP("QObject", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-draftvisual.ui", QT_TRANSLATE_NOOP("QObject", "Draft"))
                FreeCADGui.addPreferencePage(":/ui/preferences-drafttexts.ui", QT_TRANSLATE_NOOP("QObject", "Draft"))
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
FreeCADGui.addPreferencePage(":/ui/preferences-ifc.ui", QT_TRANSLATE_NOOP("QObject", "Import-Export"))
FreeCADGui.addPreferencePage(":/ui/preferences-ifc-export.ui", QT_TRANSLATE_NOOP("QObject", "Import-Export"))
FreeCADGui.addPreferencePage(":/ui/preferences-dae.ui", QT_TRANSLATE_NOOP("QObject", "Import-Export"))

FreeCAD.__unit_test__ += ["TestArch"]
