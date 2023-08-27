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
__url__ = "http://www.freecad.org"

import os
from pathlib import PurePath
import sys
from PySide import QtCore, QtGui, QtSvg

import FreeCAD
import FreeCADGui
# import Material_rc
from materialtools.cardutils import get_material_preferred_directory, get_material_preferred_save_directory


# is this still needed after the move to card utils???
unicode = str


# ************************************************************************************************
# ************************************************************************************************
class MaterialEditor:

    def __init__(self, obj=None, prop=None, material=None, card_path="", category="Solid"):

        """Initializes, optionally with an object name and a material property
        name to edit, or directly with a material dictionary."""

        self.obj = obj
        self.prop = prop
        self.category = category
        self.material = material
        self.customprops = []
        self.internalprops = []
        self.groups = []
        self.directory = get_material_preferred_directory()
        self.save_directory = get_material_preferred_save_directory()
        if self.directory is None:
            self.directory = FreeCAD.getResourceDir() + "Mod/Material"
        self.materials = {}
        self.cards = {}
        self.icons = {}
        self.initialIndex = -1
        self.edited = False
        self.card_path = card_path
        filePath = os.path.dirname(__file__) + os.sep
        self.iconPath = (filePath + "Resources" + os.sep + "icons" + os.sep)

        # load the UI file from the same directory as this script
        self.widget = FreeCADGui.PySideUic.loadUi(filePath + "Resources" + os.sep + "ui" + os.sep + "materials-editor.ui")
        # remove unused Help button
        self.widget.setWindowFlags(self.widget.windowFlags()
                                   & ~QtCore.Qt.WindowContextHelpButtonHint)

        # restore size and position
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        width = param.GetInt("MaterialEditorWidth", 441)
        height = param.GetInt("MaterialEditorHeight", 626)
        self.widget.resize(width, height)

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

        # create preview svg slots
        self.widget.PreviewRender = QtSvg.QSvgWidget(self.iconPath + "preview-rendered.svg")
        self.widget.PreviewRender.setMaximumWidth(64)
        self.widget.PreviewRender.setMinimumHeight(64)
        self.widget.topLayout.addWidget(self.widget.PreviewRender)
        self.widget.PreviewVector = QtSvg.QSvgWidget(self.iconPath + "preview-vector.svg")
        self.widget.PreviewVector.setMaximumWidth(64)
        self.widget.PreviewVector.setMinimumHeight(64)
        self.widget.topLayout.addWidget(self.widget.PreviewVector)
        self.updatePreviews(mat=material)

        buttonURL.setIcon(QtGui.QIcon(":/icons/internet-web-browser.svg"))
        buttonDeleteProperty.setEnabled(False)
        standardButtons.button(QtGui.QDialogButtonBox.Ok).setAutoDefault(False)
        standardButtons.button(QtGui.QDialogButtonBox.Cancel).setAutoDefault(False)
        self.updateCardsInCombo()
        # TODO allow to enter a custom property by pressing Enter in the lineedit
        # currently closes the dialog

        standardButtons.rejected.connect(self.reject)
        standardButtons.button(QtGui.QDialogButtonBox.Ok).clicked.connect(self.verify)
        buttonOpen.clicked.connect(self.openfile)
        buttonSave.clicked.connect(self.savefile)
        buttonURL.clicked.connect(self.openProductURL)
        comboMaterial.currentIndexChanged[int].connect(self.chooseMaterial)
        buttonAddProperty.clicked.connect(self.addCustomProperty)
        buttonDeleteProperty.clicked.connect(self.deleteCustomProperty)
        treeView.clicked.connect(self.onClickTree)

        model = QtGui.QStandardItemModel()
        treeView.setModel(model)
        treeView.setUniformRowHeights(True)
        treeView.setItemDelegate(MaterialsDelegate())
        model.itemChanged.connect(self.modelChange)

        # init model
        self.implementModel()

        # update the editor with the contents of the property, if we have one
        matProperty = None
        if self.prop and self.obj:
            matProperty = FreeCAD.ActiveDocument.getObject(self.obj).getPropertyByName(self.prop)
        elif self.material:
            matProperty = self.material

        if matProperty:
            self.updateMatParamsInTree(matProperty)
            self.widget.ComboMaterial.setCurrentIndex(0)
            # set after tree params to the none material

        if self.card_path:
            # we need the index of this path
            self.initialIndex = self.widget.ComboMaterial.findData(self.card_path)
            self.chooseMaterial(self.initialIndex)

        # TODO: What if material and card_name was given?
        # In such case ATM material is chosen, give some feedback for all those corner cases.

    def implementModel(self):

        """implements the model with the material attribute structure."""

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
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
            gg = list(group)[0]  # group dict has only one key
            top = QtGui.QStandardItem(gg)
            model.appendRow([top])
            self.groups.append(gg)

            for properName in group[gg]:
                pp = properName  # property name
                item = QtGui.QStandardItem(pp)
                item.setToolTip(group[gg][properName]["Description"])
                self.internalprops.append(pp)

                it = QtGui.QStandardItem()
                it.setToolTip(group[gg][properName]["Description"])

                tt = group[gg][properName]["Type"]
                itType = QtGui.QStandardItem(tt)

                top.appendRow([item, it, itType])
                treeView.setExpanded(top.index(), p.GetBool("TreeExpand"+gg, True))
            # top.sortChildren(0)

        # treeView.expandAll()
        self.edited = False

    def updateMatParamsInTree(self, data):

        """updates the contents of the editor with the given dictionary
           the material property keys where added to the editor already
           unknown material property keys will be added to the user defined group"""

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
                    # treat here changes in Material Card Template
                    # Norm -> StandardCode
                    if (kk == "Standard Code") and ("Norm" in data) and data["Norm"]:
                        it.setText(data["Norm"])
                        del data["Norm"]
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
        self.edited = False

    def chooseMaterial(self, index):
        if index < 0:
            return

        if self.verifyMaterial():
            """
                Save any unchanged data
            """
            self.edited = False
        else:
            return

        self.card_path = self.widget.ComboMaterial.itemData(index)
        FreeCAD.Console.PrintMessage(
            "choose_material in material editor:\n"
            "    {}\n".format(self.card_path)
        )
        if os.path.isfile(self.card_path):
            from importFCMat import read
            d = read(self.card_path)
            self.updateMatParamsInTree(d)
            # be careful with reading from materials dict
            # the card could be updated the dict not
            self.widget.ComboMaterial.setCurrentIndex(index)  # set after tree params
        else:
            FreeCAD.Console.PrintError("Material card not found: {}\n".format(self.card_path))

    def verifyMaterial(self):
        if self.edited:
            reply = QtGui.QMessageBox.question(self.widget, #FreeCADGui.getMainWindow(),
                                                translate("Material","The document has been modified."),
                                                translate("Material","Do you want to save your changes?"),
                                                QtGui.QMessageBox.Save | QtGui.QMessageBox.Discard | QtGui.QMessageBox.Cancel,
                                                QtGui.QMessageBox.Save)

            if reply == QtGui.QMessageBox.Cancel:
                return False
            if reply == QtGui.QMessageBox.Save:
                self.savefile()

        return True

    def updateCardsInCombo(self):

        """updates the contents of the materials combo with existing material cards"""

        mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
        sort_by_resources = mat_prefs.GetBool("SortByResources", False)

        # get all available materials (fill self.materials, self.cards and self.icons)
        from materialtools.cardutils import import_materials as getmats
        self.materials, self.cards, self.icons = getmats(category=self.category)

        card_name_list = []  # [ [card_name, card_path, icon_path], ... ]

        if sort_by_resources is True:
            for a_path in sorted(self.materials):
                card_name_list.append([self.cards[a_path], a_path, self.icons[a_path]])
        else:
            card_names_tmp = {}
            for path, name in self.cards.items():
                card_names_tmp[name] = path
            for a_name in sorted(card_names_tmp):
                a_path = card_names_tmp[a_name]
                card_name_list.append([a_name, a_path, self.icons[a_path]])

        card_name_list.insert(0, [None, "", ""])
        for mat in card_name_list:
            self.widget.ComboMaterial.addItem(QtGui.QIcon(mat[2]), mat[0], mat[1])

    def openProductURL(self):

        """opens the contents of the ProductURL field in an external browser."""

        model = self.widget.treeView.model()
        item = model.findItems(translate("Material", "Product URL"),
                               QtCore.Qt.MatchRecursive, 0)[0]
        group = item.parent()
        it = group.child(item.row(), 1)
        url = it.text()
        if url:
            QtGui.QDesktopServices.openUrl(QtCore.QUrl(url, QtCore.QUrl.TolerantMode))

    def modelChange(self, item):
        """
            Called when an item in the tree is modified. This will set edited to True, but this
            will be reset in the event of mass updates, such as loading a card
        """
        self.edited = True

    def verify(self, button):
        """
            Verify that the user wants to save any changed data before exiting
        """

        if self.edited:
            reply = QtGui.QMessageBox.question(self.widget, #FreeCADGui.getMainWindow(),
                                                translate("Material","The document has been modified."),
                                                translate("Material","Do you want to save your changes?"),
                                                QtGui.QMessageBox.Save | QtGui.QMessageBox.Discard | QtGui.QMessageBox.Cancel,
                                                QtGui.QMessageBox.Save)

            if reply == QtGui.QMessageBox.Cancel:
                return
            if reply == QtGui.QMessageBox.Save:
                self.savefile()

        self.accept()

    def accept(self):
        ""

        self.storeSize()
        QtGui.QDialog.accept(self.widget)

    def reject(self):
        ""

        self.storeSize()
        QtGui.QDialog.reject(self.widget)

    def storeSize(self):
        "stores the widget size"
        # store widths
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        p.SetInt("MaterialEditorWidth", self.widget.width())
        p.SetInt("MaterialEditorHeight", self.widget.height())
        root = self.widget.treeView.model().invisibleRootItem()
        for gg in range(root.rowCount()):
            group = root.child(gg)
            p.SetBool("TreeExpand"+group.text(), self.widget.treeView.isExpanded(group.index()))

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

                top = model.findItems(translate("Material", "UserDefined"),
                                      QtCore.Qt.MatchExactly, 0)[0]
                item = QtGui.QStandardItem(key)
                it = QtGui.QStandardItem(value)
                top.appendRow([item, it])
                self.customprops.append(key)
                self.edited = True

    def deleteCustomProperty(self, key=None):

        """Deletes a custom property from the editor,
        or deletes the value of an internal property."""

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

                self.edited = True

        buttonDeleteProperty.setEnabled(False)

    def onClickTree(self, index):

        """Checks if the current item is a custom or an internal property,
        and enable the delete property or delete value button."""

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

        self.updatePreviews()

    def getDict(self):
        "returns a dictionary from the contents of the editor."

        model = self.widget.treeView.model()
        if model is None:
            return {}
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
                if matvalue or (matkey == "Name"):
                    # use only keys which are not empty and the name even if empty
                    d[matkey] = matvalue
        # self.outputDict(d)
        return d

    def outputDict(self, d):
        print("MaterialEditor dictionary")
        for param in d:
            print("  {} : {}".format(param, d[param]))

    """
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
    """

    def updatePreviews(self, mat=None):
        "updates the preview images from the content of the editor"
        if not mat:
            mat = self.getDict()
        diffcol = None
        highlightcol = None
        sectioncol = None
        if "DiffuseColor" in mat:
            diffcol = mat["DiffuseColor"]
        elif "ViewColor" in mat:
            diffcol = mat["ViwColor"]
        elif "Color" in mat:
            diffcol = mat["Color"]
        if "SpecularColor" in mat:
            highlightcol = mat["SpecularColor"]
        if "SectionColor" in mat:
            sectioncol = mat["SectionColor"]
        if diffcol or highlightcol:
            fd = QtCore.QFile(self.iconPath + "preview-rendered.svg")
            if fd.open(QtCore.QIODevice.ReadOnly | QtCore.QIODevice.Text):
                svg = QtCore.QTextStream(fd).readAll()
                fd.close()
                if diffcol:
                    svg = svg.replace("#d3d7cf", self.getColorHash(diffcol, val=255))
                    svg = svg.replace("#555753", self.getColorHash(diffcol, val=125))
                if highlightcol:
                    svg = svg.replace("#fffffe", self.getColorHash(highlightcol, val=255))
                self.widget.PreviewRender.load(QtCore.QByteArray(bytes(svg, encoding="utf8")))
        if diffcol or sectioncol:
            fd = QtCore.QFile(self.iconPath + "preview-vector.svg")
            if fd.open(QtCore.QIODevice.ReadOnly | QtCore.QIODevice.Text):
                svg = QtCore.QTextStream(fd).readAll()
                fd.close()
                if diffcol:
                    svg = svg.replace("#d3d7cf", self.getColorHash(diffcol, val=255))
                    svg = svg.replace("#555753", self.getColorHash(diffcol, val=125))
                if sectioncol:
                    svg = svg.replace("#ffffff", self.getColorHash(sectioncol, val=255))
                self.widget.PreviewVector.load(QtCore.QByteArray(bytes(svg, encoding="utf8")))

    def getColorHash(self, col, val=255):
        "returns a '#000000' string from a '(0.1,0.2,0.3)' string"
        col = [float(x.strip()) for x in col.strip("()").split(",")]
        color = QtGui.QColor(int(col[0]*val), int(col[1]*val), int(col[2]*val))
        return color.name()

    def openfile(self):
        if self.verifyMaterial():
            """
                Save any unchanged data
            """
            self.edited = False
        else:
            return

        "Opens a FCMat file"
        if self.category == "Solid":
            directory = self.directory + os.sep + "StandardMaterial"
        else:
            directory = self.directory + os.sep + "FluidMaterial"
        if self.card_path is None or len(self.card_path) == 0:
            self.card_path = directory
        filetuple = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            "Open FreeCAD Material file",
            self.card_path,
            "*.FCMat"
        )
        if os.path.isfile(filetuple[0]):
            card_path_new = filetuple[0]
        else:
            FreeCAD.Console.PrintError("Error: Invalid path to the material file\n")
            return
        # we cannot simply execute findData(self.card_path) because e.g. on Windows
        # the return path has "/" as folder separator, but the paths in the ComboMaterial
        # have also some "\" in them. For example a path can look like this:
        # D:/FreeCAD-build/data/Mod\Material\FluidMaterial\Air.FCMat
        # To keep it simple, we take a path from the ComboMaterial and change only the
        # material card filename
        #
        # Using the initialIndex variable won't work before a card os selected for the
        # first time, so use index 1. Index 0 is a blank entry
        if self.widget.ComboMaterial.count() > 1:
            path = self.widget.ComboMaterial.itemData(1)
            # at first check if we have a uniform usage
            # (if a character is not present, rsplit delivers the initial string)
            testBackslash = path.rsplit('\\', 1)[0]
            testSlash = path.rsplit('/', 1)[0]
            if testBackslash == path:
                path = testBackslash.rsplit('/', 1)[0] + '/'
            elif testSlash == path:
                path = testSlash.rsplit('\\', 1)[0] + '\\'
            # since we don't know if we have to deal with slash or backslash, take the
            # longest result as path
            else:
                pathBackslash = path.rsplit('\\', 1)[0]
                pathSlash = path.rsplit('/', 1)[0]
                if len(pathBackslash) > len(pathSlash):
                    path = pathBackslash + '\\'
                else:
                    path = pathSlash + '/'
            # we know that card_path_new has uniformly either / or \ but not yet what
            testBackslash = card_path_new.rsplit('\\', 1)[0]
            if testBackslash == card_path_new:
                self.card_path = path + card_path_new.rsplit('/', 1)[1]
            else:
                self.card_path = path + card_path_new.rsplit('\\', 1)[1]
        index = self.widget.ComboMaterial.findData(self.card_path)

        # check if card_path is in known path, means it is in combo box already
        # if not print message, and give some feedbach that the card parameter are loaded
        if index == -1:
            FreeCAD.Console.PrintMessage(
                "Card path: {} not found in known cards.\n"
                "The material parameter only are loaded.\n"
                .format(self.card_path)
            )
            # a material card was chosen that is unknown, thus use its full path
            self.card_path = card_path_new
            from importFCMat import read
            materialDict = read(self.card_path)
            if materialDict:
                self.updateMatParamsInTree(materialDict)
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
        if not name:
            name = "Material"
        filetuple = QtGui.QFileDialog.getSaveFileName(
            QtGui.QApplication.activeWindow(),
            "Save FreeCAD Material file",
            self.save_directory + "/" + name + ".FCMat",
            "*.FCMat"
        )
        # a tuple of two empty strings returns True, so use the filename directly
        filename = filetuple[0]
        if filename:
            # Update the directories to the current save value
            self.save_directory = os.path.dirname(filename)
            self.directory = self.save_directory
            self.card_path = filename

            d = self.getDict()
            # self.outputDict(d)
            if d:
                # Set the card name to match the filename
                path = PurePath(filename)
                d["CardName"] = path.stem

                from importFCMat import write
                write(filename, d)
                self.edited = False
                self.updateCardsInCombo()

    def show(self):
        return self.widget.show()

    def exec_(self):
        return self.widget.exec_()


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
    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
    legacy = param.GetBool("LegacyEditor", True)
    if legacy:
        editor = MaterialEditor(obj, prop)
        editor.exec_()
    else:
        FreeCADGui.runCommand('Materials_Edit',0)


def editMaterial(material=None, card_path=None, category="Solid"):
    """editMaterial(material): opens the editor to edit the contents
    of the given material dictionary. Returns the modified material dictionary."""
    # if the material editor is opened with this def and the card_path is None
    # the combo box with the card name is empty
    # this makes sense, because the editor was not opened with a card_path,
    # but with material dictionary instead
    # TODO: add some text in combo box, may be "custom material data" or "user material data"
    # TODO: if card_path is None, all known cards could be checked,
    # if one fits exact ALL provided data, this card name could be displayed
    editor = MaterialEditor(material=material, card_path=card_path, category=category)
    result = editor.exec_()
    if result:
        return editor.getDict()
    else:
        # on cancel button an empty dict is returned
        return {}


# ************************************************************************************************
# ************************************************************************************************
"""
# some examples how to open the material editor in Python:
import MaterialEditor
MaterialEditor.openEditor()

doc = FreeCAD.open(
    FreeCAD.ConfigGet("AppHomePath") + "data/examples/FemCalculixCantilever3D.FCStd"
)
import MaterialEditor
MaterialEditor.openEditor("SolidMaterial", "Material")

import MaterialEditor
MaterialEditor.editMaterial({
    "Density": "1234.0 kg/m^3",
    "Name": "My-Material-Data",
    "PoissonRatio": "0.66",
    "YoungsModulus": "123456 MPa"
})

import MaterialEditor
MaterialEditor.editMaterial("ABS")

"""
