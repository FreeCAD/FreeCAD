# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 russ4262 (Russell Johnson)                         *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import Path.Op.Area as PathAreaOp
import Path.Op.Base as PathOp

from PySide.QtCore import QT_TRANSLATE_NOOP


__title__ = "Base CAM Pocket Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Base class and implementation for pocket operations."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectPocket(PathAreaOp.ObjectOp):
    """Base class for proxy objects of all pocket operations."""

    @classmethod
    def pocketPropertyEnumerations(cls, dataType="data"):
        """pocketPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "CutMode": [
                (translate("CAM_Pocket", "Climb"), "Climb"),
                (translate("CAM_Pocket", "Conventional"), "Conventional"),
            ],  # this is the direction that the profile runs
            "StartAt": [
                (translate("CAM_Pocket", "Center"), "Center"),
                (translate("CAM_Pocket", "Edge"), "Edge"),
            ],
            "OffsetPattern": [
                (translate("CAM_Pocket", "ZigZag"), "ZigZag"),
                (translate("CAM_Pocket", "Offset"), "Offset"),
                (translate("CAM_Pocket", "ZigZagOffset"), "ZigZagOffset"),
                (translate("CAM_Pocket", "Line"), "Line"),
                (translate("CAM_Pocket", "Grid"), "Grid"),
            ],  # Fill Pattern
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

    def areaOpFeatures(self, obj):
        """areaOpFeatures(obj) ... Pockets have a FinishDepth and work on Faces"""
        return (
            PathOp.FeatureBaseFaces
            | PathOp.FeatureFinishDepth
            | self.pocketOpFeatures(obj)
        )

    def pocketOpFeatures(self, obj):
        return 0

    def initPocketOp(self, obj):
        """initPocketOp(obj) ... overwrite to initialize subclass.
        Can safely be overwritten by subclass."""
        pass

    def areaOpSetDefaultValues(self, obj, job):
        obj.PocketLastStepOver = 0

    def pocketInvertExtraOffset(self):
        """pocketInvertExtraOffset() ... return True if ExtraOffset's direction is inward.
        Can safely be overwritten by subclass."""
        return False

    def initAreaOp(self, obj):
        """initAreaOp(obj) ... create pocket specific properties.
        Do not overwrite, implement initPocketOp(obj) instead."""
        Path.Log.track()

        # Pocket Properties
        obj.addProperty(
            "App::PropertyEnumeration",
            "CutMode",
            "Pocket",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The direction that the toolpath should go around the part ClockWise (CW) or CounterClockWise (CCW)",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtraOffset",
            "Pocket",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra offset to apply to the operation. Direction is operation dependent.",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "StartAt",
            "Pocket",
            QT_TRANSLATE_NOOP("App::Property", "Start pocketing at center or boundary"),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Pocket",
            QT_TRANSLATE_NOOP(
                "App::Property", "Percent of cutter diameter to step over on each pass"
            ),
        )
        obj.addProperty(
            "App::PropertyFloat",
            "ZigZagAngle",
            "Pocket",
            QT_TRANSLATE_NOOP("App::Property", "Angle of the zigzag pattern"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "OffsetPattern",
            "Face",
            QT_TRANSLATE_NOOP("App::Property", "Clearing pattern to use"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "MinTravel",
            "Pocket",
            QT_TRANSLATE_NOOP("App::Property", "Use 3D Sorting of Path"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "KeepToolDown",
            "Pocket",
            QT_TRANSLATE_NOOP(
                "App::Property", "Attempts to avoid unnecessary retractions."
            ),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "PocketLastStepOver",
            "Pocket",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Last Stepover Radius.  If 0, 50% of cutter is used. Tuning this can be used to improve stepover for some shapes",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseRestMachining",
            "Pocket",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Skips machining regions that have already been cleared by previous operations.",
            ),
        )

        for n in self.pocketPropertyEnumerations():
            setattr(obj, n[0], n[1])

        self.initPocketOp(obj)

    def areaOpRetractTool(self, obj):
        Path.Log.debug("retracting tool: %d" % (not obj.KeepToolDown))
        return not obj.KeepToolDown

    def areaOpUseProjection(self, obj):
        """areaOpUseProjection(obj) ... return False"""
        return False

    def areaOpAreaParams(self, obj, isHole):
        """areaOpAreaParams(obj, isHole) ... return dictionary with pocket's area parameters"""
        Path.Log.track()
        params = {}
        params["Fill"] = 0
        params["Coplanar"] = 0
        params["PocketMode"] = 1
        params["SectionCount"] = -1
        params["Angle"] = obj.ZigZagAngle
        params["FromCenter"] = obj.StartAt == "Center"
        params["PocketStepover"] = (self.radius * 2) * (float(obj.StepOver) / 100)
        extraOffset = obj.ExtraOffset.Value
        if self.pocketInvertExtraOffset():
            extraOffset = 0 - extraOffset
        params["PocketExtraOffset"] = extraOffset
        params["ToolRadius"] = self.radius
        params["PocketLastStepover"] = obj.PocketLastStepOver

        Pattern = {
            "ZigZag": 1,
            "Offset": 2,
            "ZigZagOffset": 4,
            "Line": 5,
            "Grid": 6,
        }

        params["PocketMode"] = Pattern.get(obj.OffsetPattern, 1)

        if obj.SplitArcs:
            params["Explode"] = True
            params["FitArcs"] = False

        return params

    def opOnDocumentRestored(self, obj):
        super().opOnDocumentRestored(obj)
        if not hasattr(obj, "PocketLastStepOver"):
            obj.addProperty(
                "App::PropertyPercent",
                "PocketLastStepOver",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Last Stepover Radius.  If 0, 50% of cutter is used. Tuning this can be used to improve stepover for some shapes",
                ),
            )
            obj.PocketLastStepOver = 0

        if not hasattr(obj, "UseRestMachining"):
            obj.addProperty(
                "App::PropertyBool",
                "UseRestMachining",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Skips machining regions that have already been cleared by previous operations.",
                ),
            )

        if hasattr(obj, "RestMachiningRegions"):
            obj.removeProperty("RestMachiningRegions")
        if hasattr(obj, "RestMachiningRegionsNeedRecompute"):
            obj.removeProperty("RestMachiningRegionsNeedRecompute")

        Path.Log.track()

    def areaOpPathParams(self, obj, isHole):
        """areaOpAreaParams(obj, isHole) ... return dictionary with pocket's path parameters"""
        params = {}

        CutMode = ["Conventional", "Climb"]
        params["orientation"] = CutMode.index(obj.CutMode)

        # if MinTravel is turned on, set path sorting to 3DSort
        # 3DSort shouldn't be used without a valid start point. Can cause
        # tool crash without it.
        #
        # ml: experimental feature, turning off for now (see https://forum.freecad.org/viewtopic.php?f=15&t=24422&start=30#p192458)
        # realthunder: I've fixed it with a new sorting algorithm, which I
        # tested fine, but of course need more test. Please let know if there is
        # any problem
        #
        if obj.MinTravel and obj.UseStartPoint and obj.StartPoint is not None:
            params["sort_mode"] = 3
            params["threshold"] = self.radius * 2
        return params


def SetupProperties():
    setup = PathAreaOp.SetupProperties()
    setup.append("CutMode")
    setup.append("ExtraOffset")
    setup.append("StepOver")
    setup.append("ZigZagAngle")
    setup.append("OffsetPattern")
    setup.append("StartAt")
    setup.append("MinTravel")
    setup.append("KeepToolDown")
    return setup
