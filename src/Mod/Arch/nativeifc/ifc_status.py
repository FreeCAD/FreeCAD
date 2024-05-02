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
    if doc and "IfcFilePath" in doc.PropertiesList:
        checked = True
    else:
        checked = params.GetBool("SingleDoc", False)
    toggle_lock(checked)
    set_button(lock_button, checked)
    lock_button.triggered.connect(do_lock)
    lock_button.triggered.connect(toggle_lock)
    statuswidget.addAction(lock_button)
    statuswidget.lock_button = lock_button


def toggle_lock(checked=False):
    """Sets the lock button on/off"""

    from PySide import QtCore, QtGui  # lazy loading

    # switch Std_Save and IFC_Save
    mw = FreeCADGui.getMainWindow()
    wb = FreeCADGui.activeWorkbench()
    save_action = mw.findChild(QtGui.QAction, "Std_Save")
    if checked and "IFC_Save" in FreeCADGui.listCommands():
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
    # set the lock button
    statuswidget = mw.findChild(QtGui.QToolBar, "BIMStatusWidget")
    if hasattr(statuswidget, "lock_button"):
        set_button(statuswidget.lock_button, checked)


def set_button(lock_button, checked):
    """Sets the lock button"""

    from PySide import QtGui  # lazy loading

    if checked:
        lock_button.setChecked(True)
        lock_button.setToolTip(text_on)
        icon = QtGui.QIcon(":/icons/IFC.svg")
        lock_button.setIcon(icon)
    else:
        lock_button.setChecked(False)
        lock_button.setToolTip(text_off)
        image = QtGui.QImage(":/icons/IFC.svg")
        grayscale = image.convertToFormat(QtGui.QImage.Format_Grayscale8)
        grayscale = grayscale.convertToFormat(image.format())
        grayscale.setAlphaChannel(image)
        icon = QtGui.QIcon(QtGui.QPixmap.fromImage(grayscale))
        lock_button.setIcon(icon)


def do_lock(checked):
    """Locks or unlocks the document"""

    if checked:
        lock_document()
    else:
        unlock_document()


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
            QtCore.QTimer.singleShot(100, toggle_lock)
        elif doc.Objects:
            # 3 there is no project but objects
            doc.openTransaction("Lock document")
            ifc_tools.convert_document(doc, silent=True)
            ifcfile = doc.Proxy.ifcfile
            objs = find_toplevel(doc.Objects)
            prefs, context = ifc_tools.get_export_preferences(ifcfile)
            exportIFC.export(objs, ifcfile, preferences=prefs)
            for n in [o.Name for o in doc.Objects]:
                doc.removeObject(n)
            ifc_tools.create_children(doc, ifcfile, recursive=True)
            doc.Modified = True
            doc.commitTransaction()
            doc.recompute()
        else:
            # 4 this is an empty document
            ifc_tools.convert_document(doc, silent=True)


def find_toplevel(objs):
    """Finds the top-level objects from the list"""

    # filter out any object that depend on another from the list
    nobjs = []
    for obj in objs:
        for parent in obj.InListRecursive:
            if parent in objs:
                break
        else:
            nobjs.append(obj)
    # filter out 2D objects
    objs = nobjs
    nobjs = []
    for obj in objs:
        if obj.isDerivedFrom("Part::Feature"):
            if obj.Shape.Edges and not obj.Shape.Solids:
                print("Excluding", obj.Label, "- 2D objects not supported yet")
            else:
                nobjs.append(obj)
        else:
            nobjs.append(obj)
    return nobjs


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
