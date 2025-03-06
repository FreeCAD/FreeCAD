# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains FreeCAD commands for the BIM workbench"""

import os
import sys
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class BIM_IfcProperties:

    def GetResources(self):
        return {
            "Pixmap": "BIM_IfcProperties",
            "MenuText": QT_TRANSLATE_NOOP(
                "BIM_IfcProperties", "Manage IFC properties..."
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_IfcProperties",
                "Manage the different IFC properties of your BIM objects",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        from PySide import QtCore, QtGui

        try:
            import ArchIFC

            self.ifcroles = ArchIFC.IfcTypes
        except (ImportError, AttributeError):
            import ArchComponent

            self.ifcroles = ArchComponent.IfcRoles

        # load the form and set the tree model up
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogIfcProperties.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_IfcProperties.svg"))
        self.form.groupMode.setItemIcon(2, QtGui.QIcon(":/icons/Document.svg"))
        self.model = QtGui.QStandardItemModel()
        self.form.tree.setModel(self.model)
        self.form.tree.setUniformRowHeights(True)

        # restore saved values
        self.form.onlySelected.setChecked(PARAMS.GetInt("IfcPropertiesSelectedState", 0))
        self.form.onlyVisible.setChecked(PARAMS.GetInt("IfcPropertiesVisibleState", 0))
        w = PARAMS.GetInt("BimIfcPropertiesDialogWidth", 567)
        h = PARAMS.GetInt("BimIfcPropertiesDialogHeight", 608)
        self.form.resize(w, h)

        # build objects list and fill search terms
        self.objectslist, searchterms = self.rebuildObjectsList()
        self.form.searchField.addItems(searchterms)

        # set the properties editor
        try:
            import ArchIFCSchema

            self.ptypes = list(ArchIFCSchema.IfcTypes.keys())
        except (ImportError, AttributeError):
            import ArchComponent

            self.ptypes = (
                ArchComponent.SimplePropertyTypes + ArchComponent.MeasurePropertyTypes
            )
        self.plabels = [
            "".join(map(lambda x: x if x.islower() else " " + x, t[3:]))[1:]
            for t in self.ptypes
        ]
        self.psetdefs = {}
        psetpath = os.path.join(
            FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "pset_definitions.csv"
        )
        custompath = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "CustomPsets.csv")
        self.psetdefs = self.readFromCSV(psetpath)
        self.psetdefs.update(self.readFromCSV(custompath))
        self.psetkeys = [
            "".join(map(lambda x: x if x.islower() else " " + x, t[5:]))[1:]
            for t in self.psetdefs.keys()
        ]
        self.psetkeys.sort()
        self.propmodel = QtGui.QStandardItemModel()
        self.form.treeProperties.setModel(self.propmodel)
        # self.ifcEditor.treeProperties.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.form.treeProperties.setUniformRowHeights(True)
        self.form.treeProperties.setItemDelegate(
            propertiesDelegate(container=self, ptypes=self.ptypes, plabels=self.plabels)
        )
        self.form.labelinfo.setText(
            self.form.labelinfo.text()
            + " "
            + translate("BIM", "Custom properties sets can be defined in")
            + " "
            + custompath
        )

        # set combos
        self.form.comboProperty.addItems(
            [translate("BIM", "Add property...")] + self.plabels
        )
        self.form.comboPset.addItems(
            [translate("BIM", "Add property set..."), translate("BIM", "New...")]
            + self.psetkeys
        )

        # connect signals
        self.form.tree.selectionModel().selectionChanged.connect(self.updateProperties)
        self.form.groupMode.currentIndexChanged.connect(self.update)
        self.form.onlyVisible.stateChanged.connect(self.onVisible)
        self.form.onlySelected.stateChanged.connect(self.onSelected)
        self.form.buttonBox.accepted.connect(self.accept)
        self.form.onlyMatches.stateChanged.connect(self.update)
        self.form.searchField.currentIndexChanged.connect(self.update)
        self.form.searchField.editTextChanged.connect(self.update)
        self.form.comboProperty.currentIndexChanged.connect(self.addProperty)
        self.form.comboPset.currentIndexChanged.connect(self.addPset)
        self.form.buttonDelete.clicked.connect(self.removeProperty)
        self.form.treeProperties.setSortingEnabled(True)

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        self.update()
        self.form.show()

    def rebuildObjectsList(self):

        # build objects list and fill search terms
        objectslist = {}
        searchterms = [""]
        if self.form.onlySelected.isChecked():
            objects = FreeCADGui.Selection.getSelection()
        else:
            objects = FreeCAD.ActiveDocument.Objects
        for obj in objects:
            role = self.getRole(obj)
            if role:
                if hasattr(obj, "IfcProperties") and isinstance(
                    obj.IfcProperties, dict
                ):
                    props = obj.IfcProperties
                elif hasattr(obj, "IfcClass"):
                    props = self.getNativeIfcProperties(obj)
                else:
                    props = {}
                objectslist[obj.Name] = [role, props]
                for key, val in props.items():
                    val = val.split(";;")
                    if ";;" in key:
                        # 0.19 format
                        # pset;;pname = ptype;;pvalue
                        key = key.split(";;")
                        val = [key[1]] + val
                        key = key[0]
                    if not key in searchterms:
                        searchterms.append(key)
                    if len(val) == 3:
                        if not val[0] in searchterms:
                            searchterms.append(val[0])
        return objectslist, searchterms

    def update(self, index=None):
        "updates the tree widgets in all tabs"

        self.model.clear()
        self.model.setHorizontalHeaderLabels(
            [
                translate("BIM", "Label"),
                translate("BIM", "IFC type"),
                translate("BIM", "Search results"),
            ]
        )
        # self.form.tree.header().setResizeMode(QtGui.QHeaderView.Stretch)
        # self.form.tree.resizeColumnsToContents()
        if self.form.groupMode.currentIndex() == 1:
            # group by type
            self.updateByType()
        elif self.form.groupMode.currentIndex() == 2:
            # group by model structure
            self.updateByTree()
        else:
            # group alphabetically
            self.updateDefault()
        self.model.sort(0)
        self.form.tree.setColumnWidth(0, 300)

    def getRole(self, obj):
        if hasattr(obj, "IfcType"):
            return obj.IfcType
        elif hasattr(obj, "IfcRole"):
            return obj.IfcRole
        elif hasattr(obj, "IfcClass"):
            return obj.IfcClass
        else:
            return None

    def readFromCSV(self, csvfile):
        """reads a csv file and returns a dict"""

        import csv

        result = {}
        if os.path.exists(csvfile):
            with open(csvfile, "r") as f:
                reader = csv.reader(f, delimiter=";")
                for row in reader:
                    result[row[0]] = row[1:]
        return result

    def updateByType(self):
        from PySide import QtCore, QtGui

        groups = {}
        for name, role in self.objectslist.items():
            role = role[0]
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    not self.form.onlyVisible.isChecked()
                ) or obj.ViewObject.isVisible():
                    groups.setdefault(role, []).append(name)

        for group in groups.keys():
            s1 = group + " (" + str(len(groups[group])) + ")"
            top = QtGui.QStandardItem(s1)
            self.model.appendRow([top, QtGui.QStandardItem(), QtGui.QStandardItem()])
            for name in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    it1 = QtGui.QStandardItem(obj.Label)
                    icon = obj.ViewObject.Icon
                    it1.setIcon(icon)
                    it1.setToolTip(obj.Name)
                    it2 = QtGui.QStandardItem(group)
                    if group != self.getRole(obj):
                        it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    it3 = self.getSearchResults(obj)
                    if it3:
                        top.appendRow([it1, it2, it3])
            top.sortChildren(0)
        self.form.tree.expandAll()
        self.spanTopLevels()

    def updateByTree(self):
        from PySide import QtCore, QtGui

        # order by hierarchy
        def istop(obj):
            for parent in obj.InListRecursive:
                if parent.Name in self.objectslist.keys():
                    return False
            return True

        rel = []
        deps = []
        for name in self.objectslist.keys():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if istop(obj):
                    rel.append(obj)
                else:
                    deps.append(obj)
        pa = 1
        while deps:
            for obj in rel:
                for child in obj.OutList:
                    if child in deps:
                        rel.append(child)
                        deps.remove(child)
            pa += 1
            if pa == 10:  # max 10 hierarchy levels, okay? Let's keep civilised
                rel.extend(deps)
                break

        done = {}
        for obj in rel:
            role = self.objectslist[obj.Name][0]
            if (not self.form.onlyVisible.isChecked()) or obj.ViewObject.isVisible():
                it1 = QtGui.QStandardItem(obj.Label)
                icon = obj.ViewObject.Icon
                it1.setIcon(icon)
                it1.setToolTip(obj.Name)
                it2 = QtGui.QStandardItem(role)
                if role != self.getRole(obj):
                    it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                it3 = self.getSearchResults(obj)
                ok = False
                for par in obj.InListRecursive:
                    if par.Name in done:
                        if (not hasattr(par, "Hosts")) or (obj not in par.Hosts):
                            if it3:
                                done[par.Name].appendRow([it1, it2, it3])
                            done[obj.Name] = it1
                            ok = True
                            break
                if not ok:
                    if it3:
                        self.model.appendRow([it1, it2, it3])
                    done[obj.Name] = it1
        self.form.tree.expandAll()

    def updateDefault(self):
        from PySide import QtCore, QtGui

        for name, role in self.objectslist.items():
            role = role[0]
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    not self.form.onlyVisible.isChecked()
                ) or obj.ViewObject.isVisible():
                    it1 = QtGui.QStandardItem(obj.Label)
                    icon = obj.ViewObject.Icon
                    it1.setIcon(icon)
                    it1.setToolTip(obj.Name)
                    it2 = QtGui.QStandardItem(role)
                    if role != self.getRole(obj):
                        it2.setIcon(QtGui.QIcon(":/icons/edit-edit.svg"))
                    it3 = self.getSearchResults(obj)
                    if it3:
                        self.model.appendRow([it1, it2, it3])

    def spanTopLevels(self):
        if self.form.groupMode.currentIndex() in [1, 2]:
            idx = self.model.invisibleRootItem().index()
            for i in range(self.model.rowCount()):
                if self.model.item(i, 0).hasChildren():
                    self.form.tree.setFirstColumnSpanned(i, idx, True)

    def accept(self):
        PARAMS.SetInt("BimIfcPropertiesDialogWidth", self.form.width())
        PARAMS.SetInt("BimIfcPropertiesDialogHeight", self.form.height())
        self.form.hide()

        # print(self.objectslist)
        changed = False
        for key, values in self.objectslist.items():
            obj = FreeCAD.ActiveDocument.getObject(key)
            if obj:
                if hasattr(obj, "IfcProperties"):
                    if not isinstance(obj.IfcProperties, dict):
                        FreeCAD.Console.PrintWarning(
                            translate(
                                "BIM",
                                "Warning: object %1 has old-styled IfcProperties and cannot be updated",
                            ).replace("%1", obj.Label)
                            + "\n"
                        )
                        continue
                    props = obj.IfcProperties
                elif hasattr(obj, "IfcClass"):
                    props = self.getNativeIfcProperties(obj)
                else:
                    props = {}
                if values[1] != props:
                    if not changed:
                        FreeCAD.ActiveDocument.openTransaction("Change properties")
                        changed = True
                    if hasattr(obj,"IfcClass"):
                        print("props:",props)
                        for key,value in values[1].items():
                            if ";;" in key and ";;" in value:
                                pname, pset = key.split(";;")
                                ptype, pvalue = value.split(";;")
                                from nativeifc import ifc_psets  # lazy loading
                                fctype = ifc_psets.get_freecad_type(ptype)
                                if not pname in obj.PropertiesList:
                                    obj.addProperty(fctype, pname, pset, ptype+":"+pname)
                                    ifc_psets.edit_pset(obj, pname, force=True)
                                if pvalue:
                                    setattr(obj, pname, pvalue)
                    elif not hasattr(obj, "IfcProperties"):
                        obj.addProperty(
                            "App::PropertyMap",
                            "IfcPRoperties",
                            "IFC",
                            QT_TRANSLATE_NOOP(
                                "App::Property", "IFC properties of this object"
                            ),
                        )
                    if hasattr(obj, "IfcProperties"):
                        obj.IfcProperties = values[1]
        if changed:
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

    def getNativeIfcProperties(self, obj):
        props = {}
        for p in obj.PropertiesList:
            pset = obj.getGroupOfProperty(p)
            ttip = obj.getDocumentationOfProperty(p)
            if ":" in ttip:
                ptype, pname = ttip.split(":")
                if pset not in ["Base", "IFC", "Geometry"]:
                    props[pname+";;"+pset] = ptype+";;"+str(getattr(obj,p))
        return props

    def getSearchResults(self, obj):
        from PySide import QtCore, QtGui

        text = self.form.searchField.currentText()
        if not text:
            return QtGui.QStandardItem()
        else:
            if obj.Name in self.objectslist:
                result = []
                for key, value in self.objectslist[obj.Name][1].items():
                    if ";;" in key:
                        # 0.19 format
                        key = key.split(";;")
                        pset = key[1]
                        key = key[0]
                    else:
                        pset = value.split(";;")[0]
                    if text.lower() in key.lower():
                        if not key in result:
                            result.append(key)
                    if text.lower() in pset.lower():
                        if not pset in result:
                            result.append(pset)
                if result:
                    return QtGui.QStandardItem(",".join(result))
                else:
                    if self.form.onlyMatches.isChecked():
                        return None
                    else:
                        return QtGui.QStandardItem()
            else:
                return QtGui.QStandardItem()

    def updateProperties(self, sel1=None, sel2=None):
        from PySide import QtCore, QtGui

        self.propmodel.clear()
        self.propmodel.setHorizontalHeaderLabels(
            [
                translate("Arch", "Property", None),
                translate("Arch", "Type", None),
                translate("Arch", "Value", None),
            ]
        )

        self.form.treeProperties.setColumnWidth(0, 300)
        self.form.treeProperties.setColumnWidth(1, 200)

        # gather common properties
        allprops = []
        sel = self.form.tree.selectedIndexes()
        for index in sel:
            if index.column() == 0:
                name = self.model.itemFromIndex(index).toolTip()
                if name in self.objectslist:
                    allprops.append(self.objectslist[name][1])
        psets = {}
        if allprops:
            for key in allprops[0].keys():
                value = allprops[0][key].split(";;")
                iscommon = True
                if len(value) == 3:
                    pset = value[0]
                    ptype = value[1]
                    pvalue = value[2]
                elif (len(value) == 2) and (";;" in key):  # 0.19 format
                    pset = key.split(";;")[1]
                    ptype = value[0]
                    pvalue = value[1]
                elif len(value) == 2:  # old system
                    pset = "Default property set"
                    ptype = value[0]
                    pvalue = value[1]
                else:
                    print("Error: Unparsable property:", value)
                    return
                for other in allprops[1:]:
                    if key in other.keys():
                        othervalue = other[key].split(";;")
                        if len(value) == 3:
                            otherpset = othervalue[0]
                            otherptype = othervalue[1]
                            otherpvalue = othervalue[2]
                        elif (len(value) == 2) and (";;" in key):  # 0.19 format
                            otherpset = key.split(";;")[1]
                            otherptype = othervalue[0]
                            otherpvalue = othervalue[1]
                        elif len(value) == 2:  # old system
                            otherpset = "Default property set"
                            otherptype = othervalue[0]
                            otherpvalue = othervalue[1]
                        else:
                            print("Error: Unparsable property:", othervalue)
                            return
                        if otherpset != pset:
                            iscommon = False
                        if otherptype != ptype:
                            iscommon = False
                        if otherpvalue != pvalue:
                            pvalue = "*VARIES*"
                    else:
                        iscommon = False
                if iscommon:
                    plabel = ptype
                    if ptype in self.ptypes:
                        plabel = self.plabels[self.ptypes.index(ptype)]
                    psets.setdefault(pset, []).append([key, plabel, pvalue])

        # fill the tree
        for pset, plists in psets.items():
            top = QtGui.QStandardItem(pset)
            top.setDragEnabled(False)
            top.setToolTip("PropertySet")
            self.propmodel.appendRow(
                [top, QtGui.QStandardItem(), QtGui.QStandardItem()]
            )
            for plist in plists:
                pname = plist[0]
                if ";;" in pname:
                    pname = pname.split(";;")[0]
                it1 = QtGui.QStandardItem(pname)
                it1.setDropEnabled(False)
                it2 = QtGui.QStandardItem(plist[1])
                it2.setDropEnabled(False)
                it3 = QtGui.QStandardItem(plist[2])
                it3.setDropEnabled(False)
                top.appendRow([it1, it2, it3])
            top.sortChildren(0)
        # span top levels
        idx = self.propmodel.invisibleRootItem().index()
        for i in range(self.propmodel.rowCount()):
            if self.propmodel.item(i, 0).hasChildren():
                self.form.treeProperties.setFirstColumnSpanned(i, idx, True)
        self.form.treeProperties.expandAll()

    def updateDicts(self, remove=None):
        # update the stored dicts to reflect the editor

        sel = self.form.tree.selectedIndexes()

        for row in range(self.propmodel.rowCount()):
            pset = self.propmodel.item(row, 0).text()
            if self.propmodel.item(row, 0).hasChildren():
                for childrow in range(self.propmodel.item(row, 0).rowCount()):
                    prop = self.propmodel.item(row, 0).child(childrow, 0).text()
                    if ";;" in prop:
                        prop = prop.split(";;")[0]
                    ptype = self.propmodel.item(row, 0).child(childrow, 1).text()
                    if not ptype.startswith("Ifc"):
                        ptype = self.ptypes[self.plabels.index(ptype)]
                    pvalue = self.propmodel.item(row, 0).child(childrow, 2).text()
                    if (sys.version_info.major < 3) and isinstance(prop, unicode):
                        prop = prop.encode("utf8")

                    # update objects
                    for index in sel:
                        if index.column() == 0:
                            name = self.model.itemFromIndex(index).toolTip()
                            if name in self.objectslist:
                                # print("object",name,self.objectslist[name][1])
                                if pvalue == "*VARIES*":
                                    if not (
                                        prop + ";;" + pset in self.objectslist[name][1]
                                    ):
                                        # print("adding",prop)
                                        self.objectslist[name][1][
                                            prop + ";;" + pset
                                        ] = (ptype + ";;")
                                else:
                                    pval = ptype + ";;" + pvalue
                                    if prop in self.objectslist[name][1]:
                                        if (
                                            self.objectslist[name][1][
                                                prop + ";;" + pset
                                            ]
                                            != pval
                                        ):
                                            # print("modifying",prop)
                                            self.objectslist[name][1][
                                                prop + ";;" + pset
                                            ] = pval
                                    else:
                                        # print("adding",prop)
                                        self.objectslist[name][1][
                                            prop + ";;" + pset
                                        ] = pval
        if remove:
            for index in sel:
                if index.column() == 0:
                    name = self.model.itemFromIndex(index).toolTip()
                    if name in self.objectslist:
                        for prop in remove:
                            if prop in self.objectslist[name][1]:
                                # print("deleting",prop)
                                del self.objectslist[name][1][prop]

    def addProperty(self, idx=0, pset=None, prop=None, ptype=None):
        from PySide import QtCore, QtGui

        if not self.form.tree.selectedIndexes():
            return
        if not pset:
            sel = self.form.treeProperties.selectedIndexes()
            if sel:
                item = self.propmodel.itemFromIndex(sel[0])
                if item.toolTip() == "PropertySet":
                    pset = item
        if pset:
            if not prop:
                basename = translate("Arch", "New property", None)
                # check for duplicate name
                names = []
                for i in range(self.propmodel.rowCount()):
                    topitem = self.propmodel.item(i, 0)
                    names.append(topitem.text())
                    if topitem.hasChildren():
                        for j in range(topitem.rowCount()):
                            childitem = topitem.child(j, 0)
                            names.append(childitem.text())
                suffix = 1
                newname = basename
                while newname in names:
                    newname = basename + str(suffix).zfill(3)
                    suffix += 1
                prop = newname
            if not ptype:
                if idx > 0:
                    ptype = self.plabels[idx - 1]
            if prop and ptype:
                if ptype in self.ptypes:
                    ptype = self.plabels[self.ptypes.index(ptype)]
                it1 = QtGui.QStandardItem(prop)
                it1.setDropEnabled(False)
                it2 = QtGui.QStandardItem(ptype)
                it2.setDropEnabled(False)
                it3 = QtGui.QStandardItem()
                it3.setDropEnabled(False)
                pset.appendRow([it1, it2, it3])
                self.updateDicts()
        else:
            if idx != 0:
                QtGui.QMessageBox.critical(
                    None,
                    "Error",
                    translate(
                        "BIM",
                        "Please select or create a property set first in which the new property should be placed.",
                    ),
                    QtGui.QMessageBox.Ok,
                )
        if idx != 0:
            self.form.comboProperty.setCurrentIndex(0)

    def addPset(self, idx):
        from PySide import QtCore, QtGui

        if not self.form.tree.selectedIndexes():
            return
        if idx == 1:
            name = translate("Arch", "New property set", None)
            res = QtGui.QInputDialog.getText(
                None,
                translate("BIM", "New property set"),
                translate("BIM", "Property set name:"),
                QtGui.QLineEdit.Normal,
                name,
            )
            if res[1]:
                name = res[0]
            top = QtGui.QStandardItem(name)
            top.setDragEnabled(False)
            top.setToolTip("PropertySet")
            self.propmodel.appendRow(
                [top, QtGui.QStandardItem(), QtGui.QStandardItem()]
            )
        elif idx > 1:
            psetlabel = self.psetkeys[idx - 2]
            psetdef = "Pset_" + psetlabel.replace(" ", "")
            if psetdef in self.psetdefs:
                top = QtGui.QStandardItem(psetdef)
                top.setDragEnabled(False)
                top.setToolTip("PropertySet")
                self.propmodel.appendRow(
                    [top, QtGui.QStandardItem(), QtGui.QStandardItem()]
                )
                for i in range(0, len(self.psetdefs[psetdef]), 2):
                    self.addProperty(
                        pset=top,
                        prop=self.psetdefs[psetdef][i],
                        ptype=self.psetdefs[psetdef][i + 1],
                    )
        if idx != 0:
            # span top levels
            idx = self.propmodel.invisibleRootItem().index()
            for i in range(self.propmodel.rowCount()):
                if self.propmodel.item(i, 0).hasChildren():
                    self.form.treeProperties.setFirstColumnSpanned(i, idx, True)
            self.form.treeProperties.expandAll()
            self.form.comboPset.setCurrentIndex(0)

    def removeProperty(self):
        from PySide import QtCore, QtGui

        sel = self.form.treeProperties.selectedIndexes()
        remove = []
        if sel:
            item = self.propmodel.itemFromIndex(sel[0])
            if item.toolTip() == "PropertySet":
                for i in range(item.rowCount()):
                    remove.append(item.child(i, 0).text() + ";;" + item.text())
                self.propmodel.takeRow(sel[0].row())
            else:
                pset = self.propmodel.itemFromIndex(sel[0].parent())
                itemtext = item.text()
                if isinstance(pset, QtGui.QStandardItem):
                    itemtext += ";;" + pset.text()
                remove.append(itemtext)
                pset.takeRow(sel[0].row())
            self.updateDicts(remove=remove)

    def onSelected(self, index):
        PARAMS.SetInt("IfcPropertiesSelectedState", index)
        self.objectslist, searchterms = self.rebuildObjectsList()
        self.form.searchField.clear()
        self.form.searchField.addItems(searchterms)
        self.update()

    def onVisible(self, index):
        PARAMS.SetInt("IfcPropertiesVisibleState", index)
        self.update()


if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui

    class propertiesDelegate(QtGui.QStyledItemDelegate):
        def __init__(self, parent=None, container=None, ptypes=[], plabels=[], *args):
            self.container = container
            QtGui.QStyledItemDelegate.__init__(self, parent, *args)
            self.ptypes = ptypes
            self.plabels = plabels

        def createEditor(self, parent, option, index):
            import FreeCADGui

            if index.column() == 0:  # property name
                editor = QtGui.QLineEdit(parent)
            elif index.column() == 1:  # property type
                editor = QtGui.QComboBox(parent)
            else:  # property value
                ptype = index.sibling(index.row(), 1).data()
                if "Integer" in ptype:
                    editor = QtGui.QSpinBox(parent)
                elif "Real" in ptype:
                    editor = QtGui.QDoubleSpinBox(parent)
                    editor.setDecimals(
                        FreeCAD.ParamGet(
                            "User parameter:BaseApp/Preferences/Units"
                        ).GetInt("Decimals", 2)
                    )
                elif ("Boolean" in ptype) or ("Logical" in ptype):
                    editor = QtGui.QComboBox(parent)
                    editor.addItems(["True", "False"])
                elif "Measure" in ptype:
                    editor = FreeCADGui.UiLoader().createWidget("Gui::InputField")
                    editor.setParent(parent)
                else:
                    editor = QtGui.QLineEdit(parent)
                editor.setObjectName("editor_" + ptype)
            return editor

        def setEditorData(self, editor, index):
            if index.column() == 0:
                editor.setText(index.data())
            elif index.column() == 1:
                editor.addItems(self.plabels)
                if index.data() in self.plabels:
                    idx = self.plabels.index(index.data())
                    editor.setCurrentIndex(idx)
            else:
                if "Integer" in editor.objectName():
                    try:
                        editor.setValue(int(index.data()))
                    except (TypeError, ValueError, AttributeError):
                        editor.setValue(0)
                elif "Real" in editor.objectName():
                    try:
                        editor.setValue(float(index.data()))
                    except (TypeError, ValueError, AttributeError):
                        editor.setValue(0)
                elif ("Boolean" in editor.objectName()) or (
                    "Logical" in editor.objectName()
                ):
                    try:
                        editor.setCurrentIndex(
                            ["true", "false"].index(index.data().lower())
                        )
                    except (ValueError, AttributeError):
                        editor.setCurrentIndex(1)
                elif "Measure" in editor.objectName():
                    try:
                        editor.setText(index.data())
                    except (ValueError, AttributeError):
                        editor.setValue(0)
                else:
                    editor.setText(index.data())

        def setModelData(self, editor, model, index):
            remove = []
            if index.column() == 0:
                oldtext = index.data()
                if oldtext != editor.text():
                    pset = index.parent().data()
                    remove.append(oldtext + ";;" + pset)
                basename = editor.text()
                # check for duplicate name
                names = []
                for i in range(model.rowCount()):
                    topitem = model.item(i, 0)
                    names.append(topitem.text())
                    if topitem.hasChildren():
                        for j in range(topitem.rowCount()):
                            childitem = topitem.child(j, 0)
                            names.append(childitem.text())
                suffix = 1
                newname = basename
                while newname in names:
                    newname = basename + str(suffix).zfill(3)
                    suffix += 1
                model.setData(index, newname)
            elif index.column() == 1:
                if editor.currentIndex() > -1:
                    idx = editor.currentIndex()
                    data = self.plabels[idx]
                    model.setData(index, data)
            else:
                if ("Integer" in editor.objectName()) or (
                    "Real" in editor.objectName()
                ):
                    model.setData(index, str(editor.value()))
                elif ("Boolean" in editor.objectName()) or (
                    "Logical" in editor.objectName()
                ):
                    model.setData(index, editor.currentText())
                elif "Measure" in editor.objectName():
                    model.setData(index, editor.property("text"))
                else:
                    model.setData(index, editor.text())
            self.container.updateDicts(remove)


FreeCADGui.addCommand("BIM_IfcProperties", BIM_IfcProperties())
