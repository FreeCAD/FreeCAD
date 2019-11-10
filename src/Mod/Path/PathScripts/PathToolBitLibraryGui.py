# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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


import FreeCADGui
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathToolBit as PathToolBit
import PathScripts.PathToolBitGui as PathToolBitGui
import PySide
import json
import os
import traceback
import uuid as UUID

#PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

_UuidRole = PySide.QtCore.Qt.UserRole + 1
_PathRole = PySide.QtCore.Qt.UserRole + 2

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
            col = stream.readInt32()
            #PathLog.track(row, col)
            cnt = stream.readInt32()
            for i in range(cnt):
                key = stream.readInt32()
                val = stream.readQVariant()
                #PathLog.track('    ', i, key, val, type(val))
            # I have no idea what these three integers are,
            # or if they even are three integers,
            # but it seems to work out this way.
            i0 = stream.readInt32()
            i1 = stream.readInt32()
            i2 = stream.readInt32()
            #PathLog.track('  ', i0, i1, i2)

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
        if path:
            self.libraryLoad(path)

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

    def toolDelete(self):
        PathLog.track()
        selectedRows = set([index.row() for index in self.toolTableView.selectedIndexes()])
        for row in sorted(list(selectedRows), key = lambda r: -r):
            self.model.removeRows(row, 1)

    def toolEnumerate(self):
        PathLog.track()
        for row in range(self.model.rowCount()):
            self.model.setData(self.model.index(row, 0), row + 1, PySide.QtCore.Qt.EditRole)

    def toolSelect(self, selected, deselected):
        # pylint: disable=unused-argument
        self.form.toolDelete.setEnabled(len(self.toolTableView.selectedIndexes()) > 0)

    def open(self, path=None, dialog=False):
        '''open(path=None, dialog=False) ... load library stored in path and bring up ui.
        Returns 1 if user pressed OK, 0 otherwise.'''
        if path:
            fullPath = PathToolBit.findLibrary(path)
            if fullPath:
                self.libraryLoad(fullPath)
            else:
                self.libraryOpen()
        elif dialog:
            self.libraryOpen()
        return self.form.exec_()

    def updateToolbar(self):
        if self.path:
            self.form.librarySave.setEnabled(True)
        else:
            self.form.librarySave.setEnabled(False)

    def libraryOpen(self):
        PathLog.track()
        foo = PySide.QtGui.QFileDialog.getOpenFileName(self.form, 'Tool Library', PathPreferences.lastPathToolLibrary(), '*.fctl')
        if foo and foo[0]:
            path = foo[0]
            PathPreferences.setLastPathToolLibrary(os.path.dirname(path))
            self.libraryLoad(path)

    def libraryLoad(self, path):
        self.toolTableView.setUpdatesEnabled(False)
        self.model.clear()
        self.model.setHorizontalHeaderLabels(self.columnNames())
        if path:
            with open(path) as fp:
                library = json.load(fp)
            for toolBit in library['tools']:
                nr  = toolBit['nr']
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

    def librarySave(self):
        library = {}
        tools = []
        library['version'] = 1
        library['tools'] = tools
        for row in range(self.model.rowCount()):
            toolNr   = self.model.data(self.model.index(row, 0), PySide.QtCore.Qt.EditRole)
            toolPath = self.model.data(self.model.index(row, 0), _PathRole)
            if PathPreferences.toolsStoreAbsolutePaths():
                tools.append({'nr': toolNr, 'path': toolPath})
            else:
                tools.append({'nr': toolNr, 'path': PathToolBit.findRelativePathTool(toolPath)})

        with open(self.path, 'w') as fp:
            json.dump(library, fp, sort_keys=True, indent=2)

    def librarySaveAs(self):
        foo = PySide.QtGui.QFileDialog.getSaveFileName(self.form, 'Tool Library', PathPreferences.lastPathToolLibrary(), '*.fctl')
        if foo and foo[0]:
            path = foo[0] if foo[0].endswith('.fctl') else "{}.fctl".format(foo[0])
            PathPreferences.setLastPathToolLibrary(os.path.dirname(path))
            self.path = path
            self.librarySave()
            self.updateToolbar()

    def columnNames(self):
        return ['Nr', 'Tool', 'Shape', 'Diameter']

    def setupUI(self):
        PathLog.track('+')
        self.model = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()), self.toolTableView)
        self.model.setHorizontalHeaderLabels(self.columnNames())

        self.toolTableView.setModel(self.model)
        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.selectionModel().selectionChanged.connect(self.toolSelect)

        self.form.toolAdd.clicked.connect(self.toolAdd)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolEnumerate.clicked.connect(self.toolEnumerate)

        self.form.libraryNew.clicked.connect(self.libraryNew)
        self.form.libraryOpen.clicked.connect(self.libraryOpen)
        self.form.librarySave.clicked.connect(self.librarySave)
        self.form.librarySaveAs.clicked.connect(self.librarySaveAs)

        self.toolSelect([], [])
        self.updateToolbar()
        PathLog.track('-')
