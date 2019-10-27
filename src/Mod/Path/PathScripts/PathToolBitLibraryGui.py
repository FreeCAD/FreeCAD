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
import PathScripts.PathToolBit as PathToolBit
import PathScripts.PathToolBitGui as PathToolBitGui
import PySide

import os
import traceback

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

class Delegate(PySide.QtGui.QStyledItemDelegate):

    def createEditor(self, parent, option, index):
        PathLog.track(index)
        return None
    def setEditorData(self, widget, index):
        PathLog.track(index)
    def setModelData(self, widget, model, index):
        PathLog.track(index)
    def updateEditorGeometry(self, widget, option, index):
        PathLog.track(index)
        widget.setGeometry(option.rect)

class ToolBitLibrary(object):

    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(':/panels/ToolBitLibraryEdit.ui')
        #self.form = FreeCADGui.PySideUic.loadUi('src/Mod/Path/Gui/Resources/panels/ToolBitLibraryEdit.ui')
        self.setupUI()

    def toolAdd(self):
        PathLog.track()
        try:
            foo = PathToolBitGui.GetToolFile(self.form)
            if foo:
                tool = PathToolBit.Declaration(foo)
                nr = 0
                for row in range(self.model.rowCount()):
                    itemNr = int(self.model.item(row, 0).data(PySide.QtCore.Qt.EditRole))
                    nr = max(nr, itemNr)

                toolNr = PySide.QtGui.QStandardItem()
                toolNr.setData(nr + 1, PySide.QtCore.Qt.EditRole)

                toolName = PySide.QtGui.QStandardItem()
                toolName.setData(tool['name'], PySide.QtCore.Qt.EditRole)
                toolName.setEditable(False)

                toolTemplate = PySide.QtGui.QStandardItem()
                toolTemplate.setData(os.path.splitext(os.path.basename(tool['template']))[0], PySide.QtCore.Qt.EditRole)
                toolTemplate.setEditable(False)

                toolDiameter = PySide.QtGui.QStandardItem()
                toolDiameter.setData(tool['parameter']['Diameter'], PySide.QtCore.Qt.EditRole)
                toolDiameter.setEditable(False)

                self.model.appendRow([toolNr, toolName, toolTemplate, toolDiameter])

                self.form.toolTable.resizeColumnsToContents()
            else:
                PathLog.info("no tool")
        except:
            PathLog.error('something happened')
            PathLog.error(traceback.print_exc())

    def toolDelete(self):
        PathLog.track()
    def toolUp(self):
        PathLog.track()
    def toolDown(self):
        PathLog.track()

    def columnNames(self):
        return ['Nr', 'Tool', 'Template', 'Diameter']

    def setupUI(self):
        PathLog.track('+')
        self.delegate = Delegate(self.form)
        self.model = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()), self.form)
        self.model.setHorizontalHeaderLabels(self.columnNames())

        self.form.toolTable.setModel(self.model)
        self.form.toolTable.resizeColumnsToContents()

        self.form.toolAdd.clicked.connect(self.toolAdd)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolUp.clicked.connect(self.toolUp)
        self.form.toolDown.clicked.connect(self.toolDown)

        PathLog.track('-')
