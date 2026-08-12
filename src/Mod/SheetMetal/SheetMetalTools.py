########################################################################
#
#  SheetMetalTools.py
#
#  Copyright 2024 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import importlib
import math
import os
import re

import FreeCAD
import importDXF
import importSVG
import Part

translate = FreeCAD.Qt.translate

mod_path = os.path.dirname(__file__)
icons_path = os.path.join(mod_path, "Resources", "icons")
panels_path = os.path.join(mod_path, "Resources", "panels")
language_path = os.path.join(mod_path, "translations")
params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/SheetMetal")
smEpsilon = FreeCAD.Base.Precision.approximation()
smForceRecompute = False
smObjectsToRecompute = set()
translatedPreviewText = translate("SheetMetalTools", "Preview")
cancelText = translate("SheetMetalTools", "Cancel...")
clearText = translate("SheetMetalTools", "Clear...")
selClearSelText = translate("SheetMetalTools", "Clear Selected")
selClearAllText = translate("SheetMetalTools", "Clear All")


def taskSaveDefaults(obj, varList):
    for var in varList:
        if isinstance(var, tuple):
            var, saveVar = var
        else:
            saveVar = "default" + var
        val = getattr(obj, var)
        if hasattr(val, "Value"):
            val = val.Value
        if isinstance(val, bool):
            params.SetBool(saveVar, val)
        elif isinstance(val, float):
            params.SetFloat(saveVar, val)
        elif isinstance(val, int):
            params.SetInt(saveVar, val)
        else:
            params.SetString(saveVar, str(val))


def taskRestoreDefaults(obj, varList):
    for var in varList:
        if isinstance(var, tuple):
            var, saveVar = var
        else:
            saveVar = "default" + var
        val = getattr(obj, var)
        if hasattr(val, "Value"):
            val = val.Value
        if isinstance(val, bool):
            newVal = params.GetBool(saveVar, val)
        elif isinstance(val, float):
            newVal = params.GetFloat(saveVar, val)
        elif isinstance(val, int):
            newVal = params.GetInt(saveVar, val)
        else:
            newVal = params.GetString(saveVar, str(val))
        setattr(obj, var, newVal)


def isGuiLoaded():
    if hasattr(FreeCAD, "GuiUp"):
        return FreeCAD.GuiUp
    return False


if isGuiLoaded():
    from PySide import QtCore, QtGui
    from PySide.QtWidgets import QHeaderView

    Gui = FreeCAD.Gui

    smExportDirs = {}


    def smWarnDialog(msg):
        diag = QtGui.QMessageBox(
            QtGui.QMessageBox.Warning,
            FreeCAD.Qt.translate("QMessageBox", "Error in macro MessageBox"),
            msg,
        )
        diag.setWindowModality(QtCore.Qt.ApplicationModal)
        diag.exec_()


    class SelectionObserver:

        """Synchronize the task panel widget with the selection.

        Used in task panels of SheetMetal Workbench commands that use
        QTreeWidget as a list of objects. Works after press a 'Select'
        button from a task panel to select objects and until exiting
        the selection mode.

        """

        def __new__(cls, params_):
            """Manage the creation and deletion of an Observer.

            Create the Observer when the 'Select' button is pressed from
            a task panel to select objects and delete the Observer when
            the selection is complete.

            Note:
                Called from the `taskConnectSelection` function.

            Args:
                params_: An instance of SMSelectionParameters class.

            """
            if params_.ClearButton.isVisible():
                if not hasattr(cls, "observer"):
                    setattr(cls, "observer", super().__new__(cls))
                    Gui.Selection.addObserver(getattr(cls, "observer"))
                    return getattr(cls, "observer")
                else:
                    return None
            else:
                cls._delete_observer()
                return None

        def __init__(self, selParams):
            """Initialize an instance of SelectionObserver.

            Args:
                params_: An instance of SMSelectionParameters class.

            """
            self.sp = selParams
            self.widget = selParams.dispWidget
            self.widget.destroyed.connect(self._delete_observer)

            self.widget.setMouseTracking(True)
            self.widget.entered.connect(self._highlight_element)

        def sync_selection(self):
            """Update the selection list in the task panel widget."""
            scrollbar = self.widget.verticalScrollBar()
            scrollbar_position = scrollbar.value()
            self.widget.clear()

            for obj in Gui.Selection.getCompleteSelection():
                add_qtreewidget_item(self.widget, obj.ObjectName, obj.SubElementNames[0], True)

            scrollbar.setValue(scrollbar_position)

        def addSelection(self, *args):
            """Execute when adding an object to the selection.

            Note:
                The name of this function must be the same as in
                the `FreeCADGui.Selection`.

            """
            _, obj_name, sub_name, _ = args
            add_qtreewidget_item(self.widget, obj_name, sub_name, True)

        def clearSelection(self, *args):
            """Execute when clearing the selection.

            Note:
                The name of this function must be the same as in
                the `FreeCADGui.Selection`.

            """
            self.sync_selection()

        def removeSelection(self, *args):
            """Execute when removing an object from the selection.

            Note:
                The name of this function must be the same as in
                the `FreeCADGui.Selection`.

            """
            self.sync_selection()

        @classmethod
        def _delete_observer(cls):
            """Uninstall an observer."""
            if hasattr(cls, "observer"):
                cls.observer.widget.entered.disconnect(cls._highlight_element)
                Gui.Selection.removeObserver(getattr(cls, "observer"))
                delattr(cls, "observer")

        @staticmethod
        def _highlight_element(index):
            """Highlight a 3D element correspond to QTreeWidget item.

            Find the item's position for QTreeWidget item into
            QModelIndex and using that position to get access to the
            3D element in the global selection list. After that
            preselect that element in 3D for highlighting.

            Args:
                index: QModelIndex of the hovered item in QTreeWidget.

            """
            try:
                item = Gui.Selection.getCompleteSelection()[index.row()]
            except IndexError:
                return None
            item_name = ".".join((item.ObjectName, item.SubElementNames[0]))
            parent = smGetParentBody(item.Object)
            try:
                Gui.Selection.setPreselection(parent, item_name)
            except ValueError:
                return None
            return None


    class SMSingleSelectionObserver:
        """Used for tasks that needs to be aware of selection
         changes.
         """

        def __init__(self):
            self.selParams = None

        def addSelection(self, document, obj, element, position):
            taskSingleSelectionChanged(self.selParams)


    smSingleSelObserver = SMSingleSelectionObserver()
    Gui.Selection.addObserver(smSingleSelObserver)


    class SMSelectionParameters:
        """Helper class for selection operations."""

        def __init__(self, addRemoveButton, dispWidget, obj, allowedTypes,
                     propetyName="baseObject", hideObject=True):
            self.addRemoveButton = addRemoveButton
            self.dispWidget = dispWidget
            self.obj = obj
            self.allowedTypes = allowedTypes
            self.propetyName = propetyName
            self.hideObject = hideObject

            self.SelectState = True
            self.ObjWasVisible = False
            self.OriginalText = ""
            self.AlternateText = ""
            self.ClearButton = None
            self.SelPropertyName = None
            self.AllowZeroSelection = False
            self.ConstrainToObject = None
            self.ValueChangedCallback = None
            self.ToggleMode = False
            self.Count = 0
            self.VisibilityControlledWidgets = []
            self.EnableControlledWidgets = []
            self.HideRefObject = True

        def allowAllTypes(self):
            if isinstance(self.allowedTypes, tuple):
                return len(self.allowedTypes[1]) == 0
            return len(self.allowedTypes) == 0

        # Allowed type formats:
        # 1. list of allowed subelement types: ["Face", "Edge"]
        # 2. tuple of allowed object type and list of allowed subelement
        #    types: ("Sketch", []).
        #    Empy list means all subelement types are allowed
        # 3. list of above 1 or 2: [("DatumPlane", []), ["Face"]].
        def matchAllowedType(self, selobj, selSubNames, allowedTypes):
            allowedObjType = ""
            if isinstance(allowedTypes, tuple):
                allowedObjType, allowedTypes = allowedTypes
            if allowedObjType not in selobj.TypeId:
                return False
            if len(allowedTypes) == 0:
                return True
            if isinstance(allowedTypes[0], str):
                for element in selSubNames:
                    if not smGetFeatureType(element) in allowedTypes:
                        return False
                return True
            for allowedSubType in allowedTypes:
                if self.matchAllowedType(selobj, selSubNames, allowedSubType):
                    return True
            return False
        
        def getAllowedSubObjects(self):
            origprop = getattr(self.obj, self.propetyName)
            selobj, selSubNames = origprop
            allowedObjList = []
            for subObject in selSubNames:
                if self.matchAllowedType(selobj, [subObject], self.allowedTypes):
                    allowedObjList.append(subObject)
            return allowedObjList
        
        def getOtherSubObjects(self):
            origprop = getattr(self.obj, self.propetyName)
            selobj, selSubNames = origprop
            otherObjList = []
            for subObject in selSubNames:
                if not self.matchAllowedType(selobj, [subObject], self.allowedTypes):
                    otherObjList.append(subObject)
            return otherObjList

        def getAllowedTypesList(self, allowedTypes):
            allowedObjType = ""
            if isinstance(allowedTypes, tuple):
                allowedObjType, allowedTypes = allowedTypes
            allowedTypesList = [] if allowedObjType == "" else [allowedObjType]
            for allowedSubType in allowedTypes:
                if isinstance(allowedSubType, str):
                    allowedTypesList.append(allowedSubType)
                else:
                    allowedTypesList += self.getAllowedTypesList(allowedSubType)
            return allowedTypesList

        def getAlowedTypesString(self, allowedTypes, seperator=", "):
            return seperator.join(self.getAllowedTypesList(allowedTypes))

        def verifySelection(self):
            selection = Gui.Selection.getSelectionEx()
            origprop = getattr(self.obj, self.propetyName)
            selobj = origprop[0] if isinstance(origprop, tuple) else origprop
            selSubNames = []
            if len(selection) > 0:
                selobj = selection[0].Object
                selSubNames = list(selection[0].SubElementNames)
            if selobj.isDerivedFrom("App::Link"):
                selobj = selobj.LinkedObject

            if self.ConstrainToObject is not None and not selobj is self.ConstrainToObject:
                smWarnDialog(translate("SheetMetalTools",
                    "Features are selected from a wrong object\n"
                    "Please select features from '{}' object"
                ).format(self.ConstrainToObject.Label))
                return (None, None)

            if not self.matchAllowedType(selobj, selSubNames, self.allowedTypes):
                smWarnDialog(translate("SheetMetalTools",
                    "Non valid element type selected\n"
                    "Valid element types: {}"
                ).format(self.getAlowedTypesString(self.allowedTypes)))
                return (None, None)

            return (selobj, selSubNames)

        def updateVisibilityControlledWidgets(self):
            for widget, state in self.VisibilityControlledWidgets:
                widget.setVisible(state ^ self.SelectState)
            for widget, state in self.EnableControlledWidgets:
                widget.setEnabled(state ^ self.SelectState)

        def setVisibilityControlledWidgets(self, visWidgets, enWidgets=None):
            self.VisibilityControlledWidgets = visWidgets
            self.EnableControlledWidgets = [] if enWidgets is None else enWidgets
            self.updateVisibilityControlledWidgets()


    def smSelectGreedy():
        # Compatibility with FC link version.
        if hasattr(Gui.Selection, "setSelectionStyle"):
            Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.GreedySelection)


    def smSelectNormal():
        # Compatibility with FC link version.
        if hasattr(Gui.Selection, "setSelectionStyle"):
            Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.NormalSelection)


    def smSelectSubObjects(obj, subObjects):
        # Doing this to avoid the bug in FreeCAD that does not select
        # the subobjects if the object is a binder and the function
        # is Gui.Selection.addSelection(obj, subObjects).
        docName = obj.Document.Name
        if smIsPartDesign(obj):
            bodyName = smGetParentBody(obj).Name
            for subObj in subObjects:
                subName = f"{obj.Name}.{subObj}"
                Gui.Selection.addSelection(docName, bodyName, subName)
        else:
            for subName in subObjects:
                Gui.Selection.addSelection(docName, obj.Name, subName)


    def smHideObjects(*args):
        for arg in args:
            if arg:
                obj = Gui.ActiveDocument.getObject(arg.Name)
                if obj:
                    obj.Visibility = False


    def add_qtreewidget_item(qwidget, obj_name, sub_name, showCheckBoxes=False):
        """Add an item to a QTreeWidget with presetting.

        Args:
            widget: The QTreeWidget to which the item will be added.
            obj_name: Name of the object.
            sub_name: Name of the sub-element.

        """
        item = QtGui.QTreeWidgetItem(qwidget)
        item.setIcon(0, QtGui.QIcon(":/icons/Tree_Part.svg"))
        item.setText(0, obj_name)
        item.setText(1, sub_name)
        if showCheckBoxes:
            item.setCheckState(0, QtCore.Qt.Unchecked)


    # Task panel helper code.
    def taskPopulateSelectionList(qwidget, baseObject, showCheckBoxes=False):
        qwidget.clear()
        if baseObject is None:
            return
        obj, items = baseObject
        if not isinstance(items, list):
            items = [items]
        for subf in items:
            add_qtreewidget_item(qwidget, obj.Name, subf, showCheckBoxes)


    def taskPopulateSelectionSingle(textbox, selObject):
        if selObject is None:
            textbox.setText("")
        elif isinstance(selObject, tuple):
            obj, items = selObject
            item = "None" if len(items) == 0 else items[0]
            textbox.setText(f"{obj.Name}: {item}")
        else:
            textbox.setText(selObject.Name)


    def updateTaskTitleIcon(task):
        if hasattr(task, "form"):
            if hasattr(task.obj.ViewObject.Proxy, "getIcon"):
                task.form.setWindowIcon(QtGui.QIcon(task.obj.ViewObject.Proxy.getIcon()))
        return

    def _taskMultiSelSetCheckBoxState(sp: SMSelectionParameters, state: bool):
        for index in range(sp.dispWidget.topLevelItemCount()):
            item = sp.dispWidget.topLevelItem(index)
            if state:
                item.setCheckState(0, QtCore.Qt.Unchecked)
            else:
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsUserCheckable)

    def _taskMultiSelectionModeClicked(sp: SMSelectionParameters):
        baseObj = getattr(sp.obj, sp.propetyName)
        if sp.SelectState:
            if sp.hideObject:
                sp.obj.Visibility = False
            Gui.Selection.clearSelection()
            if baseObj is not None:
                sp.ObjWasVisible = baseObj[0].Visibility
                baseObj[0].Visibility = True
                selobjs = sp.getAllowedSubObjects()
                smSelectSubObjects(baseObj[0], selobjs)
            # Gui.Selection.addSelection(baseObj[0],baseObj[1])  # Does not work on binder.
            smSelectGreedy()
            sp.addRemoveButton.setText(translatedPreviewText)
            if sp.ClearButton is not None:
                sp.ClearButton.setVisible(True)
                _taskMultiSelSetCheckBoxState(sp, True)
            sp.SelectState = False
        else:
            selObj, selSubNames = sp.verifySelection()
            if selObj is not None:
                if sp.ClearButton is not None:
                    sp.ClearButton.setVisible(False)
                    _taskMultiSelSetCheckBoxState(sp, False)
                joinedSubNames = selSubNames.copy()
                joinedSubNames.extend(sp.getOtherSubObjects())
                setattr(sp.obj, sp.propetyName, (selObj, joinedSubNames))
                # updateSelectionElements(sp.obj, sp.allowedTypes, sp.propetyName)
                baseObj = getattr(sp.obj, sp.propetyName)
                Gui.Selection.clearSelection()
                smSelectNormal()
                sp.obj.Document.recompute()
                baseObj[0].Visibility = sp.ObjWasVisible
                if sp.hideObject:
                    sp.obj.Visibility = True
                sp.addRemoveButton.setText(sp.OriginalText)
                sp.SelectState = True
                taskPopulateSelectionList(sp.dispWidget,(selObj, selSubNames), False)
                if sp.ValueChangedCallback is not None:
                    sp.ValueChangedCallback(sp, selObj, joinedSubNames)

    def _taskMultiSelGetUnSelectedItems(sp: SMSelectionParameters):
        selectedItems = []
        for index in range(sp.dispWidget.topLevelItemCount()):
            item = sp.dispWidget.topLevelItem(index)
            if not (item.checkState(0) == QtCore.Qt.Checked):
                selectedItems.append((item.text(0), item.text(1)))
        return selectedItems

    def _taskMultiSelHasSelectedItems(sp: SMSelectionParameters):
        for index in range(sp.dispWidget.topLevelItemCount()):
            item = sp.dispWidget.topLevelItem(index)
            if item.checkState(0) == QtCore.Qt.Checked:
                return True
        return False

    def _taskMultiSelCheckChanged(sp: SMSelectionParameters, 
                                  _item: QtGui.QTreeWidgetItem = None, _col: int = 0):
        if sp.ClearButton is None:
            return
        if _taskMultiSelHasSelectedItems(sp):
            sp.ClearButton.setText(selClearSelText)
        else:
            sp.ClearButton.setText(selClearAllText)
        
    def _taskMultiSelClearClicked(sp: SMSelectionParameters):
        if not _taskMultiSelHasSelectedItems(sp):
            Gui.Selection.clearSelection()
            return
        remainingItems = _taskMultiSelGetUnSelectedItems(sp)
        selSubObj = []
        for objName, subName in remainingItems:
            selSubObj.append(subName)
        baseObj = getattr(sp.obj, sp.propetyName)
        Gui.Selection.clearSelection()
        smSelectSubObjects(baseObj[0], selSubObj)


    def taskConnectSelection(addRemoveButton, treeWidget, obj, allowedTypes, clearButton=None,
                             propetyName="baseObject", hideObject=True) -> SMSelectionParameters:
        """Connects a selection button to a tree widget for selecting
         multiple objects.
         """
        sp = SMSelectionParameters(addRemoveButton, treeWidget, obj, allowedTypes,
                                   propetyName, hideObject)
        sp.ClearButton = clearButton
        baseObj = getattr(obj, propetyName)
        treeWidget.header().setSectionResizeMode(QHeaderView.ResizeToContents)
        filteredBaseObj = (baseObj[0], sp.getAllowedSubObjects())
        taskPopulateSelectionList(treeWidget, filteredBaseObj, False)
        if clearButton is not None:
            clearButton.setVisible(False)
            clearButton.clicked.connect(lambda _value: _taskMultiSelClearClicked(sp))
            
        sp.OriginalText = addRemoveButton.text()
        treeWidget.itemChanged.connect(lambda item, col: _taskMultiSelCheckChanged(sp, item, col))
        addRemoveButton.clicked.connect(lambda _value: _taskMultiSelectionModeClicked(sp))
        addRemoveButton.clicked.connect(lambda: SelectionObserver(sp))
        _taskMultiSelCheckChanged(sp)
        return sp


    def _taskGetSelectedObjects(sp: SMSelectionParameters):
        selObject = getattr(sp.obj, sp.SelPropertyName)
        if isinstance(selObject, tuple):
            selObject = selObject[0]
        baseObject = getattr(sp.obj, sp.propetyName)[0] if hasattr(sp.obj, sp.propetyName) else None
        return selObject, baseObject


    def _taskUpdateSingleSelection(sp: SMSelectionParameters):
        selObject, baseObject = _taskGetSelectedObjects(sp)
        smSingleSelObserver.selParams = None
        if baseObject is not None:
            baseObject.Visibility = sp.ObjWasVisible
        if sp.hideObject:
            sp.obj.Visibility = True
        if selObject is not None and sp.HideRefObject:
            selObject.Visibility = False
        taskPopulateSelectionSingle(sp.dispWidget, getattr(sp.obj, sp.SelPropertyName))


    def _taskSingleSelModeClicked(sp: SMSelectionParameters):
        selObject, baseObject = _taskGetSelectedObjects(sp)
        if sp.SelectState:
            Gui.Selection.clearSelection()
            if smSingleSelObserver.selParams is not None:
                return
            sp.ObjWasVisible = False
            if baseObject is not None:
                sp.ObjWasVisible = baseObject.Visibility
                baseObject.Visibility = True
            if sp.hideObject:
                sp.obj.Visibility = False
            if selObject is not None:
                selObject.Visibility = True
            smSingleSelObserver.selParams = sp
            sp.dispWidget.setText(f"Select {sp.OriginalText}...")
            sp.addRemoveButton.setText(sp.AlternateText)
            sp.SelectState = False
            sp.updateVisibilityControlledWidgets()
        else:
            if sp.ToggleMode:
                setattr(sp.obj, sp.SelPropertyName, None)
                sp.obj.Document.recompute()
            _taskUpdateSingleSelection(sp)
            sp.addRemoveButton.setText(sp.OriginalText)
            sp.SelectState = True
            sp.updateVisibilityControlledWidgets()


    def taskSingleSelectionChanged(sp: SMSelectionParameters):
        if sp is None:
            return

        selobj, selSubNames = sp.verifySelection()
        Gui.Selection.clearSelection()
        if selobj is None:
            return

        selobj, selSubNames = smUpdateLinks(sp.obj, selobj, selSubNames)
        baseObject = selobj if sp.allowAllTypes() else (selobj, selSubNames)
        setattr(sp.obj, sp.SelPropertyName, baseObject)
        if sp.ValueChangedCallback is not None:
            sp.ValueChangedCallback(sp, selobj, selSubNames)

        sp.obj.Document.recompute()
        if sp.ToggleMode:
            _taskUpdateSingleSelection(sp)
        else:
            _taskSingleSelModeClicked(sp)


    def taskConnectSelectionSingle(button, textbox, obj, SelPropertyName, allowedTypes,
                                   propetyName="baseObject", hideObject=True
    ) -> SMSelectionParameters:
        """Connects a selection button to a textbox for selecting
         a single object.
         """
        sp = SMSelectionParameters(button, textbox, obj, allowedTypes,
                                   propetyName, hideObject)
        sp.SelPropertyName = SelPropertyName
        sp.OriginalText = sp.addRemoveButton.text()
        sp.AlternateText = cancelText
        taskPopulateSelectionSingle(textbox, getattr(obj, SelPropertyName))
        button.clicked.connect(lambda _value: _taskSingleSelModeClicked(sp))
        return sp


    def taskConnectSelectionToggle(button, textbox, obj, SelPropertyName, allowedTypes,
                                   basePropetyName="baseObject", hideObject=True) -> SMSelectionParameters:
        """Connects a selection image-button to a textbox for selecting
        or deselecting a single object.
        """
        sp = taskConnectSelectionSingle(button, textbox, obj, SelPropertyName, allowedTypes,
                                        basePropetyName, hideObject)
        sp.ToggleMode = True
        sp.AlternateText = "" if sp.OriginalText == "" else clearText
        sp.SelectState = getattr(obj, SelPropertyName) is None
        button.setChecked(not sp.SelectState)
        return sp


    def _taskRecomputeObject(obj):
        if hasattr(obj, "ManualRecompute") and obj.ManualRecompute:
            return
        if hasattr(obj, "recompute"):
            obj.recompute()


    def _taskRecomputeDocument(obj=None):
        if obj is not None:
            if hasattr(obj, "ManualRecompute") and obj.ManualRecompute:
                return
            obj.Document.recompute()
        else:
            FreeCAD.ActiveDocument.recompute()


    def _taskUpdateValue(value, obj, propName, callback):
        setattr(obj, propName, value)
        try:  # Avoid intermittent changes.
            _taskRecomputeObject(obj)
        except:
            pass
        if callback is not None:
            callback(value)


    def _taskUpdateSubValue(value, obj, prop, subPropName, callback):
        setattr(prop, subPropName, value)
        try:  # Avoid intermittent changes.
            _taskRecomputeObject(obj)
        except:
            pass
        if callback is not None:
            callback(value)


    def _taskUpdateColor(formvar, obj, objvar, callback):
        value = formvar.property("color").name()
        setattr(obj, objvar, value)
        try:  # Avoid intermittent changes.
            _taskRecomputeObject(obj)
        except:
            pass
        if callback is not None:
            callback(value)


    def _taskEditFinished(obj):
        if hasattr(obj, "Object"):
            obj = obj.Object
        if hasattr(obj, "Document"):
            _taskRecomputeDocument(obj)


    def _getVarValue(obj, propName):
        if not hasattr(obj, propName):
            # Can happen if an old file is loaded and some props
            # were renamed.
            obj.recompute()
        return getattr(obj, propName)


    def taskConnectSpin(obj, formvar, propName, callback=None, bindFunction=True):
        formvar.setProperty("value", _getVarValue(obj, propName))
        if bindFunction:
            Gui.ExpressionBinding(formvar).bind(obj, propName)
        # keyboardTracking is set False to avoid recompute on every key
        # press.
        formvar.setProperty("keyboardTracking", False)
        formvar.valueChanged.connect(lambda value: _taskUpdateValue(value, obj, propName, callback))
        # formvar.editingFinished.connect(lambda: _taskEditFinished(obj))


    def taskConnectSpinSub(obj, formvar, prop, subPropName, callback=None, bindFunction=True):
        formvar.setProperty("value", getattr(prop, subPropName))

        # fixme: is there a way to bind a function to a sub property?
        if bindFunction and subPropName == "x":
            Gui.ExpressionBinding(formvar).bind(obj, "offset")

        formvar.setProperty("keyboardTracking", False)
        formvar.valueChanged.connect(
            lambda value: _taskUpdateSubValue(value, obj, prop, subPropName, callback))


    def taskConnectCheck(obj, formvar, propName, callback=None):
        formvar.setChecked(_getVarValue(obj, propName))
        if callback is not None:
            callback(formvar.isChecked())
        formvar.toggled.connect(lambda value: _taskUpdateValue(value, obj, propName, callback))


    def taskConnectEnum(obj, formvar, propName, callback=None, customList=None):
        val = _getVarValue(obj, propName)
        enumlist = obj.getEnumerationsOfProperty(propName) if customList is None else customList
        formvar.setProperty("currentIndex", enumlist.index(val))
        formvar.currentIndexChanged.connect(
                lambda value: _taskUpdateValue(value, obj, propName, callback))


    def taskConnectColor(obj, formvar, propName, callback=None):
        formvar.setProperty("color", _getVarValue(obj, propName))
        formvar.changed.connect(lambda: _taskUpdateColor(formvar, obj, propName, callback))


    def taskAccept(task):
        for varname in vars(task).keys():
            var = getattr(task, varname)
            if (isinstance(var, SMSelectionParameters)
                    and not var.SelectState
                    and not var.ToggleMode
            ):
                if isinstance(var.dispWidget, QtGui.QTreeWidget):
                    _taskMultiSelectionModeClicked(var)
                else:
                    _taskSingleSelModeClicked(var)
        FreeCAD.ActiveDocument.recompute()
        task.obj.Document.commitTransaction()
        Gui.Control.closeDialog()
        Gui.ActiveDocument.resetEdit()
        return True


    def taskReject(task):
        smSelectNormal()
        smSingleSelObserver.selParams = None
        FreeCAD.ActiveDocument.abortTransaction()
        Gui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        Gui.ActiveDocument.resetEdit()



    def taskLoadUI(*args):
        if len(args) == 1:
            path = os.path.join(panels_path, args[0])
            return Gui.PySideUic.loadUi(path)
        forms = []
        for uiFile in args:
            path = os.path.join(panels_path, uiFile)
            forms.append(Gui.PySideUic.loadUi(path))
        return forms


    def smGuiExportSketch(sketches, fileType, fileName, useDialog=True):
        activeFile = FreeCAD.ActiveDocument.FileName
        if activeFile in smExportDirs:
            fileName = os.path.join(smExportDirs[activeFile], fileName)
        else:
            dirName = os.path.dirname(activeFile)
            if dirName:
                fileName = os.path.join(dirName, fileName)
        if useDialog:
            filePath, _ = QtGui.QFileDialog.getSaveFileName(
                Gui.getMainWindow(),
                translate("SheetMetal", "Export unfold sketch"),
                fileName,                       # Default file path
                f"Vector Files (*.{fileType})"  # File type filters
            )
            if filePath:
                smExportDirs[activeFile] = os.path.dirname(filePath)
        else:
            filePath = fileName
        if filePath:
            if fileType == "dxf":
                importDXF.export(sketches, filePath)
            else:
                importSVG.export(sketches, filePath)


    def smAddNewObject(baseObj, newObj, activeBody, taskPanel=None):
        if activeBody is not None:
            activeBody.addObject(newObj)
        viewConf = GetViewConfig(baseObj)
        SetViewConfig(newObj, viewConf)
        Gui.Selection.clearSelection()
        #newObj.baseObject[0].ViewObject.Visibility = False
        baseObj.ViewObject.Visibility = False
        FreeCAD.ActiveDocument.recompute()
        if taskPanel is not None:
            dialog = taskPanel(newObj)
            updateTaskTitleIcon(dialog)
            Gui.Control.showDialog(dialog)
        return


    def smCreateNewObject(baseObj, name, allowPartDesign=True):
        doc = FreeCAD.ActiveDocument
        activeBody = None
        view = Gui.ActiveDocument.ActiveView
        if hasattr(view, "getActiveObject"):
            activeBody = view.getActiveObject("pdbody")
        if not allowPartDesign or not smIsPartDesign(baseObj):
            doc.openTransaction(name)
            newObj = doc.addObject("Part::FeaturePython", name)
            activeBody = None
        else:
            if not smIsOperationLegal(activeBody, baseObj):
                return None, None
            doc.openTransaction(name)
            newObj = doc.addObject("PartDesign::FeaturePython", name)
        return (newObj, activeBody)


    ###############################################################################################
    # View providers for part and part design
    ###############################################################################################

    class SMViewProvider:
        """A View provider for SheetMetal objects. Supports
         Part/Part-Design types.
         """

        def __init__(self, obj):
            obj.Proxy = self
            self.Object = obj.Object

        def attach(self, obj):
            self.Object = obj.Object
            return

        def setupContextMenu(self, viewObject, menu):
            action = menu.addAction(FreeCAD.Qt.translate(
                "QObject", "Edit %1").replace("%1", viewObject.Object.Label))
            action.triggered.connect(lambda: self.startDefaultEditMode(viewObject))
            return False

        def startDefaultEditMode(self, viewObject):
            viewObject.Document.setEdit(viewObject.Object, 0)

        def updateData(self, fp, prop):
            return

        def getDisplayModes(self, obj):
            modes = []
            return modes

        def setDisplayMode(self, mode):
            return mode

        def onChanged(self, vp, prop):
            return

        def __getstate__(self):
            # return {"ObjectName" : self.Object.Name}
            return None

        def __setstate__(self, state):
            self.loads(state)

        # dumps and loads replace __getstate__ and __setstate__ post v. 0.21.2
        def dumps(self):
            return None

        def loads(self, state):
            if state is not None:
                doc = FreeCAD.ActiveDocument
                self.Object = doc.getObject(state["ObjectName"])

        def claimChildren(self):
            objs = []
            if not smIsPartDesign(self.Object) and hasattr(self.Object, "baseObject"):
                objs.append(self.Object.baseObject[0])
            if hasattr(self.Object, "Sketch"):
                objs.append(self.Object.Sketch)
            return objs

        def setEdit(self, vobj, mode):
            if mode != 0:
                return None
            if not hasattr(self, "getTaskPanel"):
                return False
            taskd = self.getTaskPanel(vobj.Object)
            updateTaskTitleIcon(taskd)
            if smIsPartDesign(self.Object):
                self.Object.ViewObject.Visibility = True
            FreeCAD.ActiveDocument.openTransaction(self.Object.Name)
            Gui.Control.showDialog(taskd)
            return True

        def unsetEdit(self, _vobj, _mode):
            Gui.Control.closeDialog()
            if hasattr(self.Object, "baseObject"):
                self.Object.baseObject[0].ViewObject.Visibility = False
            self.Object.ViewObject.Visibility = True
            return False


# Else: In case no gui is loaded.
else:

    def smWarnDialog(msg):
        SMLogger.warning(msg)


    def smHideObjects(*args):
        pass


def smStripTrailingNumber(item):
    return re.sub(r"\d+$", "", item)

def smGetFeatureType(item):
    res =smStripTrailingNumber(item)
    if res.startswith("?"):
        res = res[1:]
    return res

def smAddToRecompute(obj):
    smObjectsToRecompute.add(obj)


def smRemoveFromRecompute(obj):
    smObjectsToRecompute.discard(obj)


def smBelongToBody(item, body):
    if body is None:
        return False
    for obj in body.Group:
        if obj.Name == item.Name:
            return True
    return False


def smIsSketchObject(obj):
    return obj.TypeId.startswith("Sketcher::")


def smGetParentBody(obj):
    if hasattr(obj, "getParent"):
        return obj.getParent()
    if hasattr(obj, "getParents"):  # Probably FreeCadLink version.
        if len(obj.getParents()) == 0:
            return None
        return obj.getParents()[0][0]
    return None


def smIsPartDesign(obj):
    if smIsSketchObject(obj):
        parent = smGetParentBody(obj)
        if parent is None:
            return False
        return isinstance(parent, Part.BodyBase)
    return obj.TypeId.startswith("PartDesign::")


def smIsOperationLegal(body, selobj):
    # FreeCAD.Console.PrintLog(str(selobj) + " " + str(body) + " "
    #                          + str(smBelongToBody(selobj, body)) + "\n")
    if smIsPartDesign(selobj) and not smBelongToBody(selobj, body):
        smWarnDialog(
            translate(
                "QMessageBox",
                "The selected geometry does not belong to the active Body.\n"
                "Please make the container of this item active by\n"
                "double clicking on it.",
                )
        )
        return False
    return True


def is_autolink_enabled():
    return params.GetInt("AutoLinkBendRadius", 0)


def use_old_unfolder():
    return params.GetBool("UseOldUnfolder", False)


def GetViewConfig(obj):
    if smIsSketchObject(obj):
        return None
    viewconf = {}
    if hasattr(obj.ViewObject, "ShapeColor"):
        viewconf["objShapeCol"] = obj.ViewObject.ShapeColor
        viewconf["objShapeTsp"] = obj.ViewObject.Transparency
        viewconf["objDiffuseCol"] = obj.ViewObject.DiffuseColor
        # TODO: Make the individual face colors be retained
        # needDiffuseColorExtension = (
        #         len(selobj.ViewObject.DiffuseColor) < len(selobj.Shape.Faces))
    else:
        return None
    return viewconf


def SetViewConfig(obj, viewconf):
    if hasattr(obj.ViewObject, "ShapeColor") and viewconf:
        obj.ViewObject.ShapeColor = viewconf["objShapeCol"]
        obj.ViewObject.Transparency = viewconf["objShapeTsp"]
        obj.ViewObject.DiffuseColor = viewconf["objDiffuseCol"]


def getOriginalBendObject(obj):
    for item in obj.OutListRecursive:
        if hasattr(item, "Proxy"):
            proxy = item.Proxy.__class__.__name__
            if proxy in ("SMBaseBend",
                         "SMBendWall",
                         "SMSolidBend",
                         "SMFoldWall"):
                if not getOriginalBendObject(item):
                    return item
    return None


def getElementFromTNP(tnpName):
    names = tnpName.split(".")
    if len(names) > 1:
        FreeCAD.Console.PrintWarning("Warning: Tnp Name still visible: " + tnpName + "\n")
    return names[len(names) - 1].lstrip("?")


def smIsParallel(v1, v2):
    return abs(abs(v1.normalize().dot(v2.normalize())) - 1.0) < smEpsilon


def smIsNormal(v1, v2):
    return abs(v1.dot(v2)) < smEpsilon


def smUpdateLinks(obj, selobj, selSubNames):
    """Update the links of a selected object to the proper scope of
    an object.
    """
    body1 = smGetParentBody(selobj)
    if body1 is None:
        return selobj, selSubNames
    body2 = smGetParentBody(obj)
    if body2 is body1:
        return selobj, selSubNames
    return body1, [f"{selobj.Name}.{subName}" for subName in selSubNames]


def smAddProperty(obj, proptype, name, proptip, defval=None, paramgroup="Parameters",
                  replacedname=None, readOnly=False, isHiddden=False, attribs=0):
    """Add a property to a given object.

    Args:
        obj: The object to which the property should be added.
        proptype: The type of the property (e.g., "App::PropertyLength",
            "App::PropertyBool").
        name: The name of the property. Non-translatable.
        proptip: The tooltip for the property. Need to be translated
            from outside.
        defval: The default value for the property (optional).
        paramgroup: The parameter group to which the property should
            belong (default is "Parameters"). If group name is "Hidden",
            the property will not be shown in the property editor.
        replacedname: If a property is renamed, for backward
            compatibility, add the replaced name to the old one so data
            can be extracted from it in old files.
        readOnly: Property can not be edited.
        isHiddden: Property is not shown in the property editor.

    """
    if not hasattr(obj, name):
        if paramgroup == "Hidden":
            isHiddden = True
        obj.addProperty(proptype, name, paramgroup, proptip, attribs, readOnly, isHiddden)
        if defval is not None:
            setattr(obj, name, defval)
        # Replaced name is either given or automatically search for old
        # lower case version of the same parameter.
        if replacedname is None and name[0].isupper():
            replacedname = name[0].lower() + name[1:]
        if replacedname is not None and hasattr(obj, replacedname):
            setattr(obj, name, getattr(obj, replacedname))
            #obj.removeProperty(replacedname).
            obj.setEditorMode(replacedname, 2)  # Hide


def smAddLengthProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyLength", name, proptip, defval, paramgroup)


def smAddBoolProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyBool", name, proptip, defval, paramgroup)


def smAddDistanceProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyDistance", name, proptip, defval, paramgroup)


def smAddAngleProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyAngle", name, proptip, defval, paramgroup)


def smAddFloatProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyFloat", name, proptip, defval, paramgroup)


def smAddIntProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyInteger", name, proptip, defval, paramgroup)


def smAddStringProperty(obj, name, proptip, defval, paramgroup="Parameters"):
    smAddProperty(obj, "App::PropertyString", name, proptip, defval, paramgroup)


def smAddEnumProperty(obj, name, proptip, enumlist, defval=None, paramgroup="Parameters"):
    if not hasattr(obj, name):
        _tip_ = FreeCAD.Qt.translate("App::Property", proptip)
        obj.addProperty("App::PropertyEnumeration", name, paramgroup, _tip_)
        setattr(obj, name, enumlist)
        if defval is not None:
            setattr(obj, name, defval)

def smGetThickness(obj, foldface):
    normal = foldface.normalAt(0, 0)
    theVol = obj.Volume
    if theVol < 0.0001:
        SMLogger.error(
            FreeCAD.Qt.translate(
                "Logger", "Shape is not a real 3D-object or too small for a metal-sheet!"
            )
        )
        return 0

    # Make a first estimate of the thickness.
    estimated_thk = theVol / (foldface.Area)
    #  p1 = foldface.CenterOfMass
    p1 = foldface.Vertexes[0].Point
    p2 = p1 + estimated_thk * -1.5 * normal
    e1 = Part.makeLine(p1, p2)
    thkedge = obj.common(e1)
    thk = thkedge.Length
    return thk


def smGetFaceByEdge(selItem, obj):
    selFace = None
    # Find face if Edge Selected.
    if type(selItem) == Part.Edge:
        Facelist = obj.ancestorsOfType(selItem, Part.Face)
        if Facelist[0].Area < Facelist[1].Area:
            selFace = Facelist[0]
        else:
            selFace = Facelist[1]
    elif type(selItem) == Part.Face:
        selFace = selItem
    return selFace


def smGetIntersectingFace(Face, obj):
    """Find a Face that overlap."""
    face = None
    for face in obj.Faces:
        face_common = face.common(Face)
        if face_common.Faces:
            break
    return face


def smGetIntersectingEdge(Face, obj):
    """Find an Edge that overlap."""
    edge = None
    for edge in obj.Edges:
        face_common = edge.common(Face)
        if face_common.Edges:
            break
    return edge


def smGetAllIntersectingEdges(Face, obj):
    """Find Edges that overlap."""
    return [edge for edge in obj.Edges if edge.common(Face).Edges]


def smIsEqualAngle(ang1, ang2, p=5):
    """Compare two angles with a given precision."""
    result = False
    if round(ang1 - ang2, p) == 0:
        result = True
    if round((ang1 - 2.0 * math.pi) - ang2, p) == 0:
        result = True
    if round(ang1 - (ang2 - 2.0 * math.pi), p) == 0:
        result = True
    return result


def smIsNetworkxAvailable():
    spec = importlib.util.find_spec("networkx")
    return spec is not None


def smGetSubElementName(elementName: str) -> tuple:
    """Get the object and the sub element name from a string
    (e.g. "obj.subobj" or "subobj").
    """
    elementNames = elementName.split(".")
    if len(elementNames) == 1:
        return None, elementName
    return FreeCAD.ActiveDocument.getObject(elementNames[0]), elementNames[1]


def smConvertPlaneToFace(planeShape):
    """Create a reference rectangular face to use instead of a
    datum/origin plane.
    """
    # Vertexes of the ref face.
    datump1 = FreeCAD.Vector(0, 0, 0)
    datump2 = FreeCAD.Vector(10, 0, 0)
    datump3 = FreeCAD.Vector(10, 10, 0)
    datump4 = FreeCAD.Vector(0, 10, 0)
    # Edges of the ref face.
    datumEdge1 = Part.LineSegment(datump1, datump2).toShape()
    datumEdge2 = Part.LineSegment(datump2, datump3).toShape()
    datumEdge3 = Part.LineSegment(datump3, datump4).toShape()
    datumEdge4 = Part.LineSegment(datump4, datump1).toShape()
    # Wire of the ref face.
    datumWire = Part.Wire([datumEdge1, datumEdge2, datumEdge3, datumEdge4])
    # Face of the ref face.
    datumFace = Part.Face(datumWire)
    # Put the face on the same place of datum.
    datumFace.Placement = planeShape.Placement
    return datumFace


class SMLogger:
    @staticmethod
    def _text(*args):
        return "".join(str(arg) for arg in args) + "\n"

    @staticmethod
    def error(*args):
        FreeCAD.Console.PrintError(SMLogger._text(*args))

    @staticmethod
    def log(*args):
        FreeCAD.Console.PrintLog(SMLogger._text(*args))

    @staticmethod
    def message(*args):
        FreeCAD.Console.PrintMessage(SMLogger._text(*args))

    @staticmethod
    def warning(*args):
        FreeCAD.Console.PrintWarning(SMLogger._text(*args))


class BendException(Exception):
    pass


class TreeException(Exception):
    pass


class UnfoldException(Exception):
    pass


class SMException(Exception):
    """Sheet Metal Custom Exception."""
