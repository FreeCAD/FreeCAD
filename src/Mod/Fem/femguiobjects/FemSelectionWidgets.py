# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM select widget"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemSelectWidget
#  \ingroup FEM
#  \brief FreeCAD FEM FemSelectWidget

import FreeCAD
import FreeCADGui
import femmesh.meshtools as FemMeshTools

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


class GeometryElementsSelection(QtGui.QWidget):

    def __init__(self, ref, eltypes=[], multigeom=True):
        super(GeometryElementsSelection, self).__init__()
        # init ui stuff
        FreeCADGui.Selection.clearSelection()
        self.selection_mode_solid = False
        self.sel_server = None
        self.obj_notvisible = []
        self.initElemTypes(eltypes)
        self.allow_multiple_geom_types = multigeom
        # print(self.allow_multiple_geom_types)
        self.initUI()
        # set references and fill the list widget
        self.references = []
        if ref:
            self.tuplereferences = ref
            self.get_references()
        self.rebuild_list_References()

    def initElemTypes(self, eltypes):
        self.sel_elem_types = eltypes
        # FreeCAD.Console.PrintMessage('Selection of: {} is allowed.\n'.format(self.sel_elem_types))
        self.sel_elem_text = ''
        for e in self.sel_elem_types:
            self.sel_elem_text += (e + ', ')
        self.sel_elem_text = self.sel_elem_text.rstrip(', ')
        # FreeCAD.Console.PrintMessage('Selection of: ' + self.sel_elem_text + ' is allowed.\n')
        self.selection_mode_std_print_message = "Single click on a " + self.sel_elem_text + " will add it to the list"
        self.selection_mode_solid_print_message = "Single click on a Face or Edge which belongs to one Solid will add the Solid to the list"

    def initUI(self):
        # auch ArchPanel ist coded ohne ui-file
        # title
        self.setWindowTitle(self.tr("Geometry reference selector for a ") + self.sel_elem_text)
        # button
        self.pushButton_Add = QtGui.QPushButton(self.tr("Add"))
        # label
        self._helpTextLbl = QtGui.QLabel()
        self._helpTextLbl.setWordWrap(True)
        self._helpTextLbl.setText(self.tr(
            "Click on \"Add\" and select geometric elements to add them to the list."
            " If no geometry is added to the list, all remaining ones are used."
            " The following geometry elements are allowed to select: ") + self.sel_elem_text)
        # list
        self.list_References = QtGui.QListWidget()
        # radiobutton down the list
        self.lb_selmod = QtGui.QLabel()
        self.lb_selmod.setText(self.tr("Selection mode"))
        self.rb_standard = QtGui.QRadioButton(self.tr(self.sel_elem_text.lstrip('Solid, ')))
        self.rb_solid = QtGui.QRadioButton(self.tr("Solid"))
        self.rb_standard.setChecked(True)
        self.rb_solid.setChecked(False)
        # radio butoon layout
        rbtnLayout = QtGui.QHBoxLayout()
        rbtnLayout.addWidget(self.lb_selmod)
        rbtnLayout.addWidget(self.rb_standard)
        rbtnLayout.addWidget(self.rb_solid)
        # main layout
        mainLayout = QtGui.QVBoxLayout()
        mainLayout.addWidget(self._helpTextLbl)
        mainLayout.addWidget(self.pushButton_Add)
        mainLayout.addWidget(self.list_References)
        if 'Solid' in self.sel_elem_types:
            mainLayout.addLayout(rbtnLayout)
        self.setLayout(mainLayout)
        # signals and slots
        self.list_References.itemSelectionChanged.connect(self.select_clicked_reference_shape)
        self.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.list_References.connect(self.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)
        QtCore.QObject.connect(self.pushButton_Add, QtCore.SIGNAL("clicked()"), self.add_references)
        QtCore.QObject.connect(self.rb_standard, QtCore.SIGNAL("toggled(bool)"), self.choose_selection_mode_standard)
        QtCore.QObject.connect(self.rb_solid, QtCore.SIGNAL("toggled(bool)"), self.choose_selection_mode_solid)

    def get_references(self):
        for ref in self.tuplereferences:
            for elem in ref[1]:
                self.references.append((ref[0], elem))

    def get_item_text(self, ref):
        return (ref[0].Name + ':' + ref[1])

    def get_allitems_text(self):
        items = []
        for ref in self.references:
            items.append(self.get_item_text(ref))
        return sorted(items)

    def rebuild_list_References(self, current_row=0):
        self.list_References.clear()
        for listItemName in self.get_allitems_text():
            self.list_References.addItem(listItemName)
        if current_row > self.list_References.count() - 1:  # first row is 0
            current_row = self.list_References.count() - 1
        if self.list_References.count() > 0:
            self.list_References.setCurrentItem(self.list_References.item(current_row))

    def select_clicked_reference_shape(self):
        self.setback_listobj_visibility()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
            self.sel_server = None
        if not self.sel_server:
            if not self.references:
                return
            currentItemName = str(self.list_References.currentItem().text())
            for ref in self.references:
                if self.get_item_text(ref) == currentItemName:
                    # print('found: shape: ' + ref[0].Name + ' element: ' + ref[1])
                    if not ref[0].ViewObject.Visibility:
                        self.obj_notvisible.append(ref[0])
                        ref[0].ViewObject.Visibility = True
                    FreeCADGui.Selection.clearSelection()
                    if ref[1].startswith('Solid') and (ref[0].Shape.ShapeType == 'Compound' or ref[0].Shape.ShapeType == 'CompSolid'):
                        # selection of Solids of Compounds or CompSolids is not possible, because a Solid is no Subelement
                        # since only Subelements can be selected, we're going to select all Faces of said Solids
                        solid = FemMeshTools.get_element(ref[0], ref[1])  # the method getElement(element) doesn't return Solid elements
                        if not solid:
                            return
                        faces = []
                        for fs in solid.Faces:
                            # find these faces in ref[0]
                            for i, fref in enumerate(ref[0].Shape.Faces):
                                if fs.isSame(fref):
                                    fref_elstring = 'Face' + str(i + 1)
                                    if fref_elstring not in faces:
                                        faces.append(fref_elstring)
                        for f in faces:
                            FreeCADGui.Selection.addSelection(ref[0], f)
                    else:
                        # Selection of all other element types is supported
                        FreeCADGui.Selection.addSelection(ref[0], ref[1])

    def setback_listobj_visibility(self):
        '''set back Visibility of the list objects
        '''
        FreeCADGui.Selection.clearSelection()
        for obj in self.obj_notvisible:
            obj.ViewObject.Visibility = False
        self.obj_notvisible = []

    def references_list_right_clicked(self, QPos):
        self.contextMenu = QtGui.QMenu()
        menu_item_remove_selected = self.contextMenu.addAction("Remove selected geometry")
        menu_item_remove_all = self.contextMenu.addAction("Clear list")
        if not self.references:
            menu_item_remove_selected.setDisabled(True)
            menu_item_remove_all.setDisabled(True)
        self.connect(menu_item_remove_selected, QtCore.SIGNAL("triggered()"), self.remove_selected_reference)
        self.connect(menu_item_remove_all, QtCore.SIGNAL("triggered()"), self.remove_all_references)
        parentPosition = self.list_References.mapToGlobal(QtCore.QPoint(0, 0))
        self.contextMenu.move(parentPosition + QPos)
        self.contextMenu.show()

    def remove_selected_reference(self):
        if not self.references:
            return
        currentItemName = str(self.list_References.currentItem().text())
        currentRow = self.list_References.currentRow()
        for ref in self.references:
            if self.get_item_text(ref) == currentItemName:
                self.references.remove(ref)
        self.rebuild_list_References(currentRow)

    def remove_all_references(self):
        self.references = []
        self.rebuild_list_References()

    def choose_selection_mode_standard(self, state):
        self.selection_mode_solid = not state
        if self.sel_server and not self.selection_mode_solid:
            FreeCAD.Console.PrintMessage(self.selection_mode_std_print_message + '\n')

    def choose_selection_mode_solid(self, state):
        self.selection_mode_solid = state
        if self.sel_server and self.selection_mode_solid:
            FreeCAD.Console.PrintMessage(self.selection_mode_solid_print_message + '\n')

    def add_references(self):
        '''Called if Button add_reference is triggered'''
        # in constraints EditTaskPanel the selection is active as soon as the taskpanel is open
        # here the addReference button EditTaskPanel has to be triggered to start selection mode
        self.setback_listobj_visibility()
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        if self.selection_mode_solid:  # print message on button click
            print_message = self.selection_mode_solid_print_message
        else:
            print_message = self.selection_mode_std_print_message
        if not self.sel_server:
            # if we do not check, we would start a new SelectionObserver on every click on addReference button
            # but close only one SelectionObserver on leaving the task panel
            self.sel_server = FemSelectionObserver(self.selectionParser, print_message)

    def selectionParser(self, selection):
        print('selection: ', selection[0].Shape.ShapeType, '  ', selection[0].Name, '  ', selection[1])
        if hasattr(selection[0], "Shape") and selection[1]:
            elt = selection[0].Shape.getElement(selection[1])
            ele_ShapeType = elt.ShapeType
            if self.selection_mode_solid and 'Solid' in self.sel_elem_types:
                # in solid selection mode use edges and faces for selection of a solid
                # adapt selection variable to hold the Solid
                solid_to_add = None
                if ele_ShapeType == 'Edge':
                    found_edge = False
                    for i, s in enumerate(selection[0].Shape.Solids):
                        for e in s.Edges:
                            if elt.isSame(e):
                                if not found_edge:
                                    solid_to_add = str(i + 1)
                                else:
                                    FreeCAD.Console.PrintMessage('Edge belongs to more than one solid\n')
                                    solid_to_add = None
                                found_edge = True
                elif ele_ShapeType == 'Face':
                    found_face = False
                    for i, s in enumerate(selection[0].Shape.Solids):
                        for e in s.Faces:
                            if elt.isSame(e):
                                if not found_face:
                                    solid_to_add = str(i + 1)
                                else:
                                    FreeCAD.Console.PrintMessage('Face belongs to more than one solid\n')
                                    solid_to_add = None
                                found_face = True
                if solid_to_add:
                    selection = (selection[0], 'Solid' + solid_to_add)
                    ele_ShapeType = 'Solid'
                    FreeCAD.Console.PrintMessage('selection variable adapted to hold the Solid: ' + selection[0].Shape.ShapeType + '  ' + selection[0].Name + '  ' + selection[1] + ' \n')
                else:
                    return
            if ele_ShapeType in self.sel_elem_types:
                if (self.selection_mode_solid and ele_ShapeType == 'Solid') or self.selection_mode_solid is False:
                    if selection not in self.references:
                        if self.allow_multiple_geom_types is False:  # only equal shape types are allowed to add
                            if self.has_equal_references_shape_types(ele_ShapeType):
                                self.references.append(selection)
                                self.rebuild_list_References(self.get_allitems_text().index(self.get_item_text(selection)))
                            else:
                                FreeCADGui.Selection.clearSelection()  # selected shape will not added to the list
                        else:  # multiple shape types are allowed to add
                            self.references.append(selection)
                            self.rebuild_list_References(self.get_allitems_text().index(self.get_item_text(selection)))
                    else:
                        FreeCADGui.Selection.clearSelection()  # selected shape will not added to the list
                        message = self.get_item_text(selection) + ' is in reference list already!\n'
                        FreeCAD.Console.PrintMessage(message)
                        QtGui.QMessageBox.critical(None, "Geometry already in list", message)
            else:
                FreeCADGui.Selection.clearSelection()  # selected shape will not added to the list
                message = ele_ShapeType + ' is not allowed to add to the list!\n'
                FreeCAD.Console.PrintMessage(message)
                QtGui.QMessageBox.critical(None, "Wrong shape type", message)

    def has_equal_references_shape_types(self, ref_shty=''):
        for ref in self.references:
            r = FemMeshTools.get_element(ref[0], ref[1])  # the method getElement(element) does not return Solid elements
            if not r:
                FreeCAD.Console.PrintError('Problem in retrieving element: {} \n'.format(ref[1]))
                continue
            # print('  ReferenceShape : ', r.ShapeType, ', ', ref[0].Name, ', ', ref[0].Label, ' --> ', ref[1])
            if not ref_shty:
                ref_shty = r.ShapeType
            if r.ShapeType != ref_shty:
                message = 'Multiple shape types are not allowed in the reference list.\n'
                FreeCAD.Console.PrintMessage(message)
                QtGui.QMessageBox.critical(None, "Multiple ShapeTypes not allowed", message)
                return False
        return True


class FemSelectionObserver:
    '''selection observer especially for the needs of geometry reference selection of FEM'''
    def __init__(self, parseSelectionFunction, print_message=''):
        self.parseSelectionFunction = parseSelectionFunction
        FreeCADGui.Selection.addObserver(self)
        FreeCAD.Console.PrintMessage(print_message + "!\n")

    def addSelection(self, docName, objName, sub, pos):
        selected_object = FreeCAD.getDocument(docName).getObject(objName)  # get the obj objName
        self.added_obj = (selected_object, sub)
        # on double click on a vertex of a solid sub is None and obj is the solid
        self.parseSelectionFunction(self.added_obj)
