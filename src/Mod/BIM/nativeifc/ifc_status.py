# -*- coding: utf8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 3 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""This contains nativeifc status widgets and functionality"""


import FreeCAD
import FreeCADGui


translate = FreeCAD.Qt.translate
params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")
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

    pass # TODO implement


def on_activate():
    """What happens when activating a document"""

    from PySide import QtGui  # lazy import

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

    pass # TODO implement


def set_menu(locked=False):
    """Sets the File menu items"""

    from PySide import QtCore, QtGui  # lazy loading

    # switch Std_Save and IFC_Save
    mw = FreeCADGui.getMainWindow()
    wb = FreeCADGui.activeWorkbench()
    save_action = mw.findChild(QtGui.QAction, "Std_Save")
    if locked and "IFC_Save" in FreeCADGui.listCommands():
        if not hasattr(FreeCADGui,"IFC_WBManipulator"):
            FreeCADGui.IFC_WBManipulator = IFC_WBManipulator()
        # we need to void the shortcut otherwise it keeps active
        # even if the command is not shown
        FreeCADGui.IFC_saveshortcut = save_action.shortcut()
        save_action.setShortcut("")
        FreeCADGui.addWorkbenchManipulator(FreeCADGui.IFC_WBManipulator)
        wb.reloadActive()
    else:
        if hasattr(FreeCADGui,"IFC_saveshortcut"):
            save_action.setShortcut(FreeCADGui.IFC_saveshortcut)
            del FreeCADGui.IFC_saveshortcut
        if hasattr(FreeCADGui,"IFC_WBManipulator"):
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

    from nativeifc import ifc_tools  # lazy loading

    doc = FreeCAD.ActiveDocument
    if "IfcFilePath" in doc.PropertiesList:
        # this is a locked document
        doc.openTransaction("Unlock document")
        children = [o for o in doc.Objects if not o.InList]
        project = None
        if children:
            project = ifc_tools.create_document_object(
                doc, filename=doc.IfcFilePath, silent=True
            )
            project.Group = children
        props = ["IfcFilePath", "Modified", "Proxy", "Schema"]
        props += [p for p in doc.PropertiesList if doc.getGroupOfProperty(p) == "IFC"]
        for prop in props:
            doc.removeProperty(prop)
        if project:
            project.Modified = True
        doc.commitTransaction()
        doc.recompute()


def lock_document():
    """Locks the active document"""

    from nativeifc import ifc_tools  # lazy loading
    from importers import exportIFC
    from nativeifc import ifc_geometry
    from PySide import QtCore

    doc = FreeCAD.ActiveDocument
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
            ifc_tools.convert_document(
                doc, filename=project.IfcFilePath, strategy=3, silent=True
            )
            ifcfile = doc.Proxy.ifcfile
            if rest:
                # 1b some objects are outside
                objs = find_toplevel(rest)
                prefs, context = ifc_tools.get_export_preferences(ifcfile)
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
            ifc_tools.convert_document(doc, silent=True)
            ifcfile = doc.Proxy.ifcfile
            objs = find_toplevel(doc.Objects)
            prefs, context = ifc_tools.get_export_preferences(ifcfile)
            exportIFC.export(objs, ifcfile, preferences=prefs)
            for n in [o.Name for o in doc.Objects]:
                if doc.getObject(n):
                    doc.removeObject(n)
            ifc_tools.create_children(doc, ifcfile, recursive=True)
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
                ifc_tools.create_children(doc, recursive=False)


def find_toplevel(objs):
    """Finds the top-level objects from the list"""

    import Draft
    # filter out any object that depend on another from the list
    nobjs = []
    for obj in objs:
        for parent in obj.InListRecursive:
            if parent in objs:
                # exception: The object is hosting another
                if hasattr(parent,"Host") and parent.Host == obj:
                    nobjs.append(obj)
                elif hasattr(parent,"Hosts") and obj in parent.Hosts:
                    nobjs.append(obj)
                break
        else:
            nobjs.append(obj)
    # filter out non-convertible objects
    objs = filter_out(nobjs)
    return objs


def filter_out(objs):
    """Filter out objects that should not be converted to IFC"""

    nobjs = []
    for obj in objs:
        if obj.isDerivedFrom("Part::Feature"):
            nobjs.append(obj)
        elif obj.isDerivedFrom("Mesh::Feature"):
            nobjs.append(obj)
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            if filter_out(obj.Group):
                # only append groups that contain exportable objects
                nobjs.append(obj)
            else:
                print("DEBUG: Filtering out",obj.Label)
        elif obj.isDerivedFrom("Mesh::Feature"):
            nobjs.append(obj)
        elif obj.isDerivedFrom("App::Feature"):
            if Draft.get_type(obj) in ("Dimension","LinearDimension","Layer","Text","DraftText"):
                nobjs.append(obj)
            else:
                print("DEBUG: Filtering out",obj.Label)
        else:
            print("DEBUG: Filtering out",obj.Label)
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
        return [{"remove":"Std_Save"},
                {"remove":"Std_SaveAs"},
                {"insert":"IFC_Save", "menuItem":"Std_SaveCopy"},
                {"insert":"IFC_SaveAs", "menuItem":"Std_SaveCopy"},
               ]
    def modifyToolBars(self):
        return [{"remove" : "Std_Save"},
                {"append" : "IFC_Save", "toolBar" : "File"},
               ]
