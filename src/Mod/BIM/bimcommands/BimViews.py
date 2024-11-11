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

"""The BIM Views command"""

import sys
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
UPDATEINTERVAL = 2000  # number of milliseconds between BIM Views window update
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class BIM_Views:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Views",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Views", "Views manager"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Views", "Shows or hides the views manager"
            ),
            "Accel": "Ctrl+9",
        }

    def Activated(self):
        from PySide import QtCore, QtGui

        vm = findWidget()
        bimviewsbutton = None
        mw = FreeCADGui.getMainWindow()
        st = mw.statusBar()
        statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
        if statuswidget and hasattr(statuswidget, "bimviewsbutton"):
            bimviewsbutton = statuswidget.bimviewsbutton
        if vm:
            if vm.isVisible():
                vm.hide()
                if bimviewsbutton:
                    bimviewsbutton.setChecked(False)
                PARAMS.SetBool("RestoreBimViews", False)
            else:
                vm.show()
                if bimviewsbutton:
                    bimviewsbutton.setChecked(True)
                PARAMS.SetBool("RestoreBimViews", True)
                self.update()
        else:
            vm = QtGui.QDockWidget()

            # create the dialog
            self.dialog = FreeCADGui.PySideUic.loadUi(":/ui/dialogViews.ui")
            vm.setWidget(self.dialog)
            vm.tree = self.dialog.tree
            vm.closeEvent = self.onClose

            # set context menu
            self.dialog.tree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

            # set button
            self.dialog.menu = QtGui.QMenu()
            for button in [("AddLevel", translate("BIM","Add level")),
                            ("AddProxy", translate("BIM","Add proxy")),
                            ("Delete", translate("BIM","Delete")),
                            ("Toggle", translate("BIM","Toggle on/off")),
                            ("Isolate", translate("BIM","Isolate")),
                            ("SaveView", translate("BIM","Save view position")),
                            ("Rename", translate("BIM","Rename"))]:
                action = QtGui.QAction(button[1])
                self.dialog.menu.addAction(action)
                setattr(self.dialog,"button"+button[0], action)

            # # set button icons
            self.dialog.buttonAddLevel.setIcon(QtGui.QIcon(":/icons/Arch_Floor_Tree.svg"))
            self.dialog.buttonAddProxy.setIcon(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"))
            self.dialog.buttonDelete.setIcon(QtGui.QIcon(":/icons/delete.svg"))
            self.dialog.buttonToggle.setIcon(QtGui.QIcon(":/icons/dagViewVisible.svg"))
            self.dialog.buttonIsolate.setIcon(QtGui.QIcon(":/icons/view-refresh.svg"))
            self.dialog.buttonSaveView.setIcon(QtGui.QIcon(":/icons/view-perspective.svg"))
            self.dialog.buttonRename.setIcon(
                QtGui.QIcon(":/icons/accessories-text-editor.svg")
            )

            # set tooltips
            self.dialog.buttonAddLevel.setToolTip(translate("BIM","Creates a new level"))
            self.dialog.buttonAddProxy.setToolTip(translate("BIM","Creates a new Working Plane Proxy"))
            self.dialog.buttonDelete.setToolTip(translate("BIM","Deletes the selected item"))
            self.dialog.buttonToggle.setToolTip(translate("BIM","Toggles selected items on/off"))
            self.dialog.buttonIsolate.setToolTip(translate("BIM","Turns all items off except the selected ones"))
            self.dialog.buttonSaveView.setToolTip(translate("BIM","Saves the current camera position to the selected items"))
            self.dialog.buttonRename.setToolTip(translate("BIM","Renames the selected item"))

            # connect signals
            self.dialog.buttonAddLevel.triggered.connect(self.addLevel)
            self.dialog.buttonAddProxy.triggered.connect(self.addProxy)
            self.dialog.buttonDelete.triggered.connect(self.delete)
            self.dialog.buttonToggle.triggered.connect(self.toggle)
            self.dialog.buttonIsolate.triggered.connect(self.isolate)
            self.dialog.buttonSaveView.triggered.connect(self.saveView)
            self.dialog.buttonRename.triggered.connect(self.rename)
            self.dialog.tree.itemClicked.connect(self.select)
            self.dialog.tree.itemDoubleClicked.connect(show)
            self.dialog.tree.itemChanged.connect(self.editObject)
            self.dialog.tree.customContextMenuRequested.connect(self.onContextMenu)
            # delay connecting after FreeCAD finishes setting up
            QtCore.QTimer.singleShot(UPDATEINTERVAL, self.connectDock)

            # set the dock widget
            area = PARAMS.GetInt("BimViewArea", 1)
            floating = PARAMS.GetBool("BimViewFloat", True)
            height = PARAMS.GetBool("BimViewWidth", 200)
            width = PARAMS.GetBool("BimViewHeight", 300)
            tabs = PARAMS.GetString("BimViewTabs", "")
            vm.setObjectName("BIM Views Manager")
            vm.setWindowTitle(translate("BIM", "BIM"))
            mw = FreeCADGui.getMainWindow()
            vm.setFloating(floating)
            vm.setGeometry(vm.x(), vm.y(), width, height)
            mw.addDockWidget(self.getDockArea(area), vm)
            if tabs:
                tabs = tabs.split("+")
                for tab in tabs:
                    dw = mw.findChild(QtGui.QDockWidget, tab)
                    if dw:
                        mw.tabifyDockWidget(dw, vm)
                        break

            # restore saved settings
            vm.tree.setColumnWidth(0, PARAMS.GetInt("ViewManagerColumnWidth", 100))
            vm.setFloating(PARAMS.GetBool("ViewManagerFloating", False))

            # check the status bar button
            if bimviewsbutton:
                bimviewsbutton.setChecked(True)
            PARAMS.SetBool("RestoreBimViews", True)

            self.update()

    def onClose(self, event):
        from PySide import QtGui

        st = FreeCADGui.getMainWindow().statusBar()
        statuswidget = st.findChild(QtGui.QToolBar, "BIMStatusWidget")
        if statuswidget and hasattr(statuswidget, "bimviewsbutton"):
            statuswidget.bimviewsbutton.setChecked(False)
        PARAMS.SetBool("RestoreBimViews", False)

    def connectDock(self):
        "watch for dock location"

        vm = findWidget()
        if vm:
            vm.dockLocationChanged.connect(self.onDockLocationChanged)

    def update(self, retrigger=True):
        "updates the view manager"

        from PySide import QtCore, QtGui

        vm = findWidget()
        if vm and FreeCAD.ActiveDocument:
            if vm.isVisible() and (vm.tree.state() != vm.tree.State.EditingState):
                vm.tree.clear()
                import Draft

                treeViewItems = []  # QTreeWidgetItem to Display in tree
                lvHold = []
                soloProxyHold = []
                for obj in FreeCAD.ActiveDocument.Objects:
                    t = Draft.getType(obj)
                    if obj and (
                        t
                        in [
                            "Building",
                            "BuildingPart",
                            "IfcBuilding",
                            "IfcBuildingStorey",
                        ]
                    ):
                        if (
                            t in ["Building", "IfcBuilding"]
                            or getattr(obj, "IfcType", "") == "Building"
                        ):
                            building, _ = getTreeViewItem(obj)
                            subObjs = obj.Group
                            # find every levels belongs to the building
                            for subObj in subObjs:
                                if Draft.getType(subObj) in [
                                    "BuildingPart",
                                    "Building Storey",
                                    "IfcBuildingStorey",
                                ]:
                                    lv, lvH = getTreeViewItem(subObj)
                                    subSubObjs = subObj.Group
                                    # find every working plane proxy belongs to the level
                                    for subSubObj in subSubObjs:
                                        if (
                                            Draft.getType(subSubObj)
                                            == "WorkingPlaneProxy"
                                        ):
                                            wp, _ = getTreeViewItem(subSubObj)
                                            lv.addChild(wp)
                                    lvHold.append((lv, lvH))
                            sortLvHold = sorted(lvHold, key=lambda x: x[1])
                            sortLvItems = [item[0] for item in sortLvHold]
                            for lvItem in sortLvItems:
                                building.addChild(lvItem)
                            treeViewItems.append(building)
                            lvHold.clear()

                        if (
                            t in ["Building Storey", "IfcBuildingStorey"]
                            or getattr(obj, "IfcType", "") == "Building Storey"
                        ):
                            if (
                                Draft.getType(getParent(obj))
                                in ["Building", "IfcBuilding"]
                                or getattr(getParent(obj), "IfcType", "") == "Building"
                            ):
                                continue
                            lv, lvH = getTreeViewItem(obj)
                            subObjs = obj.Group
                            # find every working plane proxy belongs to the level
                            for subObj in subObjs:
                                if Draft.getType(subObj) == "WorkingPlaneProxy":
                                    wp, _ = getTreeViewItem(subObj)
                                    lv.addChild(wp)
                            lvHold.append((lv, lvH))
                    if obj and (t == "WorkingPlaneProxy"):
                        if (
                            obj.getParent()
                            and obj.getParent().IfcType == "Building Storey"
                        ):
                            continue
                        wp, _ = getTreeViewItem(obj)
                        soloProxyHold.append(wp)
                sortLvHold = sorted(lvHold, key=lambda x: x[1])
                sortLvItems = [item[0] for item in sortLvHold]
                treeViewItems = treeViewItems + sortLvItems + soloProxyHold
                vm.tree.addTopLevelItems(treeViewItems)

                # add views
                ficon = QtGui.QIcon.fromTheme("folder", QtGui.QIcon(":/icons/folder.svg"))
                views = self.getViews()
                if views:
                    top = QtGui.QTreeWidgetItem([translate("BIM","2D Views"), ""])
                    top.setIcon(0, ficon)
                    for v in views:
                        if hasattr(v, "Label"):
                            i = QtGui.QTreeWidgetItem([v.Label, ""])
                            if hasattr(v.ViewObject, "Icon"):
                                i.setIcon(0, v.ViewObject.Icon)
                            i.setToolTip(0, v.Name)
                            top.addChild(i)
                    vm.tree.addTopLevelItem(top)

                # add pages
                pages = self.getPages()
                if pages:
                    top = QtGui.QTreeWidgetItem([translate("BIM","Sheets"), ""])
                    top.setIcon(0, ficon)
                    for p in pages:
                        i = QtGui.QTreeWidgetItem([p.Label, ""])
                        if hasattr(p.ViewObject, "Icon"):
                                i.setIcon(0, p.ViewObject.Icon)
                        i.setToolTip(0, p.Name)
                        top.addChild(i)
                    vm.tree.addTopLevelItem(top)

                # set TreeVinew Item selected if obj is selected
                objSelected = FreeCADGui.Selection.getSelection()
                objNameSelected = [obj.Label for obj in objSelected]

                allItemsInTree = getAllItemsInTree(vm.tree)
                for item in allItemsInTree:
                    if item.text(0) in objNameSelected:
                        item.setSelected(True)

        if retrigger:
            QtCore.QTimer.singleShot(UPDATEINTERVAL, self.update)

        # save state
        PARAMS.SetInt("ViewManagerColumnWidth", vm.tree.columnWidth(0))
        PARAMS.SetBool("ViewManagerFloating", vm.isFloating())

        # expand
        vm.tree.expandAll()

    def select(self, item, column=None):
        "selects a doc object corresponding to an item"

        item.setSelected(True)
        name = item.toolTip(0)
        if name:
            obj = FreeCAD.ActiveDocument.getObject(name)
            if obj:
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(obj)

    def addLevel(self):
        "adds a building part"

        import Arch

        FreeCAD.ActiveDocument.openTransaction("Create BuildingPart")
        Arch.makeFloor()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        self.update(False)

    def addProxy(self):
        "adds a WP proxy"

        import Draft

        FreeCAD.ActiveDocument.openTransaction("Create WP Proxy")
        Draft.makeWorkingPlaneProxy(FreeCAD.DraftWorkingPlane.getPlacement())
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        self.update(False)

    def delete(self):
        "deletes the selected object"

        vm = findWidget()
        if vm:
            if vm.tree.selectedItems():
                FreeCAD.ActiveDocument.openTransaction("Delete")
                for item in vm.tree.selectedItems():
                    obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
                    if obj:
                        FreeCAD.ActiveDocument.removeObject(obj.Name)
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                self.update(False)

    def rename(self):
        "renames the selected object"

        vm = findWidget()
        if vm:
            if vm.tree.selectedItems():
                if vm.tree.selectedItems():
                    item = vm.tree.selectedItems()[-1]
                    vm.tree.editItem(item, 0)

    def editObject(self, item, column):
        "renames or edit height of the actual object"

        obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
        if obj:
            if column == 0:
                obj.Label = item.text(column)
            if column == 1:
                obj.Placement.Base.z = FreeCAD.Units.parseQuantity(item.text(column))

    def toggle(self):
        "toggle selected item on/off"

        vm = findWidget()
        if vm:
            for item in vm.tree.selectedItems():
                obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
                if obj:
                    obj.ViewObject.Visibility = not (obj.ViewObject.Visibility)
            FreeCAD.ActiveDocument.recompute()

    def isolate(self):
        "turns all items off except the selected ones"

        vm = findWidget()
        if vm:
            onnames = [item.toolTip(0) for item in vm.tree.selectedItems()]
            for i in range(vm.tree.topLevelItemCount()):
                item = vm.tree.topLevelItem(i)
                if item.toolTip(0) not in onnames:
                    obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
                    if obj:
                        obj.ViewObject.Visibility = False
            FreeCAD.ActiveDocument.recompute()

    def saveView(self):
        "save the current camera angle to the selected item"

        vm = findWidget()
        if vm:
            for item in vm.tree.selectedItems():
                obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
                if obj:
                    if hasattr(obj.ViewObject.Proxy, "writeCamera"):
                        obj.ViewObject.Proxy.writeCamera()
        FreeCAD.ActiveDocument.recompute()

    def onDockLocationChanged(self, area):
        """Saves dock widget size and location"""

        PARAMS.SetInt("BimViewArea", int(area))
        mw = FreeCADGui.getMainWindow()
        vm = findWidget()
        if vm:
            PARAMS.SetBool("BimViewFloat", vm.isFloating())
            PARAMS.SetInt("BimViewWidth", vm.width())
            PARAMS.SetInt("BimViewHeight", vm.height())
            tabs = "+".join([o.objectName() for o in mw.tabifiedDockWidgets(vm)])
            PARAMS.SetString("BimViewTabs", tabs)

    def getDockArea(self, area):
        """Turns an int into a qt dock area"""

        from PySide import QtCore

        if area == 1:
            return QtCore.Qt.LeftDockWidgetArea
        elif area == 4:
            return QtCore.Qt.TopDockWidgetArea
        elif area == 8:
            return QtCore.Qt.BottomDockWidgetArea
        else:
            return QtCore.Qt.RightDockWidgetArea

    def onContextMenu(self, pos):
        """Fires the context menu"""
        self.dialog.menu.exec_(self.dialog.tree.mapToGlobal(pos))

    def getViews(self):
        """Returns a list of 2D views"""
        views = []
        for p in self.getPages():
            for v in p.Views:
                if getattr(v, "Source", None):
                    views.append(v.Source)
        return views

    def getPages(self):
        """Returns a list of TD pages"""
        return [o for o in FreeCAD.ActiveDocument.Objects if o.isDerivedFrom('TechDraw::DrawPage')]


# These functions need to be localized outside the command class, as they are used outside this module


def findWidget():
    "finds the manager widget, if present"

    from PySide import QtGui

    mw = FreeCADGui.getMainWindow()
    vm = mw.findChild(QtGui.QDockWidget, "BIM Views Manager")
    if vm:
        return vm
    return None


def show(item, column=None):
    "item has been double-clicked"

    obj = None
    vm = findWidget()
    if isinstance(item, str) or (
        (sys.version_info.major < 3) and isinstance(item, unicode)
    ):
        # called from Python code
        obj = FreeCAD.ActiveDocument.getObject(item)
    else:
        # called from GUI
        if column == 1:
            # user clicked the level field
            if vm:
                vm.tree.editItem(item, column)
                return
        else:
            # TODO find a way to not edit the object name
            obj = FreeCAD.ActiveDocument.getObject(item.toolTip(0))
    if obj:
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(obj)
        if obj.isDerivedFrom("TechDraw::DrawPage"):

            # case 1: the object is a TD page. We switch to it simply
            obj.ViewObject.Visibility=True
        elif isView(obj):

            # case 2: the object is a 2D view
            ssel = [obj]+obj.OutListRecursive
            FreeCADGui.Selection.clearSelection()
            for o in ssel:
                o.ViewObject.Visibility = True
                FreeCADGui.Selection.addSelection(o)
            if not hasattr(FreeCADGui.ActiveDocument.ActiveView, "getSceneGraph"):
                # Find first 3d view and switch to it
                for w in FreeCADGui.getMainWindow().getWindows():
                    if hasattr(w, "getSceneGraph"):
                        FreeCADGui.getMainWindow().setActiveWindow(w)
                        break
            FreeCADGui.runCommand('Std_OrthographicCamera')
            FreeCADGui.ActiveDocument.ActiveView.viewTop()
            FreeCADGui.SendMsgToActiveView("ViewSelection")
            FreeCADGui.ActiveDocument.ActiveView.viewTop()
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(obj)
        else:

            # case 3: This is maybe a BuildingPart. Place the WP on it
            FreeCADGui.runCommand("Draft_SelectPlane")
    if vm:
        # store the last double-clicked item for the BIM WPView command
        if isinstance(item, str) or (
            (sys.version_info.major < 3) and isinstance(item, unicode)
        ):
            vm.lastSelected = item
        else:
            vm.lastSelected = item.toolTip(0)


def isView(obj):
    """Returns true if this object is used as Source of a view on a TD page"""

    for p in obj.InList:
        if p.isDerivedFrom("TechDraw::DrawView"):
            if hasattr(p, "Source"):
                if p.Source == obj:
                    return True
    if getattr(obj,"DrawingView",False):
        return True
    return False


def getTreeViewItem(obj):
    """
    from FreeCAD object make the TreeWidgetItem including icon Label and LevelHeight
    and also make a level height in number to sort the order after
    """
    from PySide import QtCore, QtGui

    z = obj.Placement.Base.z
    lvHStr = FreeCAD.Units.Quantity(z, FreeCAD.Units.Length).UserString
    if z == 0:
        # override with Elevation property if available
        if hasattr(obj, "Elevation"):
            z = obj.Elevation.Value
            lvHStr = obj.Elevation.UserString
    it = QtGui.QTreeWidgetItem([obj.Label, lvHStr])
    it.setFlags(it.flags() | QtCore.Qt.ItemIsEditable)
    it.setToolTip(0, obj.Name)
    if obj.ViewObject:
        if hasattr(obj.ViewObject, "Proxy") and hasattr(
            obj.ViewObject.Proxy, "getIcon"
        ):
            it.setIcon(0, QtGui.QIcon(obj.ViewObject.Proxy.getIcon()))
    return (it, z)


def getAllItemsInTree(tree_widget):
    "return list of all items in QtreeWidget"

    def get_child_items(parent_item):
        child_items = []
        # get how many sub items
        child_count = parent_item.childCount()
        for j in range(child_count):
            child_item = parent_item.child(j)
            child_items.append(child_item)
            child_items.extend(get_child_items(child_item))

        return child_items

    all_items = []
    # get top level items
    top_level_item_count = tree_widget.topLevelItemCount()
    for i in range(top_level_item_count):
        top_level_item = tree_widget.topLevelItem(i)
        all_items.append(top_level_item)
        # iterate sub-items
        all_items.extend(get_child_items(top_level_item))

    return all_items


def getParent(obj):
    "return the first parent of this object"

    if obj.getParent():
        return obj.getParent()
    else:
        for parent in obj.InList:
            if hasattr(parent, "Group") and obj in parent.Group:
                return parent


FreeCADGui.addCommand("BIM_Views", BIM_Views())
