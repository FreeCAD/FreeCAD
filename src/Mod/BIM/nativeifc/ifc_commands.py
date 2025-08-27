# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains IFC-related FreeCAD commands"""

import FreeCAD
import FreeCADGui

from . import ifc_openshell

translate = FreeCAD.Qt.translate
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


def get_project():
    """Gets the current project"""

    from . import ifc_tools

    if FreeCADGui.Selection.getSelection():
        return ifc_tools.get_project(FreeCADGui.Selection.getSelection()[0])
    else:
        return ifc_tools.get_project(FreeCAD.ActiveDocument)


class IFC_Diff:
    """Shows a diff of the changes in the current IFC document"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP(
            "IFC_Diff", "Shows the current unsaved changes in the IFC file"
        )
        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("IFC_Diff", "IFC Diff"),
            "ToolTip": tt,
            "Accel": "I, D",
        }

    def Activated(self):
        from . import ifc_diff

        proj = get_project()
        if proj:
            diff = ifc_diff.get_diff(proj)
            ifc_diff.show_diff(diff)


class IFC_Expand:
    """Expands the children of the selected objects or document"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP(
            "IFC_Expand", "Expands the children of the selected objects or document"
        )
        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("IFC_Expand", "IFC Expand"),
            "ToolTip": tt,
            "Accel": "I, E",
        }

    def Activated(self):
        ns = []
        for obj in FreeCADGui.Selection.getSelection():
            if hasattr(obj.ViewObject, "Proxy"):
                if hasattr(obj.ViewObject.Proxy, "hasChildren"):
                    if obj.ViewObject.Proxy.hasChildren(obj):
                        no = obj.ViewObject.Proxy.expandChildren(obj)
                        ns.extend(no)
        else:
            from . import ifc_generator
            from . import ifc_tools

            document = FreeCAD.ActiveDocument
            ifc_generator.delete_ghost(document)
            ifcfile = ifc_tools.get_ifcfile(document)
            if ifcfile:
                ns = ifc_tools.create_children(
                    document, ifcfile, recursive=True, only_structure=True
                )
        if ns:
            document.recompute()
            FreeCADGui.Selection.clearSelection()
            for o in ns:
                FreeCADGui.Selection.addSelection(o)


class IFC_ConvertDocument:
    """Converts the active document to an IFC document"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP(
            "IFC_ConvertDocument", "Converts the active document to an IFC document"
        )
        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("IFC_ConvertDocument", "Convert Document"),
            "ToolTip": tt,
            # "Accel": "I, C",
        }

    def Activated(self):
        doc = FreeCAD.ActiveDocument
        if (
            hasattr(doc, "Proxy")
            and hasattr(doc.Proxy, "ifcfile")
            and doc.Proxy.ifcfile
        ):
            FreeCAD.Console.PrintError(
                translate("BIM", "The active document is already an IFC document")
            )
        else:
            from . import ifc_tools

            ifc_tools.convert_document(doc)


class IFC_MakeProject:
    """Converts the current selection to an IFC project"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP(
            "IFC_MakeProject", "Converts the current selection to an IFC project"
        )
        return {
            "Pixmap": "IFC",
            "MenuText": QT_TRANSLATE_NOOP("IFC_MakeProject", "Create IFC Project"),
            "ToolTip": tt,
            "Accel": "I, P",
        }

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        from importers import exportIFC  # lazy loading
        from . import ifc_tools
        from PySide import QtGui

        doc = FreeCAD.ActiveDocument
        objs = FreeCADGui.Selection.getSelection()
        sf = QtGui.QFileDialog.getSaveFileName(
            None,
            "Save an IFC file",
            None,
            "Industry Foundation Classes (*.ifc)",
        )
        if sf and sf[0]:
            sf = sf[0]
            if not sf.lower().endswith(".ifc"):
                sf += ".ifc"
            exportIFC.export(objs, sf)
            ifc_tools.create_document_object(doc, sf, strategy=2)
            ifc_tools.remove_tree(objs)
            doc.recompute()


class IFC_Save:
    """Saves the current IFC document"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP(
            "IFC_Save", "Saves the current IFC document"
        )
        return {
            "Pixmap": "IFC_document",
            "MenuText": QT_TRANSLATE_NOOP("IFC_Save", "Save IFC File"),
            "ToolTip": tt,
            "Accel": "Ctrl+S",
        }

    def IsActive(self):
        doc = FreeCAD.ActiveDocument
        if hasattr(doc, "IfcFilePath"):
            return True
        return False

    def Activated(self):
        from . import ifc_tools  # lazy loading

        doc = FreeCAD.ActiveDocument
        if getattr(doc, "IfcFilePath", None):
            ifc_tools.save(doc)
            gdoc = FreeCADGui.getDocument(doc.Name)
            try:
                gdoc.Modified = False
            except:
                pass
        else:
            FreeCADGui.runCommand("IFC_SaveAs")


class IFC_SaveAs:
    """Saves the current IFC document as another name"""

    def GetResources(self):
        tt = QT_TRANSLATE_NOOP(
            "IFC_SaveAs", "Saves the current IFC document as another file"
        )
        return {
            "Pixmap": "IFC_document",
            "MenuText": QT_TRANSLATE_NOOP("IFC_SaveAs", "Save IFC File As…"),
            "ToolTip": tt,
            "Accel": "Ctrl+Shift+S",
        }

    def IsActive(self):
        doc = FreeCAD.ActiveDocument
        if hasattr(doc, "IfcFilePath"):
            return True
        return False

    def Activated(self):
        from . import ifc_tools  # lazy loading
        from . import ifc_viewproviders

        doc = FreeCAD.ActiveDocument
        if ifc_viewproviders.get_filepath(doc):
            ifc_tools.save(doc)
            gdoc = FreeCADGui.getDocument(doc.Name)
            try:
                gdoc.Modified = False
            except:
                pass


def get_commands():
    """Returns a list of IFC commands"""

    return ["IFC_Diff", "IFC_Expand", "IFC_MakeProject", "IFC_UpdateIOS"]


# initialize commands
FreeCADGui.addCommand("IFC_Diff", IFC_Diff())
FreeCADGui.addCommand("IFC_Expand", IFC_Expand())
FreeCADGui.addCommand("IFC_ConvertDocument", IFC_ConvertDocument())
FreeCADGui.addCommand("IFC_MakeProject", IFC_MakeProject())
FreeCADGui.addCommand("IFC_Save", IFC_Save())
FreeCADGui.addCommand("IFC_SaveAs", IFC_SaveAs())
