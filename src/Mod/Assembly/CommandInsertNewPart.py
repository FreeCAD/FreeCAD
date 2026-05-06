# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

import re
import os
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets
    from PySide.QtGui import QIcon
    from PySide.QtCore import QTimer

import UtilsAssembly
import Preferences
import JointObject

translate = App.Qt.translate

__title__ = "Assembly Command Insert New Part"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandInsertNewPart:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Geofeaturegroup",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_InsertNewPart", "New Part"),
            "Accel": "P",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_InsertNewPart",
                "Insert a new part into the active assembly. The new part's origin can be positioned in the assembly.",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.isAssemblyCommandActive()

    def Activated(self):
        # Check if document is saved before proceeding
        doc = App.ActiveDocument
        if not doc.FileName:
            msgBox = QtWidgets.QMessageBox()
            msgBox.setIcon(QtWidgets.QMessageBox.Warning)
            msgBox.setText(
                translate(
                    "Assembly",
                    "The assembly document must be saved before inserting a new part.",
                )
            )
            msgBox.setWindowTitle(translate("Assembly", "Save Document"))
            saveButton = msgBox.addButton(
                translate("Assembly", "Save"), QtWidgets.QMessageBox.AcceptRole
            )
            msgBox.addButton(QtWidgets.QMessageBox.Cancel)
            msgBox.exec_()
            if msgBox.clickedButton() == saveButton:
                if not Gui.getDocument(doc).saveAs():
                    return
            else:
                return

        panel = TaskAssemblyNewPart()
        dialog = Gui.Control.showDialog(panel)
        if dialog is not None:
            dialog.setAutoCloseOnDeletedDocument(True)
            dialog.setDocumentName(App.ActiveDocument.Name)


class TaskAssemblyNewPart(JointObject.TaskAssemblyCreateJoint):
    def __init__(self):
        super().__init__(0, None, True)

        self.assembly = UtilsAssembly.activeAssembly()

        # Retrieve the existing layout of `self.form`
        mainLayout = self.form.layout()

        # Add a name input
        nameLayout = QtWidgets.QHBoxLayout()
        nameLabel = QtWidgets.QLabel(translate("Assembly", "Part name"))
        self.nameEdit = QtWidgets.QLineEdit()
        nameLayout.addWidget(nameLabel)
        nameLayout.addWidget(self.nameEdit)
        mainLayout.addLayout(nameLayout)
        self.nameEdit.setText(translate("Assembly", "Part"))

        # Add a checkbox
        self.createInNewFileCheck = QtWidgets.QCheckBox(
            translate("Assembly", "Create part in new file")
        )
        mainLayout.addWidget(self.createInNewFileCheck)
        self.createInNewFileCheck.setChecked(
            Preferences.preferences().GetBool("PartInNewFile", True)
        )

        # Wrap the joint creation UI in a groupbox
        jointGroupBox = QtWidgets.QGroupBox(translate("Assembly", "Joint new part origin"))
        jointLayout = QtWidgets.QVBoxLayout(jointGroupBox)
        jointLayout.addWidget(self.jForm)
        jointLayout.setContentsMargins(0, 0, 0, 0)
        jointLayout.setSpacing(0)
        mainLayout.addWidget(jointGroupBox)

        self.link = self.assembly.newObject("App::Link", "Link")
        # add the link as the first object of the joint
        Gui.Selection.addSelection(
            self.assembly.Document.Name, self.assembly.Name, self.link.Name + "."
        )

    def createPart(self):
        partName = self.nameEdit.text()
        newFile = self.createInNewFileCheck.isChecked()

        doc = self.assembly.Document
        if newFile:
            doc = App.newDocument(partName)

        part, body = UtilsAssembly.createPart(partName, doc)

        App.setActiveDocument(self.assembly.Document.Name)

        # Then we need to link the part.
        if newFile:
            # New file must be saved or we can't link
            if not Gui.getDocument(doc).saveAs():
                msgBox = QtWidgets.QMessageBox()
                msgBox.setIcon(QtWidgets.QMessageBox.Warning)
                msgBox.setText(
                    translate(
                        "Assembly",
                        "If the new document is not saved the new part cannot be linked in the assembly.",
                    )
                )
                msgBox.setWindowTitle(translate("Assembly", "Save Document"))
                saveButton = msgBox.addButton(
                    translate("Assembly", "Save"), QtWidgets.QMessageBox.AcceptRole
                )
                cancelButton = msgBox.addButton(
                    translate("Assembly", "Do not Link"), QtWidgets.QMessageBox.RejectRole
                )

                msgBox.exec_()

                if not (msgBox.clickedButton() == saveButton and Gui.getDocument(doc).saveAs()):
                    return

        self.link.LinkedObject = part
        self.link.touch()

        self.link.Label = part.Label

        # Set the body as active in the assembly doc
        self.expandLinkManually(self.link)
        doc = self.assembly.Document
        Gui.getDocument(doc).ActiveView.setActiveObject("pdbody", body)
        doc.recompute()

    def expandLinkManually(self, link):
        # Should not be necessary
        # This is a workaround of https://github.com/FreeCAD/FreeCAD/issues/17904
        mw = Gui.getMainWindow()
        trees = mw.findChildren(QtGui.QTreeWidget)

        Gui.Selection.addSelection(link)
        for tree in trees:
            for item in tree.selectedItems():
                tree.expandItem(item)

    def accept(self):
        if len(self.refs) != 2:
            # if the joint is not complete we cancel the joint but not the new part!
            self.joint.Document.removeObject(self.joint.Name)
        else:
            JointObject.solveIfAllowed(self.assembly)
            self.joint.Visibility = False
            cmds = UtilsAssembly.generatePropertySettings(self.joint)
            Gui.doCommand(cmds)

        self.deactivate()

        self.createPart()

        App.closeActiveTransaction()

        return True

    def deactivate(self):
        Preferences.preferences().SetBool("PartInNewFile", self.createInNewFileCheck.isChecked())
        super().deactivate()


if App.GuiUp:
    Gui.addCommand("Assembly_InsertNewPart", CommandInsertNewPart())
