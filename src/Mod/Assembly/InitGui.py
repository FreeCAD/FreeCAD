# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
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
# **************************************************************************/

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
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Assembly/Resources/icons/AssemblyWorkbench.svg"
        )
        self.__class__.MenuText = "Assembly"
        self.__class__.ToolTip = "Assembly workbench"

    def Initialize(self):
        global AssemblyCommandGroup

        translate = FreeCAD.Qt.translate

        # load the builtin modules
        from PySide import QtCore, QtGui
        from PySide.QtCore import QT_TRANSLATE_NOOP
        import CommandCreateAssembly, CommandInsertLink, CommandInsertNewPart, CommandCreateJoint, CommandSolveAssembly, CommandExportASMT, CommandCreateView, CommandCreateSimulation, CommandCreateBom
        import Preferences

        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addIconPath(":/icons")

        FreeCADGui.addPreferencePage(
            Preferences.PreferencesPage, QT_TRANSLATE_NOOP("QObject", "Assembly")
        )

        # build commands list
        cmdList = [
            "Assembly_CreateAssembly",
            "Assembly_Insert",
            "Assembly_SolveAssembly",
            "Assembly_CreateView",
            "Assembly_CreateSimulation",
            "Assembly_CreateBom",
        ]

        cmdListMenuOnly = [
            "Assembly_ExportASMT",
        ]

        cmdListJoints = [
            "Assembly_ToggleGrounded",
            "Separator",
            "Assembly_CreateJointFixed",
            "Assembly_CreateJointRevolute",
            "Assembly_CreateJointCylindrical",
            "Assembly_CreateJointSlider",
            "Assembly_CreateJointBall",
            "Separator",
            "Assembly_CreateJointDistance",
            "Assembly_CreateJointParallel",
            "Assembly_CreateJointPerpendicular",
            "Assembly_CreateJointAngle",
            "Separator",
            "Assembly_CreateJointRackPinion",
            "Assembly_CreateJointScrew",
            "Assembly_CreateJointGearBelt",
        ]

        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Assembly"), cmdList)
        self.appendToolbar(QT_TRANSLATE_NOOP("Workbench", "Assembly Joints"), cmdListJoints)

        self.appendMenu(
            [QT_TRANSLATE_NOOP("Workbench", "&Assembly")],
            cmdList + cmdListMenuOnly + ["Separator"] + cmdListJoints,
        )

    def Activated(self):
        # update the translation engine
        FreeCADGui.updateLocale()

        # Add task watchers to provide contextual tools in the task panel
        self.setWatchers()

    def Deactivated(self):
        FreeCADGui.Control.clearTaskWatcher()

    def ContextMenu(self, recipient):
        pass

    def setWatchers(self):
        import UtilsAssembly

        translate = FreeCAD.Qt.translate

        class AssemblyCreateWatcher:
            """Shows 'Create Assembly' when no assembly exists in the document."""

            def __init__(self):
                self.commands = ["Assembly_CreateAssembly"]
                self.title = translate("Assembly", "Create")

            def shouldShow(self):
                doc = FreeCAD.ActiveDocument

                if hasattr(doc, "RootObjects"):
                    for obj in doc.RootObjects:
                        if obj.isDerivedFrom("Assembly::AssemblyObject"):
                            return False
                return True

        class AssemblyActivateWatcher:
            """Shows 'Activate Assembly' when an assembly exists but is not active."""

            def __init__(self):
                self.commands = ["Assembly_ActivateAssembly"]
                self.title = translate("Assembly", "Activate")

            def shouldShow(self):
                doc = FreeCAD.ActiveDocument

                has_assembly = False
                if hasattr(doc, "RootObjects"):
                    for obj in doc.RootObjects:
                        if obj.isDerivedFrom("Assembly::AssemblyObject"):
                            has_assembly = True
                            break

                assembly = UtilsAssembly.activeAssembly()

                return has_assembly and (assembly is None or assembly.Document != doc)

        class AssemblyBaseWatcher:
            """Base class for watchers that require an active assembly."""

            def __init__(self):
                self.assembly = None

            def shouldShow(self):
                doc = FreeCAD.ActiveDocument

                self.assembly = UtilsAssembly.activeAssembly()
                return self.assembly is not None and self.assembly.Document == doc

        class AssemblyInsertWatcher(AssemblyBaseWatcher):
            """Shows 'Insert Component' when an assembly is active."""

            def __init__(self):
                super().__init__()
                self.commands = ["Assembly_Insert"]
                self.title = translate("Assembly", "Insert")

            def shouldShow(self):
                return super().shouldShow()

        class AssemblyGroundWatcher(AssemblyBaseWatcher):
            """Shows 'Ground' when the active assembly has no grounded parts."""

            def __init__(self):
                super().__init__()
                self.commands = ["Assembly_ToggleGrounded"]
                self.title = translate("Assembly", "Grounding")

            def shouldShow(self):
                if not super().shouldShow():
                    return False
                return (
                    UtilsAssembly.assembly_has_at_least_n_parts(1)
                    and not UtilsAssembly.isAssemblyGrounded()
                )

        class AssemblyJointsWatcher(AssemblyBaseWatcher):
            """Shows Joint, View, and BOM tools when there are enough parts."""

            def __init__(self):
                super().__init__()
                self.commands = [
                    "Assembly_CreateJointFixed",
                    "Assembly_CreateJointRevolute",
                    "Assembly_CreateJointCylindrical",
                    "Assembly_CreateJointSlider",
                    "Assembly_CreateJointBall",
                    "Separator",
                    "Assembly_CreateJointDistance",
                    "Assembly_CreateJointParallel",
                    "Assembly_CreateJointPerpendicular",
                    "Assembly_CreateJointAngle",
                ]
                self.title = translate("Assembly", "Constraints")

            def shouldShow(self):
                if not super().shouldShow():
                    return False
                return UtilsAssembly.assembly_has_at_least_n_parts(2)

        class AssemblyToolsWatcher(AssemblyBaseWatcher):
            """Shows Joint, View, and BOM tools when there are enough parts."""

            def __init__(self):
                super().__init__()
                self.commands = [
                    "Assembly_CreateView",
                    "Assembly_CreateBom",
                ]
                self.title = translate("Assembly", "Tools")

            def shouldShow(self):
                if not super().shouldShow():
                    return False
                return UtilsAssembly.assembly_has_at_least_n_parts(1)

        class AssemblySimulationWatcher(AssemblyBaseWatcher):
            """Shows 'Create Simulation' when specific motional joints exist."""

            def __init__(self):
                super().__init__()
                self.commands = ["Assembly_CreateSimulation"]
                self.title = translate("Assembly", "Simulation")

            def shouldShow(self):
                if not super().shouldShow():
                    return False

                joint_types = ["Revolute", "Slider", "Cylindrical"]
                joints = UtilsAssembly.getJointsOfType(self.assembly, joint_types)
                return len(joints) > 0

        watchers = [
            AssemblyCreateWatcher(),
            AssemblyActivateWatcher(),
            AssemblyInsertWatcher(),
            AssemblyGroundWatcher(),
            AssemblyJointsWatcher(),
            AssemblyToolsWatcher(),
            AssemblySimulationWatcher(),
        ]
        FreeCADGui.Control.addTaskWatcher(watchers)


Gui.addWorkbench(AssemblyWorkbench())
