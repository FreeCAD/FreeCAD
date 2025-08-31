# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""This module contains FreeCAD commands for the BIM workbench"""

import csv
import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")

QPROPS = [
    "Length",
    "Width",
    "Height",
    "Area",
    "HorizontalArea",
    "VerticalArea",
    "Volume",
]
TR_QPROPS = [
    translate("BIM", "Length"),
    translate("BIM", "Width"),
    translate("BIM", "Height"),
    translate("BIM", "Area"),
    translate("BIM", "Horizontal Area"),
    translate("BIM", "Vertical Area"),
    translate("BIM", "Volume"),
]
QTO_TYPES = {
    "IfcQuantityArea": "App::PropertyArea",
    "IfcQuantityCount": "App::PropertyInteger",
    "IfcQuantityLength": "App::PropertyLength",
    "IfcQuantityNumber": "App::PropertyInteger",
    "IfcQuantityTime": "App::PropertyTime",
    "IfcQuantityVolume": "App::PropertyVolume",
    "IfcQuantityWeight": "App::PropertyMass",
}


class BIM_IfcQuantities:

    def GetResources(self):
        return {
            "Pixmap": "BIM_IfcQuantities",
            "MenuText": QT_TRANSLATE_NOOP(
                "BIM_IfcQuantities", "Manage IFC Quantities"
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_IfcQuantities",
                "Manages how the quantities of different elements of the BIM project will be exported to IFC",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        from PySide import QtGui

        # build objects list
        self.objectslist = {}
        self.ifcqtolist = {}
        for obj in FreeCAD.ActiveDocument.Objects:
            role = self.getRole(obj)
            if role:
                self.objectslist[obj.Name] = role
                # support for arrays
                array = self.getArray(obj)
                for i in range(array):
                    if i > 0: # the first item already went above
                        self.objectslist[obj.Name+"+array"+str(i)] = role
        try:
            import ArchIFC

            self.ifcroles = ArchIFC.IfcTypes
        except (ImportError, AttributeError):
            import ArchComponent

            self.ifcroles = ArchComponent.IfcRoles

        # load the form and set the tree model up
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogIfcQuantities.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_IfcQuantities.svg"))
        w = PARAMS.GetInt("BimIfcQuantitiesDialogWidth", 680)
        h = PARAMS.GetInt("BimIfcQuantitiesDialogHeight", 512)
        self.form.resize(w, h)
        self.get_qtos()

        # quantities tab
        self.qmodel = QtGui.QStandardItemModel()
        self.form.quantities.setModel(self.qmodel)
        self.form.quantities.setUniformRowHeights(True)
        self.form.quantities.setItemDelegate(QtGui.QStyledItemDelegate())
        self.qmodel.dataChanged.connect(self.setChecked)
        self.form.buttonBox.accepted.connect(self.accept)
        self.form.quantities.clicked.connect(self.onClickTree)
        if hasattr(self.form.onlyVisible, "checkStateChanged"): # Qt version >= 6.7.0
            self.form.onlyVisible.checkStateChanged.connect(self.update)
        else: # Qt version < 6.7.0
            self.form.onlyVisible.stateChanged.connect(self.update)
        self.form.buttonRefresh.clicked.connect(self.update)
        self.form.buttonApply.clicked.connect(self.add_qto)

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        self.update()
        self.form.show()

    def getArray(self, obj):
        "returns a count number if this object needs to be duplicated"

        import Draft

        if len(obj.InList) == 1:
            parent = obj.InList[0]
            if Draft.getType(parent) == "Array":
                return parent.Count
        return 0

    def decamelize(self, s):
        return "".join([" " + c if c.isupper() else c for c in s]).strip(" ")

    def get_qtos(self):
        "populates the qtos combo box"

        def read_csv(csvfile):
            result = {}
            if os.path.exists(csvfile):
                with open(csvfile, "r") as f:
                    reader = csv.reader(f, delimiter=";")
                    for row in reader:
                        result[row[0]] = row[1:]
            return result

        self.qtodefs = {}
        qtopath = os.path.join(
            FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "qto_definitions.csv"
        )
        custompath = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "CustomQtos.csv")
        self.qtodefs = read_csv(qtopath)
        self.qtodefs.update(read_csv(custompath))
        self.qtokeys = [
            "".join(map(lambda x: x if x.islower() else " " + x, t[4:]))[1:]
            for t in self.qtodefs.keys()
        ]
        self.qtokeys.sort()
        self.form.comboQto.addItems(
            [translate("BIM", "Add quantity set..."),]
            + self.qtokeys
        )

    def add_qto(self):
        "Adds a standard qto set to the todo list"

        index = self.form.comboQto.currentIndex()
        if index <= 0:
            return
        if len(FreeCADGui.Selection.getSelection()) != 1:
            return
        obj = FreeCADGui.Selection.getSelection()[0]
        qto = list(self.qtodefs.keys())[index-1]
        self.ifcqtolist.setdefault(obj.Name, []).append(qto)
        self.update_line(obj.Name, qto)
        FreeCAD.Console.PrintMessage(translate("BIM", "Adding quantity set")+": "+qto+"\n")

    def apply_qto(self, obj, qto):
        "Adds a standard qto set to the object"

        val = self.qtodefs[qto]
        qset = None
        if hasattr(obj, "StepId"):
            from nativeifc import ifc_tools
            ifcfile = ifc_tools.get_ifcfile(obj)
            element = ifc_tools.get_ifc_element(obj)
            if not ifcfile or not element:
                return
            qset = ifc_tools.api_run("pset.add_qto", ifcfile, product=element, name=qto)
        for i in range(0, len(val), 2):
            qname = val[i]
            qtype = QTO_TYPES[val[i+1]]
            if not qname in obj.PropertiesList:
                obj.addProperty(qtype, qname, "Quantities", val[i+1], locked=True)
                qval = 0
                i = self.get_row(obj.Name)
                if i > -1:
                    for j, p in enumerate(QPROPS):
                        it = self.qmodel.item(i, j+1)
                        t = it.text()
                        if t:
                            t = t.replace("²","^2").replace("³","^3")
                            qval = FreeCAD.Units.Quantity(t).Value
                if qval:
                    setattr(obj, qname, qval)
                if hasattr(obj, "StepId") and qset:
                    ifc_tools.api_run("pset.edit_qto", ifcfile, qto=qset, properties={qname: qval})

    def update(self, index=None):
        """updates the tree widgets in all tabs. Index is not used,
        it is just there to match a qt slot requirement"""

        from PySide import QtCore, QtGui
        import Draft

        # quantities tab

        self.qmodel.clear()
        self.qmodel.setHorizontalHeaderLabels(
            [translate("BIM", "Label")] + TR_QPROPS
        )
        self.form.quantities.setColumnWidth(0, 200)  # TODO remember width
        quantheaders = self.form.quantities.header()  # QHeaderView instance
        quantheaders.setSectionsClickable(True)
        quantheaders.sectionClicked.connect(self.quantHeaderClicked)

        # sort by type

        groups = {}
        for name, role in self.objectslist.items():
            groups.setdefault(role, []).append(name)
        for names in groups.values():
            suffix = ""
            for name in names:
                if "+array" in name:
                    name = name.split("+array")[0]
                    suffix = " (duplicate)"
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    if (
                        not self.form.onlyVisible.isChecked()
                    ) or obj.ViewObject.isVisible():
                        if obj.isDerivedFrom("Part::Feature") and not (
                            Draft.getType(obj) == "Site"
                        ):
                            it1 = QtGui.QStandardItem(obj.Label + suffix)
                            it1.setToolTip(name + suffix)
                            it1.setEditable(False)
                            it1.setIcon(obj.ViewObject.Icon)
                            props = []
                            for prop in QPROPS:
                                it = QtGui.QStandardItem()
                                val = None
                                if hasattr(obj, prop) and (
                                    "Hidden" not in obj.getEditorMode(prop)
                                ):
                                    val = self.get_text(obj, prop)
                                    it.setText(val)
                                    it.setCheckable(True)
                                if val != None:
                                    d = None
                                    if hasattr(obj, "IfcAttributes"):
                                        d = obj.IfcAttributes
                                    elif hasattr(obj, "IfcData"):
                                        d = obj.IfcData
                                    if d:
                                        if ("Export" + prop in d) and (
                                            d["Export" + prop] == "True"
                                        ):
                                            it.setCheckState(QtCore.Qt.Checked)
                                    elif self.has_qto(obj, prop):
                                        it.setCheckState(QtCore.Qt.Checked)
                                    if val == 0:
                                        it.setIcon(QtGui.QIcon(":/icons/warning.svg"))
                                self.set_editable(it, prop)
                                props.append(it)
                            self.qmodel.appendRow([it1] + props)

    def has_qto(self, obj, prop):
        """Says if the given object has the given prop in a qto set"""

        if not "StepId" in obj.PropertiesList:
            return False
        from nativeifc import ifc_tools
        element = ifc_tools.get_ifc_element(obj)
        if not element:
            return False
        for rel in getattr(element, "IsDefinedBy", []):
            pset = rel.RelatingPropertyDefinition
            if pset.is_a("IfcElementQuantity"):
                if pset.Name in self.qtodefs:
                    if prop in self.qtodefs[pset.Name]:
                        return True
        return False

    def get_text(self, obj, prop):
        """Gets the text from a property"""

        val = getattr(obj, prop, "0")
        txt = val.getUserPreferred()[0].replace("^2", "²").replace("^3", "³")
        return txt

    def get_row(self, name):
        """Returns the row number corresponding to the given object name"""

        for i in range(self.qmodel.rowCount()):
            if self.qmodel.item(i).toolTip().split(" ")[0] == name:
                return i
        return -1

    def update_line(self, name, qto):
        """Updates a single line of the table, without updating
        the actual object"""

        from PySide import QtCore

        i = self.get_row(name)
        if i == -1:
            return
        obj = FreeCAD.ActiveDocument.getObject(name)
        qto_val = self.qtodefs[qto]
        for j, p in enumerate(QPROPS):
            it = self.qmodel.item(i, j+1)
            if p in obj.PropertiesList:
                val = self.get_text(obj, p)
                it.setText(val)
                self.set_editable(it, p)
                it.setCheckable(True)
            elif p in qto_val:
                it.setText("0")
                it.setCheckable(True)
                it.setCheckState(QtCore.Qt.Checked)
                self.set_editable(it, p)

    def set_editable(self, it, prop):
        """Checks if the given prop should be editable, and sets it"""

        if prop in ["Area", "HorizontalArea", "VerticalArea", "Volume"]:
            it.setEditable(False)
        else:
            it.setEditable(True)

    def getRole(self, obj):
        """gets the IFC class of this object"""

        if hasattr(obj, "IfcType"):
            return obj.IfcType
        elif hasattr(obj, "IfcRole"):
            return obj.IfcRole
        elif hasattr(obj, "IfcClass"):
            return obj.IfcClass
        else:
            return None

    def accept(self):
        """OK pressed"""

        PARAMS.SetInt("BimIfcQuantitiesDialogWidth", self.form.width())
        PARAMS.SetInt("BimIfcQuantitiesDialogHeight", self.form.height())
        self.form.hide()
        changed = False
        if self.ifcqtolist:
            if not changed:
                FreeCAD.ActiveDocument.openTransaction(
                    "Change quantities"
                )
            changed = True
            for key, val in self.ifcqtolist.items():
                obj = FreeCAD.ActiveDocument.getObject(key)
                if obj:
                    for qto in val:
                        self.apply_qto(obj, qto)
        for row in range(self.qmodel.rowCount()):
            name = self.qmodel.item(row, 0).toolTip()
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                for i in range(len(QPROPS)):
                    item = self.qmodel.item(row, i + 1)
                    val = item.text()
                    sav = bool(item.checkState())
                    if i < 3:  # Length, Width, Height, value can be changed
                        if hasattr(obj, QPROPS[i]):
                            if getattr(obj, QPROPS[i]).getUserPreferred()[0] != val:
                                setattr(obj, QPROPS[i], val)
                                if not changed:
                                    FreeCAD.ActiveDocument.openTransaction(
                                        "Change quantities"
                                    )
                                changed = True
                    d = None
                    if hasattr(obj, "IfcAttributes"):
                        d = obj.IfcAttributes
                        att = "IfcAttributes"
                    elif hasattr(obj, "IfcData"):
                        d = obj.IfcData
                        att = "IfcData"
                    if d:
                        if sav:
                            if (not "Export" + QPROPS[i] in d) or (
                                d["Export" + QPROPS[i]] == "False"
                            ):
                                d["Export" + QPROPS[i]] = "True"
                                setattr(obj, att, d)
                                if not changed:
                                    FreeCAD.ActiveDocument.openTransaction(
                                        "Change quantities"
                                    )
                                changed = True
                        else:
                            if "Export" + QPROPS[i] in d:
                                if d["Export" + QPROPS[i]] == "True":
                                    d["Export" + QPROPS[i]] = "False"
                                    setattr(obj, att, d)
                                    if not changed:
                                        FreeCAD.ActiveDocument.openTransaction(
                                            "Change quantities"
                                        )
                                    changed = True
                    elif "StepId" not in obj.PropertiesList:
                        FreeCAD.Console.PrintError(
                            translate(
                                "BIM", "Cannot save quantities settings for object %1"
                            ).replace("%1", obj.Label)
                            + "\n"
                        )

        if changed:
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

    def setChecked(self, id1, id2):
        sel = self.form.quantities.selectedIndexes()
        state = self.qmodel.itemFromIndex(id1).checkState()
        if len(sel) > 7:
            for idx in sel:
                if idx.column() == id1.column():
                    item = self.qmodel.itemFromIndex(idx)
                    if item.checkState() != state:
                        item.setCheckState(state)

    def quantHeaderClicked(self, col):
        from PySide import QtCore

        sel = self.form.quantities.selectedIndexes()
        state = None
        if len(sel) > 7:
            for idx in sel:
                if idx.column() == col:
                    item = self.qmodel.itemFromIndex(idx)
                    if state is None:
                        # take the state to apply from the first selected item
                        state = QtCore.Qt.Checked
                        if item.checkState():
                            state = QtCore.Qt.Unchecked
                    item.setCheckState(state)

    def onClickTree(self, index=None):

        FreeCADGui.Selection.clearSelection()
        sel = self.form.quantities.selectedIndexes()
        for index in sel:
            if index.column() == 0:
                obj = FreeCAD.ActiveDocument.getObject(
                    self.qmodel.itemFromIndex(index).toolTip()
                )
                if obj:
                    FreeCADGui.Selection.addSelection(obj)


FreeCADGui.addCommand("BIM_IfcQuantities", BIM_IfcQuantities())
