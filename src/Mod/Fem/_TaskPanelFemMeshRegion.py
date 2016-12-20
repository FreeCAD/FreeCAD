# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_TaskPanelFemMeshRegion"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package TaskPanelFemMeshRegion
#  \ingroup FEM

import FreeCAD
import FreeCADGui
from PySide import QtGui
from PySide import QtCore


class _TaskPanelFemMeshRegion:
    '''The TaskPanel for editing References property of FemMeshRegion objects'''
    def __init__(self, obj):
        FreeCADGui.Selection.clearSelection()
        self.sel_server = None
        self.obj = obj
        self.references = []
        if self.obj.References:
            self.tuplereferences = self.obj.References
            self.get_references()

        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/TaskPanelFemMeshRegion.ui")
        QtCore.QObject.connect(self.form.pushButton_Reference, QtCore.SIGNAL("clicked()"), self.add_references)
        self.form.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.list_References.connect(self.form.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)

        self.rebuild_list_References()

    def accept(self):
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        self.obj.References = self.references
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        if self.sel_server:
            FreeCADGui.Selection.removeObserver(self.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()
        return True

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
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        print_message = "Select Edges and Faces by single click on them or Solids by single click on a Vertex to add them to the list"
        import FemSelectionObserver
        self.sel_server = FemSelectionObserver.FemSelectionObserver(self.selectionParser, print_message)

    def selectionParser(self, selection):
        print('selection: ', selection[0].Shape.ShapeType, '  ', selection[0].Name, '  ', selection[1])
        if hasattr(selection[0], "Shape"):
            # get the Shape to mesh
            if len(self.obj.InList) == 1:
                shape_to_mesh = self.obj.InList[0].Part.Shape
                # check if the Shape the selected element belongs to is the Part to mesh of the mesh object
                if shape_to_mesh.isSame(selection[0].Shape):
                    if selection[1]:
                        elt = selection[0].Shape.getElement(selection[1])
                        '''
                        # we need to select a Solid out of a CompSolid !!!
                        # may be by selecting a Face and and Edge which belongs to the solid
                        # we really need some ShapeType filter for selecting shapes
                        if elt.ShapeType == "Vertex":
                            if selection[0].Shape.ShapeType == "Solid":
                                elt = selection[0].Shape
                                selection = (selection[0], '')
                            else:
                                FreeCAD.Console.PrintMessage("Selected Vertex does not belong to a Solid: " + selection[0].Name + " is a " + selection[0].Shape.ShapeType + " \n")
                        '''
                        if elt.ShapeType == "Vertex" or elt.ShapeType == 'Edge' or elt.ShapeType == 'Face' or elt.ShapeType == 'Solid':
                            if selection not in self.references:
                                self.references.append(selection)
                                self.rebuild_list_References()
                            else:
                                FreeCAD.Console.PrintMessage(selection[0].Name + ' --> ' + selection[1] + ' is in reference list already!\n')
                        else:
                            FreeCAD.Console.PrintMessage(selection[0].Name + ' --> ' + selection[1] + ' is a not supported ShapeType: ' + elt.ShapeType + ' \n')
                    else:
                        FreeCAD.Console.PrintError("No selection[1].\n")
                else:
                    FreeCAD.Console.PrintError("The selected element does not belong to the shape to mesh. Select an element of the object: " + self.obj.InList[0].Part.Name + "\n")
            else:
                FreeCAD.Console.PrintMessage(self.obj.Name + ' seam to belong to more than one mesh object. This is not supported.\n')

    def rebuild_list_References(self):
        self.form.list_References.clear()
        items = []
        for ref in self.references:
            item_name = ref[0].Name + ':' + ref[1]
            items.append(item_name)
        for listItemName in sorted(items):
            self.form.list_References.addItem(listItemName)
