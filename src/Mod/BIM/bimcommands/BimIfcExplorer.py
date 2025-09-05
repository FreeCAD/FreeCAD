# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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

import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_IfcExplorer:

    def __init__(self):
        self.tree = None

    def GetResources(self):
        import Arch_rc

        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("BIM_IfcExplorer", "IFC Explorer"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_IfcExplorer", "Opens the IFC explorer utility"),
        }

    def Activated(self):

        from PySide import QtGui

        try:
            import ifcopenshell
        except ImportError:
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "IfcOpenShell was not found on this system. IFC support is disabled",
                )
                + "\n"
            )
            return

        # setting up a font
        self.bold = QtGui.QFont()
        self.bold.setBold(True)

        # setting up a link fint
        self.linkfont = QtGui.QFont()
        self.linkfont.setUnderline(True)

        # setup a brush to paint text in system link color
        self.linkbrush = QtGui.QApplication.palette().link()

        # draw the main tree widget
        self.tree = QtGui.QTreeWidget()
        self.tree.setColumnCount(1)
        self.tree.setWordWrap(True)
        self.tree.header().setDefaultSectionSize(60)
        self.tree.header().resizeSection(0, 180)
        self.tree.header().setStretchLastSection(True)
        self.tree.headerItem().setText(0, translate("BIM", "Objects structure"))

        # draw the attributes widget
        self.attributes = QtGui.QTreeWidget()
        self.attributes.setColumnCount(2)
        self.attributes.setWordWrap(True)
        self.attributes.header().setDefaultSectionSize(60)
        self.attributes.header().resizeSection(0, 120)
        self.attributes.header().resizeSection(1, 200)
        self.attributes.header().setStretchLastSection(True)
        self.attributes.headerItem().setText(0, translate("BIM", "Attribute"))
        self.attributes.headerItem().setText(1, translate("BIM", "Value"))

        # draw the properties widget
        self.properties = QtGui.QTreeWidget()
        self.properties.setColumnCount(2)
        self.properties.setWordWrap(True)
        self.properties.header().setDefaultSectionSize(60)
        self.properties.header().resizeSection(0, 120)
        self.properties.header().resizeSection(1, 200)
        self.properties.header().setStretchLastSection(True)
        self.properties.headerItem().setText(0, translate("BIM", "Property"))
        self.properties.headerItem().setText(1, translate("BIM", "Value"))

        # create the dialog
        self.dialog = QtGui.QDialog()
        self.dialog.setObjectName("IfcExplorer")
        self.dialog.setWindowTitle(translate("BIM", "IFC Explorer"))
        self.dialog.resize(720, 540)
        toolbar = FreeCADGui.UiLoader().createWidget("Gui::ToolBar")

        layout = QtGui.QVBoxLayout()
        layout.addWidget(toolbar)
        hlayout = QtGui.QHBoxLayout()
        hlayout.addWidget(self.tree)
        layout.addLayout(hlayout)
        vlayout = QtGui.QVBoxLayout()
        hlayout.addLayout(vlayout)
        vlayout.addWidget(self.attributes)
        vlayout.addWidget(self.properties)
        self.dialog.setLayout(layout)

        # draw the toolbar buttons
        self.openAction = QtGui.QAction(translate("BIM", "Open"), None)
        self.openAction.setToolTip(translate("BIM", "Open another IFC file"))
        self.openAction.triggered.connect(self.open)
        self.openAction.setIcon(QtGui.QIcon(":/icons/document-open.svg"))
        toolbar.addAction(self.openAction)

        self.backAction = QtGui.QAction(translate("BIM", "Back"), None)
        self.backAction.setToolTip(translate("BIM", "Go back to last item selected"))
        self.backAction.triggered.connect(self.back)
        self.backAction.setIcon(QtGui.QIcon(":/icons/edit-undo.svg"))
        toolbar.addAction(self.backAction)

        self.shapeAction = QtGui.QAction(translate("BIM", "Insert"), None)
        self.shapeAction.setToolTip(
            translate(
                "BIM",
                "Inserts the selected object and its children in the active document",
            )
        )
        self.shapeAction.triggered.connect(self.insert)
        self.shapeAction.setIcon(QtGui.QIcon(":icons/Tree_Part.svg"))
        self.shapeAction.setEnabled(False)
        toolbar.addAction(self.shapeAction)

        self.meshAction = QtGui.QAction(translate("BIM", "Mesh"), None)
        self.meshAction.setToolTip(translate("BIM", "Turn mesh display on/off"))
        self.meshAction.triggered.connect(self.toggleMesh)
        self.meshAction.setCheckable(True)
        self.meshAction.setChecked(False)
        self.meshAction.setIcon(QtGui.QIcon(":/icons/DrawStyleShaded.svg"))
        toolbar.addAction(self.meshAction)

        # connect signals/slots
        self.tree.currentItemChanged.connect(self.onSelectTree)
        self.attributes.itemDoubleClicked.connect(self.onDoubleClickTree)
        self.properties.itemDoubleClicked.connect(self.onDoubleClickTree)
        self.dialog.rejected.connect(self.close)

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.dialog.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.dialog.rect().center()
        )

        # open a file and show the dialog
        self.open()
        if self.filename:
            self.dialog.show()

    def open(self):
        "opens a file"

        import ifcopenshell
        from PySide import QtGui

        self.filename = ""
        lastfolder = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/BIM"
        ).GetString("lastIfcExplorerFolder", "")
        filename = QtGui.QFileDialog.getOpenFileName(
            None,
            translate("BIM", "Select an IFC file"),
            lastfolder,
            translate("BIM", "IFC files (*.ifc)"),
        )
        if filename and filename[0]:
            self.filename = filename[0]
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").SetString(
                "lastIfcExplorerFolder", os.path.dirname(self.filename)
            )
            if not os.path.exists(self.filename):
                FreeCAD.Console.PrintError(translate("BIM", "File not found") + "\n")
                return
        else:
            return

        # set window title
        self.dialog.setWindowTitle(
            translate("BIM", "IFC Explorer") + " - " + os.path.basename(self.filename)
        )

        # clear everything
        self.tree.clear()
        self.attributes.clear()
        self.properties.clear()
        self.done = []
        self.backnav = []
        self.mesh = None
        self.products = []
        self.omeshes = {}
        self.currentmesh = None

        # read file and order contents
        self.ifc = ifcopenshell.open(self.filename)
        root = self.getEntitiesTree()

        # unable to find IfcSite
        if not root:
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "IfcSite element was not found in %s. Unable to explore.",
                )
                % self.filename + "\n"
            )
            return

        # populate tree contents
        for eid, children in root.items():
            self.addEntity(eid, children, self.tree)
        # self.tree.expandAll()

    def close(self):
        "close the dialog"

        if FreeCAD.ActiveDocument:
            if getattr(self, "mesh", None):
                FreeCAD.ActiveDocument.removeObject(self.mesh.Name)
            if getattr(self, "currentmesh", None):
                FreeCAD.ActiveDocument.removeObject(self.currentmesh.Name)

    def back(self):
        "selects the previously selected item in the tree"

        if self.backnav:
            item = self.backnav.pop()
            self.tree.setCurrentItem(item)

    def insert(self):
        "inserts selected objects in the active document"

        from importers import importIFC
        from PySide import QtCore

        doc = FreeCAD.ActiveDocument
        if doc and self.filename:
            item = self.tree.currentItem()
            if item:
                eid = item.data(0, QtCore.Qt.UserRole)
                if eid:
                    importIFC.ZOOMOUT = False
                    try:
                        importIFC.insert(self.ifc, doc.Name, only=[eid])
                    except TypeError:
                        importIFC.insert(self.filename, doc.Name, only=[eid])
                    if self.currentmesh:
                        self.currentmesh.ViewObject.hide()

    def toggleMesh(self, checked=False):
        "turns mesh display on/off"

        import Mesh
        import ifcopenshell
        from ifcopenshell import geom

        if not FreeCAD.ActiveDocument:
            doc = FreeCAD.newDocument()
            FreeCAD.setActiveDocument(doc.Name)
        if FreeCAD.ActiveDocument:
            if checked:
                if self.mesh:
                    self.mesh.ViewObject.show()
                else:
                    try:
                        from importers import importIFCHelper

                        s = importIFCHelper.getScaling(self.ifc)
                    except:
                        from importers import importIFC

                        s = importIFC.getScaling(self.ifc)
                    s *= 1000  # ifcopenshell outputs its meshes in metres
                    trf = None
                    if s != 1:
                        trf = FreeCAD.Matrix()
                        trf.scale(s, s, s)
                    basemesh = Mesh.Mesh()
                    s = geom.settings()
                    s.set(s.USE_WORLD_COORDS, True)
                    for product in self.products:
                        try:
                            m = geom.create_shape(s, product)
                            g = m.geometry
                            v = g.verts
                            f = g.faces
                            verts = [
                                FreeCAD.Vector(v[i : i + 3])
                                for i in range(0, len(v), 3)
                            ]
                            faces = [tuple(f[i : i + 3]) for i in range(0, len(f), 3)]
                            omesh = Mesh.Mesh((verts, faces))
                            if trf:
                                omesh.transform(trf)
                            self.omeshes[product.id()] = omesh
                            basemesh.addMesh(omesh)
                        except:
                            pass
                    self.mesh = FreeCAD.ActiveDocument.addObject(
                        "Mesh::Feature", "IFCMesh"
                    )
                    self.mesh.Mesh = basemesh
                    self.mesh.ViewObject.Transparency = 85
                    FreeCAD.ActiveDocument.recompute()
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(self.mesh)
                    FreeCADGui.SendMsgToActiveView("ViewSelection")
            else:
                if self.mesh:
                    self.mesh.ViewObject.hide()
                if self.currentmesh:
                    self.currentmesh.ViewObject.hide()

    def getEntitiesTree(self):
        "stacks the entities inside containers"

        root = {}

        for site in self.ifc.by_type("IfcSite"):
            root[site.id()] = self.getChildren(site)

        return root

        # entities =  ifc.by_type("IfcRoot")
        # entities += ifc.by_type("IfcRepresentation")
        # entities += ifc.by_type("IfcRepresentationItem")
        # entities += ifc.by_type("IfcRepresentationMap")
        # entities += ifc.by_type("IfcPlacement")
        # entities += ifc.by_type("IfcProperty")
        # entities += ifc.by_type("IfcPhysicalSimpleQuantity")
        # entities += ifc.by_type("IfcMaterial")
        # entities += ifc.by_type("IfcProductRepresentation")
        # entities = sorted(entities, key=lambda eid: eid.id())

    def getChildren(self, obj, keys=False):
        "returns a recursive dict of the children of this obj"

        children = {}
        if obj.is_a("IfcProduct"):
            self.products.append(obj)
        if hasattr(obj, "IsDecomposedBy"):  # building structure
            for rel in obj.IsDecomposedBy:
                if hasattr(rel, "RelatedObjects"):
                    for child in rel.RelatedObjects:
                        children[child.id()] = self.getChildren(child)
        if hasattr(obj, "ContainsElements"):  # objects inside building structure
            for rel in obj.ContainsElements:
                if hasattr(rel, "RelatedElements"):
                    for child in rel.RelatedElements:
                        children[child.id()] = self.getChildren(child)
        if hasattr(obj, "Representation"):  # Shape representation
            if obj.Representation:
                children[obj.Representation.id()] = self.getChildren(obj.Representation)
        if hasattr(obj, "Representations"):
            for rep in obj.Representations:
                children[rep.id()] = self.getChildren(rep)
        if obj.is_a("IfcShapeRepresentation"):
            for it in obj.Items:
                children[it.id()] = self.getChildren(it)
        if keys:

            def getkeys(d):
                ck = list(d.keys())
                for v in d.values():
                    ck.extend(getkeys(v))
                return ck

            return getkeys(children)
        return children

    def addEntity(self, eid, children, parent):
        "adds a given entity and its children to the given tree item"

        from PySide import QtCore, QtGui

        def get_name(e):
            try:
                return " : " + e.get_info()["Name"]
            except:
                return ""

        if not eid in self.done:
            entity = self.ifc[eid]
            item = QtGui.QTreeWidgetItem(parent)
            # item.setText(0,self.tostr(eid))
            name = ""
            if entity.is_a("IfcProduct"):
                name = get_name(entity)
                item.setFont(0, self.bold)
                if isinstance(parent, QtGui.QTreeWidgetItem):
                    parent.setExpanded(True)
            item.setText(
                0, "#" + self.tostr(eid) + " : " + self.tostr(entity.is_a()) + name
            )
            if entity.is_a() in ["IfcWall", "IfcWallStandardCase"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Wall_Tree.svg"))
            elif entity.is_a() in ["IfcBuildingElementProxy"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Component.svg"))
            elif entity.is_a() in [
                "IfcColumn",
                "IfcColumnStandardCase",
                "IfcBeam",
                "IfcBeamStandardCase",
                "IfcSlab",
                "IfcFooting",
                "IfcPile",
                "IfcTendon",
            ]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Structure_Tree.svg"))
            elif entity.is_a() in ["IfcSite"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Site_Tree.svg"))
            elif entity.is_a() in ["IfcBuilding"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Building_Tree.svg"))
            elif entity.is_a() in ["IfcBuildingStorey"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Floor_Tree.svg"))
            elif entity.is_a() in [
                "IfcWindow",
                "IfcWindowStandardCase",
                "IfcDoor",
                "IfcDoorStandardCase",
            ]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Window_Tree.svg"))
            elif entity.is_a() in ["IfcRoof"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Roof_Tree.svg"))
            elif entity.is_a() in ["IfcExtrudedAreaSolid", "IfcClosedShell"]:
                item.setIcon(0, QtGui.QIcon(":icons/Tree_Part.svg"))
            elif entity.is_a() in ["IfcFace"]:
                item.setIcon(0, QtGui.QIcon(":icons/Draft_SwitchMode.svg"))
            elif entity.is_a() in ["IfcArbitraryClosedProfileDef", "IfcPolyloop"]:
                item.setIcon(0, QtGui.QIcon(":icons/Draft_Draft.svg"))
            elif entity.is_a() in [
                "IfcPropertySingleValue",
                "IfcQuantityArea",
                "IfcQuantityVolume",
            ]:
                item.setIcon(0, QtGui.QIcon(":icons/Tree_Annotation.svg"))
            elif entity.is_a() in ["IfcMaterial"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Material.svg"))
            elif entity.is_a() in ["IfcReinforcingBar"]:
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Rebar.svg"))
            elif entity.is_a("IfcProduct"):
                item.setIcon(0, QtGui.QIcon(":icons/Arch_Component.svg"))
            item.setFirstColumnSpanned(True)
            item.setData(0, QtCore.Qt.UserRole, eid)
            for childid, grandchildren in children.items():
                self.addEntity(childid, grandchildren, item)
            self.done.append(eid)

    def addAttributes(self, eid, parent):
        "adds the attributes of the given IFC entity under the given QTreeWidgetITem"

        import ifcopenshell
        from PySide import QtGui

        entity = self.ifc[eid]

        i = 0
        while True:
            try:
                argname = entity.attribute_name(i)
            except RuntimeError:
                break
            else:
                try:
                    argvalue = getattr(entity, argname)
                except AttributeError:
                    FreeCAD.Console.PrintError(
                        translate("BIM", "Error in entity")
                        + " "
                        + self.tostr(entity)
                        + "\n"
                    )
                    break
                else:
                    if argname not in ["Id", "GlobalId"]:
                        colored = False
                        if isinstance(argvalue, ifcopenshell.entity_instance):
                            if argvalue.id() == 0:
                                t = self.tostr(argvalue)
                            else:
                                colored = True
                                t = (
                                    "#"
                                    + self.tostr(argvalue.id())
                                    + ": "
                                    + self.tostr(argvalue.is_a())
                                )
                        elif isinstance(argvalue, (list, tuple)):
                            t = ""
                        else:
                            t = self.tostr(argvalue)
                        item = QtGui.QTreeWidgetItem(parent)
                        item.setText(0, self.tostr(argname))
                        if t and (t != "None"):
                            item.setText(1, t)
                            if colored:
                                item.setForeground(1, self.linkbrush)
                                item.setFont(1, self.linkfont)
                            if argname == "Name":
                                item.setFont(1, self.bold)
                        if isinstance(argvalue, (list, tuple)):
                            j = 0
                            for argitem in argvalue:
                                colored = False
                                if isinstance(argitem, ifcopenshell.entity_instance):
                                    if argitem.id() == 0:
                                        t = self.tostr(argitem)
                                    else:
                                        colored = True
                                        t = (
                                            "#"
                                            + self.tostr(argitem.id())
                                            + ": "
                                            + self.tostr(argitem.is_a())
                                        )
                                else:
                                    t = argitem
                                t = self.tostr(t)
                                if j == 0:
                                    item.setText(1, t)
                                    if colored:
                                        item.setForeground(1, self.linkbrush)
                                        item.setFont(1, self.linkfont)
                                else:
                                    subitem = QtGui.QTreeWidgetItem(item)
                                    subitem.setText(1, t)
                                    if colored:
                                        subitem.setForeground(1, self.linkbrush)
                                        subitem.setFont(1, self.linkfont)
                                j += 1
                i += 1

    def addProperties(self, eid, parent):
        "adds properties of a given entity to the given QTReeWidgetItem"

        from PySide import QtGui

        entity = self.ifc[eid]
        if hasattr(entity, "IsDefinedBy"):
            for rel in entity.IsDefinedBy:
                if hasattr(rel, "RelatingPropertyDefinition"):
                    if rel.RelatingPropertyDefinition:
                        item = QtGui.QTreeWidgetItem(parent)
                        item.setText(
                            0,
                            "PropertySet: "
                            + self.tostr(rel.RelatingPropertyDefinition.Name),
                        )
                        item.setFont(0, self.bold)
                        item.setFirstColumnSpanned(True)
                        if hasattr(rel.RelatingPropertyDefinition, "HasProperties"):
                            for prop in rel.RelatingPropertyDefinition.HasProperties:
                                subitem = QtGui.QTreeWidgetItem(item)
                                subitem.setText(0, "Property")
                                self.addAttributes(prop.id(), subitem)

    def tostr(self, text):
        if isinstance(text, str):
            return text
        else:
            return str(text)

    def onSelectTree(self, item, previous):
        "displays attributes and properties of a tree item"

        from PySide import QtCore

        self.backnav.append(previous)
        eid = item.data(0, QtCore.Qt.UserRole)
        self.attributes.clear()
        self.addAttributes(eid, self.attributes)
        self.attributes.expandAll()
        self.properties.clear()
        self.addProperties(eid, self.properties)
        self.properties.expandAll()
        entity = self.ifc[eid]
        if entity.is_a("IfcProduct") and FreeCAD.ActiveDocument:
            self.shapeAction.setEnabled(True)
        else:
            self.shapeAction.setEnabled(False)
        if eid in self.omeshes:
            omesh = self.omeshes[eid]
        else:
            omesh = None
        children = self.getChildren(entity, keys=True)
        for k in children:
            if k in self.omeshes:
                kmesh = self.omeshes[k]
                if omesh:
                    omesh.addMesh(kmesh)
                else:
                    omesh = kmesh
        if omesh:
            if not self.currentmesh:
                self.currentmesh = FreeCAD.ActiveDocument.addObject(
                    "Mesh::Feature", "IFCObjectMesh"
                )
                self.currentmesh.ViewObject.ShapeColor = (0.4, 0.4, 1.0)
            self.currentmesh.Mesh = omesh
            self.currentmesh.ViewObject.show()
            FreeCAD.ActiveDocument.recompute()
        else:
            if self.currentmesh:
                self.currentmesh.ViewObject.hide()

    def onDoubleClickTree(self, item, column):
        "when a property or attribute is double-clicked"

        from PySide import QtCore

        if self.tree:
            txt = item.text(column)
            if txt.startswith("#"):
                eid = txt[1:].split(":")[0]
                target = self.tree.findItems(
                    "#" + eid + " :",
                    QtCore.Qt.MatchStartsWith | QtCore.Qt.MatchRecursive,
                    0,
                )
                if target:
                    self.tree.scrollToItem(target[0])
                    self.tree.setCurrentItem(target[0])


FreeCADGui.addCommand("BIM_IfcExplorer", BIM_IfcExplorer())
