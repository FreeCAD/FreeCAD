# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_LinkMake:
    def GetResources(self):
        return {
            "Pixmap": "Link",
            "MenuText": QT_TRANSLATE_NOOP("BIM_LinkMake", "Make Link"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_LinkMake",
                "Creates a Link to the selected object and immediately enables moving it",
            ),
            "Accel": "L, K",
        }

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        from draftutils.todo import ToDo

        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            FreeCAD.Console.PrintError(translate("BIM", "Select an object to link") + "\n")
            return

        doc = FreeCAD.ActiveDocument
        doc.openTransaction("Create BIM Link")

        new_links = []

        try:
            for obj in sel:
                # Create the native Link
                lnk = doc.addObject("App::Link", obj.Label + "_Link")
                lnk.LinkedObject = obj

                # We do not manipulate LinkCopyOnChange here.
                # The 'appLinkExecute' hook in the object's proxy will handle the injection of
                # shadow properties (like Hosts) to ensure the link remains lightweight.

                new_links.append(lnk)

            doc.commitTransaction()
            doc.recompute()

            # Enter Move mode
            if new_links:
                FreeCADGui.Selection.clearSelection()
                for lnk in new_links:
                    FreeCADGui.Selection.addSelection(lnk)

                # Defer the Move command to ensure the document is stable
                ToDo.delay(FreeCADGui.runCommand, "Draft_Move")

        except Exception as e:
            FreeCAD.Console.PrintError(f"BIM Link creation failed: {e}\n")
            doc.abortTransaction()


FreeCADGui.addCommand("BIM_LinkMake", BIM_LinkMake())
