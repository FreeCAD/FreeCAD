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

from __future__ import print_function

import FreeCAD
import Path
import PathScripts.PathCircularHoleBase as PathCircularHoleBase
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "Path Thread Milling Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path thread milling operation."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


def radiiInternal(majorDia, minorDia, toolDia, toolCrest=None):
    """internlThreadRadius(majorDia, minorDia, toolDia, toolCrest) ... returns the maximum radius for thread."""
    PathLog.track(majorDia, minorDia, toolDia, toolCrest)
    if toolCrest is None:
        toolCrest = 0.0
    # As it turns out metric and imperial standard threads follow the same rules.
    # The determining factor is the height of the full 60 degree triangle H.
    # - The minor diameter is 1/4 * H smaller than the pitch diameter.
    # - The major diameter is 3/8 * H bigger than the pitch diameter
    # Since we already have the outer diameter it's simpler to just add 1/8 * H
    # to get the outer tip of the thread.
    H = ((majorDia - minorDia) / 2.0) * 1.6  # (D - d)/2 = 5/8 * H
    outerTip = majorDia / 2.0 + H / 8.0
    # Compensate for the crest of the tool
    toolTip = (
        outerTip - toolCrest * 0.8660254037844386
    )  # math.sqrt(3)/2 ... 60deg triangle height
    return ((minorDia - toolDia) / 2.0, toolTip - toolDia / 2.0)


def threadPasses(count, radii, majorDia, minorDia, toolDia, toolCrest=None):
    PathLog.track(count, radii, majorDia, minorDia, toolDia, toolCrest)
    minor, major = radii(majorDia, minorDia, toolDia, toolCrest)
    dr = float(major - minor) / count
    return [major - dr * (count - (i + 1)) for i in range(count)]


class _InternalThread(object):
    """Helper class for dealing with different thread types"""

    def __init__(self, cmd, zStart, zFinal, pitch):
        self.cmd = cmd
        if zStart < zFinal:
            self.pitch = pitch
        else:
            self.pitch = -pitch
        self.hPitch = self.pitch / 2
        self.zStart = zStart
        self.zFinal = zFinal

    def overshoots(self, z):
        """overshoots(z) ... returns true if adding another half helix goes beyond the thread bounds"""
        if self.pitch < 0:
            return z + self.hPitch < self.zFinal
        return z + self.hPitch > self.zFinal

    def adjustX(self, x, dx):
        """adjustX(x, dx) ... move x by dx, the direction depends on the thread settings"""
        if self.isG3() == (self.pitch > 0):
            return x + dx
        return x - dx

    def adjustY(self, y, dy):
        """adjustY(y, dy) ... move y by dy, the direction depends on the thread settings"""
        if self.isG3():
            return y - dy
        return y - dy

    def isG3(self):
        """isG3() ... returns True if this is a G3 command"""
        return self.cmd in ["G3", "G03", "g3", "g03"]

    def isUp(self):
        """isUp() ... returns True if the thread goes from the bottom up"""
        return self.pitch > 0


def internalThreadCommands(loc, cmd, zStart, zFinal, pitch, radius, leadInOut):
    """internalThreadCommands(loc, cmd, zStart, zFinal, pitch, radius) ... returns the g-code to mill the given internal thread"""
    thread = _InternalThread(cmd, zStart, zFinal, pitch)

    yMin = loc.y - radius
    yMax = loc.y + radius

    path = []
    # at this point the tool is at a safe height (depending on the previous thread), so we can move
    # into position first, and then drop to the start height. If there is any material in the way this
    # op hasn't been setup properly.
    path.append(Path.Command("G0", {"X": loc.x, "Y": loc.y}))
    path.append(Path.Command("G0", {"Z": thread.zStart}))
    if leadInOut:
        path.append(Path.Command(thread.cmd, {"Y": yMax, "J": (yMax - loc.y) / 2}))
    else:
        path.append(Path.Command("G1", {"Y": yMax}))

    z = thread.zStart
    r = -radius
    i = 0
    while True:
        z = thread.zStart + i * thread.hPitch
        if thread.overshoots(z):
            break
        if 0 == (i & 0x01):
            y = yMin
        else:
            y = yMax
        path.append(Path.Command(thread.cmd, {"Y": y, "Z": z + thread.hPitch, "J": r}))
        r = -r
        i = i + 1

    z = thread.zStart + i * thread.hPitch
    if PathGeom.isRoughly(z, thread.zFinal):
        x = loc.x
    else:
        n = math.fabs(thread.zFinal - thread.zStart) / thread.hPitch
        k = n - int(n)
        dy = math.cos(k * math.pi)
        dx = math.sin(k * math.pi)
        y = thread.adjustY(loc.y, r * dy)
        x = thread.adjustX(loc.x, r * dx)
        path.append(
            Path.Command(thread.cmd, {"X": x, "Y": y, "Z": thread.zFinal, "J": r})
        )

    if leadInOut:
        path.append(
            Path.Command(
                thread.cmd,
                {"X": loc.x, "Y": loc.y, "I": (loc.x - x) / 2, "J": (loc.y - y) / 2},
            )
        )
    else:
        path.append(Path.Command("G1", {"X": loc.x, "Y": loc.y}))
    return path


class ObjectThreadMilling(PathCircularHoleBase.ObjectOp):
    """Proxy object for thread milling operation."""

    LeftHand = "LeftHand"
    RightHand = "RightHand"
    ThreadTypeCustom = "Custom"
    ThreadTypeMetricInternal = "MetricInternal"
    ThreadTypeImperialInternal = "ImperialInternal"
    DirectionClimb = "Climb"
    DirectionConventional = "Conventional"

    ThreadOrientations = [LeftHand, RightHand]
    ThreadTypes = [
        ThreadTypeCustom,
        ThreadTypeMetricInternal,
        ThreadTypeImperialInternal,
    ]
    Directions = [DirectionClimb, DirectionConventional]

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

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "ThreadType": [
                (translate("Path_ThreadMilling", "Custom"), "Custom"),
                (translate("Path_ThreadMilling", "Metric Internal"), "MetricInternal"),
                (
                    translate("Path_ThreadMilling", "Imperial Internal"),
                    "ImperialInternal",
                ),
            ],  # this is the direction that the profile runs
            "ThreadOrientation": [
                (translate("Path_ThreadMilling", "LeftHand"), "LeftHand"),
                (translate("Path_ThreadMilling", "RightHand"), "RightHand"),
            ],  # side of profile that cutter is on in relation to direction of profile
            "Direction": [
                (translate("Path_ThreadMilling", "Climb"), "Climb"),
                (translate("Path_ThreadMilling", "Conventional"), "Conventional"),
            ],  # side of profile that cutter is on in relation to direction of profile
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        PathLog.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        PathLog.debug(data)

        return data

    def circularHoleFeatures(self, obj):
        return PathOp.FeatureBaseGeometry

    def initCircularHoleOperation(self, obj):
        obj.addProperty(
            "App::PropertyEnumeration",
            "ThreadOrientation",
            "Thread",
            QT_TRANSLATE_NOOP("App::Property", "Set thread orientation"),
        )
        # obj.ThreadOrientation = self.ThreadOrientations
        obj.addProperty(
            "App::PropertyEnumeration",
            "ThreadType",
            "Thread",
            QT_TRANSLATE_NOOP("App::Property", "Currently only internal"),
        )
        # obj.ThreadType = self.ThreadTypes
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
                "App::Property", "Set how many passes are used to cut the thread"
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

    def threadStartDepth(self, obj):
        if obj.ThreadOrientation == self.RightHand:
            if obj.Direction == self.DirectionClimb:
                PathLog.track(obj.Label, obj.FinalDepth)
                return obj.FinalDepth
            PathLog.track(obj.Label, obj.StartDepth)
            return obj.StartDepth
        if obj.Direction == self.DirectionClimb:
            PathLog.track(obj.Label, obj.StartDepth)
            return obj.StartDepth
        PathLog.track(obj.Label, obj.FinalDepth)
        return obj.FinalDepth

    def threadFinalDepth(self, obj):
        PathLog.track(obj.Label)
        if obj.ThreadOrientation == self.RightHand:
            if obj.Direction == self.DirectionClimb:
                PathLog.track(obj.Label, obj.StartDepth)
                return obj.StartDepth
            PathLog.track(obj.Label, obj.FinalDepth)
            return obj.FinalDepth
        if obj.Direction == self.DirectionClimb:
            PathLog.track(obj.Label, obj.FinalDepth)
            return obj.FinalDepth
        PathLog.track(obj.Label, obj.StartDepth)
        return obj.StartDepth

    def threadDirectionCmd(self, obj):
        PathLog.track(obj.Label)
        if obj.ThreadOrientation == self.RightHand:
            if obj.Direction == self.DirectionClimb:
                PathLog.track(obj.Label, "G2")
                return "G2"
            PathLog.track(obj.Label, "G3")
            return "G3"
        if obj.Direction == self.DirectionClimb:
            PathLog.track(obj.Label, "G3")
            return "G3"
        PathLog.track(obj.Label, "G2")
        return "G2"

    def threadSetup(self, obj):
        # the thing to remember is that Climb, for an internal thread must always be G3
        if obj.Direction == self.DirectionClimb:
            if obj.ThreadOrientation == self.RightHand:
                return ("G3", obj.FinalDepth.Value, obj.StartDepth.Value)
            return ("G3", obj.StartDepth.Value, obj.FinalDepth.Value)
        if obj.ThreadOrientation == self.RightHand:
            return ("G2", obj.StartDepth.Value, obj.FinalDepth.Value)
        return ("G2", obj.FinalDepth.Value, obj.StartDepth.Value)

    def threadPassRadii(self, obj):
        PathLog.track(obj.Label)
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
        PathLog.track(obj.Label, loc, gcode, zStart, zFinal, pitch)

        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )

        for radius in threadPasses(
            obj.Passes,
            radiiInternal,
            obj.MajorDiameter.Value,
            obj.MinorDiameter.Value,
            float(self.tool.Diameter),
            float(self.tool.Crest),
        ):
            commands = internalThreadCommands(
                loc, gcode, zStart, zFinal, pitch, radius, obj.LeadInOut
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
        PathLog.track()
        if self.isToolSupported(obj, self.tool):
            self.commandlist.append(Path.Command("(Begin Thread Milling)"))

            (cmd, zStart, zFinal) = self.threadSetup(obj)
            pitch = obj.Pitch.Value
            if obj.TPI > 0:
                pitch = 25.4 / obj.TPI
            if pitch <= 0:
                PathLog.error("Cannot create thread with pitch {}".format(pitch))
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
            PathLog.error("No suitable Tool found for thread milling operation")

    def opSetDefaultValues(self, obj, job):
        obj.ThreadOrientation = self.RightHand
        obj.ThreadType = self.ThreadTypeMetricInternal
        obj.ThreadFit = 50
        obj.Pitch = 1
        obj.TPI = 0
        obj.Passes = 1
        obj.Direction = self.DirectionClimb
        obj.LeadInOut = True

    def isToolSupported(self, obj, tool):
        """Thread milling only supports thread milling cutters."""
        return hasattr(tool, "Diameter") and hasattr(tool, "Crest")


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
