# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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


import FreeCAD
import FreeCADGui
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathToolBit as PathToolBit
import PathScripts.PathToolBitGui as PathToolBitGui
import PathScripts.PathToolBitEdit as PathToolBitEdit
import PathScripts.PathToolControllerGui as PathToolControllerGui
import PathScripts.PathUtilsGui as PathUtilsGui
from PySide import QtCore, QtGui
import PySide
import json
import os
import traceback
import uuid as UUID
from functools import partial

# PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())

_UuidRole = PySide.QtCore.Qt.UserRole + 1
_PathRole = PySide.QtCore.Qt.UserRole + 2


def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)


class _TableView(PySide.QtGui.QTableView):
    '''Subclass of QTableView to support rearrange and copying of ToolBits'''

    def __init__(self, parent):
        PySide.QtGui.QTableView.__init__(self, parent)
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setDropIndicatorShown(True)
        self.setDragDropMode(PySide.QtGui.QAbstractItemView.InternalMove)
        self.setDefaultDropAction(PySide.QtCore.Qt.MoveAction)
        self.setSortingEnabled(True)
        self.setSelectionBehavior(PySide.QtGui.QAbstractItemView.SelectRows)
        self.verticalHeader().hide()

    def supportedDropActions(self):
        return [PySide.QtCore.Qt.CopyAction, PySide.QtCore.Qt.MoveAction]

    def _uuidOfRow(self, row):
        model = self.model()
        return model.data(model.index(row, 0), _UuidRole)

    def _rowWithUuid(self, uuid):
        model = self.model()
        for row in range(model.rowCount()):
            if self._uuidOfRow(row) == uuid:
                return row
        return None

    def _copyTool(self, uuid_, dstRow):
        model = self.model()
        model.insertRow(dstRow)
        srcRow = self._rowWithUuid(uuid_)
        for col in range(model.columnCount()):
            srcItem = model.item(srcRow, col)

            model.setData(model.index(dstRow, col), srcItem.data(PySide.QtCore.Qt.EditRole), PySide.QtCore.Qt.EditRole)
            if col == 0:
                model.setData(model.index(dstRow, col), srcItem.data(_PathRole), _PathRole)
                # Even a clone of a tool gets its own uuid so it can be identified when
                # rearranging the order or inserting/deleting rows
                model.setData(model.index(dstRow, col), UUID.uuid4(), _UuidRole)
            else:
                model.item(dstRow, col).setEditable(False)

    def _copyTools(self, uuids, dst):
        for i, uuid in enumerate(uuids):
            self._copyTool(uuid, dst + i)

    def dropEvent(self, event):
        PathLog.track()
        mime = event.mimeData()
        data = mime.data('application/x-qstandarditemmodeldatalist')
        stream = PySide.QtCore.QDataStream(data)
        srcRows = []
        while not stream.atEnd():
            # pylint: disable=unused-variable
            row = stream.readInt32()
            srcRows.append(row)
            # col = stream.readInt32()
            # PathLog.track(row, col)
            # cnt = stream.readInt32()
            # for i in range(cnt):
            #     key = stream.readInt32()
            #     val = stream.readQVariant()
            # PathLog.track('    ', i, key, val, type(val))
            # I have no idea what these three integers are,
            # or if they even are three integers,
            # but it seems to work out this way.
            # i0 = stream.readInt32()
            # i1 = stream.readInt32()
            # i2 = stream.readInt32()
            # PathLog.track('  ', i0, i1, i2)

        # get the uuids of all srcRows
        model = self.model()
        srcUuids = [self._uuidOfRow(row) for row in set(srcRows)]
        destRow = self.rowAt(event.pos().y())

        self._copyTools(srcUuids, destRow)
        if PySide.QtCore.Qt.DropAction.MoveAction == event.proposedAction():
            for uuid in srcUuids:
                model.removeRow(self._rowWithUuid(uuid))


class ToolBitLibrary(object):
    '''ToolBitLibrary is the controller for displaying/selecting/creating/editing a collection of ToolBits.'''

    def __init__(self, path=None):
        self.path = path
        self.form = FreeCADGui.PySideUic.loadUi(':/panels/ToolBitLibraryEdit.ui')
        self.toolTableView = _TableView(self.form.toolTableGroup)
        self.form.toolTableGroup.layout().replaceWidget(self.form.toolTable, self.toolTableView)
        self.form.toolTable.hide()
        self.setupUI()
        self.title = self.form.windowTitle()
        self.LibFiles = []
        if path:
            self.libraryLoad(path)

        self.form.addToolController.setEnabled(False)
        self.form.ButtonRemoveToolTable.setEnabled(False)
        self.form.ButtonRenameToolTable.setEnabled(False)

    def _toolAdd(self, nr, tool, path):
        toolNr = PySide.QtGui.QStandardItem()
        toolNr.setData(nr, PySide.QtCore.Qt.EditRole)
        toolNr.setData(path, _PathRole)
        toolNr.setData(UUID.uuid4(), _UuidRole)

        toolName = PySide.QtGui.QStandardItem()
        toolName.setData(tool['name'], PySide.QtCore.Qt.EditRole)
        toolName.setEditable(False)

        toolShape = PySide.QtGui.QStandardItem()
        toolShape.setData(os.path.splitext(os.path.basename(tool['shape']))[0], PySide.QtCore.Qt.EditRole)
        toolShape.setEditable(False)

        toolDiameter = PySide.QtGui.QStandardItem()
        toolDiameter.setData(tool['parameter']['Diameter'], PySide.QtCore.Qt.EditRole)
        toolDiameter.setEditable(False)

        self.model.appendRow([toolNr, toolName, toolShape, toolDiameter])

    def toolAdd(self):
        PathLog.track()
        # pylint: disable=broad-except
        try:
            nr = 0
            for row in range(self.model.rowCount()):
                itemNr = int(self.model.item(row, 0).data(PySide.QtCore.Qt.EditRole))
                nr = max(nr, itemNr)
            nr += 1

            for i, foo in enumerate(PathToolBitGui.GetToolFiles(self.form)):
                tool = PathToolBit.Declaration(foo)
                self._toolAdd(nr + i, tool, foo)
            self.toolTableView.resizeColumnsToContents()
        except Exception:
            PathLog.error('something happened')
            PathLog.error(traceback.print_exc())

    def selectedOrAllTools(self):
        selectedRows = set([index.row() for index in self.toolTableView.selectedIndexes()])
        if not selectedRows:
            selectedRows = list(range(self.model.rowCount()))
        tools = []
        for row in selectedRows:
            item = self.model.item(row, 0)
            toolNr = int(item.data(PySide.QtCore.Qt.EditRole))
            toolPath = item.data(_PathRole)
            tools.append((toolNr, PathToolBit.Factory.CreateFrom(toolPath)))
        return tools

    def selectedOrAllToolControllers(self):
        tools = self.selectedOrAllTools()

        userinput = PathUtilsGui.PathUtilsUserInput()
        job = userinput.chooseJob(PathUtilsGui.PathUtils.GetJobs())
        for tool in tools:
            print(tool)
            tc = PathToolControllerGui.Create(tool[1].Label, tool[1], tool[0])
            job.Proxy.addToolController(tc)
            FreeCAD.ActiveDocument.recompute()

    def toolDelete(self):
        PathLog.track()
        selectedRows = set([index.row() for index in self.toolTableView.selectedIndexes()])
        for row in sorted(list(selectedRows), key=lambda r: -r):
            self.model.removeRows(row, 1)

    def libraryDelete(self):
        PathLog.track()
        reply = QtGui.QMessageBox.question(self.form, 'Warning', "Delete " + os.path.basename(self.path) + "?", QtGui.QMessageBox.Yes | QtGui.QMessageBox.Cancel)
        if reply == QtGui.QMessageBox.Yes and len(self.path) > 0:
            os.remove(self.path)
            PathPreferences.setLastPathToolTable("")
            self.libraryOpen(filedialog=False)

    def toolEnumerate(self):
        PathLog.track()
        for row in range(self.model.rowCount()):
            self.model.setData(self.model.index(row, 0), row + 1, PySide.QtCore.Qt.EditRole)

    def toolSelect(self, selected, deselected):
        # pylint: disable=unused-argument
        sel = len(self.toolTableView.selectedIndexes()) > 0
        self.form.toolDelete.setEnabled(sel)

        if sel:
            self.form.addToolController.setEnabled(True)
        else:
            self.form.addToolController.setEnabled(False)

    def tableSelected(self, index):
        ''' loads the tools for the selected tool table '''
        name = self.form.TableList.itemWidget(self.form.TableList.itemFromIndex(index)).getTableName()
        self.libraryLoad(PathPreferences.lastPathToolLibrary() + '/' + name)
        self.form.ButtonRemoveToolTable.setEnabled(True)
        self.form.ButtonRenameToolTable.setEnabled(True)

    def open(self, path=None, dialog=False):
        '''open(path=None, dialog=False) ... load library stored in path and bring up ui.
        Returns 1 if user pressed OK, 0 otherwise.'''
        if path:
            self.libraryOpen(path, filedialog=False)
        elif dialog:
            self.libraryOpen(None,  True)
        else:
            self.libraryOpen(None,  False)
        return self.form.exec_()

    def updateToolbar(self):
        if self.path:
            self.form.librarySave.setEnabled(True)
        else:
            self.form.librarySave.setEnabled(False)

    def libraryOpen(self, path=None,  filedialog=True):
        import glob
        PathLog.track()

        # Load default search path
        path = PathPreferences.lastPathToolLibrary()

        if filedialog or len(path) == 0:
            path = PySide.QtGui.QFileDialog.getExistingDirectory(self.form, 'Tool Library Path', PathPreferences.lastPathToolLibrary())
            if len(path) > 0:
                PathPreferences.setLastPathToolLibrary(path)
            else:
                return

        # Clear view
        self.form.TableList.clear()
        self.LibFiles.clear()
        self.form.lineLibPath.clear()
        self.form.lineLibPath.insert(path)

        # Find all tool tables in directory
        for file in glob.glob(path + '/*.fctl'):
            self.LibFiles.append(file)

        self.LibFiles.sort()

        # Add all tables to list
        for table in self.LibFiles:
            listWidgetItem = QtGui.QListWidgetItem()
            listItem = ToolTableListWidgetItem()
            listItem.setTableName(os.path.basename(table))
            listItem.setIcon(QtGui.QPixmap(':/icons/Path-ToolTable.svg'))
            listWidgetItem.setSizeHint(QtCore.QSize(0, 40))
            self.form.TableList.addItem(listWidgetItem)
            self.form.TableList.setItemWidget(listWidgetItem, listItem)

        self.path = []
        self.form.ButtonRemoveToolTable.setEnabled(False)
        self.form.ButtonRenameToolTable.setEnabled(False)

        self.toolTableView.setUpdatesEnabled(False)
        self.model.clear()
        self.model.setHorizontalHeaderLabels(self.columnNames())
        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.setUpdatesEnabled(True)

        # Search last selected table
        if len(self.LibFiles) > 0:
            for idx in range(len(self.LibFiles)):
                if PathPreferences.lastPathToolTable() == os.path.basename(self.LibFiles[idx]):
                    break
            # Not found, select first entry
            if idx >= len(self.LibFiles):
                idx = 0

            # Load selected table
            self.libraryLoad(self.LibFiles[idx])
            self.form.TableList.setCurrentRow(idx)
            self.form.ButtonRemoveToolTable.setEnabled(True)
            self.form.ButtonRenameToolTable.setEnabled(True)

    def libraryLoad(self, path):
        self.toolTableView.setUpdatesEnabled(False)
        self.model.clear()
        self.model.setHorizontalHeaderLabels(self.columnNames())

        if path:
            with open(path) as fp:
                PathPreferences.setLastPathToolTable(os.path.basename(path))
                library = json.load(fp)

            for toolBit in library['tools']:
                nr = toolBit['nr']
                bit = PathToolBit.findBit(toolBit['path'])
                if bit:
                    PathLog.track(bit)
                    tool = PathToolBit.Declaration(bit)
                    self._toolAdd(nr, tool, bit)
                else:
                    PathLog.error("Could not find tool #{}: {}".format(nr, library['tools'][nr]))

            self.toolTableView.resizeColumnsToContents()

        self.toolTableView.setUpdatesEnabled(True)

        self.form.setWindowTitle("{} - {}".format(self.title, os.path.basename(path) if path else ''))
        self.path = path
        self.updateToolbar()

    def libraryNew(self):
        self.libraryLoad(None)
        self.librarySaveAs()

    def renameLibrary(self):
        name = self.form.TableList.itemWidget(self.form.TableList.currentItem()).getTableName()
        newName, ok = QtGui.QInputDialog.getText(None, translate(
            "TooltableEditor", "Rename Tooltable"), translate(
                "TooltableEditor", "Enter Name:"), QtGui.QLineEdit.Normal, name)
        if ok and newName:
            os.rename(PathPreferences.lastPathToolLibrary() + '/' + name, PathPreferences.lastPathToolLibrary() + '/' + newName)
            self.libraryOpen(filedialog=False)

    # def createToolBit(self):
    #    tool = PathToolBit.ToolBitFactory().Create()

    #    #self.dialog = PySide.QtGui.QDialog(self.form)
    #    #layout = PySide.QtGui.QVBoxLayout(self.dialog)
    #    self.editor = PathToolBitEdit.ToolBitEditor(tool, self.form.toolTableGroup)
    #    self.editor.setupUI()
    #    self.buttons = PySide.QtGui.QDialogButtonBox(
    #            PySide.QtGui.QDialogButtonBox.Ok | PySide.QtGui.QDialogButtonBox.Cancel,
    #            PySide.QtCore.Qt.Horizontal, self.dialog)
    #    layout.addWidget(self.buttons)
    #    #self.buttons.accepted.connect(accept)
    #    #self.buttons.rejected.connect(reject)
    #    print(self.dialog.exec_())

    def librarySave(self):
        library = {}
        tools = []
        library['version'] = 1
        library['tools'] = tools
        for row in range(self.model.rowCount()):
            toolNr = self.model.data(self.model.index(row, 0), PySide.QtCore.Qt.EditRole)
            toolPath = self.model.data(self.model.index(row, 0), _PathRole)
            if PathPreferences.toolsStoreAbsolutePaths():
                tools.append({'nr': toolNr, 'path': toolPath})
            else:
                tools.append({'nr': toolNr, 'path': PathToolBit.findRelativePathTool(toolPath)})

        with open(self.path, 'w') as fp:
            json.dump(library, fp, sort_keys=True, indent=2)

    def libararySaveLinuxCNC(self, path):
        with open(path, 'w') as fp:
            fp.write(";\n")

            for row in range(self.model.rowCount()):
                toolNr = self.model.data(self.model.index(row, 0), PySide.QtCore.Qt.EditRole)
                toolPath = self.model.data(self.model.index(row, 0), _PathRole)

                bit = PathToolBit.Factory.CreateFrom(toolPath)
                if bit:
                    PathLog.track(bit)

                    pocket = bit.Pocket if hasattr(bit, "Pocket") else ""
                    xoffset = bit.Xoffset if hasattr(bit, "Xoffset") else "0"
                    yoffset = bit.Yoffset if hasattr(bit, "Yoffset") else "0"
                    zoffset = bit.Zoffset if hasattr(bit, "Zoffset") else "0"
                    aoffset = bit.Aoffset if hasattr(bit, "Aoffset") else "0"
                    boffset = bit.Boffset if hasattr(bit, "Boffset") else "0"
                    coffset = bit.Coffset if hasattr(bit, "Coffset") else "0"
                    uoffset = bit.Uoffset if hasattr(bit, "Uoffset") else "0"
                    voffset = bit.Voffset if hasattr(bit, "Voffset") else "0"
                    woffset = bit.Woffset if hasattr(bit, "Woffset") else "0"

                    diameter = bit.Diameter.getUserPreferred()[0].split()[0] if hasattr(bit, "Diameter") else "0"
                    frontangle = bit.FrontAngle if hasattr(bit, "FrontAngle") else "0"
                    backangle = bit.BackAngle if hasattr(bit, "BackAngle") else "0"
                    orientation = bit.Orientation if hasattr(bit, "Orientation") else "0"
                    remark = bit.Label

                    fp.write("T%s P%s X%s Y%s Z%s A%s B%s C%s U%s V%s W%s D%s I%s J%s Q%s ; %s\n" %
                        (toolNr,
                        pocket,
                        xoffset,
                        yoffset,
                        zoffset,
                        aoffset,
                        boffset,
                        coffset,
                        uoffset,
                        voffset,
                        woffset,
                        diameter,
                        frontangle,
                        backangle,
                        orientation,
                        remark))

                    FreeCAD.ActiveDocument.removeObject(bit.Name)

                else:
                    PathLog.error("Could not find tool #{} ".format(toolNr))

    def librarySaveAs(self):
        TooltableTypeJSON = translate("PathToolLibraryManager", "Tooltable JSON (*.fctl)")
        TooltableTypeLinuxCNC = translate("PathToolLibraryManager", "LinuxCNC tooltable (*.tbl)")

        filename = PySide.QtGui.QFileDialog.getSaveFileName(self.form,
                translate("TooltableEditor", "Save toolbit library", None),
                PathPreferences.lastPathToolLibrary(), "{};;{}".format(TooltableTypeJSON,
                    TooltableTypeLinuxCNC))
        # filename = PySide.QtGui.QFileDialog.getSaveFileName(self.form, \
        #        'Tool Library', PathPreferences.lastPathToolLibrary(), '*.fctl')
        if filename and filename[0]:
            if filename[1] == TooltableTypeLinuxCNC:
                path = filename[0] if filename[0].endswith('.tbl') else "{}.tbl".format(filename[0])
                self.libararySaveLinuxCNC(path)
            else:
                path = filename[0] if filename[0].endswith('.fctl') else "{}.fctl".format(filename[0])
                PathPreferences.setLastPathToolLibrary(os.path.dirname(path))
                self.path = path
                self.librarySave()
                self.updateToolbar()
                PathPreferences.setLastPathToolTable(os.path.basename(path))
                self.libraryOpen(None, False)

    def libraryCancel(self):
        self.form.close()

    def columnNames(self):
        return ['Nr', 'Tool', 'Shape', 'Diameter']

    def toolEdit(self, selected):
        print('here')
        print(selected)
        if selected.column() == 0:
            print('nope')
        else:
            print('yep')

    def setupUI(self):
        PathLog.track('+')
        self.model = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()), self.toolTableView)
        self.model.setHorizontalHeaderLabels(self.columnNames())

        self.toolTableView.setModel(self.model)
        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.selectionModel().selectionChanged.connect(self.toolSelect)
        self.toolTableView.doubleClicked.connect(self.toolEdit)

        self.form.toolAdd.clicked.connect(self.toolAdd)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolEnumerate.clicked.connect(self.toolEnumerate)
        # self.form.createToolBit.clicked.connect(self.createToolBit)

        self.form.ButtonAddToolTable.clicked.connect(self.libraryNew)
        self.form.ButtonRemoveToolTable.clicked.connect(self.libraryDelete)
        self.form.ButtonRenameToolTable.clicked.connect(self.renameLibrary)

        # self.form.libraryNew.clicked.connect(self.libraryNew)
        self.form.libraryOpen.clicked.connect(partial(self.libraryOpen, filedialog=True))
        self.form.librarySave.clicked.connect(self.librarySave)
        self.form.librarySaveAs.clicked.connect(self.librarySaveAs)
        self.form.libraryCancel.clicked.connect(self.libraryCancel)

        self.form.addToolController.clicked.connect(self.selectedOrAllToolControllers)

        self.form.TableList.clicked.connect(self.tableSelected)

        self.toolSelect([], [])
        self.updateToolbar()
        PathLog.track('-')


class ToolTableListWidgetItem(QtGui.QWidget):
    toolMoved = QtCore.Signal()

    def __init__(self):
        super(ToolTableListWidgetItem, self).__init__()

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
        icon = icon.scaled(22, 22)
        self.iconQLabel.setPixmap(icon)

    # def dragEnterEvent(self, e):
    #    currentToolTable = self.tlm.getCurrentTableName()
    #    thisToolTable = self.getTableName()

    def dropEvent(self, e):
        selectedTools = e.source().selectedIndexes()
        print("Drop: {}, {}".format(selectedTools, selectedTools[1].data()))
