# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
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

"""Document observer to act on documents containing NativeIFC objects"""

import FreeCAD


params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")


def add_observer():
    """Adds this observer to the running FreeCAD instance"""

    FreeCAD.BIMobserver = ifc_observer()
    FreeCAD.addDocumentObserver(FreeCAD.BIMobserver)


def remove_observer():
    """Removes this observer if present"""

    if hasattr(FreeCAD, "BIMobserver"):
        FreeCAD.removeDocumentObserver(FreeCAD.BIMobserver)
        del FreeCAD.BIMobserver


class ifc_observer:
    """A general document observer that handles IFC objects"""

    def __init__(self):
        # if there is a document open when the observer starts,
        # check it
        if FreeCAD.ActiveDocument:
            self.slotActivateDocument(FreeCAD.ActiveDocument)

    def slotStartSaveDocument(self, doc, value):
        """Save all IFC documents in this doc"""

        from PySide import QtCore  # lazy loading

        self.docname = doc.Name
        # delay execution to not get caught under the wait sursor
        # that occurs when the saveAs file dialog is shown
        # TODO find a more solid way
        QtCore.QTimer.singleShot(100, self.save)

    def slotDeletedObject(self, obj):
        """Deletes the corresponding object in the IFC document"""

        from . import ifc_tools  # lazy loading

        proj = ifc_tools.get_project(obj)
        if not proj:
            return
        if not hasattr(obj, "Proxy"):
            return
        if getattr(obj.Proxy, "nodelete", False):
            return
        ifc_tools.remove_ifc_element(obj)

    def slotChangedDocument(self, doc, prop):
        """Watch document IFC properties"""

        # only look at locked IFC documents
        if "IfcFilePath" not in doc.PropertiesList:
            return

        from . import ifc_tools  # lazy import

        if prop == "Schema":
            schema = doc.Schema
            ifcfile = ifc_tools.get_ifcfile(doc)
            if ifcfile:
                if schema != ifcfile.wrapped_data.schema_name():
                    # TODO display warming
                    ifcfile, migration_table = ifc_tools.migrate_schema(ifcfile, schema)
                    doc.Proxy.ifcfile = ifcfile
                    # migrate children
                    for old_id, new_id in migration_table.items():
                        child = [
                            o
                            for o in doc.Objects
                            if getattr(o, "StepId", None) == old_id
                        ]
                        if len(child) == 1:
                            child[0].StepId = new_id
        elif prop == "Label":
            ifcfile = ifc_tools.get_ifcfile(doc)
            project = ifc_tools.get_ifc_element(doc)
            ifc_tools.set_attribute(ifcfile, project, "Name", doc.Label)

    def slotCreatedObject(self, obj):
        """If this is an IFC document, turn the object into IFC"""

        doc = getattr(obj, "Document", None)
        if doc:
            if hasattr(doc, "IfcFilePath"):
                from PySide import QtCore  # lazy loading

                self.objname = obj.Name
                self.docname = obj.Document.Name
                # delaying to make sure all other properties are set
                QtCore.QTimer.singleShot(100, self.convert)

    def slotActivateDocument(self, doc):
        """Check if we need to lock"""

        from . import ifc_status

        ifc_status.on_activate()

    def slotRemoveDynamicProperty(self, obj, prop):

        from . import ifc_psets
        ifc_psets.remove_property(obj, prop)

    # implementation methods

    def fit_all(self):
        """Fits the view"""

        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.SendMsgToActiveView("ViewFit")

    def save(self):
        """Saves all IFC documents contained in self.docname Document"""

        if not hasattr(self, "docname"):
            return
        if self.docname not in FreeCAD.listDocuments():
            return
        doc = FreeCAD.getDocument(self.docname)
        del self.docname
        projects = []
        if hasattr(doc, "IfcFilePath") and hasattr(doc, "Modified"):
            if doc.Modified:
                projects.append(doc)
        else:
            for obj in doc.findObjects(Type="Part::FeaturePython"):
                if hasattr(obj, "IfcFilePath") and hasattr(obj, "Modified"):
                    if obj.Modified:
                        projects.append(obj)
        if projects:
            from . import ifc_tools  # lazy loading
            from . import ifc_viewproviders

            ask = params.GetBool("AskBeforeSaving", True)
            if ask and FreeCAD.GuiUp:
                import Arch_rc
                import FreeCADGui

                dlg = FreeCADGui.PySideUic.loadUi(":/ui/dialogExport.ui")
                result = dlg.exec_()
                if not result:
                    return
                ask = dlg.checkAskBeforeSaving.isChecked()
                params.SetBool("AskBeforeSaving", ask)

            for project in projects:
                if getattr(project.Proxy, "ifcfile", None):
                    if project.IfcFilePath:
                        ifc_tools.save(project)
                    else:
                        ifc_viewproviders.get_filepath(project)
                        ifc_tools.save(project)

    def convert(self):
        """Converts an object to IFC"""

        if not hasattr(self, "objname") or not hasattr(self, "docname"):
            return
        if self.docname not in FreeCAD.listDocuments():
            return
        doc = FreeCAD.getDocument(self.docname)
        if not doc:
            return
        obj = doc.getObject(self.objname)
        if not obj:
            return
        if "StepId" in obj.PropertiesList:
            return
        del self.docname
        del self.objname
        if obj.isDerivedFrom("Part::Feature") \
        or "IfcType" in obj.PropertiesList \
        or "CreateSpreadsheet" in obj.PropertiesList:
            FreeCAD.Console.PrintLog("Converting " + obj.Label + " to IFC\n")
            from . import ifc_geometry  # lazy loading
            from . import ifc_tools  # lazy loading

            newobj = ifc_tools.aggregate(obj, doc)
            ifc_geometry.add_geom_properties(newobj)
            doc.recompute()
