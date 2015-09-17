#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtGui
    from PySide import QtCore
    from pivy import coin


__title__ = "FemShellThickness"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


def makeFemShellThickness(thickness=20.0, name="ShellThickness"):
    '''makeFemShellThickness([thickness], [name]): creates an shellthickness object to define a plate thickness'''
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython", name)
    _FemShellThickness(obj)
    obj.Thickness = thickness
    if FreeCAD.GuiUp:
        _ViewProviderFemShellThickness(obj.ViewObject)
    return obj


class _CommandFemShellThickness:
    "The Fem_ShellThickness command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-shell-thickness',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_ShellThickness", "FEM Shell Plate Thickness Definition ..."),
                'Accel': "C, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_ShellThickness", "Creates a FEM Shell Thickness")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemShellThickness")
        FreeCADGui.addModule("FemShellThickness")
        FreeCADGui.doCommand("FemShellThickness.makeFemShellThickness()")
        FreeCADGui.doCommand("App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member = App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member + [App.ActiveDocument.ActiveObject]")

    def IsActive(self):
        True if FemGui.getActiveAnalysis() else False


class _FemShellThickness:
    "The FemShellThickness object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength", "Thickness", "ShellThickness", "set thickness of the shell elements")
        obj.addProperty("App::PropertyLinkSubList", "References", "ShellThickness", "List of shell thickness shapes")
        obj.Proxy = self
        self.Type = "FemShellThickness"

    def execute(self, obj):
        return


class _ViewProviderFemShellThickness:
    "A View Provider for the FemShellThickness object"
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-shell-thickness.svg"

    def attach(self, vobj):
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
        taskd = _FemShellThicknessTaskPanel(self.Object)
        taskd.obj = vobj.Object
        # taskd.update()    When is this needed ?
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        self.setEdit(vobj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _FemShellThicknessTaskPanel:
    '''The TaskPanel for editing References property of FemShellThickness objects'''
    def __init__(self, obj):
        FreeCADGui.Selection.clearSelection()
        self.sel_server = None
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/FemShellThickness.ui")

        QtCore.QObject.connect(self.form.pushButton_Reference, QtCore.SIGNAL("clicked()"), self.add_references)
        self.form.list_References.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.list_References.connect(self.form.list_References, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.references_list_right_clicked)

        self.previous_references = self.obj.References
        self.references = self.obj.References
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
            refname_to_compare_listentry = ref[0].Name + '-->' + ref[1]
            if refname_to_compare_listentry == currentItemName:
                self.references.remove(ref)
        self.rebuild_list_References()

    def add_references(self):
        '''Called if Button add_reference is triggered'''
        # in constraints EditTaskPanel the selection is active as soon as the taskpanel is open
        # here the addReference button EditTaskPanel has to be triggered to start selection mode
        FreeCADGui.Selection.clearSelection()
        # start SelectionObserver and parse the function to add the References to the widget
        self.sel_server = ReferenceShapeSelectionObserver(self.selectionParser)

    def selectionParser(self, selsub):
        sel = selsub[0]
        sub = selsub[1]
        # print 'selection: ', sel.Shape.ShapeType, '  ', sel.Name, '  ', sub
        if hasattr(sel, "Shape"):
            elt = sel.Shape.getElement(sub)
            if elt.ShapeType == 'Face':
                if selsub not in self.references:
                    self.references.append(selsub)
                    self.rebuild_list_References()
                else:
                    print sel.Name, '-->', sub, ' is already in reference list!'

        else:
            print 'Selection has no shape!'

    def rebuild_list_References(self):
        self.form.list_References.clear()
        items = []
        for i in self.references:
            item_name = i[0].Name + '-->' + i[1]
            items.append(item_name)
        for listItemName in sorted(items):
            listItem = QtGui.QListWidgetItem(listItemName, self.form.list_References)  # listItem =   is needed


class ReferenceShapeSelectionObserver:
    '''ReferenceShapeSelectionObserver
       started on click  button addReference'''
    def __init__(self, parseSelectionFunction):
        self.parseSelectionFunction = parseSelectionFunction
        FreeCADGui.Selection.addObserver(self)
        FreeCAD.Console.PrintMessage("Select Faces to add them to the list!\n")

    def addSelection(self, docName, objName, sub, pos):
        selected_object = FreeCAD.getDocument(docName).getObject(objName)  # get the obj objName
        self.added_obj = (selected_object, sub)
        if lsub:         # on doubleClick the solid is selected and sub will be empty
            self.parseSelectionFunction(self.added_obj)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_ShellThickness', _CommandFemShellThickness())
