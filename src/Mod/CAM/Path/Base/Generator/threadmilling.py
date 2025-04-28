# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
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
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Thread Milling generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "CAM thread milling operation."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def _comment(commandlist, msg):
    """Nice for debugging to insert markers into generated g-code"""
    if False:
        commandlist.append(Path.Command("(------- {} -------)".format(msg)))


class _Thread(object):
    """Helper class for dealing with different thread types"""

    def __init__(self, cmd, zStart, zFinal, pitch, internal):
        self.cmd = cmd
        if zStart < zFinal:
            self.pitch = pitch
        else:
            self.pitch = -pitch
        self.halfPitch = self.pitch / 2
        self.zStart = zStart
        self.zFinal = zFinal
        self.internalThread = internal

    def overshoots(self, z):
        """overshoots(z) ... returns true if adding another half helix goes beyond the thread bounds. Negative pitch spirals down"""
        if self.pitch < 0:
            return z + self.halfPitch < self.zFinal
        return z + self.halfPitch > self.zFinal

    def adjustX(self, x, dx):
        """adjustX(x, dx) ... move x by dx, the direction depends on the thread settings"""
        if self.isG3() == (self.pitch > 0):
            return x + dx
        return x - dx

    def adjustY(self, y, dy):
        """adjustY(y, dy) ... move y by dy, same for both directions"""
        return y - dy

    def isG3(self):
        """isG3() ... returns True if this is a G3 command"""
        return self.cmd in ["G3", "G03", "g3", "g03"]

    def arcOpposite(self):
        return "G2" if self.isG3() else "G3"

    def LeadInOutCmd(self):
        if self.internalThread:
            return self.cmd
        return self.arcOpposite()


def generate(
    center,
    cmd,
    zStart,
    zFinal,
    pitch,
    radius,
    leadInOut,
    retractOffset,
    start,
    feedRateAdj,
    tool_radius,
):
    """generate(center, cmd, zStart, zFinal, pitch, radius, leadInOut, retractOffset, start, feedRateAdj, tooldiam)
    ... returns the g-code to mill the given internal thread"""
    thread = _Thread(cmd, zStart, zFinal, pitch, radius > retractOffset)

    # FeedRateCheckbox.checked
    r = radius  # radius of path, ie spindle axis path radius
    descentRate = thread.halfPitch / (math.pi * r)  # helix vertical gradient d(xy)/dz
    if not feedRateAdj:
        feedRateRatio = 0
    elif thread.internalThread:  # always adj for outer tip feedrate
        feedRateRatio = r / (r + tool_radius)  #  (hole_rad-tool_rad)/hole_rad
    elif (r - tool_radius) * 200 < r:
        feedRateRatio = 200  # prevent div zero
    else:  # external thread: increase spindle feed rate to maintain inner-cut chip-load
        feedRateRatio = r / (r - tool_radius)

    adjParamDict = {}
    if r != 0:  # r == 0 is plunge cut at vert feedrate, feedRateAdj irrel
        adjParamDict["DR"] = descentRate
        if feedRateAdj:
            adjParamDict["FR"] = feedRateRatio

    paramDict = dict(adjParamDict)

    yMin = center.y - radius
    yMax = center.y + radius

    ThrCommandList = []
    # at this point the tool is at a safe height (depending on the previous thread), so we can move
    # into position first, and then drop to the start height. If there is any material in the way this
    # op hasn't been setup properly.
    if leadInOut:
        _comment(ThrCommandList, "lead-in")
        if start is None:
            ThrCommandList.append(
                Path.Command("G0", {"X": center.x, "Y": center.y + retractOffset})
            )
            ThrCommandList.append(Path.Command("G0", {"Z": thread.zStart}))
        else:
            paramDict = dict(adjParamDict)
            paramDict.update(
                {
                    "X": center.x,
                    "Y": center.y + retractOffset,
                    "Z": thread.zStart,
                    "I": (center.x - start.x) / 2,
                    "J": (center.y - start.y + retractOffset) / 2,
                    "K": (thread.zStart - start.z) / 2,
                }
            )
            ThrCommandList.append(Path.Command(thread.arcOpposite(), paramDict))

        paramDict = dict(adjParamDict)
        paramDict.update({"Y": yMax, "J": (yMax - (center.y + retractOffset)) / 2})
        ThrCommandList.append(Path.Command(thread.LeadInOutCmd(), paramDict))
        _comment(ThrCommandList, "lead-in")
    else:
        if start is None:
            ThrCommandList.append(
                Path.Command("G0", {"X": center.x, "Y": center.y + retractOffset})
            )
            ThrCommandList.append(Path.Command("G0", {"Z": thread.zStart}))
        else:
            ThrCommandList.append(
                Path.Command(
                    "G0", {"X": center.x, "Y": center.y + retractOffset, "Z": thread.zStart}
                )
            )
        ThrCommandList.append(Path.Command("G1", {"Y": yMax}))

    z = thread.zStart
    r = -radius
    i = 0
    while not Path.Geom.isRoughly(z, thread.zFinal):
        if thread.overshoots(z):
            break

        if i & 0x01:
            y = yMax
        else:
            y = yMin
        paramDict = dict(adjParamDict)
        paramDict.update({"Y": y, "Z": z + thread.halfPitch, "J": r})
        ThrCommandList.append(Path.Command(thread.cmd, paramDict))

        r = -r
        i += 1
        z += thread.halfPitch

    # find remaining fractional arc segment to end of thread
    if Path.Geom.isRoughly(z, thread.zFinal):
        x = center.x
        y = yMin if (i & 0x01) else yMax
    else:
        numSemiArcs = math.fabs(thread.zFinal - thread.zStart) / thread.halfPitch
        arcFraction = numSemiArcs - int(numSemiArcs)
        dy = r * math.cos(arcFraction * math.pi)
        dx = r * math.sin(arcFraction * math.pi)
        y = thread.adjustY(center.y, dy)
        x = thread.adjustX(center.x, dx)
        _comment(ThrCommandList, "finish-thread")
        paramDict = dict(adjParamDict)
        paramDict.update({"X": x, "Y": y, "Z": thread.zFinal, "J": r})
        ThrCommandList.append(Path.Command(thread.cmd, paramDict))
        _comment(ThrCommandList, "finish-thread")

    a = math.atan2(y - center.y, x - center.x)
    dx = math.cos(a) * (radius - retractOffset)
    dy = math.sin(a) * (radius - retractOffset)
    Path.Log.debug("")
    Path.Log.debug("a={}: dx={:.2f}, dy={:.2f}".format(a / math.pi * 180, dx, dy))

    retractX = x - dx
    retractY = y - dy
    Path.Log.debug("({:.2f}, {:.2f}) -> ({:.2f}, {:.2f})".format(x, y, retractX, retractY))

    if leadInOut:
        _comment(ThrCommandList, "lead-out")
        paramDict = dict(adjParamDict)
        paramDict.update({"X": retractX, "Y": retractY, "I": -dx / 2, "J": -dy / 2})
        ThrCommandList.append(Path.Command(thread.LeadInOutCmd(), paramDict))
        _comment(ThrCommandList, "lead-out")
    else:
        ThrCommandList.append(Path.Command("G1", {"X": retractX, "Y": retractY}))

    return (ThrCommandList, FreeCAD.Vector(retractX, retractY, thread.zFinal))
