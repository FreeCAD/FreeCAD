# SPDX-License-Identifier: LGPL-2.1-or-later

"""BIM Stair Designer command."""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class Arch_StairDesigner:
    """Create a new parametric Stair Designer stair."""

    def GetResources(self):
        return {
            "Pixmap": "Arch_Stairs",
            "MenuText": QT_TRANSLATE_NOOP("Arch_StairDesigner", "Stair Designer"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_StairDesigner",
                "Creates a parametric stair with generated plan and component objects",
            ),
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        import Draft
        from stairdesigner import make_stair
        from stairdesigner.taskpanels import StairDesignerTaskPanel

        doc = FreeCAD.ActiveDocument
        gui_doc = FreeCADGui.ActiveDocument
        gui_doc.openCommand(translate("BIM", "Create Stair Designer"))
        stair = make_stair()
        if not stair:
            gui_doc.abortCommand()
            return

        Draft.autogroup(stair)
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(stair)
        doc.recompute()
        panel = StairDesignerTaskPanel(stair, is_creating=True)
        dialog = FreeCADGui.Control.showDialog(panel)
        if dialog is not None:
            dialog.setAutoCloseOnDeletedDocument(True)
            dialog.setDocumentName(doc.Name)


FreeCADGui.addCommand("Arch_StairDesigner", Arch_StairDesigner())
