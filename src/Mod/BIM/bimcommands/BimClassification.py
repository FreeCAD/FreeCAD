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

"""The BIM Classification command"""

import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class BIM_Classification:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Classification",
            "MenuText": QT_TRANSLATE_NOOP(
                "BIM_Classification", "Manage Classification"
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Classification",
                "Manages classification systems and apply classification to objects",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        import Draft
        from PySide import QtCore, QtGui
        from bimcommands import BimMaterial

        # init checks
        if not hasattr(self, "Classes"):
            self.Classes = {}
        self.isEditing = None
        current = None

        # load the form and set the tree model up
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogClassification.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_Classification.svg"))
        self.form.groupMode.setItemIcon(0, QtGui.QIcon(":/icons/Arch_SectionPlane_Tree.svg")) # Alphabetical
        self.form.groupMode.setItemIcon(1, QtGui.QIcon(":/icons/IFC.svg")) # Type
        self.form.groupMode.setItemIcon(2, QtGui.QIcon(":/icons/Arch_Material.svg")) # Material
        self.form.groupMode.setItemIcon(3, QtGui.QIcon(":/icons/Document.svg")) # Model structure

        # restore saved values
        self.form.onlyVisible.setChecked(PARAMS.GetInt("BimClassificationVisibleState", 0))
        self.form.checkPrefix.setChecked(PARAMS.GetInt("BimClassificationSystemNamePrefix", 1))
        w = PARAMS.GetInt("BimClassificationDialogWidth", 629)
        h = PARAMS.GetInt("BimClassificationDialogHeight", 516)
        self.form.resize(w, h)

        # add modified search box from bimmaterial
        searchBox = BimMaterial.MatLineEdit(self.form)
        searchBox.setPlaceholderText(translate("BIM", "Search..."))
        searchBox.setToolTip(translate("BIM", "Searches classes"))
        self.form.search = searchBox
        self.form.horizontalLayout_2.addWidget(searchBox)

        # set help line
        self.form.labelDownload.setText(
            self.form.labelDownload.text().replace(
                "%s", os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification")
            )
        )

        # hide materials list if we are editing a particular object
        if len(FreeCADGui.Selection.getSelection()) == 1:
            self.isEditing = FreeCADGui.Selection.getSelection()[0]
            pl = self.isEditing.PropertiesList
            if ("StandardCode" in pl) or ("IfcClass" in pl):
                self.form.groupMaterials.hide()
                self.form.buttonApply.hide()
                self.form.buttonRename.hide()
                self.form.setWindowTitle(
                    translate("BIM", "Editing") + " " + self.isEditing.Label
                )
                if "IfcClass" in pl:
                    # load existing class if needed
                    from nativeifc import ifc_classification
                    ifc_classification.show_classification(self.isEditing)
                if "StandardCode" in pl:
                    current = self.isEditing.StandardCode
                elif "Classification" in self.isEditing.PropertiesList:
                    current = self.isEditing.Classification

        # fill materials list
        self.objectslist = {}
        self.matlist = {}
        self.labellist = {}
        for obj in FreeCAD.ActiveDocument.Objects:
            if "StandardCode" in obj.PropertiesList:
                if Draft.getType(obj) in ["Material", "MultiMaterial"]:
                    self.matlist[obj.Name] = obj.StandardCode
                else:
                    self.objectslist[obj.Name] = obj.StandardCode
                self.labellist[obj.Name] = obj.Label
            elif "Classification" in obj.PropertiesList:
                self.objectslist[obj.Name] = obj.Classification
                self.labellist[obj.Name] = obj.Label

        # fill objects list
        if not self.isEditing:
            self.updateObjects()

        # fill available classifications
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetString(
            "DefaultClassificationSystem", ""
        )
        presetdir = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Classification")
        if os.path.isdir(presetdir):
            presets = []
            for f in os.listdir(presetdir):
                if f.lower().endswith(".xml") or f.lower().endswith(".ifc"):
                    n = os.path.splitext(f)[0]
                    if not n in presets:
                        presets.append(n)
                        self.form.comboSystem.addItem(n)
                        if n == p:
                            self.form.comboSystem.setCurrentIndex(
                                self.form.comboSystem.count() - 1
                            )

        # connect signals
        self.form.comboSystem.currentIndexChanged.connect(self.updateClasses)
        self.form.buttonApply.clicked.connect(self.apply)
        self.form.buttonRename.clicked.connect(self.rename)
        self.form.search.textEdited.connect(self.updateClasses)
        self.form.buttonBox.accepted.connect(self.accept)
        self.form.groupMode.currentIndexChanged.connect(self.updateObjects)
        self.form.treeClass.itemDoubleClicked.connect(self.apply)
        self.form.search.up.connect(self.onUpArrow)
        self.form.search.down.connect(self.onDownArrow)
        if hasattr(self.form.onlyVisible, "checkStateChanged"): # Qt version >= 6.7.0
            self.form.onlyVisible.checkStateChanged.connect(self.onVisible)
            self.form.checkPrefix.checkStateChanged.connect(self.onPrefix)
        else: # Qt version < 6.7.0
            self.form.onlyVisible.stateChanged.connect(self.onVisible)
            self.form.checkPrefix.stateChanged.connect(self.onPrefix)
        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        self.updateClasses()

        # select current classification
        if current:
            system, classification = current.split(" ", 1)
            print("searching for",classification)
            if system in self.Classes:
                self.form.comboSystem.setCurrentText(system)
                res = self.form.treeClass.findItems(
                    classification,
                    QtCore.Qt.MatchExactly|QtCore.Qt.MatchRecursive,
                    0
                )
                if res:
                    self.form.treeClass.setCurrentItem(res[0])

        self.form.show()

        self.form.search.setFocus()

    def updateObjects(self, idx=None):
        # store current state of tree into self.objectslist before redrawing

        for row in range(self.form.treeObjects.topLevelItemCount()):
            child = self.form.treeObjects.topLevelItem(row)
            if child.toolTip(0):
                if child.toolTip(0) in self.objectslist:
                    self.objectslist[child.toolTip(0)] = child.text(1)
                elif child.toolTip(0) in self.matlist:
                    self.matlist[child.toolTip(0)] = child.text(1)
                self.labellist[child.toolTip(0)] = child.text(0)
            for childrow in range(child.childCount()):
                grandchild = child.child(childrow)
                if grandchild.toolTip(0):
                    if grandchild.toolTip(0) in self.objectslist:
                        self.objectslist[grandchild.toolTip(0)] = grandchild.text(1)
                    elif grandchild.toolTip(0) in self.matlist:
                        self.matlist[grandchild.toolTip(0)] = grandchild.text(1)
                    self.labellist[grandchild.toolTip(0)] = grandchild.text(0)

        self.form.treeObjects.clear()

        if self.form.groupMode.currentIndex() == 1:
            # group by type
            self.updateByType()
        elif self.form.groupMode.currentIndex() == 2:
            # group by material
            self.updateByMaterial()
        elif self.form.groupMode.currentIndex() == 3:
            # group by model structure
            self.updateByTree()
        else:
            # group alphabetically
            self.updateDefault()

        # resize columns - no resizeSection in pyside2
        # self.form.treeObjects.header().resizeSection(0,int(self.form.treeObjects.width()/2))
        # self.form.treeObjects.header().resizeSection(1,int(self.form.treeObjects.width()/2))

    def updateByType(self):
        from PySide import QtCore, QtGui
        import Draft

        groups = {}
        for name in self.objectslist.keys():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj and hasattr(obj, "IfcType"):
                groups.setdefault(obj.IfcType, []).append(name)
            elif obj and hasattr(obj, "IfcRole"):
                groups.setdefault(obj.IfcRole, []).append(name)
            else:
                groups.setdefault("Undefined", []).append(name)
        groups["Materials"] = self.matlist.keys()
        d = self.objectslist.copy()
        d.update(self.matlist)

        for group in groups.keys():
            mit = QtGui.QTreeWidgetItem([group, ""])
            self.form.treeObjects.addTopLevelItem(mit)
            for name in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    if (
                        (not self.form.onlyVisible.isChecked())
                        or obj.ViewObject.isVisible()
                        or (Draft.getType(obj) in ["Material", "MultiMaterial"])
                    ):
                        it = QtGui.QTreeWidgetItem([self.labellist[name], d[name]])
                        it.setIcon(0, self.getIcon(obj))
                        it.setToolTip(0, name)
                        mit.addChild(it)
            mit.sortChildren(0, QtCore.Qt.AscendingOrder)
        self.form.treeObjects.expandAll()
        # self.spanTopLevels()

    def updateByMaterial(self):
        from PySide import QtCore, QtGui

        groups = {}
        claimed = []
        for name in self.matlist.keys():
            mat = FreeCAD.ActiveDocument.getObject(name)
            if mat:
                children = [
                    par.Name
                    for par in mat.InList
                    if par.Name in self.objectslist.keys()
                ]
                groups[name] = children
                claimed.extend(children)
        groups["Undefined"] = [o for o in self.objectslist.keys() if not o in claimed]

        for group in groups.keys():
            matobj = FreeCAD.ActiveDocument.getObject(group)
            if matobj:
                mit = QtGui.QTreeWidgetItem(
                    [self.labellist[group], self.matlist[group]]
                )
                mit.setIcon(0, self.getIcon(matobj))
                mit.setToolTip(0, group)
            else:
                mit = QtGui.QTreeWidgetItem(["Undefined", ""])
            self.form.treeObjects.addTopLevelItem(mit)
            for name in groups[group]:
                obj = FreeCAD.ActiveDocument.getObject(name)
                if obj:
                    if (
                        not self.form.onlyVisible.isChecked()
                    ) or obj.ViewObject.isVisible():
                        it = QtGui.QTreeWidgetItem(
                            [self.labellist[name], self.objectslist[name]]
                        )
                        it.setIcon(0, self.getIcon(obj))
                        it.setToolTip(0, name)
                        mit.addChild(it)
            mit.sortChildren(0, QtCore.Qt.AscendingOrder)
        self.form.treeObjects.expandAll()
        # self.spanTopLevels()

    def updateByTree(self):
        from PySide import QtGui

        # order by hierarchy
        def istop(obj):
            for parent in obj.InList:
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
        # materials first
        mit = QtGui.QTreeWidgetItem(["Materials", ""])
        self.form.treeObjects.addTopLevelItem(mit)
        for name, code in self.matlist.items():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                it = QtGui.QTreeWidgetItem([self.labellist[name], code])
                it.setIcon(0, self.getIcon(obj))
                it.setToolTip(0, name)
                mit.addChild(it)
        # objects next
        for obj in rel:
            code = self.objectslist[obj.Name]
            if (not self.form.onlyVisible.isChecked()) or obj.ViewObject.isVisible():
                it = QtGui.QTreeWidgetItem([self.labellist[obj.Name], code])
                it.setIcon(0, self.getIcon(obj))
                it.setToolTip(0, name)
                ok = False
                for par in obj.InListRecursive:
                    if par.Name in done:
                        if (not hasattr(par, "Hosts")) or (obj not in par.Hosts):
                            done[par.Name].addChild(it)
                            done[obj.Name] = it
                            ok = True
                            break
                if not ok:
                    self.form.treeObjects.addTopLevelItem(it)
                    done[obj.Name] = it
        self.form.treeObjects.expandAll()

    def updateDefault(self):
        from PySide import QtGui
        import Draft

        d = self.objectslist.copy()
        d.update(self.matlist)
        for name, code in d.items():
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                if (
                    (not self.form.onlyVisible.isChecked())
                    or obj.ViewObject.isVisible()
                    or (Draft.getType(obj) in ["Material", "MultiMaterial"])
                ):
                    it = QtGui.QTreeWidgetItem([self.labellist[name], code])
                    it.setIcon(0, self.getIcon(obj))
                    it.setToolTip(0, name)
                    self.form.treeObjects.addTopLevelItem(it)
                    if obj in FreeCADGui.Selection.getSelection():
                        self.form.treeObjects.setCurrentItem(it)

    def updateClasses(self, search=""):
        from PySide import QtGui

        self.form.treeClass.clear()

        # save as default
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetString(
            "DefaultClassificationSystem", self.form.comboSystem.currentText()
        )

        if isinstance(search, int):
            search = ""
        if self.form.search.text():
            search = self.form.search.text()
        if search:
            search = search.lower()

        system = self.form.comboSystem.currentText()
        if not system:
            return

        if not system in self.Classes:
            self.Classes[system] = self.build(system)
        if not self.Classes[system]:
            return

        for c in self.Classes[system]:
            it = None
            first = True
            if not c[1]:
                c[1] = ""
            if search:
                if (search in c[0].lower()) or (search in c[1].lower()):
                    it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                    it.setToolTip(0, c[1])
                    self.form.treeClass.addTopLevelItem(it)
            else:
                it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                it.setToolTip(0, c[1])
                self.form.treeClass.addTopLevelItem(it)
            if c[2]:
                self.addChildren(c[2], it, search)
            if it and first:
                # select first entry
                self.form.treeClass.setCurrentItem(it)
                first = False

    def addChildren(self, children, parent, search=""):
        from PySide import QtGui

        if children:
            for c in children:
                it = None
                if not c[1]:
                    c[1] = ""
                if search:
                    if (search in c[0].lower()) or (search in c[1].lower()):
                        it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                        it.setToolTip(0, c[1])
                        self.form.treeClass.addTopLevelItem(it)
                else:
                    it = QtGui.QTreeWidgetItem([c[0] + " " + c[1]])
                    it.setToolTip(0, c[1])
                    if parent:
                        parent.addChild(it)
                if c[2]:
                    self.addChildren(c[2], it, search)

    def build(self, system):
        # try to load the IFC first
        preset = os.path.join(
            FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".ifc"
        )
        if os.path.exists(preset):
            return self.build_ifc(system)
        else:
            preset = os.path.join(
                FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".xml"
            )
            if os.path.exists(preset):
                return self.build_xml(system)
            else:
                FreeCAD.Console.PrintError(
                    "Unable to find classification file:" + system + "\n"
                )
                return []

    def build_ifc(self, system):
        # builds from ifc instead of xml

        class Item:
            def __init__(self, parent=None):
                self.parent = parent
                self.ID = None
                self.Name = None
                self.children = []

        preset = os.path.join(
            FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".ifc"
        )
        if not os.path.exists(preset):
            return None
        import ifcopenshell

        f = ifcopenshell.open(preset)
        classes = f.by_type("IfcClassificationReference")
        rootclass = f.by_type("IfcClassification")
        if rootclass:
            rootclass = rootclass[0]
        else:
            return None
        root = Item()
        classdict = {rootclass.id(): root}
        for cl in classes:
            currentItem = Item()
            currentItem.Name = cl.Name
            currentItem.Description = cl.Description
            currentItem.ID = cl.Identification
            if cl.ReferencedSource:
                if cl.ReferencedSource.id() in classdict:
                    currentItem.parent = classdict[cl.ReferencedSource.id()]
                    classdict[cl.ReferencedSource.id()].children.append(currentItem)
            classdict[cl.id()] = currentItem
        return [self.listize(c) for c in root.children]

    def build_xml(self, system):
        class Item:
            def __init__(self, parent=None):
                self.parent = parent
                self.ID = None
                self.Name = None
                self.children = []

        preset = os.path.join(
            FreeCAD.getUserAppDataDir(), "BIM", "Classification", system + ".xml"
        )
        if not os.path.exists(preset):
            return None
        import codecs
        import re

        d = Item()
        with codecs.open(preset, "r", "utf-8") as f:
            currentItem = d
            for l in f:
                if "<Item>" in l:
                    currentItem = Item(currentItem)
                    currentItem.parent.children.append(currentItem)
                if "</Item>" in l:
                    currentItem = currentItem.parent
                elif currentItem and re.findall(r"<ID>(.*?)</ID>", l):
                    currentItem.ID = re.findall(r"<ID>(.*?)</ID>", l)[0]
                elif currentItem and re.findall(r"<Name>(.*?)</Name>", l):
                    currentItem.Name = re.findall(r"<Name>(.*?)</Name>", l)[0]
                elif (
                    currentItem
                    and re.findall(r"<Description>(.*?)</Description>", l)
                    and not currentItem.Name
                ):
                    currentItem.Name = re.findall(
                        "<Description>(.*?)</Description>", l
                    )[0]
        return [self.listize(c) for c in d.children]

    def listize(self, item):
        return [item.ID, item.Name, [self.listize(it) for it in item.children]]

    def apply(self, item=None, col=None):
        if (
            self.form.treeObjects.selectedItems()
            and len(self.form.treeClass.selectedItems()) == 1
        ):
            c = self.form.treeClass.selectedItems()[0].text(0)
            if self.form.checkPrefix.isChecked():
                c = self.form.comboSystem.currentText() + " " + c
            for m in self.form.treeObjects.selectedItems():
                if m.toolTip(0):
                    m.setText(1, c)

    def rename(self):
        if (
            self.form.treeObjects.selectedItems()
            and len(self.form.treeClass.selectedItems()) == 1
        ):
            c = self.form.treeClass.selectedItems()[0].toolTip(0)
            for m in self.form.treeObjects.selectedItems():
                if m.toolTip(0):
                    m.setText(0, c)

    def accept(self):
        if not self.isEditing:
            changed = False
            for row in range(self.form.treeObjects.topLevelItemCount()):
                child = self.form.treeObjects.topLevelItem(row)
                items = [child]
                items.extend(
                    [child.child(childrow) for childrow in range(child.childCount())]
                )
                for item in items:
                    code = item.text(1)
                    label = item.text(0)
                    if item.toolTip(0):
                        obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
                        if obj:
                            if hasattr(obj, "StandardCode"):
                                if code != obj.StandardCode:
                                    if not changed:
                                        FreeCAD.ActiveDocument.openTransaction(
                                            "Change standard codes"
                                        )
                                        changed = True
                                    obj.StandardCode = code
                            elif hasattr(obj, "IfcClass"):
                                if not "Classification" in obj.PropertiesList:
                                    obj.addProperty("App::PropertyString", "Classification", "IFC", locked=True)
                                if code != obj.Classification:
                                    if not changed:
                                        FreeCAD.ActiveDocument.openTransaction(
                                            "Change standard codes"
                                        )
                                        changed = True
                                    obj.Classification = code
                            if label != obj.Label:
                                if not changed:
                                    FreeCAD.ActiveDocument.openTransaction(
                                        "Change standard codes"
                                    )
                                    changed = True
                                obj.Label = label
            if changed:
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
        else:
            # Close the form if user has pressed Enter and did not
            # select anything
            if len(self.form.treeClass.selectedItems()) < 1:
                self.form.close()
                return

            code = self.form.treeClass.selectedItems()[0].text(0)
            pl = self.isEditing.PropertiesList
            if ("StandardCode" in pl) or ("IfcClass" in pl):
                FreeCAD.ActiveDocument.openTransaction("Change standard codes")
                if self.form.checkPrefix.isChecked():
                    code = self.form.comboSystem.currentText() + " " + code
                if "StandardCode" in pl:
                    self.isEditing.StandardCode = code
                else:
                    if not "Classification" in self.isEditing.PropertiesList:
                        self.isEditing.addProperty("App::PropertyString", "Classification", "IFC", locked=True)
                    self.isEditing.Classification = code
                if hasattr(self.isEditing.ViewObject, "Proxy") and hasattr(
                    self.isEditing.ViewObject.Proxy, "setTaskValue"
                ):
                    self.isEditing.ViewObject.Proxy.setTaskValue("FieldCode", code)
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
        p.SetInt("BimClassificationDialogWidth", self.form.width())
        p.SetInt("BimClassificationDialogHeight", self.form.height())
        self.form.hide()
        return True

    def onUpArrow(self):
        if self.form:
            i = self.form.treeClass.currentItem()
            if self.form.treeClass.itemAbove(i):
                self.form.treeClass.setCurrentItem(self.form.treeClass.itemAbove(i))

    def onDownArrow(self):
        if self.form:
            i = self.form.treeClass.currentItem()
            if self.form.treeClass.itemBelow(i):
                self.form.treeClass.setCurrentItem(self.form.treeClass.itemBelow(i))

    def onVisible(self, index):
        PARAMS.SetInt("BimClassificationVisibleState", getattr(index, "value", index))
        self.updateObjects()

    def onPrefix(self, index):
        PARAMS.SetInt("BimClassificationSystemNamePrefix", getattr(index, "value", index))

    def getIcon(self,obj):
        """returns a QIcon for an object"""

        from PySide import QtGui
        import Arch_rc

        if hasattr(obj.ViewObject, "Icon"):
            return obj.ViewObject.Icon
        elif hasattr(obj.ViewObject, "Proxy") and hasattr(obj.ViewObject.Proxy, "getIcon"):
            icon = obj.ViewObject.Proxy.getIcon()
            if icon.startswith("/*"):
                return QtGui.QIcon(QtGui.QPixmap(icon))
            else:
                return QtGui.QIcon(icon)
        else:
            return QtGui.QIcon(":/icons/Arch_Component.svg")


FreeCADGui.addCommand("BIM_Classification", BIM_Classification())
