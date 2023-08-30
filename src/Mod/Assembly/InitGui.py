# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2023 Ondsel <development@ondsel.com>                    *
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

import Assembly_rc

class AssemblyCommandGroup:
    def __init__(self, cmdlist, menu, tooltip=None):
        self.cmdlist = cmdlist
        self.menu = menu
        if tooltip is None:
            self.tooltip = menu
        else:
            self.tooltip = tooltip

    def GetCommands(self):
        return tuple(self.cmdlist)

    def GetResources(self):
        return {"MenuText": self.menu, "ToolTip": self.tooltip}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            return True
        return False


class AssemblyWorkbench(Workbench):
    "Assembly workbench"

    def __init__(self):
        print("Loading Assembly workbench...")
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Assembly/Resources/icons/AssemblyWorkbench.svg"
        )
        self.__class__.MenuText = "Assembly"
        self.__class__.ToolTip = "Assembly workbench"

    def Initialize(self):
        print("Initializing Assembly workbench...")
        global AssemblyCommandGroup


        translate = FreeCAD.Qt.translate

        # load the builtin modules
        from PySide import QtCore, QtGui
        from PySide.QtCore import QT_TRANSLATE_NOOP
        import Commands
        from Preferences import PreferencesPage
        # from Preferences import preferences

        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addIconPath(":/icons")

        FreeCADGui.addPreferencePage(PreferencesPage, QT_TRANSLATE_NOOP("QObject", "Assembly"))

        # build commands list
        cmdlist = ["Assembly_CreateAssembly", "Assembly_InsertLink"]

        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Assembly"), cmdlist)

        self.appendMenu(
            [QT_TRANSLATE_NOOP("Workbench", "&Assembly")],
            cmdlist
            + ["Separator"],
        )


        print("Assembly workbench loaded")

    def Activated(self):
        # update the translation engine
        FreeCADGui.updateLocale()

    def Deactivated(self):
        pass

    def ContextMenu(self, recipient):
        pass


Gui.addWorkbench(AssemblyWorkbench())

# FreeCAD.addImportType()
