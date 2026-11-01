# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import FreeCADGui
import Path
import Path.Dressup.ProfileTrim as DressupProfileTrim
import Path.Dressup.Utils as PathDressup
from PySide.QtCore import QT_TRANSLATE_NOOP


class ProfileTrimViewProvider:
    def __init__(self, vobj):
        self.attach(vobj)

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def claimChildren(self):
        return [self.obj.Base]

    def onDelete(self, vobj, args=None):
        return vobj.Object.Proxy.onDelete(vobj.Object, args)

    def getIcon(self):
        return ":/icons/CAM_Dressup.svg"


class CommandProfileTrim:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupProfileTrim", "Profile Trim"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupProfileTrim", "Trim the start and end of an open profile toolpath"
            ),
        }

    def IsActive(self):
        return bool(PathDressup.selection())

    def Activated(self):
        base = PathDressup.selection(verbose=True)
        if not base:
            return
        FreeCAD.ActiveDocument.openTransaction("Create Profile Trim Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.ProfileTrim")
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument.getObject('{}')".format(base.Name))
        FreeCADGui.doCommand("Path.Dressup.Gui.ProfileTrim.Create(base)")
        FreeCAD.ActiveDocument.recompute()


def Create(base, name="DressupProfileTrim"):
    FreeCAD.ActiveDocument.openTransaction("Create a Profile Trim dressup")
    obj = DressupProfileTrim.Create(base, name)
    if obj:
        obj.ViewObject.Proxy = ProfileTrimViewProvider(obj.ViewObject)
        obj.Base.ViewObject.Visibility = False
    FreeCAD.ActiveDocument.commitTransaction()
    return obj


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_DressupProfileTrim", CommandProfileTrim())
