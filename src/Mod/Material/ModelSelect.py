# ***************************************************************************
# *   Copyright (c) 2023 David Carter <dcarter@dvidcarter.ca>               *
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
__author__ = "David Carter"
__url__ = "http://www.freecad.org"

import os
from pathlib import PurePath, Path
import sys
from PySide import QtCore, QtGui, QtSvg

import FreeCAD
import FreeCADGui
# import Material_rc
from materialtools.cardutils import get_material_preferred_directory, get_material_preferred_save_directory
import Material


# is this still needed after the move to card utils???
unicode = str


# ************************************************************************************************
# ************************************************************************************************
class ModelSelect:

    # def __init__(self, obj=None, prop=None, material=None, card_path="", category="Solid"):
    def __init__(self):

        filePath = os.path.dirname(__file__) + os.sep
        self.iconPath = (filePath + "Resources" + os.sep + "icons" + os.sep)

        # load the UI file from the same directory as this script
        self.widget = FreeCADGui.PySideUic.loadUi(filePath + "Resources" + os.sep + "ui" + os.sep + "ModelSelect.ui")
        # remove unused Help button
        self.widget.setWindowFlags(self.widget.windowFlags()
                                   & ~QtCore.Qt.WindowContextHelpButtonHint)

        # restore size and position
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        width = param.GetInt("ModelSelectEditorWidth", 441)
        height = param.GetInt("ModelSelectEditorHeight", 626)
        self.widget.resize(width, height)

        widget = self.widget
        standardButtons = widget.standardButtons
        treeView = widget.treeView
        treeView.setHeaderHidden(True)

        standardButtons.accepted.connect(self.accept)
        standardButtons.rejected.connect(self.reject)
        treeView.clicked.connect(self.onClickTree)

        self.createModelTree()

        self.selectedItem = None

        from materialtools.MaterialModels import getModelLibraries
        getModelLibraries()

        # init model
        # self.implementModel()

    def createModelTree(self):

        """implements the model with the material attribute structure."""

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        widget = self.widget
        treeView = widget.treeView
        model = QtGui.QStandardItemModel()
        treeView.setModel(model)
        treeView.setUniformRowHeights(True)

        folder = Path(FreeCAD.getResourceDir()) / "Mod/Material/Resources/Models"
        # resources["System"] = (builtin_mat_dir, ":/icons/freecad.svg")

        root = model.invisibleRootItem()
        mgr=Material.ModelManager()
        libraries = mgr.ModelLibraries
        for library in libraries:
            folder = Path(library[1])
            top = QtGui.QStandardItem(library[0])
            self.widget.treeView.setExpanded(root.index(), True)
            root.appendRow(top)
            self.addModels(top, folder, p)

        # top = QtGui.QStandardItem(gg)
        # model.appendRow([top])

    def addModels(self, top, folder, param):
        """
            Recursively add library folders and models
        """

        for child in folder.iterdir():
            if child.is_dir():
                # item = QtGui.QTreeWidgetItem(top, [str(child.stem())])
                print("Add model dir '{0}'".format(str(child.stem)))
                item = QtGui.QStandardItem(str(child.stem))
                top.appendRow([item])
                self.widget.treeView.setExpanded(top.index(), True)
                self.addModels(item, folder / child.stem, param)
            else:
                lower = child.suffix.lower()
                if lower == ".yml":
                    print("Add model '{0}'".format(str(child.stem)))
                    item = QtGui.QStandardItem(str(child.stem))
                    # item.setIcon(0, icon)
                    top.appendRow([item])
                    
                    self.widget.treeView.setExpanded(top.index(), True)

                    # # Save the relative path of the card as user data
                    # userData = {}
                    # userData["library"] = library
                    # userData["path"] = child.relative_to(library)
                    # # print("User data: [{0}, {1}]".format(userData["library"], userData["path"]))
                    # item.setData(0, QtCore.Qt.UserRole, userData)
                    # # widget_item.data(0, Qt.UserRole).toPyObject()    

    def onClickTree(self, index):

        """Checks if the current item is a custom or an internal property,
        and enable the delete property or delete value button."""

        # userData = current.data(0, QtCore.Qt.UserRole) #.toPyObject()
        # if userData is not None:
        #     # print("User data: [{0}, {1}]".format(userData["library"], userData["path"]))
        #     self.chooseMaterial(Path(userData["library"]) / userData["path"])
        # else:
        #     # print("User data: None")
        #     pass
        widget = self.widget
        treeView = widget.treeView
        model = treeView.model()
        ind = treeView.selectedIndexes()[0]
        item = model.itemFromIndex(ind)
        text = item.text()
        print("Selected '{0}'".format(text))

        self.selectedItem = item

    def accept(self):
        ""

        self.storeSize()
        # self.selectedItem = None

        QtGui.QDialog.accept(self.widget)

    def reject(self):
        ""

        self.storeSize()
        QtGui.QDialog.reject(self.widget)

    def storeSize(self):
        "stores the widget size"
        # store widths
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        p.SetInt("ModelSelectEditorWidth", self.widget.width())
        p.SetInt("ModelSelectEditorHeight", self.widget.height())


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

    def show(self):
        return self.widget.show()

    def exec_(self):
        if self.widget.exec_():
            item = self.selectedItem
            if item is not None:
                print("ModelSelect item selected '{0}'".format(item.text()))
            else:
                print("No model selected")
        else:
            print("Model select canceled")


# ************************************************************************************************
# ************************************************************************************************
class MaterialsDelegate(QtGui.QStyledItemDelegate):

    """provides display and editing facilities for data items from a model."""

    def __init__(self):
        ""

        self.matproperty = ""
        self.Value = ""
        super(MaterialsDelegate, self).__init__()

    def createEditor(self, parent, option, index):

        """returns the widget used to change data from the model."""

        model = index.model()
        column = index.column()

        item = model.itemFromIndex(index)
        group = item.parent()
        if not group:
            return

        if column == 1:
            row = index.row()

            PP = group.child(row, 0)
            self.matproperty = PP.text().replace(" ", "")  # remove spaces

            TT = group.child(row, 2)
            if TT:
                Type = TT.text()
            else:
                Type = "String"

            VV = group.child(row, 1)
            self.Value = VV.text()

            editor = matProperWidget(parent, self.matproperty, Type, self.Value)

        elif column == 0:
            if group.text() == "User defined":
                editor = matProperWidget(parent)
            else:
                return

        else:
            return

        return editor

    def setEditorData(self, editor, index):

        """provides the widget with data to manipulate."""

        Type = editor.property("Type")
        model = index.model()
        item = model.itemFromIndex(index)

        if Type == "Color":
            color = editor.property("color")
            color = tuple([v/255.0 for v in color.getRgb()])
            item.setText(str(color))

        elif Type == "File":
            lineEdit = editor.children()[1]
            item.setText(lineEdit.text())

        elif Type == "Float":
            # avoid rounding artifacts
            inputValue = float('%.6g' % editor.value())
            item.setText(str(inputValue))

        elif Type == "Quantity":
            if not hasattr(FreeCAD.Units, self.matproperty):
                FreeCAD.Console.PrintError(
                    "Error: property '{}' is a quantity but has no unit defined\n"
                    .format(self.matproperty)
                )
                return
            # we must use the unit of the input value because the
            # "Gui::QuantitySpinBox" uses e.g. for the density always the mm-based unit
            # kg/mm^3, also when the input value is in kg/^3.
            # E.g. when the input is e.g. "7875 kg/m^3" and we would pass "7875" as rawValue
            # and "kg/m^3" as unit, we would get "7875 kg/mm^3" as result. If we try to be
            # clever and input "7875e-6" as rawValue and "kg/m^3" as unit we get
            # "7875e-6 kg/m^3" as result. If we input "7875e-6" as rawValue and "kg/mm^3"
            # as unit we get also "7875e-6 kg/m^3" as result.
            if not self.Value:
                # for empty (not yet set properties) we use the matproperty unit
                unit = getattr(FreeCAD.Units, self.matproperty)
                quantity = FreeCAD.Units.Quantity(1, unit)
                item.setText(str(editor.value()) + " " + quantity.getUserPreferred()[2])
            else:
                # since we checked we have a quantity, we can use split() to get the unit
                item.setText(str(editor.value()) + " " + self.Value.split()[1])

        else:
            super(MaterialsDelegate, self).setEditorData(editor, index)


# ************************************************************************************************
# ************************************************************************************************
def matProperWidget(parent=None, matproperty=None, Type="String", Value=None,
                    minimum=None, maximum=None, stepsize=None, precision=12):

    """customs widgets for the material stuff."""

    # FIXME
    # Workaround for problem from here:
    # https://forum.freecad.org/viewtopic.php?f=18&t=56912&start=20#p516811
    # set precision to 12
    # Better would be a similar system like used in FEM material task panel
    # if the value in the InputField has not changed the data is not changed
    # why does the value changes if the user clicks in
    # the value and unit should exact stay as it is displayed before the user
    # clicks in the field

    # the user defined properties are of Type String and thus uses a "Gui::PrefLineEdit"

    ui = FreeCADGui.UiLoader()

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
        # We don't use a Gui::QuantitySpinBox widget because depending on the value,
        # the unit might change. For some inputs there is no way to do this right,
        # see the comment above in "def setEditorData". Moreover it is error-prone to provide
        # a unit as in the Gui::QuantitySpinBox widget because users likely delete the unit or
        # change it accidentally. It is therefore better not to display the unit while editing.
        widget = ui.createWidget("Gui::DoubleSpinBox")

        if minimum is None:
            widget.setMinimum(-1*sys.float_info.max)
        else:
            widget.setMinimum(minimum)
        if maximum is None:
            widget.setMaximum(sys.float_info.max)
        else:
            widget.setMaximum(maximum)

        # we must increase the digits before we can set the value
        # 6 is sufficient as some metarial cards use e.g. "0.000011"
        widget.setDecimals(6)

        # for properties with an underscored number (vectorial values),
        # we must strip the part after the first underscore to obtain the bound unit
        # since in cardutils.py in def get_material_template
        # the underscores were removed, we must check for numbers
        import re
        if re.search(r'\d', matproperty):
            matpropertyNum = matproperty.rstrip('0123456789')
            matproperty = matpropertyNum

        if Value:
            widget.setValue(float(Value.split()[0]))

    elif Type == "Integer":
        widget = ui.createWidget("Gui::UIntSpinBox")
        if minimum is None:
            widget.setMinimum(0)
        else:
            widget.setMinimum(minimum)
        if maximum is None:
            widget.setMaximum(sys.maxsize)
        else:
            widget.setMaximum(maximum)

    elif Type == "Float":
        widget = ui.createWidget("Gui::DoubleSpinBox")
        # the magnetic permeability is the parameter for which many decimals matter
        # the most however, even for this, 6 digits are sufficient
        widget.setDecimals(6)
        # for almost all Float parameters of materials a step of 1 would be too large
        widget.setSingleStep(0.1)
        if minimum is None:
            widget.setMinimum(sys.float_info.min)
        else:
            widget.setMinimum(minimum)
        if maximum is None:
            widget.setMaximum(sys.float_info.max)
        else:
            widget.setMaximum(maximum)
        widget.setValue(float(Value))

    elif Type == "Enumerator":
        widget = ui.createWidget("Gui::PrefComboBox")

    elif Type == "Boolean":
        widget = ui.createWidget("Gui::PrefComboBox")
        widget.insertItems(0, ["", "False", "True"])

    elif Type == "Vector":
        widget = ui.createWidget("Gui::PrefLineEdit")

    elif Type == "Color":
        widget = ui.createWidget("Gui::PrefColorButton")
        if Value:
            value = string2tuple(Value)
            color = QtGui.QColor()
            color.setRgb(value[0], value[1], value[2], value[3])
            widget.setProperty("color", color)

    else:
        widget = QtGui.QLineEdit()

    widget.setProperty("Type", Type)

    widget.setParent(parent)

    return widget


def string2tuple(string):
    "provisionally"
    value = string[1:-1]
    value = value.split(",")
    value = [int(float(v)*255) for v in value]
    value = tuple(value)
    return value


def translate(context, text):
    "translates text"
    return text  # TODO use Qt translation mechanism here


def openEditor(obj=None, prop=None):
    """openEditor([obj,prop]): opens the editor, optionally with
    an object name and material property name to edit"""
    editor = ModelSelect()
    editor.exec_()
