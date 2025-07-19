from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Base.Util as PathUtil
import Path.Dressup.Array as DressupArray
import Path.Main.Stock as PathStock
import PathScripts.PathUtils as PathUtils

from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import PathGui


class DressupArrayViewProvider(object):
    def __init__(self, vobj):
        self.attach(vobj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.panel = None

    def claimChildren(self):
        return [self.obj.Base]

    def onDelete(self, vobj, args=None):
        if vobj.Object and vobj.Object.Proxy:
            vobj.Object.Proxy.onDelete(vobj.Object, args)
        return True

    def setEdit(self, vobj, mode=0):
        return True

    def unsetEdit(self, vobj, mode=0):
        pass

    def setupTaskPanel(self, panel):
        pass

    def clearTaskPanel(self):
        pass


class CommandPathDressupArray:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupArray", "Array"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupArray",
                "Creates an array from a selected toolpath",
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            Path.Log.error(translate("CAM_DressupArray", "Select one toolpath object") + "\n")
            return
        baseObject = selection[0]

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Path Array Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Array")
        FreeCADGui.doCommand(
            "Path.Dressup.Gui.Array.Create(App.ActiveDocument.%s)" % baseObject.Name
        )
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


def Create(base, name="DressupPathArray"):
    FreeCAD.ActiveDocument.openTransaction("Create an Array dressup")
    obj = DressupArray.Create(base, name)
    obj.ViewObject.Proxy = DressupArrayViewProvider(obj.ViewObject)
    obj.Base.ViewObject.Visibility = False
    FreeCAD.ActiveDocument.commitTransaction()
    return obj


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupArray", CommandPathDressupArray())
