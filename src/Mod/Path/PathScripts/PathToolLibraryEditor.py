# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
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

from __future__ import print_function
from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import PathScripts
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathToolBitLibraryCmd as PathToolBitLibraryCmd
import PathScripts.PathToolEdit as PathToolEdit
import PathScripts.PathToolLibraryManager as ToolLibraryManager
import PathScripts.PathUtils as PathUtils


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


class EditorPanel:
    def __init__(self, job, cb):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolLibraryEditor.ui")
        self.TLM = ToolLibraryManager.ToolLibraryManager()
        listname = self.TLM.getCurrentTableName()

        if listname:
            self.loadToolTables()

        self.job = job
        self.cb = cb

    def toolEditor(self, tool):
        dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolEdit.ui")
        editor = PathToolEdit.ToolEditor(tool, dialog.toolEditor, dialog)
        editor.setupUI()
        return editor

    def accept(self):
        pass

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        pass

    def setFields(self):
        pass

    def open(self):
        pass

    def getType(self, tooltype):
        "gets a combobox index number for a given type or vice versa"
        toolslist = Path.Tool.getToolTypes(Path.Tool())
        if isinstance(tooltype, str):
            if tooltype in toolslist:
                return toolslist.index(tooltype)
            else:
                return 0
        else:
            return toolslist[tooltype]

    def getMaterial(self, material):
        """gets a combobox index number for a given material or vice versa"""
        matslist = Path.Tool.getToolMaterials(Path.Tool())
        if isinstance(material, str):
            if material in matslist:
                return matslist.index(material)
            else:
                return 0
        else:
            return matslist[material]

    def addTool(self):
        """adds new tool to the current tool table"""
        tool = Path.Tool()
        editor = self.toolEditor(tool)

        r = editor.Parent.exec_()
        if r:
            editor.accept()
            listname = self.TLM.getCurrentTableName()
            self.TLM.addnew(listname, editor.Tool)
            self.loadTable(listname)

    def delete(self):
        """deletes the selected tool"""
        listname = self.TLM.getCurrentTableName()
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                t = model.index(i, 1)
                self.TLM.delete(int(t.data()), listname)
        self.loadTable(listname)
        self.toolSelectionChanged()

    def editTool(self, currItem):
        """load the tool edit dialog"""
        if not currItem:
            currItem = self.form.ToolsList.selectedIndexes()[1]

        row = currItem.row()
        value = currItem.sibling(row, 1).data()
        listname = self.TLM.getCurrentTableName()
        toolnum = int(value)
        tool = self.TLM.getTool(listname, toolnum)
        editor = self.toolEditor(tool)

        r = editor.Parent.exec_()
        if r:
            editor.accept()
            if self.TLM.updateTool(listname, toolnum, editor.Tool) is True:
                self.loadTable(listname)

    def moveUp(self):
        """moves a tool to a lower number, if possible"""
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.TLM.getCurrentTableName()
            success, newNum = self.TLM.moveup(number, listname)
            if success:
                self.loadTable(listname)
                self.updateSelection(newNum)

    def moveDown(self):
        """moves a tool to a higher number, if possible"""
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.TLM.getCurrentTableName()
            success, newNum = self.TLM.movedown(number, listname)
            if success:
                self.loadTable(listname)
                self.updateSelection(newNum)

    def duplicate(self):
        """duplicated the selected tool in the current tool table"""
        item = self.form.ToolsList.selectedIndexes()[1].data()
        if item:
            number = int(item)
            listname = self.TLM.getCurrentTableName()
            success, newNum = self.TLM.duplicate(number, listname)
            if success:
                self.loadTable(listname)
                self.updateSelection(newNum)

    def updateSelection(self, number):
        """update the tool list selection to track moves"""
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            if int(model.index(i, 1).data()) == number:
                self.form.ToolsList.selectRow(i)
                self.form.ToolsList.model().item(i, 0).setCheckState(QtCore.Qt.Checked)
                return

    def importFile(self):
        """imports a tooltable from a file"""
        filename = QtGui.QFileDialog.getOpenFileName(
            self.form,
            translate("Path_ToolTable", "Open tooltable", None),
            None,
            "{};;{};;{}".format(
                self.TLM.TooltableTypeJSON,
                self.TLM.TooltableTypeXML,
                self.TLM.TooltableTypeHeekscad,
            ),
        )
        if filename[0]:
            listname = self.TLM.getNextToolTableName()
            if self.TLM.read(filename, listname):
                self.loadToolTables()

    def exportFile(self):
        """export a tooltable to a file"""
        filename = QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("Path_ToolTable", "Save tooltable", None),
            None,
            "{};;{};;{}".format(
                self.TLM.TooltableTypeJSON,
                self.TLM.TooltableTypeXML,
                self.TLM.TooltableTypeLinuxCNC,
            ),
        )
        if filename[0]:
            listname = self.TLM.getCurrentTableName()
            self.TLM.write(filename, listname)

    def toolSelectionChanged(self, index=None):
        """updates the ui when tools are selected"""
        if index:
            self.form.ToolsList.selectRow(index.row())

        self.form.btnCopyTools.setEnabled(False)
        self.form.ButtonDelete.setEnabled(False)
        self.form.ButtonUp.setEnabled(False)
        self.form.ButtonDown.setEnabled(False)
        self.form.ButtonEdit.setEnabled(False)
        self.form.ButtonDuplicate.setEnabled(False)

        model = self.form.ToolsList.model()
        checkCount = 0
        checkList = []
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                checkCount += 1
                checkList.append(i)
                self.form.btnCopyTools.setEnabled(True)

        # only allow moving or deleting a single tool at a time.
        if checkCount == 1:
            # make sure the row is highlighted when the check box gets ticked
            self.form.ToolsList.selectRow(checkList[0])
            self.form.ButtonDelete.setEnabled(True)
            self.form.ButtonUp.setEnabled(True)
            self.form.ButtonDown.setEnabled(True)
            self.form.ButtonEdit.setEnabled(True)
            self.form.ButtonDuplicate.setEnabled(True)

        if len(PathUtils.GetJobs()) == 0:
            self.form.btnCopyTools.setEnabled(False)

    def copyTools(self):
        """copy selected tool"""
        tools = []
        model = self.form.ToolsList.model()
        for i in range(model.rowCount()):
            item = model.item(i, 0)
            if item.checkState():
                item = model.index(i, 1)
                tools.append(item.data())
        if len(tools) == 0:
            return

        targets = self.TLM.getJobList()
        currList = self.TLM.getCurrentTableName()

        for target in targets:
            if target == currList:
                targets.remove(target)

        if len(targets) == 0:
            FreeCAD.Console.PrintWarning("No Path Jobs in current document")
            return
        elif len(targets) == 1:
            targetlist = targets[0]
        else:
            form = FreeCADGui.PySideUic.loadUi(":/panels/DlgToolCopy.ui")
            form.cboTarget.addItems(targets)
            r = form.exec_()
            if r is False:
                return None
            else:
                targetlist = form.cboTarget.currentText()

        for toolnum in tools:
            tool = self.TLM.getTool(currList, int(toolnum))
            PathLog.debug("tool: {}, toolnum: {}".format(tool, toolnum))
            if self.job:
                label = "T{}: {}".format(toolnum, tool.Name)
                tc = PathScripts.PathToolController.Create(
                    label, tool=tool, toolNumber=int(toolnum)
                )
                self.job.Proxy.addToolController(tc)
            else:
                for job in FreeCAD.ActiveDocument.findObjects("Path::Feature"):
                    if (
                        isinstance(job.Proxy, PathScripts.PathJob.ObjectJob)
                        and job.Label == targetlist
                    ):
                        label = "T{}: {}".format(toolnum, tool.Name)
                        tc = PathScripts.PathToolController.Create(
                            label, tool=tool, toolNumber=int(toolnum)
                        )
                        job.Proxy.addToolController(tc)
        if self.cb:
            self.cb()
        FreeCAD.ActiveDocument.recompute()

    def tableSelected(self, index):
        """loads the tools for the selected tool table"""
        name = self.form.TableList.itemWidget(
            self.form.TableList.itemFromIndex(index)
        ).getTableName()
        self.loadTable(name)

    def loadTable(self, name):
        """loads the tools for the selected tool table"""
        tooldata = self.TLM.getTools(name)
        if tooldata:
            self.form.ToolsList.setModel(tooldata)
            self.form.ToolsList.resizeColumnsToContents()
            self.form.ToolsList.horizontalHeader().setResizeMode(
                self.form.ToolsList.model().columnCount() - 1, QtGui.QHeaderView.Stretch
            )
            self.setCurrentToolTableByName(name)

    def addNewToolTable(self):
        """adds new tool to selected tool table"""
        name = self.TLM.addNewToolTable()
        self.loadToolTables()
        self.loadTable(name)

    def loadToolTables(self):
        """Load list of available tool tables"""
        self.form.TableList.clear()
        model = self.form.ToolsList.model()
        if model:
            model.clear()
        if len(self.TLM.getToolTables()) > 0:
            for table in self.TLM.getToolTables():
                listWidgetItem = QtGui.QListWidgetItem()
                listItem = ToolTableListWidgetItem(self.TLM)
                listItem.setTableName(table.Name)
                listItem.setIcon(QtGui.QPixmap(":/icons/Path_ToolTable.svg"))
                listItem.toolMoved.connect(self.reloadReset)
                listWidgetItem.setSizeHint(QtCore.QSize(0, 40))
                self.form.TableList.addItem(listWidgetItem)
                self.form.TableList.setItemWidget(listWidgetItem, listItem)
            # Load the first tooltable
            self.loadTable(self.TLM.getCurrentTableName())

    def reloadReset(self):
        """reloads the current tooltable"""
        name = self.TLM.getCurrentTableName()
        self.loadTable(name)

    def setCurrentToolTableByName(self, name):
        """get the current tool table"""
        item = self.getToolTableByName(name)
        if item:
            self.form.TableList.setCurrentItem(item)

    def getToolTableByName(self, name):
        """returns the listWidgetItem for the selected name"""
        for i in range(self.form.TableList.count()):
            tableName = self.form.TableList.itemWidget(
                self.form.TableList.item(i)
            ).getTableName()
            if tableName == name:
                return self.form.TableList.item(i)
        return False

    def removeToolTable(self):
        """delete the selected tool table"""
        self.TLM.deleteToolTable()
        self.loadToolTables()

    def renameTable(self):
        """provides dialog for new tablename and renames the selected tool table"""
        name = self.TLM.getCurrentTableName()
        newName, ok = QtGui.QInputDialog.getText(
            None,
            translate("Path_ToolTable", "Rename Tooltable"),
            translate("Path_ToolTable", "Enter Name:"),
            QtGui.QLineEdit.Normal,
            name,
        )
        if ok and newName:
            index = self.form.TableList.indexFromItem(
                self.getToolTableByName(name)
            ).row()
            reloadTables = self.TLM.renameToolTable(newName, index)
            if reloadTables:
                self.loadToolTables()
                self.loadTable(newName)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        # Connect Signals and Slots
        self.form.ButtonNewTool.clicked.connect(self.addTool)
        self.form.ButtonImport.clicked.connect(self.importFile)
        self.form.ButtonExport.clicked.connect(self.exportFile)
        self.form.ButtonDown.clicked.connect(self.moveDown)
        self.form.ButtonUp.clicked.connect(self.moveUp)
        self.form.ButtonDelete.clicked.connect(self.delete)
        self.form.ButtonEdit.clicked.connect(self.editTool)
        self.form.ButtonDuplicate.clicked.connect(self.duplicate)
        self.form.btnCopyTools.clicked.connect(self.copyTools)

        self.form.ToolsList.doubleClicked.connect(self.editTool)
        self.form.ToolsList.clicked.connect(self.toolSelectionChanged)

        self.form.TableList.clicked.connect(self.tableSelected)
        self.form.TableList.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.TableList.itemChanged.connect(self.renameTable)

        self.form.ButtonAddToolTable.clicked.connect(self.addNewToolTable)
        self.form.ButtonAddToolTable.setToolTip(
            translate("Path_ToolTable", "Add New Tool Table")
        )
        self.form.ButtonRemoveToolTable.clicked.connect(self.removeToolTable)
        self.form.ButtonRemoveToolTable.setToolTip(
            translate("Path_ToolTable", "Delete Selected Tool Table")
        )
        self.form.ButtonRenameToolTable.clicked.connect(self.renameTable)
        self.form.ButtonRenameToolTable.setToolTip(
            translate("Path_ToolTable", "Rename Selected Tool Table")
        )

        self.setFields()


class ToolTableListWidgetItem(QtGui.QWidget):

    toolMoved = QtCore.Signal()

    def __init__(self, TLM):
        super(ToolTableListWidgetItem, self).__init__()

        self.tlm = TLM
        self.setAcceptDrops(True)

        self.mainLayout = QtGui.QHBoxLayout()
        self.iconQLabel = QtGui.QLabel()
        self.tableNameLabel = QtGui.QLabel()
        self.mainLayout.addWidget(self.iconQLabel, 0)
        self.mainLayout.addWidget(self.tableNameLabel, 1)
        self.setLayout(self.mainLayout)

    def setTableName(self, text):
        self.tableNameLabel.setText(text)

    def getTableName(self):
        return self.tableNameLabel.text()

    def setIcon(self, icon):
        icon = icon.scaled(24, 24)
        self.iconQLabel.setPixmap(icon)

    def dragEnterEvent(self, e):
        currentToolTable = self.tlm.getCurrentTableName()
        thisToolTable = self.getTableName()

        if not currentToolTable == thisToolTable:
            e.accept()
        else:
            e.ignore()

    def dropEvent(self, e):
        selectedTools = e.source().selectedIndexes()
        if selectedTools:
            toolData = selectedTools[1].data()

            if toolData:
                self.tlm.moveToTable(int(toolData), self.getTableName())
                self.toolMoved.emit()


class CommandToolLibraryEdit:
    def __init__(self):
        pass

    def edit(self, job=None, cb=None):
        if PathPreferences.toolsUseLegacyTools():
            editor = EditorPanel(job, cb)
            editor.setupUi()
            editor.form.exec_()
        else:
            if PathToolBitLibraryCmd.CommandToolBitLibraryLoad.Execute(job):
                if cb:
                    cb()

    def GetResources(self):
        return {
            "Pixmap": "Path_ToolTable",
            "MenuText": QT_TRANSLATE_NOOP("Path_ToolTable", "Tool Manager"),
            "Accel": "P, T",
            "ToolTip": QT_TRANSLATE_NOOP("Path_ToolTable", "Tool Manager"),
        }

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        self.edit()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_ToolLibraryEdit", CommandToolLibraryEdit())
