# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
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
# ***************************************************************************/

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
        import CommandCreateAssembly, CommandInsertLink, CommandCreateJoint
        from Preferences import PreferencesPage

        # from Preferences import preferences

        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addIconPath(":/icons")

        FreeCADGui.addPreferencePage(PreferencesPage, QT_TRANSLATE_NOOP("QObject", "Assembly"))

        # build commands list
        cmdlist = ["Assembly_CreateAssembly", "Assembly_InsertLink"]
        cmdListJoints = [
            "Assembly_CreateJointFixed",
            "Assembly_CreateJointRevolute",
            "Assembly_CreateJointCylindrical",
            "Assembly_CreateJointSlider",
            "Assembly_CreateJointBall",
            "Assembly_CreateJointPlanar",
            "Assembly_CreateJointParallel",
            "Assembly_CreateJointTangent",
        ]

        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Assembly"), cmdlist)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Assembly Joints"), cmdListJoints)

        self.appendMenu(
            [QT_TRANSLATE_NOOP("Workbench", "&Assembly")],
            cmdlist + ["Separator"] + cmdListJoints,
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
