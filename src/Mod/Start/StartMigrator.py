# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2024 The FreeCAD Project Association AISBL               *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import FreeCAD


def _remove_from_list(prefs, pref_name):
    # Remove Start and Web from a preference that consists of a comma-separated list of workbenches
    mods = prefs.GetString(pref_name, "").split(",")
    if "StartWorkbench" in mods:
        mods.remove("StartWorkbench")
    if "WebWorkbench" in mods:
        mods.remove("WebWorkbench")
    prefs.SetString(pref_name, ",".join(mods))


class StartMigrator2024:
    """In April 2024 the old Start workbench was retired, and replaced with the current Start command. This function
    cleans up references to the old workbench, migrating the settings where appropriate, and deleting them where they
    are no longer relevant. Web was also removed, without replacement.
    TODO: Remove the 2024 migration code when enough time has passed that it is no longer useful"""

    def __init__(self):
        self.completion_indicator = "Migration2024Complete"
        self.start_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start")
        self.general_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        self.migration_2024_complete = self.start_prefs.GetBool(self.completion_indicator, False)

    def run_migration(self):
        if self.migration_2024_complete:
            # Refuse to run if it's already been done
            return
        FreeCAD.Console.PrintMessage("Migrating Start Workbench to Start command... ")
        self._update_startup_flags()
        self._remove_wb_references()
        self._remove_commands()
        self._remove_toolbars()
        self._remove_deprecated_parameters()
        self._mark_complete()
        FreeCAD.Console.PrintMessage("done.\n")

    # If the old Start workbench was set as the Autoload Module, reconfigure it so the Start command is run at startup,
    # and set the Autoload module to "PartDesignWorkbench"
    def _update_startup_flags(self):
        autoload_module = self.general_prefs.GetString("AutoloadModule", "StartWorkbench")
        if autoload_module == "StartWorkbench":
            self.start_prefs.SetBool("ShowOnStartup", True)
            self.general_prefs.SetString("AutoloadModule", "PartDesignWorkbench")
        else:
            self.start_prefs.SetBool("ShowOnStartup", False)

    # Remove all references to the StartWorkbench in the various workbench lists
    def _remove_wb_references(self):
        general_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        last_module = general_prefs.GetString("LastModule", "")
        if last_module in ["StartWorkbench", "WebWorkbench"]:
            general_prefs.RemString("LastModule")
        _remove_from_list(general_prefs, "BackgroundAutoloadModules")
        treeview_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Workbenches")
        _remove_from_list(treeview_prefs, "Ordered")
        _remove_from_list(treeview_prefs, "Disabled")

    # Remove any commands that begin with "Start_" or "Web_" (except "Start_Start", the current command)
    def _remove_commands(self):
        command_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Commands")
        commands = command_prefs.GetStrings()
        for command in commands:
            if command.startswith("Web_") or (
                command.startswith("Start_") and command != "Start_Start"
            ):
                command_prefs.RemString(command)

    # Remove any persistent toolbars
    def _remove_toolbars(self):
        tux_prefs = FreeCAD.ParamGet("User parameter:Tux/PersistentToolbars/User")
        groups = tux_prefs.GetGroups()
        if "StartWorkbench" in groups:
            tux_prefs.RemGroup("StartWorkbench")
        if "WebWorkbench" in groups:
            tux_prefs.RemGroup("WebWorkbench")

    # Delete old Start preferences
    def _remove_deprecated_parameters(self):
        show_on_startup = self.start_prefs.GetBool("ShowOnStartup", True)
        show_examples = self.start_prefs.GetBool("ShowExamples", True)
        close_start = self.start_prefs.GetBool("closeStart", False)
        custom_folder = self.start_prefs.GetString("ShowCustomFolder", "")
        self.start_prefs.Clear()
        self.start_prefs.SetBool("ShowOnStartup", show_on_startup)
        self.start_prefs.SetBool("ShowExamples", show_examples)
        self.start_prefs.SetBool("CloseStart", close_start)
        # ShowCustomFolder renamed to CustomFolder in 1.1
        self.start_prefs.SetString("CustomFolder", custom_folder)

    # Indicate that this migration has been run
    def _mark_complete(self):
        self.start_prefs.SetBool(self.completion_indicator, True)
