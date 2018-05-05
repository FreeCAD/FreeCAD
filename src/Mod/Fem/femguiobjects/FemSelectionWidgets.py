# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "FemSelectWidget"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


from PySide import QtGui
from PySide import QtCore

import FreeCADGui as Gui


class _Selector(QtGui.QWidget):

    def __init__(self):
        super(_Selector, self).__init__()
        self._references = []
        self._register = dict()

        addBtn = QtGui.QPushButton(self.tr("Add"))
        delBtn = QtGui.QPushButton(self.tr("Remove"))
        addBtn.clicked.connect(self._add)
        delBtn.clicked.connect(self._del)

        btnLayout = QtGui.QHBoxLayout()
        btnLayout.addWidget(addBtn)
        btnLayout.addWidget(delBtn)

        self._model = QtGui.QStandardItemModel()
        self._view = SmallListView()
        self._view.setModel(self._model)

        self._helpTextLbl = QtGui.QLabel()
        self._helpTextLbl.setWordWrap(True)

        mainLayout = QtGui.QVBoxLayout()
        mainLayout.addWidget(self._helpTextLbl)
        mainLayout.addLayout(btnLayout)
        mainLayout.addWidget(self._view)
        self.setLayout(mainLayout)

    def references(self):
        return [entry for entry in self._references if entry[1]]

    def setReferences(self, references):
        self._references = []
        self._updateReferences(references)

    def setHelpText(self, text):
        self._helpTextLbl.setText(text)

    @QtCore.Slot()
    def _add(self):
        selection = self.getSelection()
        self._updateReferences(selection)

    @QtCore.Slot()
    def _del(self):
        selected = self._view.selectedIndexes()
        for index in selected:
            identifier = self._model.data(index)
            obj, sub = self._register[identifier]
            refIndex = self._getIndex(obj)
            entry = self._references[refIndex]
            newSub = tuple((x for x in entry[1] if x != sub))
            self._references[refIndex] = (obj, newSub)
            self._model.removeRow(index.row())

    def _updateReferences(self, selection):
        for obj, subList in selection:
            index = self._getIndex(obj)
            for sub in subList:
                entry = self._references[index]
                if sub not in entry[1]:
                    self._addToWidget(obj, sub)
                    newEntry = (obj, entry[1] + (sub,))
                    self._references[index] = newEntry

    def _addToWidget(self, obj, sub):
        identifier = "%s::%s" % (obj.Name, sub)
        item = QtGui.QStandardItem(identifier)
        self._model.appendRow(item)
        self._register[identifier] = (obj, sub)

    def _getIndex(self, obj):
        for i, entry in enumerate(self._references):
            if entry[0] == obj:
                return i
        self._references.append((obj, tuple()))
        return len(self._references) - 1

    def getSelection(self):
        raise NotImplementedError()


class BoundarySelector(_Selector):

    def __init__(self):
        super(BoundarySelector, self).__init__()
        self.setWindowTitle(self.tr("Select Faces/Edges/Vertexes"))
        self.setHelpText(self.tr(
            "To add references select them in the 3D view and then"
            " click \"Add\"."))

    def getSelection(self):
        selection = []
        for selObj in Gui.Selection.getSelectionEx():
            if selObj.HasSubObjects:
                item = (selObj.Object, tuple(selObj.SubElementNames))
                selection.append(item)
        return selection


class SolidSelector(_Selector):

    def __init__(self):
        super(SolidSelector, self).__init__()
        self.setWindowTitle(self.tr("Select Solids"))
        self.setHelpText(self.tr(
            "Select elements part of the solid that shall be added"
            " to the list. To than add the solid click \"Add\"."))

    def getSelection(self):
        selection = []
        for selObj in Gui.Selection.getSelectionEx():
            solids = set()
            for sub in self._getObjects(selObj.Object, selObj.SubElementNames):
                s = self._getSolidOfSub(selObj.Object, sub)
                if s is not None:
                    solids.add(s)
            if solids:
                item = (selObj.Object, tuple(solids))
                selection.append(item)
        return selection

    def _getObjects(self, obj, names):
        objects = []
        shape = obj.Shape
        for n in names:
            if n.startswith("Face"):
                objects.append(shape.Faces[int(n[4:]) - 1])
            elif n.startswith("Edge"):
                objects.append(shape.Edges[int(n[4:]) - 1])
            elif n.startswith("Vertex"):
                objects.append(shape.Vertexes[int(n[6:]) - 1])
            elif n.startswith("Solid"):
                objects.append(shape.Solids[int(n[5:]) - 1])
        return objects

    def _getSolidOfSub(self, obj, sub):
        foundSolids = set()
        if sub.ShapeType == "Solid":
            for solidId, solid in enumerate(obj.Shape.Solids):
                if sub.isSame(solid):
                    foundSolids.add("Solid" + str(solidId + 1))
        elif sub.ShapeType == "Face":
            for solidId, solid in enumerate(obj.Shape.Solids):
                if(self._findSub(sub, solid.Faces)):
                    foundSolids.add("Solid" + str(solidId + 1))
        elif sub.ShapeType == "Edge":
            for solidId, solid in enumerate(obj.Shape.Solids):
                if(self._findSub(sub, solid.Edges)):
                    foundSolids.add("Solid" + str(solidId + 1))
        elif sub.ShapeType == "Vertex":
            for solidId, solid in enumerate(obj.Shape.Solids):
                if(self._findSub(sub, solid.Vertexes)):
                    foundSolids.add("Solid" + str(solidId + 1))
        if len(foundSolids) == 1:
            return iter(foundSolids).next()
        return None

    def _findSub(self, sub, subList):
        for i, s in enumerate(subList):
            if s.isSame(sub):
                return True
        return False


class SmallListView(QtGui.QListView):

    def sizeHint(self):
        return QtCore.QSize(50, 50)
