# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

__name__ = "PathOpGuiFeatures"
__title__ = "Path Operation UI feature base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base classes for Path operation's UI features."

# Standard
import importlib
# Third-party
# FreeCAD
import FreeCADGui
import PathScripts.PathGetPoint as PathGetPoint
import PathScripts.PathGui as PathGui
import PathScripts.path_features.features as OpFeatures


# Pointers to existing module imports
PathUtils  = OpFeatures.PathUtils
PathLog    = OpFeatures.PathLog
FreeCAD    = PathGui.FreeCAD
PathGeom   = PathGui.PathGeom
QtCore     = PathGui.PySide.QtCore
QtGui      = PathGui.PySide.QtGui


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class TaskPanelPage(object):
    '''Base class for all task panel pages.'''

    # task panel interaction framework
    def __init__(self, obj, features):
        '''__init__(obj, features) ... framework initialisation.
        Do not overwrite, implement initPage(obj) instead.'''
        self.obj = obj
        self.job = PathUtils.findParentJob(obj)
        self.form = self.getForm()
        self.signalDirtyChanged = None
        self.setClean()
        self.setTitle('-')
        self.setIcon(None)
        self.features = features
        self.isdirty = False
        self.parent = None
        self.panelTitle = 'Operation'

        # Set later
        self.propEnums = None  # Will contain updated enumeration lists
        self.OpClass = None  # Will be reference to operation class

        # Set object maps for syncing visibility and enumeration lists
        self.setObjectMaps()

    def setParent(self, parent):
        '''setParent() ... used to transfer parent object link to child class.
        Do not overwrite.'''
        self.parent = parent

    def onDirtyChanged(self, callback):
        '''onDirtyChanged(callback) ...
        Set callback when dirty state changes.
        Do not overwrite.'''
        self.signalDirtyChanged = callback

    def setDirty(self):
        '''setDirty() ...
        Mark receiver as dirty, causing the model to be recalculated
        if OK or Apply is pressed.'''
        self.isdirty = True
        if self.signalDirtyChanged:
            self.signalDirtyChanged(self)

    def setClean(self):
        '''setClean() ...
        Mark receiver as clean, indicating there is no need
        to recalculate the model even if the user presses OK or Apply.'''
        self.isdirty = False
        if self.signalDirtyChanged:
            self.signalDirtyChanged(self)

    def pageGetFields(self):
        '''pageGetFields() ... internal callback.
        Do not overwrite, implement getFields(obj) instead.'''
        self.getFields(self.obj)
        self.setDirty()

    def pageSetFields(self):
        '''pageSetFields() ... internal callback.
        Do not overwrite, implement setFields(obj) instead.'''
        self.setFields(self.obj)

    def pageCleanup(self):
        '''pageCleanup() ... internal callback.
        Do not overwrite, implement cleanupPage(obj) instead.'''
        self.cleanupPage(self.obj)

    def pageRegisterSignalHandlers(self):
        '''pageRegisterSignalHandlers() .. Internal callback.
        Registers a callback for all signals returned
        by `getSignalsForUpdate(obj)`.
        Do not overwrite, implement `getSignalsForUpdate(obj)`.
        and/or registerSignalHandlers(obj) instead.'''
        for signal in self.getSignalsForUpdate(self.obj):
            signal.connect(self.pageGetFields)
        self.registerSignalHandlers(self.obj)

    def pageUpdateData(self, obj, prop):
        '''pageUpdateData(obj, prop) ... internal callback.
        Do not overwrite, implement updateData(obj) instead.'''
        self.updateData(obj, prop)

    def setTitle(self, title):
        '''setTitle(title) ... sets a title for the page.'''
        self.title = title

    def getTitle(self, obj):
        '''getTitle(obj) ... return title to be used for the receiver page.
        The default implementation returns what was previously
        set with `setTitle(title)`.
        Can safely be overwritten by subclasses.'''
        return self.title

    def setIcon(self, icon):
        '''setIcon(icon) ... sets the icon for the page.'''
        self.icon = icon

    def getIcon(self, obj):
        '''getIcon(obj) ... return icon for page or None.
        Can safely be overwritten by subclasses.'''
        return self.icon

    # subclass interface
    def initPage(self, obj):
        '''initPage(obj) ... Overwrite to customize UI for specific model.
        Note that this function is invoked after all page controllers
        have been created.
        Should be overwritten by subclasses.'''
        pass

    def cleanupPage(self, obj):
        '''cleanupPage(obj) ...
        Overwrite to perform any cleanup tasks before page is destroyed.
        Can safely be overwritten by subclasses.'''
        pass

    def modifyStandardButtons(self, buttonBox):
        '''modifyStandardButtons(buttonBox) ...
        Overwrite if the task panel standard buttons need to be modified.
        Can safely be overwritten by subclasses.'''
        pass

    def getForm(self):
        '''getForm() ... return UI form for this page.
        Must be overwritten by subclasses.'''
        pass

    def getFields(self, obj):
        '''getFields(obj) ...
        Overwrite to transfer values from UI to obj's properties.
        Can safely be overwritten by subclasses.'''
        pass

    def setFields(self, obj):
        '''setFields(obj) ...
        Overwrite to transfer obj's property values to UI.
        Can safely be overwritten by subclasses.'''
        pass

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ...
        Return signals which, when triggered, cause the receiver
        to update the model.
        See also `registerSignalHandlers(obj)`.
        Can safely be overwritten by subclasses.'''
        return []

    def registerSignalHandlers(self, obj):
        '''registerSignalHandlers(obj) ...
        Overwrite to register custom signal handlers.
        In case an update of a model is not the desired operation
        of a signal invocation (see `getSignalsForUpdate(obj)`), this function
        can be used to register signal handlers manually.
        Can safely be overwritten by subclasses.'''
        pass

    def updateData(self, obj, prop):
        '''updateData(obj, prop) ...
        Overwrite if the receiver needs to react to property changes
        that might not have been caused by the receiver itself.
        Sometimes a model will recalculate properties based on a change
        of another property. In order to keep the UI up to date with such
        changes this function can be used.

        Please note that the callback is synchronous with the property
        assignment operation. Also note that the notification is invoked
        regardless of the actual value of the property assignment. In other
        words it also fires if a property gets assigned the same value
        it already has.

        Taking above observations into account the implementation has to take
        care that it doesn't overwrite modified UI values
        by invoking `setFields(obj)`. This can happen if a subclass
        unconditionally transfers all values in `getFields(obj)` to the model
        and just calls `setFields(obj)` in this callback.

        In such a scenario the first property assignment will cause all changes
        in the UI of the other fields to be overwritten by setFields(obj).
        You have been warned.'''
        pass

    def updateSelection(self, obj, sel):
        '''updateSelection(obj, sel) ...
        overwrite to customize UI depending on current selection.
        Can safely be overwritten by subclasses.'''
        pass

    def setObjectMaps(self):
        '''setObjectMaps() ...
        These two object maps are used to sync object property
        visibility in the Data tab of the Property View list
        with their GUI counterparts in the Tasks editor window.
        Overwrite in the operation's TaskPanel subclass.'''

        '''Place entries {'property_name': 'UI_form_input_name'} for object
        properties that will have changes in visibility. This map should
        mirror that returned by `opDeterminePropEditorModes(obj)`
        of the operation's class. For property entries that don't have
        an input in the Qt UI panel, set values to `None`.'''
        self.visibilityMap = {}

        '''Place entries for object properties that will have changes
        to their enumeration lists. This map should mirror that returned
        by `opDeterminePropEnumerations(obj)` of the operation's class.
        Only insert entries here that have an combobox in the Qt UI panel.'''
        self.enumerationMap = {}

    def custom_editor_mode_actions(self, modes_dict):
        '''custom_editor_mode_actions(modes_dict) ...
        Custom modifications to editor modes and related UI panel elements,
        and custom actions based on updated editor modes.
        Can safely be overwritten.
        '''
        pass

    # helpers
    def selectInComboBox(self, name, combo):
        '''selectInComboBox(name, combo) ...
        helper function to select a specific value in a combo box.'''
        index = combo.findText(name, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.blockSignals(True)
            combo.setCurrentIndex(index)
            combo.blockSignals(False)

    def setupToolController(self, obj, combo):
        '''setupToolController(obj, combo) ...
        helper function to setup obj's ToolController
        in the given combo box.'''
        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        combo.blockSignals(True)
        combo.clear()
        combo.addItems(labels)
        combo.blockSignals(False)

        if obj.ToolController is None:
            obj.ToolController = PathUtils.findToolController(obj)
        if obj.ToolController is not None:
            self.selectInComboBox(obj.ToolController.Label, combo)

    def updateToolController(self, obj, combo):
        '''updateToolController(obj, combo) ...
        helper function to update obj's ToolController property if a different
        one has been selected in the combo box.'''
        tc = PathUtils.findToolController(obj, combo.currentText())
        if obj.ToolController != tc:
            obj.ToolController = tc

    def setupCoolant(self, obj, combo):
        '''setupCoolant(obj, combo) ...
        helper function to setup obj's Coolant option.'''
        job = PathUtils.findParentJob(obj)
        options = job.SetupSheet.CoolantModes
        combo.blockSignals(True)
        combo.clear()
        combo.addItems(options)
        combo.blockSignals(False)

        if hasattr(obj, 'CoolantMode'):
            self.selectInComboBox(obj.CoolantMode, combo)

    def updateCoolant(self, obj, combo):
        '''updateCoolant(obj, combo) ...
        helper function to update obj's Coolant property if a different
        one has been selected in the combo box.'''
        option = combo.currentText()
        if hasattr(obj, 'CoolantMode'):
            if obj.CoolantMode != option:
                obj.CoolantMode = option

    # Methods for dynamic updates in operation UI panels.
    # Initial operations with usage:  Profile, Slot, Surface, Waterline
    def set_op_class_attribute(self, OpName):
        '''set_op_class_attribute(OpName) ... 
        Sets the self.OpClass attribute for the page's class.
        This attribute holds a pointer to the active operation class to which
        this GUI page is referring.
        '''
        if hasattr(self.obj, 'Operation') and self.obj.Operation != 'None':
            # obj has `Operation` property (for selecting strategy)
            if self.obj.Proxy.pathOperation:
                self.OpClass = self.obj.Proxy.pathOperation
                return
        if self.obj.Proxy.pathOperation:
            self.OpClass = self.obj.Proxy.pathOperation
            return

        OpModuleName = 'PathScripts.Path' + OpName
        OpClassName = 'Object' + OpName
        try:
            mod = importlib.import_module(OpModuleName)
            self.OpClass = getattr(mod, OpClassName)
        except Exception as ee:
            msg = translate('PathOpGui',
                            'Unable to import operation module or class.')
            PathLog.warning(msg)

    def update_titled_panel(self, panelTitle):
        '''update_titled_panel(panelTitle) ...
        Function to call the `on_Base_Geometry_change()` GUI method of the
        page whose panel title is as indicated.'''
        if not hasattr(self, 'parent'):
            return
        parent = getattr(self, 'parent')
        if not parent:
            return
        if not hasattr(parent, 'featurePages'):
            return
        for page in parent.featurePages:
            if hasattr(page, 'panelTitle'):
                if (page.panelTitle == panelTitle and
                        hasattr(page, 'on_Base_Geometry_change')):
                    page.on_Base_Geometry_change()
                    break

    def apply_prop_editor_modes(self):
        '''apply_prop_editor_modes() ...
        Retrieves property editor modes from operation, then applies
        the same visibilities to the GUI counterparts in the Tasks
        panel editor window using object map: self.visibilityMap.

        Using this overall process reduces the visibility logic to
        a single instance in the operation class, and then applies
        that visibility scene to the Tasks panel editor window
        with this method.
        '''
        # Get active visibilities based on current object conditions
        modes_dict = self.OpClass.propertyEditorModes
        self.custom_editor_mode_actions(modes_dict)
        # Loop through editor modes
        for prop, mode in modes_dict.items():
            # Identify corresponding UI element
            if prop in self.visibilityMap and self.visibilityMap[prop]:
                attr = self.visibilityMap[prop]
                lbl = attr + '_label'
                # Set element visibility
                element = getattr(self.form, attr)
                element.show() if mode == 0 else element.hide()
                # Set element's label (if present) visibility
                if hasattr(self.form, lbl):
                    label = getattr(self.form, lbl)
                    label.show() if mode == 0 else label.hide()

    def sync_combobox_with_enumerations(self):
        '''sync_combobox_with_enumerations() ...
        Called to sycronize updated property enumeration lists with
        available choices within combo boxes.
        Only combo boxes mapped within the self.enumerationMap dictionary
        will be synchronized.
        '''
        if len(self.enumerationMap) == 0:
            return

        # Update operation's parameters: enumeration lists and editor modes
        self.OpClass.updateParameters()
        # Get enumeration lists and save to UI page's class attribute
        enum_dict = self.OpClass.propertyEnumerations
        self.propEnums = enum_dict
        # print_message = FreeCAD.Console.PrintMessage
        # print_message('enum_dict: {}\n\n'.format(enum_dict))

        # sync combo box for each in the enumeration map
        for prop, element in self.enumerationMap.items():
            enum_list = enum_dict[prop]
            cBox = getattr(self.form, element)
            cBox.blockSignals(True)
            cBox.clear()
            cBox.addItems(enum_list)
            cBox.blockSignals(False)
            # update property if current value not in new enumuration list
            prop_value = getattr(self.obj, prop)
            if prop_value not in enum_list:
                setattr(self.obj, prop, enum_list[0])
# Eclass


class TaskPanelBaseGeometryPage(TaskPanelPage):
    '''Page controller for the base geometry.'''
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataObjectSub = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, obj, features):
        super(TaskPanelBaseGeometryPage, self).__init__(obj, features)

        self.panelTitle = 'Base Geometry'
        self.OpIcon = ":/icons/Path-BaseGeometry.svg"
        self.setIcon(self.OpIcon)

    def getForm(self):
        panel = FreeCADGui.PySideUic.loadUi(":/panels/PageBaseGeometryEdit.ui")
        self.modifyPanel(panel)
        return panel

    def modifyPanel(self, panel):
        '''modifyPanel(panel) ...
        Helper method to modify the current panel (form) passed to it.
        '''
        # Determine if Job operations are available with Base Geometry
        availableOps = list()
        ops = self.job.Operations.Group
        for op in ops:
            if hasattr(op, 'Base') and isinstance(op.Base, list):
                if len(op.Base) > 0:
                    availableOps.append(op.Label)

        # Load available operations into combobox
        if len(availableOps) > 0:
            # Populate the operations list
            panel.geometryImportList.blockSignals(True)
            panel.geometryImportList.clear()
            availableOps.sort()
            for opLbl in availableOps:
                panel.geometryImportList.addItem(opLbl)
            panel.geometryImportList.blockSignals(False)
        else:
            panel.geometryImportList.hide()
            panel.geometryImportButton.hide()

    def getTitle(self, obj):
        return translate('features_gui', "Base Geometry")

    def getFields(self, obj):
        pass

    def setFields(self, obj):
        self.form.baseList.blockSignals(True)
        self.form.baseList.clear()
        for base in self.obj.Base:
            for sub in base[1]:
                item = QtGui.QListWidgetItem("%s.%s" % (base[0].Label, sub))
                item.setData(self.DataObject, base[0])
                item.setData(self.DataObjectSub, sub)
                self.form.baseList.addItem(item)
        self.form.baseList.blockSignals(False)
        self.resizeBaseList()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        for item in self.form.baseList.selectedItems():
            obj = item.data(self.DataObject)
            sub = item.data(self.DataObjectSub)
            if sub:
                FreeCADGui.Selection.addSelection(obj, sub)
            else:
                FreeCADGui.Selection.addSelection(obj)
        # FreeCADGui.updateGui()

    def supportsVertexes(self):
        return self.features & OpFeatures.FeatureBaseVertexes

    def supportsEdges(self):
        return self.features & OpFeatures.FeatureBaseEdges

    def supportsFaces(self):
        return self.features & OpFeatures.FeatureBaseFaces

    def supportsPanels(self):
        return self.features & OpFeatures.FeatureBasePanels

    def featureName(self):
        if self.supportsEdges() and self.supportsFaces():
            return 'features'
        if self.supportsFaces():
            return 'faces'
        if self.supportsEdges():
            return 'edges'
        return 'nothing'

    def selectionSupportedAsBaseGeometry(self, selection, ignoreErrors):
        msg = translate("PathProject",
                        ("Please select %s from a single solid" %
                         self.featureName()))
        if len(selection) != 1:
            if not ignoreErrors:
                FreeCAD.Console.PrintError(msg + '\n')
                PathLog.debug(msg)
            return False
        sel = selection[0]
        if sel.HasSubObjects:
            if (not self.supportsVertexes() and
                    selection[0].SubObjects[0].ShapeType == "Vertex"):
                if not ignoreErrors:
                    msg = translate("PathProject",
                                    "Vertexes are not supported")
                    PathLog.error(msg)
                return False
            if (not self.supportsEdges() and
                    selection[0].SubObjects[0].ShapeType == "Edge"):
                if not ignoreErrors:
                    msg = translate("PathProject", "Edges are not supported")
                    PathLog.error(msg)
                return False
            if (not self.supportsFaces() and
                    selection[0].SubObjects[0].ShapeType == "Face"):
                if not ignoreErrors:
                    msg = translate("PathProject", "Faces are not supported")
                    PathLog.error(msg)
                return False
        else:
            if not self.supportsPanels() or 'Panel' not in sel.Object.Name:
                msg = translate("PathProject",
                                ("Please select %s of a solid" %
                                 self.featureName()))
                if not ignoreErrors:
                    PathLog.error(msg)
                return False
        return True

    def addBaseGeometry(self, selection):
        PathLog.track(selection)
        if self.selectionSupportedAsBaseGeometry(selection, False):
            sel = selection[0]
            for sub in sel.SubElementNames:
                self.obj.Proxy.addBase(self.obj, sel.Object, sub)
            return True
        return False

    def addBase(self):
        if self.addBaseGeometry(FreeCADGui.Selection.getSelectionEx()):
            # self.obj.Proxy.execute(self.obj)
            self.setFields(self.obj)
            self.setDirty()
            self.update_titled_panel('Operation')

    def deleteBase(self):
        PathLog.track()
        selected = self.form.baseList.selectedItems()
        for item in selected:
            self.form.baseList.takeItem(self.form.baseList.row(item))
            self.setDirty()
        self.updateBase()
        self.update_titled_panel('Operation')
        self.resizeBaseList()

    def updateBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            item = self.form.baseList.item(i)
            obj = item.data(self.DataObject)
            sub = item.data(self.DataObjectSub)
            if sub:
                base = (obj, str(sub))
                newlist.append(base)
        PathLog.debug("Setting new base: %s -> %s" % (self.obj.Base, newlist))
        self.obj.Base = newlist

        # self.obj.Proxy.execute(self.obj)
        # FreeCAD.ActiveDocument.recompute()

    def clearBase(self):
        self.obj.Base = []
        self.setDirty()
        self.update_titled_panel('Operation')
        self.resizeBaseList()

    def importBaseGeometry(self):
        opLabel = str(self.form.geometryImportList.currentText())
        ops = FreeCAD.ActiveDocument.getObjectsByLabel(opLabel)
        if ops.__len__() > 1:
            msg = translate('PathOpGui', 'Mulitiple operations are labeled as')
            msg += " {}\n".format(opLabel)
            FreeCAD.Console.PrintWarning(msg)
        for (base, subList) in ops[0].Base:
            FreeCADGui.Selection.addSelection(base, subList)
        self.addBase()

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.clearBase.clicked.connect(self.clearBase)
        self.form.geometryImportButton.clicked.connect(self.importBaseGeometry)

    def pageUpdateData(self, obj, prop):
        if prop in ['Base']:
            self.setFields(obj)

    def updateSelection(self, obj, sel):
        if self.selectionSupportedAsBaseGeometry(sel, True):
            self.form.addBase.setEnabled(True)
        else:
            self.form.addBase.setEnabled(False)

    def resizeBaseList(self):
        # Set base geometry list window to resize based on contents
        # Code reference:
        # https://stackoverflow.com/questions/6337589/qlistwidget-adjust-size-to-content
        qList = self.form.baseList
        # qList.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        col = qList.width()  # 300
        row = (qList.count() + qList.frameWidth()) * 15
        qList.setFixedSize(col, row)
# Eclass


class TaskPanelBaseLocationPage(TaskPanelPage):
    '''Page controller for base locations. Uses PathGetPoint.'''

    DataLocation = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, obj, features):
        super(TaskPanelBaseLocationPage, self).__init__(obj, features)

        # members initialized later
        self.editRow = None
        self.panelTitle = 'Base Location'

    def getForm(self):
        panel = ":/panels/PageBaseLocationEdit.ui"
        self.formLoc = FreeCADGui.PySideUic.loadUi(panel)
        horizontalHeader = self.formLoc.baseList.horizontalHeader()

        if QtCore.qVersion()[0] == '4':
            horizontalHeader.setResizeMode(QtGui.QHeaderView.Stretch)
        else:
            horizontalHeader.setSectionResizeMode(QtGui.QHeaderView.Stretch)
        self.getPoint = PathGetPoint.TaskPanel(self.formLoc.addRemoveEdit)
        return self.formLoc

    def modifyStandardButtons(self, buttonBox):
        self.getPoint.buttonBox = buttonBox

    def getTitle(self, obj):
        return translate('features_gui', "Base Location")

    def getFields(self, obj):
        pass

    def setFields(self, obj):
        baseList = self.formLoc.baseList
        baseList.blockSignals(True)
        baseList.clearContents()
        baseList.setRowCount(0)
        for location in self.obj.Locations:
            baseList.insertRow(baseList.rowCount())

            item = QtGui.QTableWidgetItem("%.2f" % location.x)
            item.setData(self.DataLocation, location.x)
            baseList.setItem(baseList.rowCount()-1, 0, item)

            item = QtGui.QTableWidgetItem("%.2f" % location.y)
            item.setData(self.DataLocation, location.y)
            baseList.setItem(baseList.rowCount()-1, 1, item)
        baseList.resizeColumnToContents(0)
        baseList.blockSignals(False)
        self.itemActivated()

    def removeLocation(self):
        deletedRows = []
        selected = self.formLoc.baseList.selectedItems()
        for item in selected:
            row = self.formLoc.baseList.row(item)
            if row not in deletedRows:
                deletedRows.append(row)
                self.formLoc.baseList.removeRow(row)
        self.updateLocations()
        FreeCAD.ActiveDocument.recompute()

    def updateLocations(self):
        PathLog.track()
        locations = []
        for i in range(self.formLoc.baseList.rowCount()):
            x = self.formLoc.baseList.item(i, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(i, 1).data(self.DataLocation)
            location = FreeCAD.Vector(x, y, 0)
            locations.append(location)
        self.obj.Locations = locations

    def addLocation(self):
        self.getPoint.getPoint(self.addLocationAt)

    def addLocationAt(self, point, obj):
        if point:
            locations = self.obj.Locations
            locations.append(point)
            self.obj.Locations = locations
            FreeCAD.ActiveDocument.recompute()

    def editLocation(self):
        selected = self.formLoc.baseList.selectedItems()
        if selected:
            row = self.formLoc.baseList.row(selected[0])
            self.editRow = row
            x = self.formLoc.baseList.item(row, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(row, 1).data(self.DataLocation)
            start = FreeCAD.Vector(x, y, 0)
            self.getPoint.getPoint(self.editLocationAt, start)

    def editLocationAt(self, point, obj):
        if point:
            colX = self.formLoc.baseList.item(self.editRow, 0)
            colY = self.formLoc.baseList.item(self.editRow, 1)
            colX.setData(self.DataLocation, point.x)
            colY.setData(self.DataLocation, point.y)
            self.updateLocations()
            FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        if self.formLoc.baseList.selectedItems():
            self.form.removeLocation.setEnabled(True)
            self.form.editLocation.setEnabled(True)
        else:
            self.form.removeLocation.setEnabled(False)
            self.form.editLocation.setEnabled(False)

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.formLoc.addLocation.clicked.connect(self.addLocation)
        self.formLoc.removeLocation.clicked.connect(self.removeLocation)
        self.formLoc.editLocation.clicked.connect(self.editLocation)

    def pageUpdateData(self, obj, prop):
        if prop in ['Locations']:
            self.setFields(obj)
# Eclass


class TaskPanelHeightsPage(TaskPanelPage):
    '''Page controller for heights.'''

    def __init__(self, obj, features):
        super(TaskPanelHeightsPage, self).__init__(obj, features)

        # members initialized later
        self.clearanceHeight = None
        self.safeHeight = None
        self.panelTitle = 'Heights'
        self.OpIcon = ":/icons/Path-Heights.svg"
        self.setIcon(self.OpIcon)

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageHeightsEdit.ui")

    def initPage(self, obj):
        safe = self.form.safeHeight
        clear = self.form.clearanceHeight
        self.safeHeight = PathGui.QuantitySpinBox(safe, obj, 'SafeHeight')
        self.clearanceHeight = \
            PathGui.QuantitySpinBox(clear, obj, 'ClearanceHeight')

    def getTitle(self, obj):
        return translate("Path", "Heights")

    def getFields(self, obj):
        self.safeHeight.updateProperty()
        self.clearanceHeight.updateProperty()

    def setFields(self, obj):
        self.safeHeight.updateSpinBox()
        self.clearanceHeight.updateSpinBox()

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.safeHeight.editingFinished)
        signals.append(self.form.clearanceHeight.editingFinished)
        return signals

    def pageUpdateData(self, obj, prop):
        if prop in ['SafeHeight', 'ClearanceHeight']:
            self.setFields(obj)
# Eclass


class TaskPanelDepthsPage(TaskPanelPage):
    '''Page controller for depths.'''

    def __init__(self, obj, features):
        super(TaskPanelDepthsPage, self).__init__(obj, features)

        # members initialized later
        self.startDepth = None
        self.finalDepth = None
        self.finishDepth = None
        self.stepDown = None
        self.panelTitle = 'Depths'
        self.OpIcon = ":/icons/Path-Depths.svg"
        self.setIcon(self.OpIcon)

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageDepthsEdit.ui")

    def haveStartDepth(self):
        return OpFeatures.FeatureDepths & self.features

    def haveFinalDepth(self):
        return (OpFeatures.FeatureDepths & self.features and not
                OpFeatures.FeatureNoFinalDepth & self.features)

    def haveFinishDepth(self):
        return (OpFeatures.FeatureDepths & self.features and
                OpFeatures.FeatureFinishDepth & self.features)

    def haveStepDown(self):
        return OpFeatures.FeatureStepDown & self.features

    def initPage(self, obj):
        msg = translate('features_gui',
                        ('FinalDepth cannot be modified for this operation.'))
        msg += '\n'
        msg += translate('features_gui',
                         ('If it is necessary to set the FinalDepth manually,'
                          ' please select a different operation.'))

        if self.haveStartDepth():
            start = self.form.startDepth
            self.startDepth = \
                PathGui.QuantitySpinBox(start, obj, 'StartDepth')
        else:
            self.form.startDepth.hide()
            self.form.startDepthLabel.hide()
            self.form.startDepthSet.hide()

        if self.haveFinalDepth():
            final = self.form.finalDepth
            self.finalDepth = \
                PathGui.QuantitySpinBox(final, obj, 'FinalDepth')
        else:
            if self.haveStartDepth():
                self.form.finalDepth.setEnabled(False)
                self.form.finalDepth.setToolTip(msg)
            else:
                self.form.finalDepth.hide()
                self.form.finalDepthLabel.hide()
            self.form.finalDepthSet.hide()

        if self.haveStepDown():
            stepDown = self.form.stepDown
            self.stepDown = PathGui.QuantitySpinBox(stepDown, obj, 'StepDown')
        else:
            self.form.stepDown.hide()
            self.form.stepDownLabel.hide()

        if self.haveFinishDepth():
            finish = self.form.finishDepth
            self.finishDepth = \
                PathGui.QuantitySpinBox(finish, obj, 'FinishDepth')
        else:
            self.form.finishDepth.hide()
            self.form.finishDepthLabel.hide()

    def getTitle(self, obj):
        return translate('features_gui', "Depths")

    def getFields(self, obj):
        if self.haveStartDepth():
            self.startDepth.updateProperty()
        if self.haveFinalDepth():
            self.finalDepth.updateProperty()
        if self.haveStepDown():
            self.stepDown.updateProperty()
        if self.haveFinishDepth():
            self.finishDepth.updateProperty()

    def setFields(self, obj):
        if self.haveStartDepth():
            self.startDepth.updateSpinBox()
        if self.haveFinalDepth():
            self.finalDepth.updateSpinBox()
        if self.haveStepDown():
            self.stepDown.updateSpinBox()
        if self.haveFinishDepth():
            self.finishDepth.updateSpinBox()
        self.updateSelection(obj, FreeCADGui.Selection.getSelectionEx())

    def getSignalsForUpdate(self, obj):
        signals = []
        if self.haveStartDepth():
            signals.append(self.form.startDepth.editingFinished)
        if self.haveFinalDepth():
            signals.append(self.form.finalDepth.editingFinished)
        if self.haveStepDown():
            signals.append(self.form.stepDown.editingFinished)
        if self.haveFinishDepth():
            signals.append(self.form.finishDepth.editingFinished)
        return signals

    def registerSignalHandlers(self, obj):
        if self.haveStartDepth():
            clicked = self.form.startDepthSet.clicked
            clicked.connect(lambda: self.depthSet(obj,
                                                  self.startDepth,
                                                  'StartDepth'))
        if self.haveFinalDepth():
            clicked = self.form.finalDepthSet.clicked
            clicked.connect(lambda: self.depthSet(obj,
                                                  self.finalDepth,
                                                  'FinalDepth'))

    def pageUpdateData(self, obj, prop):
        if prop in ['StartDepth', 'FinalDepth', 'StepDown', 'FinishDepth']:
            self.setFields(obj)

    def depthSet(self, obj, spinbox, prop):
        z = self.selectionZLevel(FreeCADGui.Selection.getSelectionEx())
        if z is not None:
            PathLog.debug("depthSet(%s, %s, %.2f)" % (obj.Label, prop, z))
            if spinbox.expression():
                obj.setExpression(prop, None)
                self.setDirty()
            spinbox.updateSpinBox(FreeCAD.Units.Quantity(z,
                                                         FreeCAD.Units.Length))
            if spinbox.updateProperty():
                self.setDirty()
        else:
            PathLog.info("depthSet(-)")

    def selectionZLevel(self, sel):
        if len(sel) == 1 and len(sel[0].SubObjects) == 1:
            sub = sel[0].SubObjects[0]
            if 'Vertex' == sub.ShapeType:
                return sub.Z
            if PathGeom.isHorizontal(sub):
                if 'Edge' == sub.ShapeType:
                    return sub.Vertexes[0].Z
                if 'Face' == sub.ShapeType:
                    return sub.BoundBox.ZMax
        return None

    def updateSelection(self, obj, sel):
        if self.selectionZLevel(sel) is not None:
            self.form.startDepthSet.setEnabled(True)
            self.form.finalDepthSet.setEnabled(True)
        else:
            self.form.startDepthSet.setEnabled(False)
            self.form.finalDepthSet.setEnabled(False)
# Eclass
