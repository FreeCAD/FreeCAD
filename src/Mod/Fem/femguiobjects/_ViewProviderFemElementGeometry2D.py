# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_ViewProviderFemElementGeometry2D"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementGeometry2D
#  \ingroup FEM

import FreeCAD
import FreeCADGui


# for the panel
from PySide import QtCore
from PySide import QtGui


class _ViewProviderFemElementGeometry2D:
    "A View Provider for the FemElementGeometry2D object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-shell-thickness.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Standard")

    def getDisplayModes(self, obj):
        return ["Standard"]

    def getDefaultDisplayMode(self):
        return "Standard"

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        taskd = _TaskPanelFemElementGeometry2D(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not doc.getInEdit():
            doc.setEdit(vobj.Object.Name)
        else:
            FreeCAD.Console.PrintError('Active Task Dialog found! Please close this one first!\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemElementGeometry2D:
    '''The TaskPanel for editing References property of FemElementGeometry2D objects'''

    def __init__(self, obj):
        FreeCADGui.Selection.clearSelection()
        self.sel_server = None
        self.obj = obj
        self.obj_notvisible = []

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry2D.ui")
        QtCore.QObject.connect(self.form.if_thickness, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.thickness_changed)
        QtCore.QObject.connect(self.form.pushButton_Reference, QtCore.SIGNAL("clicked()"), self.add_references)
        self.form.list_References.itemSelectionChanged.connect(self.select_clicked_reference_shape)
        self.form.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.list_References.connect(self.form.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)

        self.get_shellthickness_props()
        self.update()

    def accept(self):
        self.setback_listobj_visibility()
        self.set_shellthickness_props()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        self.setback_listobj_visibility()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def get_shellthickness_props(self):
        self.thickness = self.obj.Thickness
        self.references = []
        if self.obj.References:
            self.tuplereferences = self.obj.References
            self.get_references()

    def set_shellthickness_props(self):
        self.obj.References = self.references
        self.obj.Thickness = self.thickness

    def update(self):
        'fills the widgets'
        self.form.if_thickness.setText(self.thickness.UserString)
        self.rebuild_list_References()

    def thickness_changed(self, base_quantity_value):
        self.thickness = base_quantity_value

    def get_references(self):
        for ref in self.tuplereferences:
            for elem in ref[1]:
                self.references.append((ref[0], elem))

    def references_list_right_clicked(self, QPos):
        self.form.contextMenu = QtGui.QMenu()
        menu_item = self.form.contextMenu.addAction("Remove Reference")
        if not self.references:
            menu_item.setDisabled(True)
        self.form.connect(menu_item, QtCore.SIGNAL("triggered()"), self.remove_reference)
        parentPosition = self.form.list_References.mapToGlobal(QtCore.QPoint(0, 0))
        self.form.contextMenu.move(parentPosition + QPos)
        self.form.contextMenu.show()

    def remove_reference(self):
        if not self.references:
            return
        currentItemName = str(self.form.list_References.currentItem().text())
        for ref in self.references:
            refname_to_compare_listentry = ref[0].Name + ':' + ref[1]
            if refname_to_compare_listentry == currentItemName:
                self.references.remove(ref)
        self.rebuild_list_References()

    def add_references(self):
        '''Called if Button add_reference is triggered'''
        # in constraints EditTaskPanel the selection is active as soon as the taskpanel is open
        # here the addReference button EditTaskPanel has to be triggered to start selection mode
        self.setback_listobj_visibility()
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        print_message = "Select Faces by single click on them to add them to the list"
        if not self.sel_server:
            # if we do not check, we would start a new SelectionObserver on every click on addReference button
            # but close only one SelectionObserver on leaving the task panel
            from . import FemSelectionObserver
            self.sel_server = FemSelectionObserver.FemSelectionObserver(self.selectionParser, print_message)

    def selectionParser(self, selection):
        # print('selection: ', selection[0].Shape.ShapeType, '  ', selection[0].Name, '  ', selection[1])
        if hasattr(selection[0], "Shape"):
            if selection[1]:
                elt = selection[0].Shape.getElement(selection[1])
                if elt.ShapeType == 'Face':
                    if selection not in self.references:
                        self.references.append(selection)
                        self.rebuild_list_References()
                    else:
                        FreeCAD.Console.PrintMessage(selection[0].Name + ' --> ' + selection[1] + ' is in reference list already!\n')

    def rebuild_list_References(self):
        self.form.list_References.clear()
        items = []
        for ref in self.references:
            item_name = ref[0].Name + ':' + ref[1]
            items.append(item_name)
        for listItemName in sorted(items):
            self.form.list_References.addItem(listItemName)

    def select_clicked_reference_shape(self):
        self.setback_listobj_visibility()
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
            self.sel_server = None
        if not self.sel_server:
            if not self.references:
                return
            currentItemName = str(self.form.list_References.currentItem().text())
            for ref in self.references:
                refname_to_compare_listentry = ref[0].Name + ':' + ref[1]
                if refname_to_compare_listentry == currentItemName:
                    # print( 'found: shape: ' + ref[0].Name + ' element: ' + ref[1])
                    if not ref[0].ViewObject.Visibility:
                        self.obj_notvisible.append(ref[0])
                        ref[0].ViewObject.Visibility = True
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(ref[0], ref[1])

    def setback_listobj_visibility(self):
        '''set back Visibility of the list objects
        '''
        for obj in self.obj_notvisible:
            obj.ViewObject.Visibility = False
        self.obj_notvisible = []
