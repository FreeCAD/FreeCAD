# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Base.Generator.threadmilling as threadmilling
import Path.Op.Base as PathOp
import Path.Op.CircularHoleBase as PathCircularHoleBase
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Thread Milling Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "CAM thread milling operation."

# math.sqrt(3)/2 ... 60deg triangle height
SQRT_3_DIVIDED_BY_2 = 0.8660254037844386

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

# Constants
LeftHand = "LeftHand"
RightHand = "RightHand"
ThreadTypeCustomExternal = "CustomExternal"
ThreadTypeCustomInternal = "CustomInternal"
ThreadTypeImperialExternal2A = "ImperialExternal2A"
ThreadTypeImperialExternal3A = "ImperialExternal3A"
ThreadTypeImperialInternal2B = "ImperialInternal2B"
ThreadTypeImperialInternal3B = "ImperialInternal3B"
ThreadTypeMetricExternal4G6G = "MetricExternal4G6G"
ThreadTypeMetricExternal6G = "MetricExternal6G"
ThreadTypeMetricInternal6H = "MetricInternal6H"
DirectionClimb = "Climb"
DirectionConventional = "Conventional"

ThreadOrientations = [LeftHand, RightHand]

ThreadTypeData = {
    ThreadTypeImperialExternal2A: "imperial-external-2A.csv",
    ThreadTypeImperialExternal3A: "imperial-external-3A.csv",
    ThreadTypeImperialInternal2B: "imperial-internal-2B.csv",
    ThreadTypeImperialInternal3B: "imperial-internal-3B.csv",
    ThreadTypeMetricExternal4G6G: "metric-external-4G6G.csv",
    ThreadTypeMetricExternal6G: "metric-external-6G.csv",
    ThreadTypeMetricInternal6H: "metric-internal-6H.csv",
}

ThreadTypesExternal = [
    ThreadTypeCustomExternal,
    ThreadTypeImperialExternal2A,
    ThreadTypeImperialExternal3A,
    ThreadTypeMetricExternal4G6G,
    ThreadTypeMetricExternal6G,
]
ThreadTypesInternal = [
    ThreadTypeCustomInternal,
    ThreadTypeImperialInternal2B,
    ThreadTypeImperialInternal3B,
    ThreadTypeMetricInternal6H,
]
ThreadTypesImperial = [
    ThreadTypeImperialExternal2A,
    ThreadTypeImperialExternal3A,
    ThreadTypeImperialInternal2B,
    ThreadTypeImperialInternal3B,
]
ThreadTypesMetric = [
    ThreadTypeMetricExternal4G6G,
    ThreadTypeMetricExternal6G,
    ThreadTypeMetricInternal6H,
]
ThreadTypes = ThreadTypesInternal + ThreadTypesExternal
Directions = [DirectionClimb, DirectionConventional]


def _isThreadInternal(obj):
    return obj.ThreadType in ThreadTypesInternal


def threadSetupInternal(obj, zTop, zBottom):
    Path.Log.track()
    if obj.ThreadOrientation == RightHand:
        # Right hand thread, G2, top down -> conventional milling
        if obj.Direction == DirectionConventional:
            return ("G2", zTop, zBottom)
        # For climb milling we need to cut the thread from the bottom up
        # in the opposite direction -> G3
        return ("G3", zBottom, zTop)
    # Left hand thread, G3, top down -> climb milling
    if obj.Direction == DirectionClimb:
        return ("G3", zTop, zBottom)
    # for conventional milling, cut bottom up with G2
    return ("G2", zBottom, zTop)


def threadSetupExternal(obj, zTop, zBottom):
    Path.Log.track()
    if obj.ThreadOrientation == RightHand:
        # right hand thread, G2, top down -> climb milling
        if obj.Direction == DirectionClimb:
            return ("G2", zTop, zBottom)
        # for conventional, mill bottom up the other way around
        return ("G3", zBottom, zTop)
    # left hand thread, G3, top down -> conventional milling
    if obj.Direction == DirectionConventional:
        return ("G3", zTop, zBottom)
    # for climb milling need to go bottom up and the other way
    return ("G2", zBottom, zTop)


def threadSetup(obj):
    """Return (cmd, zbegin, zend) of thread milling operation"""
    Path.Log.track()

    zTop = obj.StartDepth.Value
    zBottom = obj.FinalDepth.Value

    if _isThreadInternal(obj):
        return threadSetupInternal(obj, zTop, zBottom)
    else:
        return threadSetupExternal(obj, zTop, zBottom)


def threadRadii(internal, majorDia, minorDia, toolDia, toolCrest):
    """threadRadii(majorDia, minorDia, toolDia, toolCrest) ... returns the minimum and maximum radius for thread."""
    Path.Log.track(internal, majorDia, minorDia, toolDia, toolCrest)
    if toolCrest is None:
        toolCrest = 0.0
    # As it turns out metric and imperial standard threads follow the same rules.
    # The determining factor is the height of the full 60 degree triangle H.
    # - The minor diameter is 1/4 * H smaller than the pitch diameter.
    # - The major diameter is 3/8 * H bigger than the pitch diameter
    # Since we already have the outer diameter it's simpler to just add 1/8 * H
    # to get the outer tip of the thread.
    H = ((majorDia - minorDia) / 2.0) * 1.6  # (D - d)/2 = 5/8 * H
    if internal:
        # mill inside out
        outerTip = majorDia / 2.0 + H / 8.0
        # Compensate for the crest of the tool
        toolTip = outerTip - toolCrest * SQRT_3_DIVIDED_BY_2
        radii = ((minorDia - toolDia) / 2.0, toolTip - toolDia / 2.0)
    else:
        # mill outside in
        innerTip = minorDia / 2.0 - H / 4.0
        # Compensate for the crest of the tool
        toolTip = innerTip - toolCrest * SQRT_3_DIVIDED_BY_2
        radii = ((majorDia + toolDia) / 2.0, toolTip + toolDia / 2.0)
    Path.Log.track(radii)
    return radii


def threadPasses(count, radii, internal, majorDia, minorDia, toolDia, toolCrest):
    Path.Log.track(count, radii, internal, majorDia, minorDia, toolDia, toolCrest)
    # the logic goes as follows, total area to be removed:
    #   A = H * W  ... where H is the depth and W is half the width of a thread
    #     H = k * sin(30) = k * 1/2  -> k = 2 * H
    #     W = k * cos(30) = k * sqrt(3)/2
    #     -> W = (2 * H) * sqrt(3) / 2 = H * sqrt(3)
    #   A = sqrt(3) * H^2
    # Each pass has to remove the same area
    #   An = A / count = sqrt(3) * H^2 / count
    # Because each successive pass doesn't have to remove the aera of the previous
    # passes the result for the height:
    #   Ai = (i + 1) * An = (i + 1) * sqrt(3) * Hi^2 = sqrt(3) * H^2 / count
    #   Hi = sqrt(H^2 * (i + 1) / count)
    #   Hi = H * sqrt((i + 1) / count)
    minor, major = radii(internal, majorDia, minorDia, toolDia, toolCrest)
    H = float(major - minor)
    Hi = [H * math.sqrt((i + 1) / count) for i in range(count)]

    # For external threads threadRadii returns the radii in reverse order because that's
    # the order in which they have to get milled. As a result H ends up being negative
    # and the math for internal and external threads is identical.
    passes = [minor + h for h in Hi]
    Path.Log.debug(f"threadPasses({minor}, {major}) -> H={H} : {Hi}  --> {passes}")

    return passes


def elevatorRadius(obj, center, internal, tool):
    """elevatorLocation(obj, center, internal, tool) ... return suitable location for the tool elevator"""
    Path.Log.track(center, internal, tool.Diameter)
    if internal:
        dy = float(obj.MinorDiameter - tool.Diameter) / 2 - 1
        if dy < 0:
            if obj.MinorDiameter < tool.Diameter:
                Path.Log.error(
                    "The selected tool is too big (d={}) for milling a thread with minor diameter D={}".format(
                        tool.Diameter, obj.MinorDiameter
                    )
                )
            dy = 0
    else:
        dy = float(obj.MajorDiameter + tool.Diameter) / 2 + 1

    return dy


class ObjectThreadMilling(PathCircularHoleBase.ObjectOp):
    """Proxy object for thread milling operation."""

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """
        Path.Log.track()

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "ThreadType": [
                (
                    translate("CAM_ThreadMilling", "Custom External"),
                    ThreadTypeCustomExternal,
                ),
                (
                    translate("CAM_ThreadMilling", "Custom Internal"),
                    ThreadTypeCustomInternal,
                ),
                (
                    translate("CAM_ThreadMilling", "Imperial External (2A)"),
                    ThreadTypeImperialExternal2A,
                ),
                (
                    translate("CAM_ThreadMilling", "Imperial External (3A)"),
                    ThreadTypeImperialExternal3A,
                ),
                (
                    translate("CAM_ThreadMilling", "Imperial Internal (2B)"),
                    ThreadTypeImperialInternal2B,
                ),
                (
                    translate("CAM_ThreadMilling", "Imperial Internal (3B)"),
                    ThreadTypeImperialInternal3B,
                ),
                (
                    translate("CAM_ThreadMilling", "Metric External (4G6G)"),
                    ThreadTypeMetricExternal4G6G,
                ),
                (
                    translate("CAM_ThreadMilling", "Metric External (6G)"),
                    ThreadTypeMetricExternal6G,
                ),
                (
                    translate("CAM_ThreadMilling", "Metric Internal (6H)"),
                    ThreadTypeMetricInternal6H,
                ),
            ],
            "ThreadOrientation": [
                (
                    translate("CAM_ThreadMilling", "LeftHand"),
                    LeftHand,
                ),
                (
                    translate("CAM_ThreadMilling", "RightHand"),
                    RightHand,
                ),
            ],
            "Direction": [
                (
                    translate("CAM_ThreadMilling", "Climb"),
                    DirectionClimb,
                ),
                (
                    translate("CAM_ThreadMilling", "Conventional"),
                    DirectionConventional,
                ),
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

    def circularHoleFeatures(self, obj):
        Path.Log.track()
        return PathOp.FeatureBaseGeometry

    def initCircularHoleOperation(self, obj):
        Path.Log.track()
        obj.addProperty(
            "App::PropertyEnumeration",
            "ThreadOrientation",
            "Thread",
            QT_TRANSLATE_NOOP("App::Property", "Set thread orientation"),
        )
        # obj.ThreadOrientation = ThreadOrientations
        obj.addProperty(
            "App::PropertyEnumeration",
            "ThreadType",
            "Thread",
            QT_TRANSLATE_NOOP("App::Property", "Currently only internal"),
        )
        # obj.ThreadType = ThreadTypes
        obj.addProperty(
            "App::PropertyString",
            "ThreadName",
            "Thread",
            QT_TRANSLATE_NOOP(
                "App::Property", "Defines which standard thread was chosen"
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "MajorDiameter",
            "Thread",
            QT_TRANSLATE_NOOP("App::Property", "Set thread's major diameter"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "MinorDiameter",
            "Thread",
            QT_TRANSLATE_NOOP("App::Property", "Set thread's minor diameter"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Pitch",
            "Thread",
            QT_TRANSLATE_NOOP(
                "App::Property", "Set thread's pitch - used for metric threads"
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "TPI",
            "Thread",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Set thread's TPI (turns per inch) - used for imperial threads",
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "ThreadFit",
            "Thread",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Override to control how loose or tight the threads are milled",
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "Passes",
            "Operation",
            QT_TRANSLATE_NOOP(
                "App::Property", "Set how many passes are used to cut the thread"
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Direction",
            "Operation",
            QT_TRANSLATE_NOOP("App::Property", "Direction of thread cutting operation"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadInOut",
            "Operation",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Set to True to get lead in and lead out arcs at the start and end of the thread cut",
            ),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ClearanceOp",
            "Operation",
            QT_TRANSLATE_NOOP(
                "App::Property", "Operation to clear the inside of the thread"
            ),
        )

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    def threadPassRadii(self, obj):
        Path.Log.track(obj.Label)
        rMajor = (obj.MajorDiameter.Value - self.tool.Diameter) / 2.0
        rMinor = (obj.MinorDiameter.Value - self.tool.Diameter) / 2.0
        if obj.Passes < 1:
            obj.Passes = 1
        rPass = (rMajor - rMinor) / obj.Passes
        passes = [rMajor]
        for i in range(1, obj.Passes):
            passes.append(rMajor - rPass * i)
        return list(reversed(passes))

    def executeThreadMill(self, obj, loc, gcode, zStart, zFinal, pitch):
        Path.Log.track(obj.Label, loc, gcode, zStart, zFinal, pitch)
        elevator = elevatorRadius(obj, loc, _isThreadInternal(obj), self.tool)

        move2clearance = Path.Command(
            "G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid}
        )
        self.commandlist.append(move2clearance)

        start = None
        for radius in threadPasses(
            obj.Passes,
            threadRadii,
            _isThreadInternal(obj),
            obj.MajorDiameter.Value,
            obj.MinorDiameter.Value,
            float(self.tool.Diameter),
            float(self.tool.Crest),
        ):
            if not start is None:
                # and not _isThreadInternal(obj):
                # and not obj.LeadInOut:
                # external thread without lead in/out have to go up and over
                # in other words we need a move to clearance and not take any
                # shortcuts when moving to the elevator position
                self.commandlist.append(move2clearance)
                start = None
            commands, start = threadmilling.generate(
                loc,
                gcode,
                zStart,
                zFinal,
                pitch,
                radius,
                obj.LeadInOut,
                elevator,
                start,
            )

            for cmd in commands:
                p = cmd.Parameters
                if cmd.Name in ["G0"]:
                    p.update({"F": self.vertRapid})
                if cmd.Name in ["G1", "G2", "G3"]:
                    p.update({"F": self.horizFeed})
                cmd.Parameters = p
            self.commandlist.extend(commands)

        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )

    def circularHoleExecute(self, obj, holes):
        Path.Log.track()
        if self.isToolSupported(obj, self.tool):
            self.commandlist.append(Path.Command("(Begin Thread Milling)"))

            (cmd, zStart, zFinal) = threadSetup(obj)
            pitch = obj.Pitch.Value
            if obj.TPI > 0:
                pitch = 25.4 / obj.TPI
            if pitch <= 0:
                Path.Log.error("Cannot create thread with pitch {}".format(pitch))
                return

            # rapid to clearance height
            for loc in holes:
                self.executeThreadMill(
                    obj,
                    FreeCAD.Vector(loc["x"], loc["y"], 0),
                    cmd,
                    zStart,
                    zFinal,
                    pitch,
                )
        else:
            Path.Log.error("No suitable Tool found for thread milling operation")

    def opSetDefaultValues(self, obj, job):
        Path.Log.track()
        obj.ThreadOrientation = RightHand
        obj.ThreadType = ThreadTypeMetricInternal6H
        obj.ThreadFit = 50
        obj.Pitch = 1
        obj.TPI = 0
        obj.Passes = 1
        obj.Direction = DirectionClimb
        obj.LeadInOut = False

    def isToolSupported(self, obj, tool):
        """Thread milling only supports thread milling cutters."""
        support = hasattr(tool, "Diameter") and hasattr(tool, "Crest")
        Path.Log.track(tool.Label, support)
        return support


def SetupProperties():
    setup = []
    setup.append("ThreadOrientation")
    setup.append("ThreadType")
    setup.append("ThreadName")
    setup.append("ThreadFit")
    setup.append("MajorDiameter")
    setup.append("MinorDiameter")
    setup.append("Pitch")
    setup.append("TPI")
    setup.append("Passes")
    setup.append("Direction")
    setup.append("LeadInOut")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a thread milling operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectThreadMilling(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj
