# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD material editor"
__author__ = "Yorik van Havre, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import os
import sys
from PySide import QtCore, QtGui
# from PySide import QtUiTools, QtSvg

import FreeCAD
import FreeCADGui

# is this still needed after the move to card utils???
if sys.version_info.major >= 3:
    unicode = str


class MaterialEditor:

    def __init__(self, obj=None, prop=None, material=None, card_path=''):

        """Initializes, optionally with an object name and a material property
        name to edit, or directly with a material dictionary."""

        self.obj = obj
        self.prop = prop
        self.material = material
        self.customprops = []
        self.internalprops = []
        self.groups = []
        self.directory = FreeCAD.getResourceDir() + "Mod/Material"
        self.materials = {}
        self.cards = {}
        self.icons = {}
        self.card_path = card_path

        # load the UI file from the same directory as this script
        self.widget = FreeCADGui.PySideUic.loadUi(
            os.path.dirname(__file__) + os.sep + "materials-editor.ui"
        )

        # additional UI fixes and tweaks
        widget = self.widget
        buttonURL = widget.ButtonURL
        buttonDeleteProperty = widget.ButtonDeleteProperty
        buttonAddProperty = widget.ButtonAddProperty
        standardButtons = widget.standardButtons
        buttonOpen = widget.ButtonOpen
        buttonSave = widget.ButtonSave
        comboMaterial = widget.ComboMaterial
        treeView = widget.treeView

        # temporarily hide preview fields, as they are not used yet
        # TODO : implement previews
        widget.PreviewGroup.hide()

        buttonURL.setIcon(QtGui.QIcon(":/icons/internet-web-browser.svg"))
        buttonDeleteProperty.setEnabled(False)
        standardButtons.button(QtGui.QDialogButtonBox.Ok).setAutoDefault(False)
        standardButtons.button(QtGui.QDialogButtonBox.Cancel).setAutoDefault(False)
        self.updateCardsInCombo()
        # TODO allow to enter a custom property by pressing Enter in the lineedit
        # currently closes the dialog

        standardButtons.rejected.connect(self.reject)
        standardButtons.accepted.connect(self.accept)
        buttonOpen.clicked.connect(self.openfile)
        buttonSave.clicked.connect(self.savefile)
        buttonURL.clicked.connect(self.openProductURL)
        comboMaterial.currentIndexChanged[int].connect(self.chooseMaterial)
        buttonAddProperty.clicked.connect(self.addCustomProperty)
        buttonDeleteProperty.clicked.connect(self.deleteCustomProperty)
        treeView.clicked.connect(self.checkDeletable)

        model = QtGui.QStandardItemModel()
        treeView.setModel(model)
        treeView.setUniformRowHeights(True)
        treeView.setItemDelegate(MaterialsDelegate())

        # init model
        self.implementModel()

        # update the editor with the contents of the property, if we have one
        d = None
        if self.prop and self.obj:
            d = FreeCAD.ActiveDocument.getObject(self.obj).getPropertyByName(self.prop)
        elif self.material:
            d = self.material

        if d:
            self.updateMatParamsInTree(d)
            self.widget.ComboMaterial.setCurrentIndex(0)
            # set after tree params to the none material

        if self.card_path:
            # we need the index of this path
            index = self.widget.ComboMaterial.findData(self.card_path)
            self.chooseMaterial(index)

        # TODO: What if material and card_name was given?
        # In such case ATM material is chosen, give some feedback for all those corner cases.

    def implementModel(self):

        '''implements the model with the material attribute structure.'''

        widget = self.widget
        treeView = widget.treeView
        model = treeView.model()
        model.setHorizontalHeaderLabels(["Property", "Value", "Type"])

        treeView.setColumnWidth(0, 250)
        treeView.setColumnWidth(1, 250)
        treeView.setColumnHidden(2, True)

        from materialtools.cardutils import get_material_template
        template_data = get_material_template(True)

        for group in template_data:
            gg = list(group.keys())[0]  # group dict has only one key
            top = QtGui.QStandardItem(gg)
            model.appendRow([top])
            self.groups.append(gg)

            for properName in group[gg]:
                pp = properName  # property name
                item = QtGui.QStandardItem(pp)
                self.internalprops.append(pp)

                it = QtGui.QStandardItem()

                tt = group[gg][properName]['Type']
                itType = QtGui.QStandardItem(tt)

                top.appendRow([item, it, itType])

            top.sortChildren(0)

        treeView.expandAll()

    def updateMatParamsInTree(self, data):

        '''updates the contents of the editor with the given dictionary
           the material property keys where added to the editor already
           not known material property keys will be added to the user defined group'''

        # print(data)
        model = self.widget.treeView.model()
        root = model.invisibleRootItem()
        for gg in range(root.rowCount() - 1):
            group = root.child(gg, 0)
            for pp in range(group.rowCount()):
                item = group.child(pp, 0)
                it = group.child(pp, 1)
                kk = self.collapseKey(item.text())

                try:
                    value = data[kk]
                    it.setText(value)
                    del data[kk]
                except KeyError:
                    it.setText("")

        userGroup = root.child(gg + 1, 0)
        userGroup.setRowCount(0)
        self.customprops = []

        for k, i in data.items():
            k = self.expandKey(k)
            item = QtGui.QStandardItem(k)
            it = QtGui.QStandardItem(i)
            userGroup.appendRow([item, it])
            self.customprops.append(k)

    def chooseMaterial(self, index):
        if index < 0:
            return
        self.card_path = self.widget.ComboMaterial.itemData(index)
        FreeCAD.Console.PrintMessage(
            'choose_material in material editor:\n'
            '    {}\n'.format(self.card_path)
        )
        if os.path.isfile(self.card_path):
            from importFCMat import read
            d = read(self.card_path)
            self.updateMatParamsInTree(d)
            # be careful with reading from materials dict
            # the card could be updated the dict not
            self.widget.ComboMaterial.setCurrentIndex(index)  # set after tree params
        else:
            FreeCAD.Console.PrintError('material card not found: {}\n'.format(self.card_path))

    def updateCardsInCombo(self):

        '''updates the contents of the materials combo with existing material cards'''

        mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
        sort_by_resources = mat_prefs.GetBool("SortByResources", False)

        # get all available materials (fill self.materials, self.cards and self.icons)
        from materialtools.cardutils import import_materials as getmats
        self.materials, self.cards, self.icons = getmats()

        card_name_list = []  # [ [card_name, card_path, icon_path], ... ]

        if sort_by_resources is True:
            for a_path in sorted(self.materials.keys()):
                card_name_list.append([self.cards[a_path], a_path, self.icons[a_path]])
        else:
            card_names_tmp = {}
            for path, name in self.cards.items():
                card_names_tmp[name] = path
            for a_name in sorted(card_names_tmp.keys()):
                a_path = card_names_tmp[a_name]
                card_name_list.append([a_name, a_path, self.icons[a_path]])

        card_name_list.insert(0, [None, '', ''])
        for mat in card_name_list:
            self.widget.ComboMaterial.addItem(QtGui.QIcon(mat[2]), mat[0], mat[1])

    def openProductURL(self):

        '''opens the contents of the ProductURL field in an external browser.'''

        model = self.widget.treeView.model()
        item = model.findItems(translate("Material", "Product URL"),
                               QtCore.Qt.MatchRecursive, 0)[0]
        group = item.parent()
        it = group.child(item.row(), 1)
        url = it.text()
        if url:
            QtGui.QDesktopServices.openUrl(QtCore.QUrl(url, QtCore.QUrl.TolerantMode))

    def accept(self):
        ""

        QtGui.QDialog.accept(self.widget)

    def reject(self):
        ""

        QtGui.QDialog.reject(self.widget)

    def expandKey(self, key):
        "adds spaces before caps in a KeyName"
        nk = ""
        for ln in key:
            if ln.isupper():
                if nk:
                    # this allows for series of caps, such as ProductURL
                    if not nk[-1].isupper():
                        nk += " "
            nk += ln
        return nk

    def collapseKey(self, key):
        "removes the spaces in a Key Name"
        nk = ""
        for ln in key:
            if ln != " ":
                nk += ln
        return nk

    def addCustomProperty(self, key=None, value=None):
        "Adds a custom property to the editor, optionally with a value."

        if not key:
            key = self.widget.EditProperty.text()

        if key:
            model = self.widget.treeView.model()
            item = model.findItems(key, QtCore.Qt.MatchRecursive, 0)
            if not item:

                top = model.findItems(translate("Material", "User defined"),
                                      QtCore.Qt.MatchExactly, 0)[0]
                item = QtGui.QStandardItem(key)
                it = QtGui.QStandardItem(value)
                top.appendRow([item, it])
                self.customprops.append(key)

    def deleteCustomProperty(self, key=None):

        '''Deletes a custom property from the editor,
        or deletes the value of an internal property.'''

        widget = self.widget
        treeView = widget.treeView
        model = treeView.model()
        buttonDeleteProperty = widget.ButtonDeleteProperty

        if not key:

            index = treeView.selectedIndexes()[0]
            item = model.itemFromIndex(index)
            key = item.text()

        if key:
            item = model.findItems(key, QtCore.Qt.MatchRecursive, 0)
            if item:

                index = model.indexFromItem(item[0])
                topIndex = index.parent()
                top = model.itemFromIndex(topIndex)
                row = item[0].row()

                if key in self.customprops:
                    top.takeRow(row)
                    self.customprops.remove(key)
                    buttonDeleteProperty.setProperty("text", "Delete property")

                elif key in self.internalprops:
                    it = top.child(row, 1)
                    it.setText("")
                    buttonDeleteProperty.setProperty("text", "Delete value")

        buttonDeleteProperty.setEnabled(False)

    def checkDeletable(self, index):

        '''Checks if the current item is a custom or an internal property,
        and enable the delete property or delete value button.'''

        widget = self.widget
        buttonDeleteProperty = widget.ButtonDeleteProperty
        treeView = widget.treeView
        model = treeView.model()
        ind = treeView.selectedIndexes()[0]
        item = model.itemFromIndex(ind)
        text = item.text()

        if text in self.customprops:
            buttonDeleteProperty.setEnabled(True)
            buttonDeleteProperty.setProperty("text", "Delete property")

        elif text in self.internalprops:
            indParent = ind.parent()
            group = model.itemFromIndex(indParent)
            row = item.row()
            it = group.child(row, 1)
            buttonDeleteProperty.setProperty("text", "Delete value")
            if it.text():
                buttonDeleteProperty.setEnabled(True)
            else:
                buttonDeleteProperty.setEnabled(False)

        else:
            buttonDeleteProperty.setEnabled(False)
            buttonDeleteProperty.setProperty("text", "Delete property")

    def getDict(self):
        "returns a dictionary from the contents of the editor."

        model = self.widget.treeView.model()
        root = model.invisibleRootItem()

        d = {}
        for gg in range(root.rowCount()):
            group = root.child(gg)
            for row in range(group.rowCount()):
                kk = group.child(row, 0).text()
                ii = group.child(row, 1).text()

                # TODO the following should be translated back to english
                # since text(0) could be translated
                matkey = self.collapseKey(str(kk))
                matvalue = unicode(ii)
                if matvalue or (matkey == 'Name'):
                    # use only keys which are not empty and the name even if empty
                    d[matkey] = matvalue
        # self.outputDict(d)
        return d

    def outputDict(self, d):
        print('MaterialEditor dictionary')
        for param in d:
            print('  {} : {}'.format(param, d[param]))

    '''
    def setTexture(self, pattern):
        "displays a texture preview if needed"
        self.widget.PreviewVector.hide()
        if pattern:
            try:
                import DrawingPatterns
            except:
                print("DrawingPatterns not found")
            else:
                pattern = DrawingPatterns.buildFileSwatch(pattern, size=96, png=True)
                if pattern:
                    self.widget.PreviewVector.setPixmap(QtGui.QPixmap(pattern))
                    self.widget.PreviewVector.show()
    '''

    def openfile(self):
        "Opens a FCMat file"
        filetuple = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            'Open FreeCAD Material file',
            self.directory,
            '*.FCMat'
        )
        self.card_path = filetuple[0]
        index = self.widget.ComboMaterial.findData(self.card_path)
        print(index)

        # check if card_path is in known path, means it is in combo box already
        # if not print message, and give some feedbach that the card parameter are loaded
        if os.path.isfile(self.card_path):
            if index == -1:
                FreeCAD.Console.PrintMessage(
                    'Card path: {} not found in known cards.'
                    'The material parameter only are loaded.\n'
                    .format(self.card_path)
                )
                from importFCMat import read
                d = read(self.card_path)
                if d:
                    self.updateMatParamsInTree(d)
                    self.widget.ComboMaterial.setCurrentIndex(0)
                    # set combo box to the none material after tree params
            else:
                self.chooseMaterial(index)
        self.directory = os.path.dirname(self.card_path)

    def savefile(self):
        "Saves a FCMat file."

        model = self.widget.treeView.model()
        item = model.findItems(translate("Material", "Name"),
                               QtCore.Qt.MatchRecursive, 0)[0]
        group = item.parent()
        it = group.child(item.row(), 1)
        name = it.text()
        if sys.version_info.major < 3:
            if isinstance(name, unicode):
                name = name.encode("utf8")
        if not name:
            name = "Material"
        filetuple = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(),
            'Save FreeCAD Material file',
            self.directory + '/' + name + '.FCMat',
            '*.FCMat'
        )
        # a tuple of two empty strings returns True, so use the filename directly
        filename = filetuple[0]
        if filename:
            self.directory = os.path.dirname(filename)
            # should not be resource dir but user result dir instead
            d = self.getDict()
            # self.outputDict(d)
            if d:
                from importFCMat import write
                write(filename, d)
                self.updateCardsInCombo()

    def show(self):
        return self.widget.show()

    def exec_(self):
        return self.widget.exec_()


class MaterialsDelegate(QtGui.QStyledItemDelegate):

    '''provides display and editing facilities for data items from a model.'''

    def __init__(self):
        ""

        super(MaterialsDelegate, self).__init__()

    def createEditor(self, parent, option, index):

        '''returns the widget used to change data from the model.'''

        model = index.model()
        column = index.column()

        item = model.itemFromIndex(index)
        group = item.parent()
        if not group:
            return

        if column == 1:

            row = index.row()

            PP = group.child(row, 0)
            matproperty = PP.text().replace(" ", "")  # remove spaces

            TT = group.child(row, 2)

            if TT:
                Type = TT.text()

            else:
                Type = "String"

            VV = group.child(row, 1)
            Value = VV.text()

            editor = matProperWidget(parent, matproperty, Type, Value)

        elif column == 0:

            if group.text() == "User defined":
                editor = matProperWidget(parent)

            else:
                return

        else:

            return

        return editor

    def setEditorData(self, editor, index):

        '''provides the widget with data to manipulate.'''

        Type = editor.property('Type')
        model = index.model()
        item = model.itemFromIndex(index)

        if Type == "Color":

            color = editor.property('color')
            color = color.getRgb()
            item.setText(str(color))

        elif Type == "File":

            lineEdit = editor.children()[1]
            item.setText(lineEdit.text())

        else:

            super(MaterialsDelegate, self).setEditorData(editor, index)


ui = FreeCADGui.UiLoader()


def matProperWidget(parent=None, matproperty=None, Type="String", Value=None,
                    minimum=None, maximum=None, stepsize=None, precision=None):

    '''customs widgets for the material stuff.'''

    if Type == "String":

        widget = ui.createWidget("Gui::PrefLineEdit")

    elif Type == "URL":

        widget = ui.createWidget("Gui::PrefLineEdit")

    elif Type == "File":

        widget = ui.createWidget("Gui::FileChooser")
        if Value:
            lineEdit = widget.children()[1]
            lineEdit.setText(Value)

    elif Type == "Quantity":

        widget = ui.createWidget("Gui::InputField")
        if hasattr(FreeCAD.Units, matproperty):
            unit = getattr(FreeCAD.Units, matproperty)
            quantity = FreeCAD.Units.Quantity(1, unit)
            widget.setProperty('unit', quantity.getUserPreferred()[2])
        else:
            FreeCAD.Console.PrintError('Not known unit for property: {}\n'.format(matproperty))

    elif Type == "Integer":

        widget = ui.createWidget("Gui::UIntSpinBox")

    elif Type == "Float":

        widget = ui.createWidget("Gui::PrefDoubleSpinBox")

    elif Type == "Enumerator":

        widget = ui.createWidget("Gui::PrefComboBox")

    elif Type == "Boolean":

        widget = ui.createWidget("Gui::PrefComboBox")
        widget.insertItems(0, ['', 'False', 'True'])

    elif Type == "Vector":

        widget = ui.createWidget("Gui::PrefLineEdit")

    elif Type == "Color":

        widget = ui.createWidget("Gui::PrefColorButton")
        if Value:
            value = string2tuple(Value)
            color = QtGui.QColor()
            color.setRgb(value[0], value[1], value[2], value[3])
            widget.setProperty('color', color)

    else:

        widget = QtGui.QLineEdit()

    if minimum is not None:
        widget.setProperty('minimum', minimum)
    if maximum is not None:
        widget.setProperty('maximum', maximum)
    if stepsize is not None:
        widget.setProperty('stepsize', stepsize)
    if precision is not None:
        widget.setProperty('precision', precision)

    widget.setProperty('Type', Type)

    widget.setParent(parent)

    return widget


def string2tuple(string):
    "provisionally"
    value = string[1:-1]
    value = value.split(',')
    value = [int(v) for v in value]
    value = tuple(value)
    return value


def translate(context, text):
    "translates text"
    return text  # TODO use Qt translation mechanism here


def openEditor(obj=None, prop=None):
    """openEditor([obj,prop]): opens the editor, optionally with
    an object name and material property name to edit"""
    editor = MaterialEditor(obj, prop)
    editor.exec_()


def editMaterial(material=None, card_path=None):
    """editMaterial(material): opens the editor to edit the contents
    of the given material dictionary. Returns the modified material dictionary."""
    # if the material editor is opened with this def and the card_path is None
    # the combo box with the card name is empty
    # this makes sense, because the editor was not opened with a card_path,
    # but with material dictionary instead
    # TODO: add some text in combo box, may be "custom material data" or "user material data"
    # TODO: if card_path is None, all known cards could be checked,
    # if one fits exact ALL provided data, this card name could be displayed
    editor = MaterialEditor(material=material, card_path=card_path)
    result = editor.exec_()
    if result:
        return editor.getDict()
    else:
        # on cancel button an empty dict is returned
        return {}


'''
# some examples how to open the material editor in Python:
import MaterialEditor
MaterialEditor.openEditor()

doc = FreeCAD.open(
    FreeCAD.ConfigGet("AppHomePath") + 'data/examples/FemCalculixCantilever3D.FCStd'
)
import MaterialEditor
MaterialEditor.openEditor('SolidMaterial', 'Material')

import MaterialEditor
MaterialEditor.editMaterial({
    'Density': '1234.0 kg/m^3',
    'Name': 'My-Material-Data',
    'PoissonRatio': '0.66',
    'YoungsModulus': '123456 MPa'
})

import MaterialEditor
MaterialEditor.editMaterial('ABS')

'''
