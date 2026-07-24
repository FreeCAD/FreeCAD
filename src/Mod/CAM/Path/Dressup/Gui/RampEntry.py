# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 Pekka Roivainen <pekkaroi@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import FreeCADGui
import Path
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils

from Path.Base.Generator.ramp_entry import RampEntry
from PySide.QtCore import QT_TRANSLATE_NOOP

import math

translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectDressup:
    def __init__(self, obj, base):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Angle of ramp"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Method",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Select ramping method."
                "\n\nRamp Method 0: Helix like path."
                "\nRamp Method 1: Ramp down at rampangle along the path"
                " and move backwards to the original plunge end point."
                "\nRamp Method 2: Travel at start depth along the path"
                " and Ramp backwards at rampangle along the path to the original plunge end point."
                "\nRamp Method 3: Ramp down along the path until traveled half of the Z distance,"
                " change direction and ramp backwards to the original plunge end point.",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseStartDepth",
            "StartDepth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Should the dressup ignore motion commands above DressupStartDepth",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "DressupStartDepth",
            "StartDepth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The depth where the ramp dressup is enabled."
                "\nAbove this depth ramps are not generated,"
                " but motion commands are passed through as is.",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "RampVertical",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Calculate ramp angle from vertical plane"),
        )

        # populate the enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        self.obj = obj
        obj.Proxy = self
        obj.Base = base

        self.setEditorProperties(obj)

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """PropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "Method": [
                (translate("CAM_DressupRampEntry", "Helix"), "Helix"),
                (translate("CAM_DressupRampEntry", "RampMethod1"), "RampMethod1"),
                (translate("CAM_DressupRampEntry", "RampMethod2"), "RampMethod2"),
                (translate("CAM_DressupRampEntry", "RampMethod3"), "RampMethod3"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "UseStartDepth" and not obj.Document.Restoring:
            self.setEditorProperties(obj)
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def setEditorProperties(self, obj):
        mode = 0 if obj.UseStartDepth else 2
        obj.setEditorMode("DressupStartDepth", mode)
        obj.setEditorMode("RampVertical", 2)

    def onDocumentRestored(self, obj):
        # Remove RampFeedRate + CustomFeedRate properties, but keep the values around temporarily
        # This is required for tool controller migration: if a TC migrates with onDocumentRestored
        # called after this, the prior ramp feed rate still needs to be accessible.
        if hasattr(obj, "RampFeedRate"):
            obj.Proxy.RampFeedRate = obj.RampFeedRate
            obj.removeProperty("RampFeedRate")

        if hasattr(obj, "CustomFeedRate"):
            tmp = obj.CustomFeedRate.Value
            for prop, exp in obj.ExpressionEngine:
                if prop == "CustomFeedRate":
                    tmp = exp
            obj.Proxy.CustomFeedRate = tmp
            obj.removeProperty("CustomFeedRate")

        if not hasattr(obj, "RampVertical"):
            obj.addProperty(
                "App::PropertyBool",
                "RampVertical",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Calculate ramp angle from vertical plane"),
            )
            obj.RampVertical = True

        self.setEditorProperties(obj)

    def setup(self, obj):
        obj.Angle = 60
        obj.Method = obj.Proxy.propertyEnumerations(dataType="data")[0][1].index("RampMethod3")
        baseOp = PathDressup.baseOp(obj.Base)
        if hasattr(baseOp, "StartDepth"):
            obj.setExpression("DressupStartDepth", f"{baseOp.Name}.StartDepth")

    def execute(self, obj):
        if not obj.Base:
            obj.Path = Path.Path()
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            obj.Path = Path.Path()
            return
        if not obj.Base.Path:
            obj.Path = Path.Path()
            return
        if not PathDressup.baseOp(obj.Base).Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            return

        if obj.Angle >= 90:
            obj.Angle = 89.9
        elif obj.Angle <= 0:
            obj.Angle = 0.1

        if obj.RampVertical:
            angle_rad = math.pi / 2 - math.radians(obj.Angle.Value)
        else:
            angle_rad = math.radians(obj.Angle.Value)

        args = {
            "commands": PathUtils.getPathWithPlacement(obj.Base).Commands,
            "method": obj.Proxy.propertyEnumerations(dataType="data")[0][1].index(obj.Method),
            "angle_rad": angle_rad,
            "tc": PathDressup.toolController(obj.Base),
            "ignoreAbove": obj.DressupStartDepth.Value if obj.UseStartDepth else None,
        }

        obj.Path = Path.Path(RampEntry(**args).generate())


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group") and self.obj.Base.Name in [o.Name for o in i.Group]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                self.obj.Base.ViewObject.Visibility = False

    def claimChildren(self):
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        if mode == 1:
            FreeCADGui.runCommand("Std_TransformManip")
        return True

    def unsetEdit(self, vobj, mode=0):
        pass

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        Path.Log.debug("Deleting Dressup")
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getIcon(self):
        if getattr(PathDressup.baseOp(self.obj), "Active", True):
            return ":/icons/CAM_Dressup.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupRampEntry", "Ramp Entry"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupRampEntry",
                "Creates a ramp entry dress-up object from a selected toolpath",
            ),
        }

    def IsActive(self):
        return bool(PathDressup.selection())

    def Activated(self):
        # check that the selection contains exactly what we want
        op = PathDressup.selection(verbose=True)
        if not op:
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create RampEntry Dressup")
        FreeCADGui.addModule("Path.Dressup.Gui.RampEntry")
        FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.getObject('{op.Name}')")
        FreeCADGui.doCommand("Path.Dressup.Gui.RampEntry.Create(base)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


def Create(baseObject, name="DressupRampEntry", mode=0):
    """
    Create(baseObject, name='DressupRampEntry', mode=0) … create ramp entry dressup object for the given base path.

    import Path.Dressup.Gui.RampEntry as rampentry
    rampentry.Create(basePath)
    """
    if not baseObject.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupRampEntry", "The selected object is not a path") + "\n"
        )
        return None

    if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupRampEntry", "Select a profile object"))
        return None

    FreeCAD.ActiveDocument.openTransaction("Create a DressupRampEntry")
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    dbo = ObjectDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.Proxy.addOperation(obj, baseObject)
    dbo.setup(obj)
    ViewProviderDressup(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, mode)

    return obj


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupRampEntry", CommandPathDressup())

Path.Log.notice("Loading CAM_DressupRampEntry… done\n")
