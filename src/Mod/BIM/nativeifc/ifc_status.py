# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""This contains nativeifc status widgets and functionality"""

import csv
import os

import FreeCAD
import FreeCADGui

translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")
text_on = translate("BIM", "Strict IFC mode is ON (all objects are IFC)")
text_off = translate("BIM", "Strict IFC mode is OFF (IFC and non-IFC objects allowed)")


def set_status_widget(statuswidget):
    """Adds the needed controls to the status bar"""

    from PySide import QtGui  # lazy import
    import Arch_rc

    # lock button
    lock_button = QtGui.QAction()
    icon = QtGui.QIcon(":/icons/IFC.svg")
    lock_button.setIcon(icon)
    lock_button.setCheckable(True)
    doc = FreeCAD.ActiveDocument
    statuswidget.addAction(lock_button)
    statuswidget.lock_button = lock_button
    if doc and "IfcFilePath" in doc.PropertiesList:
        checked = True
    else:
        checked = False
    # set the button first, without converting the document
    lock_button.setChecked(checked)
    on_toggle_lock(checked, noconvert=True)
    lock_button.triggered.connect(on_toggle_lock)
    set_properties_editor(statuswidget)


def set_properties_editor(statuswidget):
    """Adds additional buttons to the properties editor"""

    if hasattr(statuswidget, "propertybuttons"):
        statuswidget.propertybuttons.show()
    else:
        from PySide import QtCore, QtGui  # lazy loading

        mw = FreeCADGui.getMainWindow()
        editor = mw.findChild(QtGui.QTabWidget, "propertyTab")
        if editor:
            pTabCornerWidget = QtGui.QWidget()
            pButton1 = QtGui.QToolButton(pTabCornerWidget)
            pButton1.setText("")
            pButton1.setToolTip(translate("BIM", "Add IFC property..."))
            pButton1.setIcon(QtGui.QIcon(":/icons/IFC.svg"))
            pButton1.clicked.connect(on_add_property)
            pButton2 = QtGui.QToolButton(pTabCornerWidget)
            pButton2.setText("")
            pButton2.setToolTip(translate("BIM", "Add standard IFC Property Set..."))
            pButton2.setIcon(QtGui.QIcon(":/icons/BIM_IfcProperties.svg"))
            pButton2.clicked.connect(on_add_pset)
            pHLayout = QtGui.QHBoxLayout(pTabCornerWidget)
            pHLayout.addWidget(pButton1)
            pHLayout.addWidget(pButton2)
            pHLayout.setSpacing(2)
            pHLayout.setContentsMargins(2, 2, 0, 0)
            pHLayout.insertStretch(0)
            editor.setCornerWidget(pTabCornerWidget, QtCore.Qt.BottomRightCorner)
            statuswidget.propertybuttons = pTabCornerWidget
            QtCore.QTimer.singleShot(0, pTabCornerWidget.show)


def on_add_property():
    """When the 'add property' button is clicked"""

    sel = FreeCADGui.Selection.getSelection()
    if not sel:
        return
    from PySide import QtGui  # lazy loading
    from . import ifc_psets

    obj = sel[0]
    psets = list(set([obj.getGroupOfProperty(p) for p in obj.PropertiesList]))
    psets = [p for p in psets if p]
    psets = [p for p in psets if p not in ["Base", "IFC", "Geometry"]]
    mw = FreeCADGui.getMainWindow()
    editor = mw.findChild(QtGui.QTabWidget, "propertyTab")
    pset = None
    if editor:
        wid = editor.currentWidget()
        if wid and wid.objectName() == "propertyEditorData":
            if wid.currentIndex().parent():
                pset = wid.currentIndex().parent().data()
            else:
                pset = wid.currentIndex().data()
    form = FreeCADGui.PySideUic.loadUi(":/ui/dialogAddProperty.ui")
    # center the dialog over FreeCAD window
    form.move(mw.frameGeometry().topLeft() + mw.rect().center() - form.rect().center())
    form.field_pset.clear()
    form.field_pset.addItems(psets)
    if pset and (pset in psets):
        form.field_pset.setCurrentIndex(psets.index(pset))
    # TODO check for name duplicates while typing
    # execute
    result = form.exec_()
    if not result:
        return
    pname = form.field_name.text()
    if pname in obj.PropertiesList:
        print("DEBUG: property already exists", pname)
        return
    pset = form.field_pset.currentText()
    if not pset:
        # TODO disable the OK button if empty
        t = translate("BIM", "No Property set provided")
        FreeCAD.Console.PrintError(t + "\n")
    ptype = form.field_type.currentIndex()
    ptype = [
        "IfcLabel",
        "IfcBoolean",
        "IfcInteger",
        "IfcReal",
        "IfcLengthMeasure",
        "IfcAreaMeasure",
    ][ptype]
    fctype = ifc_psets.get_freecad_type(ptype)
    FreeCAD.ActiveDocument.openTransaction(translate("BIM", "add property"))
    for obj in sel:
        obj.addProperty(fctype, pname, pset, ptype + ":" + pname)
        ifc_psets.edit_pset(obj, pname, force=True)
    FreeCAD.ActiveDocument.commitTransaction()


def on_add_pset():
    """When the 'add pset' button is pressed"""

    def read_csv(csvfile):
        result = {}
        if os.path.exists(csvfile):
            with open(csvfile, "r") as f:
                reader = csv.reader(f, delimiter=";")
                for row in reader:
                    result[row[0]] = row[1:]
        return result

    def get_fcprop(ifcprop):
        if ifcprop == "IfcLengthMeasure":
            return "App::PropertyDistance"
        elif ifcprop == "IfcPositiveLengthMeasure":
            return "App::PropertyLength"
        elif ifcprop in ["IfcBoolean", "IfcLogical"]:
            return "App::PropertyBool"
        elif ifcprop == "IfcInteger":
            return "App::PropertyInteger"
        elif ifcprop == "IfcReal":
            return "App::PropertyFloat"
        elif ifcprop == "IfcAreaMeasure":
            return "App::PropertyArea"
        return "App::PropertyString"

    sel = FreeCADGui.Selection.getSelection()
    if not sel:
        return
    from . import ifc_psets

    obj = sel[0]
    mw = FreeCADGui.getMainWindow()
    # read standard psets
    psetpath = os.path.join(
        FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "pset_definitions.csv"
    )
    custompath = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "CustomPsets.csv")
    psetdefs = read_csv(psetpath)
    psetdefs.update(read_csv(custompath))
    psetkeys = list(psetdefs.keys())
    psetkeys.sort()
    form = FreeCADGui.PySideUic.loadUi(":/ui/dialogAddPSet.ui")
    # center the dialog over FreeCAD window
    form.move(mw.frameGeometry().topLeft() + mw.rect().center() - form.rect().center())
    form.field_pset.clear()
    form.field_pset.addItems(psetkeys)
    # execute
    result = form.exec_()
    if not result:
        return
    pset = form.field_pset.currentText()
    existing_psets = list(set([obj.getGroupOfProperty(p) for p in obj.PropertiesList]))
    if pset in existing_psets:
        t = translate("BIM", "Property set already exists")
        FreeCAD.Console.PrintError(t + ": " + pset + "\n")
        return
    props = [psetdefs[pset][i : i + 2] for i in range(0, len(psetdefs[pset]), 2)]
    props = [[p[0], p[1]] for p in props]
    FreeCAD.ActiveDocument.openTransaction(translate("BIM", "add property set"))
    for obj in sel:
        existing_psets = list(set([obj.getGroupOfProperty(p) for p in obj.PropertiesList]))
        if pset not in existing_psets:
            ifc_psets.add_pset(obj, pset)
        for prop in props:
            if prop[0] in obj.PropertiesList:
                t = translate("BIM", "Property already exists")
                FreeCAD.Console.PrintWarning(t + ": " + obj.Label + "," + prop[0] + "\n")
            else:
                obj.addProperty(get_fcprop(prop[1]), prop[0], pset, prop[1] + ":" + prop[0])
    FreeCAD.ActiveDocument.commitTransaction()


def on_toggle_lock(checked=None, noconvert=False, setchecked=False):
    """When the toolbar button is pressed"""

    if checked is None:
        checked = get_lock_status()
    set_menu(checked)
    set_button(checked, setchecked)
    if not noconvert:
        if checked:
            lock_document()
        else:
            unlock_document()


def on_open():
    """What happens when opening an existing document"""

    pass  # TODO implement


def on_activate():
    """What happens when activating a document"""

    from PySide import QtGui  # lazy import

    # always reset the menu to normal first
    set_menu(False)
    if FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
        return
    doc = FreeCAD.ActiveDocument
    if doc and "IfcFilePath" in doc.PropertiesList:
        checked = True
    else:
        checked = False
    mw = FreeCADGui.getMainWindow()
    statuswidget = mw.findChild(QtGui.QToolBar, "BIMStatusWidget")
    if hasattr(statuswidget, "lock_button"):
        statuswidget.lock_button.setChecked(checked)
    on_toggle_lock(checked, noconvert=True)


def on_new():
    """What happens when creating a new document"""

    pass  # TODO implement


def set_menu(locked=False):
    """Sets the File menu items"""

    from PySide import QtGui  # lazy loading

    # switch Std_Save and IFC_Save
    mw = FreeCADGui.getMainWindow()
    wb = FreeCADGui.activeWorkbench()
    save_action = mw.findChild(QtGui.QAction, "Std_Save")
    if locked and "IFC_Save" in FreeCADGui.listCommands():
        if not hasattr(FreeCADGui, "IFC_WBManipulator"):
            FreeCADGui.IFC_WBManipulator = IFC_WBManipulator()
        # we need to void the shortcut otherwise it keeps active
        # even if the command is not shown
        FreeCADGui.IFC_saveshortcut = save_action.shortcut()
        save_action.setShortcut("")
        FreeCADGui.addWorkbenchManipulator(FreeCADGui.IFC_WBManipulator)
        wb.reloadActive()
    else:
        if hasattr(FreeCADGui, "IFC_saveshortcut"):
            save_action.setShortcut(FreeCADGui.IFC_saveshortcut)
            del FreeCADGui.IFC_saveshortcut
        if hasattr(FreeCADGui, "IFC_WBManipulator"):
            FreeCADGui.removeWorkbenchManipulator(FreeCADGui.IFC_WBManipulator)
            del FreeCADGui.IFC_WBManipulator
        wb.reloadActive()


def set_button(checked=False, setchecked=False):
    """Sets the lock button"""

    from PySide import QtGui  # lazy loading

    mw = FreeCADGui.getMainWindow()
    statuswidget = mw.findChild(QtGui.QToolBar, "BIMStatusWidget")
    if hasattr(statuswidget, "lock_button"):
        lock_button = statuswidget.lock_button
        if checked:
            lock_button.setToolTip(text_on)
            icon = QtGui.QIcon(":/icons/IFC.svg")
            lock_button.setIcon(icon)
            if setchecked:
                lock_button.setChecked(True)
        else:
            lock_button.setToolTip(text_off)
            image = QtGui.QImage(":/icons/IFC.svg")
            grayscale = image.convertToFormat(QtGui.QImage.Format_Grayscale8)
            grayscale = grayscale.convertToFormat(image.format())
            grayscale.setAlphaChannel(image)
            icon = QtGui.QIcon(QtGui.QPixmap.fromImage(grayscale))
            lock_button.setIcon(icon)
            if setchecked:
                lock_button.setChecked(False)


def unlock_document():
    """Unlocks the active document"""

    from . import ifc_tools  # lazy loading

    doc = FreeCAD.ActiveDocument
    if not doc:
        return
    if "IfcFilePath" in doc.PropertiesList:
        # this is a locked document
        doc.openTransaction("Unlock document")
        children = [o for o in doc.Objects if not o.InList]
        project = None
        if children:
            project = ifc_tools.create_document_object(doc, filename=doc.IfcFilePath, silent=True)
            project.Group = children
        props = ["IfcFilePath", "Modified", "Proxy", "Schema"]
        props += [p for p in doc.PropertiesList if doc.getGroupOfProperty(p) == "IFC"]
        for prop in props:
            doc.setPropertyStatus(prop, "-LockDynamic")
            doc.removeProperty(prop)
        if project:
            project.Modified = True
        doc.commitTransaction()
        doc.recompute()


def lock_document():
    """Locks the active document"""

    from . import ifc_tools  # lazy loading
    from importers import exportIFC
    from . import ifc_geometry
    from . import ifc_export
    from PySide import QtCore

    doc = FreeCAD.ActiveDocument
    if not doc:
        return
    products = []
    spatial = []
    ifcfile = None
    if "IfcFilePath" not in doc.PropertiesList:
        # this is not a locked document
        projects = [o for o in doc.Objects if getattr(o, "Class", None) == "IfcProject"]
        if len(projects) == 1:
            # 1 there is a project already
            project = projects[0]
            children = project.OutListRecursive
            rest = [o for o in doc.Objects if o not in children and o != project]
            doc.openTransaction("Lock document")
            ifc_tools.convert_document(doc, filename=project.IfcFilePath, strategy=3, silent=True)
            ifcfile = doc.Proxy.ifcfile
            if rest:
                # 1b some objects are outside
                objs = find_toplevel(rest)
                prefs, context = ifc_export.get_export_preferences(ifcfile)
                products = exportIFC.export(objs, ifcfile, preferences=prefs)
                for product in products.values():
                    if not getattr(product, "ContainedInStructure", None):
                        if not getattr(product, "FillsVoids", None):
                            if not getattr(product, "VoidsElements", None):
                                if not getattr(product, "Decomposes", None):
                                    new = ifc_tools.create_object(product, doc, ifcfile)
                                    children = ifc_tools.create_children(
                                        new, ifcfile, recursive=True
                                    )
                                    for o in [new] + children:
                                        ifc_geometry.add_geom_properties(o)
                for n in [o.Name for o in rest]:
                    doc.removeObject(n)
            else:
                # 1a all objects are already inside a project
                pass
            doc.removeObject(project.Name)
            doc.Modified = True
            # all objects have been deleted, we need to show at least something
            if not doc.Objects:
                ifc_tools.create_children(doc, ifcfile, recursive=True)
            doc.commitTransaction()
            doc.recompute()
        elif len(projects) > 1:
            # 2 there is more than one project
            FreeCAD.Console.PrintError(
                "Unable to lock this document because it contains several IFC documents\n"
            )
            QtCore.QTimer.singleShot(100, on_toggle_lock)
        elif doc.Objects:
            # 3 there is no project but objects
            doc.openTransaction("Lock document")
            objs = find_toplevel(doc.Objects)
            deletelist = [o.Name for o in doc.Objects]
            # ifc_export.export_and_convert(objs, doc)
            ifc_export.direct_conversion(objs, doc)
            for n in deletelist:
                if doc.getObject(n):
                    doc.removeObject(n)
            doc.IfcFilePath = ""
            doc.Modified = True
            doc.commitTransaction()
            doc.recompute()
        else:
            # 4 this is an empty document
            doc.openTransaction("Create IFC document")
            ifc_tools.convert_document(doc)
            doc.commitTransaction()
            doc.recompute()
        # reveal file contents if needed
        if "IfcFilePath" in doc.PropertiesList:
            create = True
            for o in doc.Objects:
                # scan for site or building
                if getattr(o, "IfcClass", "") in ("IfcSite", "IfcBuilding"):
                    create = False
                    break
            if create:
                if not ifcfile:
                    ifcfile = doc.Proxy.ifcfile
                ifc_tools.create_children(doc, ifcfile, recursive=False)


def find_toplevel(objs):
    """Finds the top-level objects from the list"""

    # filter out any object that depend on another from the list
    nobjs = []
    for obj in objs:
        for parent in obj.InListRecursive:
            if parent in objs:
                # exception: The object is hosting another
                if hasattr(parent, "Host") and parent.Host == obj:
                    nobjs.append(obj)
                elif hasattr(parent, "Hosts") and obj in parent.Hosts:
                    nobjs.append(obj)
                break
        else:
            nobjs.append(obj)
    # filter out non-convertible objects
    objs = filter_out(nobjs)
    return objs


def filter_out(objs):
    """Filter out objects that should not be converted to IFC"""

    import Draft

    nobjs = []
    for obj in objs:
        if obj.isDerivedFrom("Part::Feature"):
            nobjs.append(obj)
        elif obj.isDerivedFrom("Mesh::Feature"):
            nobjs.append(obj)
        elif Draft.is_group(obj):
            if filter_out(obj.Group):
                # only append groups that contain exportable objects
                nobjs.append(obj)
            else:
                print("DEBUG: Filtering out", obj.Label)
        elif obj.isDerivedFrom("App::Feature"):
            if Draft.get_type(obj) in (
                "Dimension",
                "LinearDimension",
                "Layer",
                "Text",
                "DraftText",
            ):
                nobjs.append(obj)
            else:
                print("DEBUG: Filtering out", obj.Label)
        else:
            print("DEBUG: Filtering out", obj.Label)
    return nobjs


def get_lock_status():
    """Returns the status of the IFC lock button"""

    if not FreeCAD.GuiUp:
        return PARAMS.GetBool("SingleDoc")
    from PySide import QtGui

    mw = FreeCADGui.getMainWindow()
    statuswidget = mw.findChild(QtGui.QToolBar, "BIMStatusWidget")
    if hasattr(statuswidget, "lock_button"):
        return statuswidget.lock_button.isChecked()
    else:
        return False


# add entry to File menu
# https://github.com/FreeCAD/FreeCAD/pull/10933
class IFC_WBManipulator:
    def modifyMenuBar(self):
        return [
            {"remove": "Std_Save"},
            {"remove": "Std_SaveAs"},
            {"insert": "IFC_Save", "menuItem": "Std_SaveCopy"},
            {"insert": "IFC_SaveAs", "menuItem": "Std_SaveCopy"},
        ]

    def modifyToolBars(self):
        return [
            {"remove": "Std_Save"},
            {"append": "IFC_Save", "toolBar": "File"},
        ]
